

#ifndef __DYNAMIC_BITVECTOR_H__
#define __DYNAMIC_BITVECTOR_H__

#include <random>
#include <vector>


typedef uint32_t ui32;


// This class represents a collection of B bits.
// All operations will be linear in B implemented
// using bitwise operations.
template <unsigned int B>
struct Datablock {
    ui32 sz;       // number of bits in this block
    ui32 ones;     // number of 1-bits in this block

    // This field holds the actual stream of bits. Each
    // element of the array is a block of data. Within a
    // block, the less significant bits of a word comes
    // before the more significant bits. The blocks with
    // smaller indices will come before the blocks with
    // larger indices.
    uint64_t data[B];

    Datablock() : sz(0), ones(0) {
        std::fill(data, data + MAX_WORDS, 0);
    }

    // The index of the block in which contains the
    // pos'th bit.
    static inline ui32 block_index(ui32 pos) {
        return pos >> 6;
    }
    // The index of the bit within block that contains the
    // pos'th bit of this stream.
    static inline ui32 bit_index(ui32 pos) {
        return pos & 63;
    }

    // Get the pos'th bit in this stream.
    inline bool get_bit(ui32 pos) {
        ui32 w = block_index(pos);
        ui32 b = bit_index(pos);
        return (data[w] >> b) & 1U;
    }
    // Set the pos'th bit in this stream.
    inline void set_bit(ui32 pos, bool bit) {
        ui32 w = block_index(pos);
        ui32 b = bit_index(pos);
        if ((bool)((data[w] >> b) & 1U) == bit)
            return;

        if (bit) {
            ones++;
            data[w] |= (1ULL << b);
        } else {
            ones--;
            data[w] &= ~(1ULL << b);
        }
    }

    // Compute the number of 1's in w
    static inline ui32 ones_in_word(uint64_t w) {
        // bithack[n] is the number of 1's in
        // a 4 bit number n
        static const unsigned short bithack[16] = {
            0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4
        };
        ui32 count = 0;
        while (w != 0) {
            count += bithack[w & 0xF];
            w = w >> 4;
        }
        return count;
    }
    // Compute the number of 1's in w in the range [pos, pos+s)
    static inline ui32 ones_chunk(uint64_t w, ui32 pos, ui32 s) {
        w = w >> pos;
        // avoid undefined 1ULL << 64 problem
        if (s >= 64) return ones_in_word(w);
        w = w & ((1ULL << s) - 1);
        return ones_in_word(w);
    }
    // Compute the number of 1's in the s bits in the back,
    // or the s most significant bits.
    static inline ui32 ones_trailing(uint64_t w, ui32 s) {
        return ones_chunk(w, 64ULL - s, s);
    }
    // Compute the number of 1's in the s bits in the front,
    // or the s least significant bits.
    static inline ui32 ones_leading(uint64_t w, ui32 s) {
        return ones_chunk(w, 0, s);
    }

    ui32 ones_count(ui32 start_pos, ui32 s) {
        ui32 count = 0;
        ui32 start_w = block_index(start_pos);
        ui32 start_b = bit_index(start_pos);
        ui32 end_w = block_index(start_pos + s);
        ui32 end_b = bit_index(start_pos + s);

        if (start_w == end_w)
            return ones_chunk(data[start_w], start_b, end_b - start_b);

        count += ones_trailing(data[start_w], 64-start_b);
        for (ui32 i = start_w; i < end_w; i++)
            count += ones_in_word(data[i]);
        if (end_b > 0)
            count += ones_leading(data[end_b], end_b);

        return count;
    }

    // Insert the bit in the pos'th position in the stream
    void insert_at(ui32 pos, bool bit) {
        // shift all the block after the block that contains the pos'th bit
        ui32 w = block_index(pos);
        for (ui32 i = 0; block_index(sz) - i > w; i++) {
            ui32 curIndex = block_index(sz) - i;
            data[curIndex] = (data[curIndex] << 1) | (data[curIndex - 1] >> 63);
        }
        // inserting the bit in the block containing the pos'th bit
        ui32 b = bit_index(pos);
        uint64_t fm = (1ULL<<b) - 1; // mask for the fixed bits
        uint64_t sm = ~fm;           // mask for the shift bits
        auto word = data[w];
        data[w] = ((word & sm) << 1) | (word & fm);
        if (bit) {
            data[w] = data[w] | (1ULL << b);
            ones++;
        }
        sz++;
    }

