
#ifndef __DYNAMIC_BITVECTOR_BTREE_H__
#define __DYNAMIC_BITVECTOR_BTREE_H__


#include <random>
#include <vector>
#include <cstdint>
#include <iostream>

#include "utils.h"
#include "ContiguousAllocator.h"
#include "Datablock.h"






using ui32 = uint32_t;


template <ui32 B, ui32 LeafBits>
class DynamicBitvectorBTree {
public:

private:
    static constexpr ui32 idx_null = UINT32_MAX;

    static constexpr ui32 MAX_BITS = 2 * LeafBits;
    static constexpr ui32 MIN_BITS = LeafBits / 2;
    static constexpr ui32 MAX_WORDS = (MAX_BITS + 63) / 64; // ceil(MAX_BITS / 64)
    static constexpr ui32 MIN_WORDS = (MIN_BITS + 63) / 64; // ceil(MIN_BITS / 64)

    struct Node {
        // Pointers within a tree
        ui32 idx_parent;
        ui32 idx_in_parent;
        // For leafs it points to leaf indices
        ui32 children[B];
        ui32 sub_sz[B];
        ui32 sub_ones[B];

        ui32 sz_child;

        bool is_leaf;

        Node() : idx_parent(idx_null), idx_in_parent(0), sz_child(0), is_leaf(false) {};

        inline ui32 total_size() const {
            ui32 count = 0;
            for (ui32 i = 0; i < sz_child; i++)
                count += sub_sz[i];
            return count;
        }
        inline ui32 total_ones() const {
            ui32 count = 0;
            for (ui32 i = 0; i < sz_child; i++)
                count += sub_ones[i];
            return count;
        }

        inline void copy_to(Node *other, const ui32 &src_pos, const ui32 &dest_pos, const ui32 &s) const {
            for (ui32 i = 0; i < s; i++) {
                other->children[dest + i] = src_pos->children[src_pos + i];
                other->sub_sz[dest + i] = src_pos->children[src_pos + i];
                other->sub_ones[dest + i] = src_pos->children[src_pos + i];
            }
        }
        inline void set_child_vals(const ui32 &index, const ui32 &idx, const ui32 &sz, const ui32 &ones) {
            children[index] = idx;
            sub_sz[index] = sz;
            sub_ones[index] = ones;
        }
        inline void insert_at(const ui32 &index, const ui32 &idx, const ui32 &sz, const ui32 &ones) {
            for (ui32 i = 1; i <= sz_child - 1 - at; i++) {
                n1->children[sz_child - i] = n1->children[sz_child - i - 1];
                n1->sub_sz[sz_child - i] = n1->sub_sz[sz_child - i - 1];
                n1->sub_ones[sz_child - i] = n1->sub_ones[sz_child - i - 1];
            }
            set_child_vals(index, idx, sz, ones);
            n1->child_sz++;
        }
        static void split_balance(Node *n1, Node *n2, const ui32 &at, const ui32 &idx, const ui32 &sz, const ui32 &ones) {
            const ui32 total_sz = n1->sz_child + 1;
            const ui32 sz2 = total_sz / 2;   // size of the second block
            const ui32 sz1 = total_sz - sz2; // size of the first block
            ui32 sz_orig = n1->sz_child;

            n1->sz_child = sz1;
            n2->sz_child = sz2;

            // this case never really happens
            if (sz2 <= 0) {
                n1->set_child_vals(0, idx, sz, ones);
                return;
            }

            if (at == sz1 - 1) {
                n1->copy_to(n2, sz1 - 1, 0, sz2);
                n1->set_child_vals(at, idx, sz, ones);
                return;
            }

            if (at == sz1) {
                n1->copy_to(n2, sz1, 1, sz2 - 1);
                n2->set_child_vals(0, idx, sz, ones);
                return;
            }

            if (at > sz1) {
                n1->copy_to(n2, sz1, 0, at - sz1);
                n1->copy_to(n2, at, at - sz1 + 1, sz_orig - at);
                n2->set_child_vals(at - sz1, idx, sz, ones);
            } else {
                n1->sz_child = sz1 - 1;
                n1->copy_to(b2, sz1 - 1, 0, sz2);
                n1->set_child_vals(at, idx, sz, ones);
                n1->insert_at(at, idx, sz, ones);
            }
        }
    };

