

#ifndef __DYNAMIC_BITVECTOR_H__
#define __DYNAMIC_BITVECTOR_H__

#include <random>
#include <vector>
#include <cstdint>
#include <iostream>

#include "utils.h"
#include "Datablock.h"


using ui32 = uint32_t;

// Treap implementation of a compact dynamic bitvector, which is
// an array of bits supporting the following operations, all in
// O(log n) time:
// 
// select(i,x): Return the position of the ith x
// rank(i,x):   Return the number of x's in [0...i)
// erase(i):    Erase the ith element and shift everything back
// insert(i,b): Insert b to be the ith element and shift everything
//
// The implementation is compact, meaning that the bits are packed
// in an uint64_t (64 bits). However, due to the dynamic nature,
// we can't simply just store an array of uint64_t's. Thus, we
// split the bits into Datablocks, which is a fixed size version of
// a dynamic bitvector supporting operations in O(n) time. Then, to
// guarantee an even split, we ensure at run time that the number
// of bits in each block is always within certain range, which is
// controlled by the compile time parameter B, for the aveerage
// number of bits in each block. Next, we store all these blocks in
// an implicit treap for time efficient searching. When inserting
// or deleting bits, if the number of bits move outside of the
// range allowed, we might borrow, split, or merge with neighboring
// blocks to guarantee the range.
//
// Implementation details: we store the nodes in a vector for cache
// efficiency, thus all node pointers are 32 bit indexes to an
// array. For nonrecursive searching, we store parent pointers. For
// run time guarantee of the number of bits in each datablock, we
// store the in order successor and predecessor. We also store the
// order statistics for rank and select, and we store a random
// priority for treap.
// 
// For the range of elements, a natural choice is to be bounded in
// [B/2, 2B]. Instead, we choose [floor(2B/3), ceil(4B/3)]. For a
// bound [l, u], we essentially only need 2l < u for the splitting
// and merging to be within bound, and the solution such that 2l
// is closest to u with u+l = 2B for symmetry is
//    u = ceil( (4B-1)/3 )
//    l = floor( (2B+1)/3 )
// Then we remove the -1 and +1's for simplicity in formulas.
// Thus, let 
//  MAX_BITS  = ceil(4B/3)        = maximum number of bits in a
//                                  data block
//  MIN_BITS  = floor(2B/3)       = minimum number of bits in a
//                                  data block
//  MAX_WORDS = ceil(MAX_BITS/64) = number of uint64_t's needed to
//                                  store the maximum number of
//                                  bits
//  MIN_WORDS = ceil(MIN_BITS/64) = number of uint64_t's needed to
//                                  store the minimum number of
//                                  bits
// 
// Thus, in total, the number of bytes in each Node is
//      8 * 4 + sizeof(Datablock<MAX_WORDS>)
//    = 32 + 8 + 8 * MAX_WORDS
//    = 40 + 8 * MAX_WORDS
// Let n be the number of bits in this DynamicBitvector, then we
// need at least n/MIN_BITS nodes, which requires in total
//   (n/MIN_BITS) * (40 + 8 * MAX_WORDS)
// number of bytes to store n bits. Using the formula for
// MIN_BITS and MAX_WORDS without all the ceiling and floors,
// the number of bytes needed is 60 * n/B + n/4. This is the
// best we can hope for in this implementation, as a byte can
// hold at most 8 bits and we can only use 4 bits on average.
// 
// Lastly, all operations runs in O(Blog(n)) time, which means
// that larger B allows for more compact representation but
// slower speed, and smaller B allows for faster operations
// but more space.
template <unsigned int B>
class DynamicBitvector {
public:

private:
    static constexpr ui32 idx_null = UINT32_MAX;