    // Remove the pos'th bit in the stream
    void remove_at(ui32 pos) {
        // Update count before it gets destroyed
        if (get_bit(pos))
            ones--;

        // First remove the bit from the block containing it
        ui32 w = block_index(pos);
        ui32 b = bit_index(pos);
        uint64_t fm = (1ULL << b) - 1; // mask for the fixed bits
        uint64_t sm = ~fm;             // mask for the shift bits
        sm = sm & (sm - 1);            // throw away the bit we remove
        auto word = data[w];
        data[w] = ((word & sm) >> 1) | (word & fm);

        // Repeatedly append the last bit of this block as the
        // first bit in the previous block, then shift this
        // entire block. block_index(sz-1) isis the block index
        // containing the previous last bit in the stream
        for (ui32 i = w + 1; i <= block_index(sz - 1); i++) {
            data[i - 1] = data[i - 1] | ((data[i] & 1ULL) << 63);
            data[i] = data[i] >> 1;
        }
        sz--;
    }

    ui32 copy_to(Datablock *dest, ui32 src_pos, ui32 dest_pos, ui32 n) {
        for (int i = 0; i < n; i++)
            dest->set_bit(dest_pos + i, get_bit(src_pos + i));
    }
    void remove_front(ui32 n);
    void remove_back(ui32 n);

    static void split_block(const Datablock *src, Datablock *b1, Datablock *b2, ui32 at);
    static void append_block(Datablock *src, Datablock *append);
    static void balance_block(Datablock *b1, Datablock *b2);
    static void insert_block(Datablock *b, ui32 at, bool bit);

    static bool insert_balance(Datablock *b1, Datablock *b2, ui32 at, bool bit) {

    }
    static bool delete_balance(Datablock *b1, Datablock *b2, ui32 at) {

    }
    static bool delete_merge_left(Datablock *b1, Datablock *b2, ui32 at) {

    }
    static bool delete_merge_right(Datablock *b1, Datablock *b2, ui32 at) {

    }
};




template <unsigned int B>
class DynamicBitvector {
public:


public:
    static constexpr ui32 idx_null = SIZE_MAX;

    static constexpr ui32 MAX_BITS = (4 * B + 1) / 3; // this is ceil( (4B-1)/3 )
    static constexpr ui32 MIN_BITS = (2 * B + 1) / 3; // this is floor( (2B+1)/3 )
    static constexpr ui32 MAX_WORDS = (MAX_BITS + 63) / 64; // ceil(MAX_BITS / 64)
    static constexpr ui32 MIN_WORDS = (MIN_BITS + 63) / 64; // ceil(MIN_BITS / 64)

    struct Node {
        ui32 parent;
        ui32 left;
        ui32 right;
        ui32 prev;         // doubly-linked list of data blocks
        ui32 next;
        ui32 sub_sz;       // total bits in subtree, including this block
        ui32 sub_ones;     // total 1-bits in subtree, including this block
        ui32 prio;       // random heap priority for treap
        Datablock<MAX_WORDS> bits;
        Node() : parent(idx_null), left(idx_null), right(idx_null), prev(idx_null), next(idx_null), sub_sz(0), sub_ones(0), bits() {};
    };

    std::vector<Node> nodes;
    std::vector<ui32> free_list;
    ui32 root;
    std::mt19937_64 rng;

    // allocate or recycle a node
    ui32 alloc_node() {
        ui32 idx;
        if (!free_list.empty()) {
            idx = free_list.back(); free_list.pop_back();
            nodes[idx] = Node();
        } else {
            idx = nodes.size();
            nodes.emplace_back();
        }
        Node &n = nodes[idx];
        n.prio = rng();

        return idx;
    }

    void dealloc_node(ui32 idx) {
        free_list.push_back(idx);
    }

    // recompute subtree aggregates
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

    void pull_propagate(ui32 idx) {
        while (idx != idx_null) {
            pull(idx);
            idx = nodes[idx].parent;
        }
    }