    struct Leaf {
        // Pointers within a tree
        ui32 idx_parent;
        ui32 idx_in_parent;

        Datablock<MAX_WORDS> bits;
        ui32 prev, next;
    };

    ContiguousAllocator<Node> nodes;
    ContiguousAllocator<Leaf> leafs;

    ui32 root;
    ui32 sub_sz, sub_ones;

    // Find the index of the leaf containing the pos'th bit.
    // out_pos_loc is a return value for the local index
    // of the bit inside this leaf.
    ui32 find_pos(ui32 pos, ui32 *out_pos_loc) const {

        ui32 idx = root;
        while (true) {
            const Node &n = nodes[idx];
            ui32 i;
            for (i = 0; i < n.sz_child; i++) {
                if (pos < n.sub_sz[i])
                    break;
                pos -= n.sub_sz[i];
            }
            idx = n.children[i];

            if (n.is_leaf)
                break;
        }

        *out_pos_loc = pos;
        return idx;
    }

    // Find the index of the leaf containing the pos'th bit.
    // out_pos_loc is a return value for the local index
    // of the bit inside this leaf.
    // out_ones is the number of ones before this index
    // in the entire array.
    ui32 find_pos(ui32 pos, ui32 *out_pos_loc, ui32 *out_ones) const {
        if (pos >= size())
            throw std::out_of_range("");
        ui32 ones = 0;
        ui32 idx = root;
        while (true) {
            const Node &n = nodes[idx];
            ui32 i;
            for (i = 0; i < n.sz_child; i++) {
                if (pos < n.sub_sz[i])
                    break;
                pos -= n.sub_sz[i];
                ones += n.sub_ones[i];
            }
            idx = n.children[i];

            if (n.is_leaf)
                break;
        }

        *out_pos_loc = pos;
        *out_ones = ones + leafs[idx].bits.rank1(pos);
        return idx;
    }

    ui32 pull_propagate_full() {

    }

    ui32 pull_propagate_increase_size(const ui32 &idx_leaf) {
        ui32 idx = leafs[idx_leaf].idx_parent;
        ui32 idx_in = leafs[idx_leaf].idx_in_parent;
        while (idx != idx_null) {
            Node &n = nodes[idx];
            n.sub_sz[idx_in]++;
            idx = n.idx_parent;
            idx_in = n.idx_in_parent;
        }
    }
    ui32 pull_propagate_decrease_size(const ui32 &idx_leaf) {
        ui32 idx = leafs[idx_leaf].idx_parent;
        ui32 idx_in = leafs[idx_leaf].idx_in_parent;
        while (idx != idx_null) {
            Node &n = nodes[idx];
            n.sub_sz[idx_in]--;
            idx = n.idx_parent;
            idx_in = n.idx_in_parent;
        }
    }
    ui32 pull_propagate_increase_ones(const ui32 &idx_leaf) {
        ui32 idx = leafs[idx_leaf].idx_parent;
        ui32 idx_in = leafs[idx_leaf].idx_in_parent;
        while (idx != idx_null) {
            Node &n = nodes[idx];
            n.sub_sz[idx_in]++;
            n.sub_ones[idx_in]++;
            idx = n.idx_parent;
            idx_in = n.idx_in_parent;
        }
    }
    ui32 pull_propagate_decrease_ones(const ui32 &idx_leaf) {
        ui32 idx = leafs[idx_leaf].idx_parent;
        ui32 idx_in = leafs[idx_leaf].idx_in_parent;
        while (idx != idx_null) {
            Node &n = nodes[idx];
            n.sub_sz[idx_in]--;
            n.sub_ones[idx_in]--;
            idx = n.idx_parent;
            idx_in = n.idx_in_parent;
        }
    }