    //static constexpr ui32 MAX_BITS = (4 * B + 2) / 3;       // this is ceil( 4B/3 )
    //static constexpr ui32 MIN_BITS = (2 * B) / 3;           // this is floor( 2B/3 )
    static constexpr ui32 MAX_BITS = 2 * B;
    static constexpr ui32 MIN_BITS = B / 2;
    static constexpr ui32 MAX_WORDS = (MAX_BITS + 63) / 64; // ceil(MAX_BITS / 64)
    static constexpr ui32 MIN_WORDS = (MIN_BITS + 63) / 64; // ceil(MIN_BITS / 64)

    struct Node {

        // Pointers within a tree
        ui32 parent;
        ui32 left, right;
        ui32 prev, next;  // doubly-linked list of data blocks

        // Order statistics
        ui32 sub_sz;   // total bits in subtree, including this block
        ui32 sub_ones; // total 1-bits in subtree, including this block

        // random heap priority for treap
        ui32 prio;     

        // Actual storage for the data
        Datablock<MAX_WORDS> bits;

        Node() : parent(idx_null), left(idx_null), right(idx_null), prev(idx_null), next(idx_null), sub_sz(0), sub_ones(0), bits() {};
    };

    // The memory for the nodes
    std::vector<Node> nodes;
    ui32 root;

    // We handle memory ourselves with a list of free nodes.
    std::vector<ui32> free_list;

    // Random number generator for treap.
    std::minstd_rand rng;

    // Actual uniform distribution generator
    std::uniform_int_distribution<ui32> dist;

    // Allocate a new node by recycling or adjusting the vector
    ui32 alloc_node() {
        ui32 idx;
        if (!free_list.empty()) {
            idx = free_list.back();
            free_list.pop_back();
        } else {
            idx = (ui32)nodes.size();
            nodes.push_back(Node());
        }
        Node &n = nodes[idx];
        n = Node();
        n.prio = dist(rng);
        return idx;
    }
    // Deallocate or release a node.
    void dealloc_node(ui32 idx) {
        free_list.push_back(idx);
    }

    // Recompute subtree aggregates
    void pull(ui32 idx) {
        auto &node = nodes[idx];
        node.sub_sz = node.bits.sz;
        node.sub_ones = node.bits.ones;
        if (node.left != idx_null) {
            node.sub_sz += nodes[node.left].sub_sz;
            node.sub_ones += nodes[node.left].sub_ones;
        }
        if (node.right != idx_null) {
            node.sub_sz += nodes[node.right].sub_sz;
            node.sub_ones += nodes[node.right].sub_ones;
        }
    }
    // Recompute subtree aggregates for all the ancestors
    // from a given node
    void pull_propagate(ui32 idx) {
        while (idx != idx_null) {
            pull(idx);
            idx = nodes[idx].parent;
        }
    }

    // Rotate right at idx and update all subtree
    // aggregates and return the new node at the current
    // place.
    ui32 rotate_right(ui32 idx) {
        ui32 L = nodes[idx].left;
        nodes[idx].left = nodes[L].right;
        if (nodes[idx].left != idx_null)
            nodes[nodes[idx].left].parent = idx;
        nodes[L].right = idx;
        nodes[L].parent = nodes[idx].parent;
        if (nodes[idx].parent != idx_null) {
            if (nodes[nodes[idx].parent].left == idx)
                nodes[nodes[idx].parent].left = L;
            else
                nodes[nodes[idx].parent].right = L;
        }
        nodes[idx].parent = L;

        pull(idx);
        pull(L);
        return L;
    }
    // Rotate left at idx and update all subtree
    // aggregates and return the new node at the current
    // place.
    ui32 rotate_left(ui32 idx) {
        ui32 R = nodes[idx].right;
        nodes[idx].right = nodes[R].left;
        if (nodes[idx].right != idx_null)
            nodes[nodes[idx].right].parent = idx;
        nodes[R].left = idx;
        nodes[R].parent = nodes[idx].parent;
        if (nodes[idx].parent != idx_null) {
            if (nodes[nodes[idx].parent].left == idx)
                nodes[nodes[idx].parent].left = R;
            else
                nodes[nodes[idx].parent].right = R;
        }
        nodes[idx].parent = R;

        pull(idx);
        pull(R);
        return R;
    }