    // rotate right at idx
    ui32 rotate_right(ui32 idx) {
        ui32 L = nodes[idx].left;
        nodes[idx].left = nodes[L].right;
        if (nodes[idx].left != idx_null)
            nodes[nodes[idx].left].parent = idx;
        nodes[L].right = idx;
        nodes[L].parent = nodes[idx].parent;
        if (nodes[nodes[idx].parent].left == idx)
            nodes[nodes[idx].parent].left = L;
        else
            nodes[nodes[idx].parent].right = L;
        nodes[idx].parent = L;

        pull(idx);
        pull(L);
        return L;
    }
    // rotate left at idx
    ui32 rotate_left(ui32 idx) {
        ui32 R = nodes[idx].right;
        nodes[idx].right = nodes[R].left;
        if (nodes[idx].right != idx_null)
            nodes[nodes[idx].right].parent = idx;
        nodes[R].left = idx;
        nodes[R].parent = nodes[idx].parent;
        if (nodes[nodes[idx].parent].left == idx)
            nodes[nodes[idx].parent].left = R;
        else
            nodes[nodes[idx].parent].right = R;
        nodes[idx].parent = R;

        pull(idx);
        pull(R);
        return R;
    }

    ui32 detach(ui32 idx) {
        if (idx == idx_null)
            return idx_null;
        
        const Node node_cur = nodes[idx];
        if (node_cur.left == idx_null) {
            
        }
    }

    ui32 bubble(ui32 idx) {
        if (idx == idx_null)
            return idx_null;

        Node &node_cur = nodes[idx];
        if (node_cur.left != idx_null && nodes[node_cur.left].prio > node_cur.prio)
            return rotate_right(idx);
        else if (node_cur.right != idx_null && nodes[node_cur.right].prio > node_cur.prio)
            return rotate_left(idx);
    }

    void bubble_pull_propagate(ui32 idx) {
        while (idx != idx_null) {
            pull(idx);
            idx = bubble(idx);
            idx = nodes[idx].parent;
        }
    }



    void ll_insert_after(ui32 at, ui32 idx) {
        Node &node_at = nodes[at];
        Node &node_insert = nodes[idx];

        node_insert.next = node_at.next;
        node_insert.prev = at;

        if (node_at.next != idx_null)
            nodes[node_at.next].prev = idx;
        node_at.next = idx;
    }

    void ll_detach(ui32 idx) {
        if (idx == idx_null)
            return;
        const Node &node_cur = nodes[idx];
        if (node_cur.prev != idx_null)
            nodes[node_cur.prev].next = node_cur.next;
        if (node_cur.next != idx_null)
            nodes[node_cur.next].prev = node_cur.prev;
    }


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

    ui32 bisect_pos(ui32 pos, ui32 *pos_loc) {
        if (pos >= size())
            throw std::out_of_range("");

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

        *pos_loc = pos - n.sz;
        return idx;
    }

    ui32 bisect_pos(ui32 pos, ui32 *pos_loc, ui32 *out_ones) {
        if (pos >= size())
            throw std::out_of_range("");

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

        *pos_loc = pos - n.sz;
        *out_ones = ones + nodes[idx].bits.ones_count(0, pos_loc);
        return idx;
    }

    ui32 bisect_ones(ui32 pos, ui32 *pos_loc) {
        if (pos >= size())
            throw std::out_of_range("");

        ui32 idx = root;
        ui32 count = 0;

        while (idx != idx_null) {
            const auto &n = nodes[idx];
            ui32 ls = (n.left == idx_null ? 0 : nodes[n.left].sub_sz);
            ui32 lo = (n.left == idx_null ? 0 : nodes[n.left].sub_ones);
            if (pos < ls) {
                idx = n.left;
                continue;
            }
            if (pos == ls)
                return count + lo;
            pos -= ls;
            count += lo;
            if (pos >= n.bits.sz) {
                pos -= n.bits.sz;
                count += n.bits.ones;
                idx = n.right;
                continue;

            }
            break;
        }
    }

public:
    DynamicBitvector() : root(idx_null), rng(std::random_device{}()) {};
    ~DynamicBitvector();

    // total number of bits
    ui32 size() const {
        return (root == idx_null ? 0 : nodes[root].sub_sz);
    }