    void insert_split_true(ui32 idx_leaf, ui32 pos_loc) {
        ui32 idx_right = leafs.alloc();
        Leaf &leaf_cur = leafs[idx_leaf];
        Leaf &leaf_right = leafs[idx_right];

        Datablock<MAX_WORDS>::insert_balance(&leaf_cur.bits, &leaf_right.bits, pos_loc, true);
        leaf_right.idx_parent = leaf_cur.idx_parent;
        // fix linked list
        ll_insert_after(idx, idx_right);

        ui32 idx_cur = idx_leaf;
        ui32 idx_other = idx_right;
        ui32 sz_other = leaf_right.bits.sz;
        ui32 ones_other = leaf_right.bits.ones;

        bool need_split = nodes[idx_parent].sz_child >= B;

        if (need_split) {
            ui32 idx_node_new = nodes.alloc();
            ui32 idx_parent = leaf_cur.idx_parent;
            ui32 idx_in_parent = leaf_cur.idx_in_parent;
            Node &node_new = nodes[idx_node_new];
            Node &node_par = nodes[idx_parent];

            node_par.sub_sz[idx_in_parent]++;
            node_par.sub_sz[idx_in_parent] -= sz_other;
            node_par.sub_ones[idx_in_parent]++;
            node_par.sub_ones[idx_in_parent] -= ones_other;

            Node::split_balance(&node_par, &node_new, idx_in_parent + 1, idx_other, sz_other, ones_other);

            if (idx_in_parent + 1 < node_par.sz_child) {
                for (ui32 i = idx_in_parent + 1; i < node_par.sz_child; i++) {
                    Leaf &child_cur = leafs[node_par.children[i]];
                    child_cur.idx_in_parent = i;
                }
            }
            for (ui32 i = 0; i < node_new.sz_child; i++) {
                Leaf &child_cur = leafs[node_new.children[i]];
                child_cur.idx_in_parent = i;
                child_cur.idx_parent = idx_node_new;
            }

            idx_cur = idx_parent;
            idx_other = idx_node_new;
            sz_other = node_new.total_size();
            ones_other = node_new.total_ones();
        } 

        if (need_split) {
            ui32 idx_parent = nodes[idx_cur].idx_parent;
            while (idx_parent != idx_null && nodes[idx_parent].sz_child >= B) {
                ui32 idx_node_new = nodes.alloc();
                Node &node_new = nodes[idx_node_new];
                Node &node_par = nodes[idx_parent];

                node_par.sub_sz[idx_in_parent]++;
                node_par.sub_sz[idx_in_parent] -= sz_other;
                node_par.sub_ones[idx_in_parent]++;
                node_par.sub_ones[idx_in_parent] -= ones_other;

                Node::split_balance(&node_par, &node_new, idx_in_parent + 1, idx_other, sz_other, ones_other);
                node_new.is_leaf = false;
                if (idx_in_parent + 1 < node_par.sz_child) {
                    for (ui32 i = idx_in_parent + 1; i < node_par.sz_child; i++) {
                        Node &child_cur = nodes[node_par.children[i]];
                        child_cur.idx_in_parent = i;
                    }
                }
                for (ui32 i = 0; i < node_new.sz_child; i++) {
                    Node &child_cur = nodes[node_new.children[i]];
                    child_cur.idx_in_parent = i;
                    child_cur.idx_parent = idx_node_new;
                }


                idx_cur = idx_parent;
                idx_parent = node_par.idx_parent;
                idx_in_parent = node_par.idx_in_parent;
                idx_other = idx_node_new;
                sz_other = node_new.total_size();
                ones_other = node_new.total_ones();
            }
        }

        if (idx_parent == idx_null) {
            // changing the root
            ui32 idx_node_new = nodes.alloc();
            Node &new_root = nodes[idx_node_new];
            new_root.sz_child = 2;
            new_root.children[0] = idx_cur;
            new_root.children[1] = idx_other;
            new_root.sub_sz[0] = sub_sz + 1 - sz_other;
            new_root.sub_sz[1] = sz_other;
            new_root.sub_ones[0] = sub_ones + 1 - ones_other;
            new_root.sub_ones[1] = ones_other;
            nodes[idx_cur].idx_parent = idx_node_new;
            nodes[idx_cur].idx_in_parent = 0;
            nodes[idx_other].idx_parent = idx_node_new;
            nodes[idx_other].idx_in_parent = 1;

            root = idx_node_new;
        } else {
            Node &node_parent = nodes[idx_parent];
            node_parent.sub_sz[idx_in_parent]++;
            node_parent.sub_sz[idx_in_parent] -= sz_other;
            node_parent.sub_ones[idx_in_parent]++;
            node_parent.sub_ones[idx_in_parent] -= ones_other;
            node_parent.insert_at(idx_in_parent + 1, idx_other, sz_other, ones_other);
            if (need_split) {
                for (ui32 i = idx_in_parent + 1; i < node_parent.sz_child; i++) {
                    nodes[node_parent.children[i]].idx_in_parent = i;
                }
            } else {
                for (ui32 i = idx_in_parent + 1; i < node_parent.sz_child; i++) {
                    leafs[node_parent.children[i]].idx_in_parent = i;
                }
            }
        }

        sub_sz++;
        sub_ones++;
    }
    void insert_split_false(ui32 idx_leaf, ui32 pos_loc) {
        ui32 idx_right = leafs.alloc();
        Leaf &leaf_cur = leafs[idx_leaf];
        Leaf &leaf_right = leafs[idx_right];

        Datablock<MAX_WORDS>::insert_balance(&leaf_cur.bits, &leaf_right.bits, pos_loc, false);
        leaf_right.idx_parent = leaf_cur.idx_parent;
        // fix linked list
        ll_insert_after(idx, idx_right);

        ui32 idx_cur = idx_leaf;
        ui32 idx_other = idx_right;
        ui32 sz_other = leaf_right.bits.sz;
        ui32 ones_other = leaf_right.bits.ones;

        bool need_split = nodes[idx_parent].sz_child >= B;

        if (need_split) {
            ui32 idx_node_new = nodes.alloc();
            ui32 idx_parent = leaf_cur.idx_parent;
            ui32 idx_in_parent = leaf_cur.idx_in_parent;
            Node &node_new = nodes[idx_node_new];
            Node &node_par = nodes[idx_parent];

            node_par.sub_sz[idx_in_parent]++;
            node_par.sub_sz[idx_in_parent] -= sz_other;
            node_par.sub_ones[idx_in_parent] -= ones_other;

            Node::split_balance(&node_par, &node_new, idx_in_parent + 1, idx_other, sz_other, ones_other);

            if (idx_in_parent + 1 < node_par.sz_child) {
                for (ui32 i = idx_in_parent + 1; i < node_par.sz_child; i++) {
                    Leaf &child_cur = leafs[node_par.children[i]];
                    child_cur.idx_in_parent = i;
                }
            }
            for (ui32 i = 0; i < node_new.sz_child; i++) {
                Leaf &child_cur = leafs[node_new.children[i]];
                child_cur.idx_in_parent = i;
                child_cur.idx_parent = idx_node_new;
            }

            idx_cur = idx_parent;
            idx_other = idx_node_new;
            sz_other = node_new.total_size();
            ones_other = node_new.total_ones();
        }

        if (need_split) {
            ui32 idx_parent = nodes[idx_cur].idx_parent;
            while (idx_parent != idx_null && nodes[idx_parent].sz_child >= B) {
                ui32 idx_node_new = nodes.alloc();
                Node &node_new = nodes[idx_node_new];
                Node &node_par = nodes[idx_parent];

                node_par.sub_sz[idx_in_parent]++;
                node_par.sub_sz[idx_in_parent] -= sz_other;
                node_par.sub_ones[idx_in_parent] -= ones_other;

                Node::split_balance(&node_par, &node_new, idx_in_parent + 1, idx_other, sz_other, ones_other);
                node_new.is_leaf = false;
                if (idx_in_parent + 1 < node_par.sz_child) {
                    for (ui32 i = idx_in_parent + 1; i < node_par.sz_child; i++) {
                        Node &child_cur = nodes[node_par.children[i]];
                        child_cur.idx_in_parent = i;
                    }
                }
                for (ui32 i = 0; i < node_new.sz_child; i++) {
                    Node &child_cur = nodes[node_new.children[i]];
                    child_cur.idx_in_parent = i;
                    child_cur.idx_parent = idx_node_new;
                }


                idx_cur = idx_parent;
                idx_parent = node_par.idx_parent;
                idx_in_parent = node_par.idx_in_parent;
                idx_other = idx_node_new;
                sz_other = node_new.total_size();
                ones_other = node_new.total_ones();
            }
        }

        if (idx_parent == idx_null) {
            // changing the root
            ui32 idx_node_new = nodes.alloc();
            Node &new_root = nodes[idx_node_new];
            new_root.sz_child = 2;
            new_root.children[0] = idx_cur;
            new_root.children[1] = idx_other;
            new_root.sub_sz[0] = sub_sz + 1 - sz_other;
            new_root.sub_sz[1] = sz_other;
            new_root.sub_ones[0] = sub_ones + 1 - ones_other;
            new_root.sub_ones[1] = ones_other;
            nodes[idx_cur].idx_parent = idx_node_new;
            nodes[idx_cur].idx_in_parent = 0;
            nodes[idx_other].idx_parent = idx_node_new;
            nodes[idx_other].idx_in_parent = 1;

            root = idx_node_new;
        } else {
            Node &node_parent = nodes[idx_parent];
            node_parent.sub_sz[idx_in_parent]++;
            node_parent.sub_sz[idx_in_parent] -= sz_other;
            node_parent.sub_ones[idx_in_parent] -= ones_other;
            node_parent.insert_at(idx_in_parent + 1, idx_other, sz_other, ones_other);
            if (need_split) {
                for (ui32 i = idx_in_parent + 1; i < node_parent.sz_child; i++) {
                    nodes[node_parent.children[i]].idx_in_parent = i;
                }
            } else {
                for (ui32 i = idx_in_parent + 1; i < node_parent.sz_child; i++) {
                    leafs[node_parent.children[i]].idx_in_parent = i;
                }
            }
        }

        sub_sz++;
    }

public:
    DynamicBitvectorBTree() : root(idx_null), rng(std::random_device()()), dist(0, UINT32_MAX) {};
    ~DynamicBitvectorBTree() {};

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
        return sub_sz;
    }
    ui32 ones() const {
        return sub_ones;
    }

    // access bit
    bool get_bit(ui32 pos) const {
        if (pos >= size())
            throw std::out_of_range("get: pos out of range");

        ui32 pos_loc;
        ui32 idx = find_pos(pos, &pos_loc);
        return leafs[idx].bits.get_bit(pos_loc);
    }

    bool get_bit_rank(ui32 pos, ui32 *rank1) const {
        if (pos >= size())
            throw std::out_of_range("get: pos out of range");

        ui32 pos_loc;
        ui32 idx = find_pos(pos, &pos_loc, rank1);
        return leafs[idx].bits.get_bit(pos_loc);
    }

    void set_bit(ui32 pos, bool bit) {
        if (pos >= size())
            throw std::out_of_range("get: pos out of range");

        ui32 pos_loc;
        ui32 idx = bisect_pos(pos, &pos_loc);

        if (leafs[idx].bits.test_set_bit(pos_loc, bit))
            pull_propagate(idx);
    }

    // Compute the number of 1's in [0..pos)
    ui32 rank1(ui32 pos) const {
        if (pos >= size()) {
            return sub_ones;
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
        ui32 idx = bisect_pos(pos, &pos_loc);

        Leaf *leaf_cur = &leafs[idx];

        if (leaf_cur->bits.sz + 1 <= MAX_BITS) {
            // If there isn't overflow
            // Also handles when there's an empty node
            leaf_cur->bits.insert_at(pos_loc, bit);
            if (bit)
                pull_propagate_increase_ones(idx);
            else
                pull_propagate_increase_size(idx);
            return;
        }

        // If there is overflow.
        // Never balance with neighboring blocks, just split whenever
        // because if we balance with neighbor, then we need to update
        // the count for the neighbor which is annoying
        if (bit)
            insert_split_true(idx, pos_loc);
        else
            insert_split_false(idx, pos_loc);
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




#endif //__DYNAMIC_BITVECTOR_BTREE_H__