    // Helper function.
    // Assuming at most one child potentially violates the
    // heap property, this function rotating left or right
    // to store the heap property.
    ui32 bubble(ui32 idx) {
        if (idx == idx_null)
            return idx_null;

        Node &node_cur = nodes[idx];
        if (node_cur.left != idx_null && nodes[node_cur.left].prio > node_cur.prio)
            return rotate_right(idx);
        else if (node_cur.right != idx_null && nodes[node_cur.right].prio > node_cur.prio)
            return rotate_left(idx);
        return idx;
    }
    // Helper function.
    // Assuming at most one child at this node potentially
    // violates the heap property, this function rotating
    // left or right to store the heap property at this
    // node and all of its ancestors.
    void bubble_pull_propagate(ui32 idx) {
        while (idx != idx_null) {
            pull(idx);
            ui32 idx_new = bubble(idx);
            if (idx_new == idx) {
                // we can stop bubbling now
                idx = nodes[idx].parent;
                break;
            }
            if (nodes[idx_new].parent == idx_null)
                root = idx_new;
            idx_new = nodes[idx_new].parent;
        }
        while (idx != idx_null) {
            pull(idx);
            idx = nodes[idx].parent;
        }
    }
    // Helper function.
    // Take the left child and attach it to the parent of
    // the current node
    ui32 bubble_left(ui32 idx) {
        Node &node_cur = nodes[idx];
        ui32 idx_left = node_cur.left;
        ui32 idx_parent = node_cur.parent;

        if (idx_left != idx_null) {
            nodes[idx_left].parent = idx_parent;
        }
        if (idx_parent != idx_null) {
            if (nodes[idx_parent].left == idx)
                nodes[idx_parent].left = idx_left;
            else
                nodes[idx_parent].right = idx_left;
        }
        return idx_left;
    }
    // Helper function.
    // Take the right child and attach it to the parent of
    // the current node
    ui32 bubble_right(ui32 idx) {
        Node &node_cur = nodes[idx];
        ui32 idx_right = node_cur.right;
        ui32 idx_parent = node_cur.parent;

        if (idx_right != idx_null) {
            nodes[idx_right].parent = idx_parent;
        }
        if (idx_parent != idx_null) {
            if (nodes[idx_parent].left == idx)
                nodes[idx_parent].left = idx_right;
            else
                nodes[idx_parent].right = idx_right;
        }
        return idx_right;
    }

    // Insert an element into the linked list structure
    // in the nodes
    void ll_insert_after(ui32 at, ui32 idx) {
        Node &node_at = nodes[at];
        Node &node_insert = nodes[idx];

        node_insert.next = node_at.next;
        node_insert.prev = at;

        if (node_at.next != idx_null)
            nodes[node_at.next].prev = idx;
        node_at.next = idx;
    }

    // Detach an element in the linked list structure
    // in the nodes
    void ll_detach(ui32 idx) {
        if (idx == idx_null)
            return;
        const Node &node_cur = nodes[idx];
        if (node_cur.prev != idx_null)
            nodes[node_cur.prev].next = node_cur.next;
        if (node_cur.next != idx_null)
            nodes[node_cur.next].prev = node_cur.prev;
    }

    // Insert idx_min as the smallest element in the
    // subtree rooted at idx, or the in order successor
    // of idx, and return the new element for fixing
    // the parents
    ui32 insert_min(ui32 idx, ui32 idx_min) {
        if (idx == idx_null)
            return idx_min;

        ui32 idx_parent = nodes[idx].parent;
        ui32 idx_cur = idx;

        while (nodes[idx_cur].left != idx_null)
            idx_cur = nodes[idx_cur].left;

        nodes[idx_cur].left = idx_min;
        nodes[idx_min].parent = idx_cur;
        pull(idx_cur);
        if (nodes[idx_min].prio > nodes[idx_cur].prio)
            idx_cur = rotate_right(idx_cur);

        while (nodes[idx_cur].parent != idx_parent) {
            idx_cur = nodes[idx_cur].parent;
            pull(idx_cur);
            if (nodes[nodes[idx_cur].left].prio > nodes[idx_cur].prio)
                idx_cur = rotate_right(idx_cur);
        }

        return idx_cur;
    }