    // access bit
    bool get_bit(ui32 pos) const {
        if (pos >= size())
            throw std::out_of_range("get: pos out of range");

        ui32 pos_loc;
        ui32 idx = bisect_pos(pos, &pos_loc);
        return nodes[idx].bits.get_bit(pos_loc);
    }

    void set_bit(ui32 pos, bool bit) const {
        if (pos >= size())
            throw std::out_of_range("get: pos out of range");

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

    // insert bit at pos in [0..size()]
    void insert(ui32 pos, bool bit) {
        ui32 pos_loc;
        ui32 idx = bisect_pos(pos, &pos_loc);
        Node &node_cur = nodes[idx];
        ui32 sz_left = (node_cur.left == idx_null ? 0 : nodes[node_cur.left].sub_sz);
        ui32 sz_cur = node_cur.bits.sz;

        if (node_cur.bits.sz + 1 <= MAX_BITS) {
            // If there isn't overflow
            node_cur.bits.insert_at(pos_loc, bit);
            pull_propagate(idx_cur);
            return;
        }

        // If there is overflow.
        // Never balance with neighboring blocks, just split whenever
        // because if we balance with neighbor, then we need to update
        // the count for the neighbor which is annoying

        ui32 idx_right = alloc_node();
        auto &node_right = nodes[idx_right];

        Datablock<MAX_WORDS>::insert_balance(&node_cur.bits, &node_right.bits, pos_loc, bit);

        // fix linked list
        ll_insert_after(idx_cur, idx_right);

        node_cur.right = insert_min(node_cur.right, idx_right);

        // technically not needed, but logic is scrambled, hard to remove it
        // based on what each function does
        pull(idx_cur); 

        bubble_pull_propagate(idx_cur);
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
            pull_propagate(node_cur);
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

            Datablock<MAX_WORDS>::delete_balance(&node_prev.bits, &node_cur.bits, pos_loc);

            // If the left subtree of the current node is not empty,
            // then the in order predecessor is in the left subtree,
            // in which case one pull propagate on the predecessor is
            // enough. If the left subtree is empty, however, then
            // the predecessor must be one of the ancestors of the
            // current node, in which one pull propagate on the
            // current node is enough.
            if (node_cur.left != idx_null)
                pull_propagate(node_prev.prev);
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
                // if the successor is in the right subtree of node_cur
                // then we will merge to the current node
                Datablock<MAX_WORDS>::delete_merge_left(&node_cur.bits, &node_next.bits, pos_loc);
                ll_detach(idx_next);
                idx_next = rotate_left(idx_next);
                nodes[idx_next].left = idx_null;
                pull_propagate(idx_next);
                return;
            }

            // If the right subtree is empty, the successor must be
            // one of its ancestor. In which case we merge to the ancestor
            // and remove the current node.
            Datablock<MAX_WORDS>::delete_merge_right(&node_cur.bits, &node_next.bits, pos_loc);
            ll_detach(idx);
            ui32 idx = rotate_right(idx);
            nodes[idx].right = idx_null;
            pull_propagate(idx);
            return;
        }

        if (node_cur.prev != idx_null) {
            // merge with the in order predecessor
            ui32 idx_prev = node_cur.prev;
            Node &node_prev = nodes[idx_prev];
            if (node_cur.left != idx_null) {
                // if the predecessor is in the left subtree of node_cur
                // then we will merge to the current node
                Datablock<MAX_WORDS>::delete_merge_right(&node_prev.bits, &node_cur.bits, pos_loc);
                ll_detach(idx_prev);
                idx_next = rotate_right(idx_prev);
                nodes[idx_prev].left = idx_null;
                pull_propagate(idx_prev);
                return;
            }

            // If the right subtree is empty, the successor must be
            // one of its ancestor. In which case we merge to the ancestor
            // and remove the current node.
            Datablock<MAX_WORDS>::delete_merge_right(&node_prev.bits, &node_cur.bits, pos_loc);
            ll_detach(idx);
            idx = rotate_right(idx);
            nodes[idx].right = idx_null;
            pull_propagate(idx);
            return;
        }

        // the last case has to be true, or there's an error



    }
};



#endif // __DYNAMIC_BITVECTOR_H__