    // Insert idx_max as the largest element in the
    // subtree rooted at idx, and return the new
    // element for fixing the parents
    ui32 insert_max(ui32 idx, ui32 idx_max) {
        if (idx == idx_null)
            return idx_max;

        ui32 idx_parent = nodes[idx].parent;
        ui32 idx_cur = idx;

        while (nodes[idx_cur].right != idx_null)
            idx_cur = nodes[idx_cur].right;

        nodes[idx_cur].right = idx_max;
        nodes[idx_max].parent = idx_cur;
        pull(idx_cur);
        if (nodes[idx_max].prio > nodes[idx_cur].prio)
            idx_cur = rotate_left(idx_cur);

        while (nodes[idx_cur].parent != idx_parent) {
            idx_cur = nodes[idx_cur].parent;
            pull(idx_cur);
            if (nodes[nodes[idx_cur].right].prio > nodes[idx_cur].prio)
                idx_cur = rotate_left(idx_cur);
        }

        return idx_cur;
    }

    // Using bisection to find the node containing the
    // pos'th bit.
    // out_pos_loc is a return value for the local index
    // of the bit inside this node.
    ui32 bisect_pos(ui32 pos, ui32 *out_pos_loc) const {
#if defined(_BOUND_CHECKING)
        if (pos >= size())
            throw std::out_of_range("");
#endif
        ui32 idx = root;

        while (true) {
            const Node &n = nodes[idx];
            ui32 ls = (n.left == idx_null ? 0 : nodes[n.left].sub_sz);
            if (pos < ls) {
                idx = n.left;
                continue;
            }
            pos -= ls;
            if (pos >= n.bits.sz) {
                pos -= n.bits.sz;
                idx = n.right;
                continue;
            }
            break;
        }

        *out_pos_loc = pos;
        return idx;
    }

    // Using bisection to find the node containing the
    // pos'th bit.
    // out_pos_loc is a return value for the local index
    // of the bit inside this node.
    // out_ones is the number of ones before this index
    // in the entire array.
    ui32 bisect_pos(ui32 pos, ui32 *out_pos_loc, ui32 *out_ones) const {
#if defined(_BOUND_CHECKING)
        if (pos >= size())
            throw std::out_of_range("");
#endif
        ui32 idx = root;
        ui32 ones = 0;
        while (true) {
            const Node &n = nodes[idx];
            ui32 ls = (n.left == idx_null ? 0 : nodes[n.left].sub_sz);
            if (pos < ls) {
                idx = n.left;
                continue;
            }
            pos -= ls;
            ones += (n.left == idx_null ? 0 : nodes[n.left].sub_ones);
            if (pos >= n.bits.sz) {
                pos -= n.bits.sz;
                ones += n.bits.ones;
                idx = n.right;
                continue;
            }
            break;
        }

        *out_pos_loc = pos;
        *out_ones = ones + nodes[idx].bits.rank1(pos);
        return idx;
    }

    // Returns the largest node inside the subtree
    // rooted at idx
    ui32 node_last(ui32 idx) const {
        if (idx == idx_null)
            return idx_null;
        while (nodes[idx].right != idx_null)
            idx = nodes[idx].right;
        return idx;
    }
    // Returns the smallest node inside the subtree
    // rooted at idx
    ui32 node_first(ui32 idx) const {
        if (idx == idx_null)
            return idx_null;
        while (nodes[idx].left != idx_null)
            idx = nodes[idx].left;
        return idx;
    }

    // Bisection on the number of ones
    ui32 bisect_ones(ui32 pos, ui32 *pos_loc, ui32 *pos_before) const {
#if defined(_BOUND_CHECKING)
        if (root == idx_null || nodes[root].sub_ones >= pos)
            throw std::out_of_range("");
#endif

        ui32 idx = root;
        ui32 count = 0;

        while (idx != idx_null) {
            const auto &n = nodes[idx];
            ui32 ls = (n.left == idx_null ? 0 : nodes[n.left].sub_sz);
            ui32 lo = (n.left == idx_null ? 0 : nodes[n.left].sub_ones);
            if (pos < lo) {
                idx = n.left;
                continue;
            }
            count += ls;
            pos -= lo;
            if (pos >= n.bits.ones) {
                pos -= n.bits.ones;
                count += n.bits.sz;
                idx = n.right;
                continue;
            }
            break;
        }
        *pos_loc = nodes[idx].bits.select1(pos);
        *pos_before = count;
        return idx;
    }

public:
    DynamicBitvector() : root(idx_null), rng(std::random_device()()), dist(0, UINT32_MAX) {};
    ~DynamicBitvector() {};

    void reserve(ui32 bits_sz) {
        ui32 max_nodes = (bits_sz + MIN_BITS - 1) / MIN_BITS;
        free_list.reserve(max_nodes);
        nodes.reserve(max_nodes);
    }

    void init_size(ui32 sz) {
        ui32 k_max = sz / MIN_BITS;
        ui32 k_min = (sz + (MAX_BITS - 1)) / MAX_BITS;
        ui32 k = sz / B;

        if (k_max < k_min || k_max <= 0) {
            ui32 prev = idx_null;
            for (; sz >= B; sz -= B) {
                ui32 idx = alloc_node();
                nodes[idx].bits.expand(B);
                nodes[idx].sub_sz = B;
                root = insert_max(root, idx);
                if (prev != idx_null)
                    ll_insert_after(prev, idx);
                prev = idx;
            }
            if (sz > 0) {
                ui32 idx = alloc_node();
                nodes[idx].bits.expand(sz);
                nodes[idx].sub_sz = sz;
                root = insert_max(root, idx);
                if (prev != idx_null)
                    ll_insert_after(prev, idx);
            }
            return;
        }
        if (k > k_max)
            k = k_max;
        if (k < k_min)
            k = k_min;

        ui32 node_sz = sz / k;
        ui32 node_ones = sz % k;

        ui32 prev = idx_null;
        for (ui32 i = 0; i < k; i++) {
            ui32 idx = alloc_node();
            if (i < node_ones) {
                nodes[idx].bits.expand(node_sz + 1);
                nodes[idx].sub_sz = node_sz + 1;
            } else {
                nodes[idx].bits.expand(node_sz);
                nodes[idx].sub_sz = node_sz;
            }
            root = insert_max(root, idx);
            if (prev != idx_null)
                ll_insert_after(prev, idx);
            prev = idx;
        }
    }

    // total number of bits
    ui32 size() const {
        return (root == idx_null ? 0 : nodes[root].sub_sz);
    }
    ui32 ones() const {
        return (root == idx_null ? 0 : nodes[root].sub_ones);
    }

    // access bit
    bool get_bit(ui32 pos) const {
#if defined(_BOUND_CHECKING)
        if (pos >= size())
            throw std::out_of_range("get: pos out of range");
#endif
        ui32 pos_loc;
        ui32 idx = bisect_pos(pos, &pos_loc);
        return nodes[idx].bits.get_bit(pos_loc);
    }

    bool get_bit_rank(ui32 pos, ui32 *rank1) const {
#if defined(_BOUND_CHECKING)
        if (pos >= size())
            throw std::out_of_range("get: pos out of range");
#endif
        ui32 pos_loc;
        ui32 idx = bisect_pos(pos, &pos_loc, rank1);
        return nodes[idx].bits.get_bit(pos_loc);
    }

    void set_bit(ui32 pos, bool bit) {
#if defined(_BOUND_CHECKING)
        if (pos >= size())
            throw std::out_of_range("get: pos out of range");
#endif
        ui32 pos_loc;
        ui32 idx = bisect_pos(pos, &pos_loc);

        if (nodes[idx].bits.get_bit(pos_loc) == bit)
            return;

        nodes[idx].bits.set_bit(pos_loc, bit);
        pull_propagate(idx);
    }

    // Compute the number of 1's in [0..pos)
    ui32 rank1(ui32 pos) const {
        if (pos >= size()) {
            return (root == idx_null ? 0 : nodes[root].sub_ones);
        }

        ui32 pos_loc;
        ui32 count;
        ui32 idx = bisect_pos(pos, &pos_loc, &count);

        return count;
    }
    // Compute the number of 0's in [0..pos)
    ui32 rank0(ui32 pos) const {
        return pos - rank1(pos);
    }
    // Compute the number bits in [0..pos) that is equal to bit.
    ui32 rank(ui32 pos, bool bit) {
        if (bit) return rank1(pos);
        else return rank0(pos);
    }

    // Compute the s'th 1 in [0..size())
    ui32 select1(ui32 s) const {
        ui32 pos_loc, pos_before;
        bisect_ones(s, &pos_loc, &pos_before);
        return pos_loc + pos_before;
    }

    // insert bit at pos in [0..size()]
    void insert(ui32 pos, bool bit) {
        ui32 pos_loc;
        ui32 idx;
        if (pos < size())
            idx = bisect_pos(pos, &pos_loc);
        else {
            idx = node_last(root);
            if (idx == idx_null) {
                root = alloc_node();
                idx = root;
            }
            pos_loc = nodes[idx].bits.sz;
        }
        Node *node_cur = &(nodes[idx]);
        ui32 sz_left = (node_cur->left == idx_null ? 0 : nodes[node_cur->left].sub_sz);
        ui32 sz_cur = node_cur->bits.sz;

        if (node_cur->bits.sz + 1 <= MAX_BITS) {
            // If there isn't overflow
            // Also handles when there's an empty node
            node_cur->bits.insert_at(pos_loc, bit);
            pull_propagate(idx);
            return;
        }

        // If there is overflow.
        // Never balance with neighboring blocks, just split whenever
        // because if we balance with neighbor, then we need to update
        // the count for the neighbor which is annoying

        ui32 idx_right = alloc_node();
        node_cur = &(nodes[idx]); // if vector was reallocated, the old address would have been garbage
        Node &node_right = nodes[idx_right];

        Datablock<MAX_WORDS>::insert_balance(&node_cur->bits, &node_right.bits, pos_loc, bit);
        node_right.sub_sz = node_right.bits.sz;
        node_right.sub_ones = node_right.bits.ones;

        // fix linked list
        ll_insert_after(idx, idx_right);

        node_cur->right = insert_min(node_cur->right, idx_right);
        nodes[node_cur->right].parent = idx;
        // technically not needed, but logic is scrambled, hard to remove it
        // based on what each function does
        pull(idx);

        bubble_pull_propagate(idx);
    }

    // erase bit at pos in [0..size())
    void erase(ui32 pos) {
        ui32 pos_loc;
        ui32 idx = bisect_pos(pos, &pos_loc);
        Node &node_cur = nodes[idx];
        ui32 sz_left = (node_cur.left == idx_null ? 0 : nodes[node_cur.left].sub_sz);
        const ui32 sz_cur = node_cur.bits.sz;

        if (sz_cur > MIN_BITS) {
            // if the current node has enough bits to delete one bit
            node_cur.bits.remove_at(pos_loc);
            pull_propagate(idx);
            return;
        }

        // if there is not enough space
        // then we borrow from neighbor
        // or merge with the neighbor as a last resort

        if (node_cur.next != idx_null &&
            nodes[node_cur.next].bits.sz + sz_cur > 2 * MIN_BITS) {
            // borrow from the in order successor
            Node &node_next = nodes[node_cur.next];

            Datablock<MAX_WORDS>::delete_balance(&node_cur.bits, &node_next.bits, pos_loc);

            // If the right subtree of the current node is not empty,
            // then the in order successor is in the right subtree,
            // in which case one pull propagate on the successor is
            // enough. If the right subtree is empty, however, then
            // the successor must be one of the ancestors of the
            // current node, in which one pull propagate on the
            // current node is enough.
            if (node_cur.right != idx_null)
                pull_propagate(node_cur.next);
            else
                pull_propagate(idx);
            return;
        }

        if (node_cur.prev != idx_null &&
            nodes[node_cur.prev].bits.sz + sz_cur > 2 * MIN_BITS) {
            // borrow from the in order predecessor
            Node &node_prev = nodes[node_cur.prev];

            Datablock<MAX_WORDS>::delete_balance(&node_prev.bits, &node_cur.bits, pos_loc + node_prev.bits.sz);

            // If the left subtree of the current node is not empty,
            // then the in order predecessor is in the left subtree,
            // in which case one pull propagate on the predecessor is
            // enough. If the left subtree is empty, however, then
            // the predecessor must be one of the ancestors of the
            // current node, in which one pull propagate on the
            // current node is enough.
            if (node_cur.left != idx_null)
                pull_propagate(node_cur.prev);
            else
                pull_propagate(idx);
            return;
        }

        // Now we merge, really don't want to do this fuckkkk
        // but this will definitely happen with more queries

        if (node_cur.next != idx_null) {
            // merge with the in order successor
            ui32 idx_next = node_cur.next;
            Node &node_next = nodes[idx_next];
            if (node_cur.right != idx_null) {
                // the successor is in the right subtree of node_cur
                // so we will merge to the current node
                Datablock<MAX_WORDS>::delete_merge_left(&node_cur.bits, &node_next.bits, pos_loc);
                ll_detach(idx_next);
                ui32 idx_next_parent = node_next.parent;
                bubble_right(idx_next);
                dealloc_node(idx_next);
                pull_propagate(idx_next_parent);
                return;
            }

            // If the right subtree is empty, the successor must be
            // one of its ancestor. In which case we merge to the ancestor
            // and remove the current node.
            Datablock<MAX_WORDS>::delete_merge_right(&node_cur.bits, &node_next.bits, pos_loc);
            ll_detach(idx);
            ui32 idx_parent = node_cur.parent;
            bubble_left(idx);
            dealloc_node(idx);
            pull_propagate(idx_parent);
            return;
        }

        if (node_cur.prev != idx_null) {
            // merge with the in order predecessor
            ui32 idx_prev = node_cur.prev;
            Node &node_prev = nodes[idx_prev];
            if (node_cur.left != idx_null) {
                // the predecessor is in the left subtree of node_cur
                // so we will merge to the current node
                Datablock<MAX_WORDS>::delete_merge_right(&node_prev.bits, &node_cur.bits, pos_loc + node_prev.bits.sz);
                ll_detach(idx_prev);
                ui32 idx_prev_parent = node_prev.parent;
                bubble_left(idx_prev);
                dealloc_node(idx_prev);
                pull_propagate(idx_prev_parent);
                return;
            }

            // If the left subtree is empty, the predecessor must be
            // one of its ancestor. In which case we merge to the ancestor
            // and remove the current node.
            Datablock<MAX_WORDS>::delete_merge_left(&node_prev.bits, &node_cur.bits, pos_loc + node_prev.bits.sz);
            ll_detach(idx);
            ui32 idx_parent = node_cur.parent;
            bubble_right(idx);
            dealloc_node(idx);
            pull_propagate(idx_parent);
            return;
        }

        // This happens when the tree is just a single node,
        // so we are going to just delete it.
        nodes[idx].bits.remove_at(pos);
        pull_propagate(idx);
    }

    void print() const {
        if (size() <= 0)
            return;
        std::ostream &s = std::cout;
        ui32 loc_pos = 0;
        ui32 node = bisect_pos(0, &loc_pos);
        while (node != idx_null) {
            nodes[node].bits.print(s);
            node = nodes[node].next;
        }
    }
};



#endif // __DYNAMIC_BITVECTOR_H__