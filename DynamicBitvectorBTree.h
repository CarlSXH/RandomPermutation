
#ifndef __DYNAMIC_BITVECTOR_BTREE_H__
#define __DYNAMIC_BITVECTOR_BTREE_H__


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

    struct NodeHeader {
        // Pointers within a tree
        ui32 idx_parent;
        ui32 idx_in_parent;
        ui32 idx_payload;
        bool is_leaf;

        NodeHeader() : idx_parent(idx_null), idx_in_parent(0), idx_payload(idx_null), is_leaf(false) {};
    };

    struct NodePayload {
        // For leafs it points to leaf indices
        ui32 children[B];
        ui32 sub_sz[B];
        ui32 sub_ones[B];

        ui32 sz_child;

        NodePayload() : sz_child(0) {};

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

        inline void copy_to(NodePayload *other, const ui32 &src_pos, const ui32 &dest_pos, const ui32 &s) const {
            for (ui32 i = 0; i < s; i++) {
                other->children[dest_pos + i] = children[src_pos + i];
                other->sub_sz[dest_pos + i] = sub_sz[src_pos + i];
                other->sub_ones[dest_pos + i] = sub_ones[src_pos + i];
            }
        }
        inline void set_child_vals(const ui32 &index, const ui32 &idx, const ui32 &sz, const ui32 &ones) {
            children[index] = idx;
            sub_sz[index] = sz;
            sub_ones[index] = ones;
        }
        inline void insert_at(const ui32 &index, const ui32 &idx, const ui32 &sz, const ui32 &ones) {
            for (ui32 i = sz_child; i > index; i--) {
                children[i] = children[i - 1];
                sub_sz[i] = sub_sz[i - 1];
                sub_ones[i] = sub_ones[i - 1];
            }
            set_child_vals(index, idx, sz, ones);
            sz_child++;
        }
        inline void remove_at(const ui32 &index) {
            for (ui32 i = index; i < sz_child - 1; i++) {
                children[i] = children[i + 1];
                sub_sz[i] = sub_sz[i + 1];
                sub_ones[i] = sub_ones[i + 1];
            }
            sz_child--;
        }
        inline void shift_right(const ui32 &s) {
            for (ui32 i = sz_child + s; i > s; i--) {
                children[i - 1] = children[i - 1 - s];
                sub_sz[i - 1] = sub_sz[i - 1 - s];
                sub_ones[i - 1] = sub_ones[i - 1 - s];
            }
        }
        inline void shift_left(const ui32 &s) {
            for (ui32 i = 0; i < sz_child - s; i++) {
                children[i] = children[i + s];
                sub_sz[i] = sub_sz[i + s];
                sub_ones[i] = sub_ones[i + s];
            }
        }
        static void split_balance(NodePayload *n1, NodePayload *n2, const ui32 &at, const ui32 &idx, const ui32 &sz, const ui32 &ones) {
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
                n1->copy_to(n2, sz1 - 1, 0, sz2);
                n1->set_child_vals(at, idx, sz, ones);
                n1->insert_at(at, idx, sz, ones);
            }
        }
    };

    struct LeafPayload {
        Datablock<MAX_WORDS> bits;
    };

    ContiguousAllocator<NodeHeader> node_headers;
    ContiguousAllocator<NodePayload> node_payloads;
    ContiguousAllocator<LeafPayload> leaf_payloads;

    ui32 root;
    ui32 sub_sz, sub_ones;


    // Find the index of the node header containing the pos'th
    // bit. out_pos_loc is a return value for the local index
    // of the bit inside this leaf.
    ui32 find_pos(ui32 pos, ui32 *out_pos_loc) const {
        if (pos >= size())
            throw std::out_of_range("");

        ui32 idx = root;
        while (!node_headers[idx].is_leaf) {
            const NodePayload &np = node_payloads[node_headers[idx].idx_payload];
            ui32 i;
            for (i = 0; i < np.sz_child; i++) {
                if (pos < np.sub_sz[i])
                    break;
                pos -= np.sub_sz[i];
            }
            idx = np.children[i];
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
        while (!node_headers[idx].is_leaf) {
            const NodePayload &np = node_payloads[node_headers[idx].idx_payload];
            ui32 i;
            for (i = 0; i < np.sz_child; i++) {
                if (pos < np.sub_sz[i])
                    break;
                pos -= np.sub_sz[i];
                ones += np.sub_ones[i];
            }
            idx = np.children[i];
        }

        *out_pos_loc = pos;
        *out_ones = ones + leaf_payloads[node_headers[idx].idx_payload].bits.rank1(pos);
        return idx;
    }

    template<int sz_change, int bit_change>
    void fixup_size_ones(const ui32 &idx) {
        const NodeHeader &header_orig = node_headers[idx];
        ui32 idx_parent = header_orig.idx_parent;
        ui32 idx_in = header_orig.idx_in_parent;
        while (idx_parent != idx_null) {
            const NodeHeader &header_cur = node_headers[idx_parent];
            NodePayload &node_cur = node_payloads[header_cur.idx_payload];
            node_cur.sub_sz[idx_in] += sz_change;
            node_cur.sub_ones[idx_in] += bit_change;
            idx_parent = header_cur.idx_parent;
            idx_in = header_cur.idx_in_parent;
        }
    }

    template<bool bit>
    void fixup_bal_sub(ui32 idx_leaf_more, ui32 sz_more_by, ui32 ones_more_by_1, ui32 idx_leaf_less, ui32 sz_less_by, ui32 ones_less_by) {
        while (idx_leaf_more != idx_leaf_less) {
            const NodeHeader &header_more = node_headers[idx_leaf_more];
            const NodeHeader &header_less = node_headers[idx_leaf_less];
            const NodeHeader &header_par_more = node_headers[header_more.idx_parent];
            const NodeHeader &header_par_less = node_headers[header_less.idx_parent];
            NodePayload &par_more = node_payloads[header_par_more.idx_payload];
            NodePayload &par_less = node_payloads[header_par_less.idx_payload];

            par_more.sub_sz[header_more.idx_in_parent] += sz_more_by;
            par_more.sub_ones[header_more.idx_in_parent] += ones_more_by_1;
            par_more.sub_ones[header_more.idx_in_parent]--;
            par_less.sub_sz[header_less.idx_in_parent] -= sz_less_by;
            par_less.sub_ones[header_less.idx_in_parent] -= ones_less_by;

            idx_leaf_more = header_par_more.idx_parent;
            idx_leaf_less = header_par_less.idx_parent;
        }
        
        if (bit)
            fixup_size_ones<-1, -1>(idx_leaf_more);
        else
            fixup_size_ones<-1, 0>(idx_leaf_more);
    }

    inline void insert_balance(ui32 idx_left, ui32 idx_right, ui32 idx_payload_left, ui32 idx_payload_right, ui32 idx_new_node, ui32 pos_insert, ui32 idx_other, ui32 sz_other, ui32 ones_other) {
        NodePayload &node_left = node_payloads[idx_payload_left];
        NodePayload &node_right = node_payloads[idx_payload_right];

        NodePayload::split_balance(&node_left, &node_right, pos_insert, idx_other, sz_other, ones_other);
        if (pos_insert < node_left.sz_child) {
            for (ui32 i = pos_insert; i < node_left.sz_child; i++) {
                NodeHeader &header_child = node_headers[node_left.children[i]];
                header_child.idx_in_parent = idx_left;
            }
        }
        for (ui32 i = 0; i < node_right.sz_child; i++) {
            NodeHeader &header_child = node_headers[node_right.children[i]];
            header_child.idx_in_parent = i;
            header_child.idx_parent = idx_right;
        }
    }

    template<bool bit>
    void insert_split(ui32 idx_leaf, ui32 pos_loc) {
        ui32 idx_other = node_headers.alloc();
        ui32 idx_cur = idx_leaf;
        ui32 sz_other;
        ui32 ones_other;
        {
            ui32 idx_right_leaf = leaf_payloads.alloc();
            NodeHeader &header_cur = node_headers[idx_leaf];
            NodeHeader &header_right = node_headers[idx_other];
            LeafPayload &leaf_cur = leaf_payloads[header_cur.idx_payload];
            LeafPayload &leaf_right = leaf_payloads[idx_right_leaf];

            Datablock<MAX_WORDS>::insert_balance(&leaf_cur.bits, &leaf_right.bits, pos_loc, bit);
            header_right.idx_parent = header_cur.idx_parent;
            header_right.is_leaf = true;
            header_right.idx_payload = idx_right_leaf;
            ll_insert_after(idx_leaf, idx_right_leaf);

            sz_other = leaf_right.bits.sz;
            ones_other = leaf_right.bits.ones;
        }

        while (node_headers[idx_cur].idx_parent != idx_null &&
               node_payloads[node_headers[node_headers[idx_cur].idx_parent].idx_payload].sz_child >= B) {

            ui32 idx_header_new = node_headers.alloc();
            ui32 idx_node_new = node_payloads.alloc();
            NodeHeader &header_new = node_headers[idx_header_new];
            NodePayload &node_new = node_payloads[idx_node_new];

            NodeHeader &header_cur = node_headers[idx_cur];
            NodeHeader &header_par = node_headers[header_cur.idx_parent];
            NodePayload &node_par = node_payloads[header_par.idx_payload];


            node_par.sub_sz[header_cur.idx_in_parent]++;
            node_par.sub_sz[header_cur.idx_in_parent] -= sz_other;
            if (bit)
                node_par.sub_ones[header_cur.idx_in_parent]++;
            node_par.sub_ones[header_cur.idx_in_parent] -= ones_other;

            header_new.is_leaf = false;
            header_new.idx_parent = header_cur.idx_in_parent;
            header_new.idx_payload = idx_node_new;

            insert_balance(header_cur.idx_parent, idx_header_new, header_par.idx_payload, idx_node_new, header_cur.idx_in_parent + 1, idx_other, sz_other, ones_other);

            idx_cur = header_cur.idx_parent;
            idx_other = idx_header_new;
            sz_other = node_new.total_size();
            ones_other = node_new.total_ones();
        }

        if (node_headers[idx_cur].idx_parent == idx_null) {
            // changing the root
            ui32 idx_header_new = node_headers.alloc();
            ui32 idx_node_new = node_payloads.alloc();
            NodeHeader &header_new = node_headers[idx_header_new];
            NodePayload &node_new = node_payloads[idx_node_new];
            node_new.sz_child = 2;
            node_new.children[0] = idx_cur;
            node_new.children[1] = idx_other;
            node_new.sub_sz[0] = sub_sz + 1 - sz_other;
            node_new.sub_sz[1] = sz_other;
            if (bit)
                node_new.sub_ones[0] = sub_ones + 1 - ones_other;
            else
                node_new.sub_ones[0] = sub_ones - ones_other;
            node_new.sub_ones[1] = ones_other;
            header_new.idx_parent = idx_null;
            header_new.idx_payload = idx_node_new;
            header_new.is_leaf = false;
            node_headers[idx_cur].idx_parent = idx_header_new;
            node_headers[idx_cur].idx_in_parent = 0;
            node_headers[idx_other].idx_parent = idx_header_new;
            node_headers[idx_other].idx_in_parent = 1;

            root = idx_header_new;
        } else {
            NodeHeader &header_parent = node_headers[node_headers[idx_cur].idx_parent];
            NodePayload &node_parent = node_payloads[header_parent.idx_payload];
            ui32 idx_in_parent = node_headers[idx_cur].idx_in_parent;
            node_parent.sub_sz[idx_in_parent]++;
            node_parent.sub_sz[idx_in_parent] -= sz_other;
            if (bit)
                node_parent.sub_ones[idx_in_parent]++;
            node_parent.sub_ones[idx_in_parent] -= ones_other;
            node_parent.insert_at(idx_in_parent + 1, idx_other, sz_other, ones_other);
            for (ui32 i = idx_in_parent + 1; i < node_parent.sz_child; i++)
                node_headers[node_parent.children[i]].idx_in_parent = i;
            node_headers[idx_other].idx_parent = node_headers[idx_cur].idx_parent;
            if (bit)
                fixup_size_ones<1, 1>(node_headers[idx_cur].idx_parent);
            else
                fixup_size_ones<1, 0>(node_headers[idx_cur].idx_parent);
        }
    }

    void balance_payload(NodePayload *left, NodePayload *right, ui32 idx_left, ui32 idx_right) {
        ui32 total_sz = left->sz_child + right->sz_child;
        ui32 right_sz = total_sz / 2;
        ui32 left_sz = total_sz - right_sz;

        if (right_sz < right->sz_child) {
            ui32 s = right->sz_child - right_sz;
            right->shift_right(s);
            left->copy_to(right, left_sz, 0, s);
            left->sz_child = left_sz;
            right->sz_child = right_sz;

            for (ui32 i = 0; i < right->sz_child; i++) {
                NodeHeader &header_child = right->children[i];
                header_child.idx_parent = idx_right;
                header_child.idx_in_parent = i;
            }
        } else if (right_sz > right->sz_child) {
            ui32 s = right_sz - right->sz_child;
            ui32 left_new_index = left->sz_child;
            right->copy_to(left, 0, left_new_index, s);
            right->left_shift(s);

            left->sz_child = left_sz;
            right->sz_child = right_sz;

            for (ui32 i = left_new_index; i < left->sz_child; i++) {
                NodeHeader &header_child = left->children[i];
                header_child.idx_parent = idx_left;
                header_child.idx_in_parent = i;
            }

            for (ui32 i = 0; i < right->sz_child; i++) {
                NodeHeader &header_child = right->children[i];
                header_child.idx_parent = idx_right;
                header_child.idx_in_parent = i;
            }
        }
    }

    void merge_left(NodePayload *left, NodePayload *right, ui32 idx_left) {
        right->copy_to(left, 0, left->sz_child, right->sz_child);
        ui32 update_loc = left->sz_child;
        left->sz_child += right->sz_child;
        for (ui32 i = update_loc; i < left->sz_child; i++) {
            NodeHeader &header_child = left->children[i];
            header_child.idx_parent = idx_left;
            header_child.idx_in_parent = i;
        }
    }

    void delete_node(ui32 idx_node) {
        NodeHeader &header_node = node_headers[idx_node];
        if (header_node.is_leaf)
            leaf_payloads.dealloc(header_node.idx_payload);
        else
            node_payloads.dealloc(header_node.idx_payload);
        node_headers.dealloc(idx_node);
    }

    template<int sz_change, int bit_change>
    void borrow_merge(ui32 idx_underflow) {

        while (node_payloads[node_headers[idx_underflow].idx_payload].sz_child < B) {
            NodeHeader &header_cur = node_headers[idx_underflow];

            if (header_cur.idx_parent == idx_null) {
                root = idx_underflow;
                return;
            }

            NodeHeader &header_par = node_headers[header_cur.idx_parent];
            NodePayload &node_par = node_payloads[header_par.idx_payload];
            NodePayload &node_cur = node_payloads[header_cur.idx_payload];

            bool has_right = header_cur.idx_in_parent + 1 < node_par.sz_child;
            bool has_left = header_cur.idx_in_parent > 0;
            NodeHeader *header_right = nullptr, *header_left = nullptr;
            NodePayload *node_right = nullptr, *node_left = nullptr;

            if (has_right) {
                header_right = &node_headers[node_par.children[header_cur.idx_in_parent + 1]];
                node_right = &node_payloads[header_right->idx_payload];
            }
            if (has_left) {
                header_left = &node_headers[node_par.children[header_cur.idx_in_parent - 1]];
                node_left = &node_payloads[header_left->idx_payload];
            }

            bool can_borrow_right = has_right && node_right->sz_child + node_cur.sz_child >= 2 * B;
            bool can_borrow_left = has_left &&node_left->sz_child + node_cur.sz_child >= 2 * B;
            if (can_borrow_right || can_borrow_left) {
                NodePayload *left, *right;
                ui32 idx_left, idx_right;
                ui32 idx_in_left;
                if (can_borrow_right) {
                    left = &node_cur;
                    right = node_right;
                    idx_left = idx_underflow;
                    idx_right = node_par.children[header_cur.idx_in_parent + 1];
                    idx_in_left = header_cur.idx_in_parent;
                } else {
                    left = node_left;
                    right = &node_cur;
                    idx_left = node_par.children[header_cur.idx_in_parent - 1];
                    idx_right = idx_underflow;
                    idx_in_left = header_cur.idx_in_parent - 1;
                }
                balance_payload(left, right, idx_left, idx_right);

                ui32 total_sz = node_par.sub_sz[idx_in_left] + node_par.sub_sz[idx_in_left + 1] + sz_change;
                ui32 total_ones = node_par.sub_ones[idx_in_left] + node_par.sub_ones[idx_in_left + 1] + bit_change;

                ui32 right_sz = right->total_size();
                ui32 right_ones = right->total_ones();
                node_par.sub_sz[idx_in_left] = total_sz - right_sz;
                node_par.sub_sz[idx_in_left + 1] = right_sz;
                node_par.sub_ones[idx_in_left] = total_ones - right_ones;
                node_par.sub_ones[idx_in_left + 1] = right_ones;
                break;
            }

            NodePayload *left, *right;
            ui32 idx_left, idx_delete;

            if (has_left) {
                left = node_left;
                right = &node_cur;
                idx_left = node_par.children[header_cur.idx_in_parent - 1];
                idx_delete = idx_underflow;
            } else {
                left = &node_cur;
                right = node_right;
                idx_left = idx_underflow;
                idx_delete = node_par.children[header_cur.idx_in_parent + 1];
            }
            merge_left(left, right, idx_left);
            delete_node(idx_delete);
            idx_underflow = idx_left;
        }

        fixup_size_ones<sz_change, bit_change>(idx_underflow);
    }

public:
    DynamicBitvectorBTree() : root(idx_null), sub_sz(0), sub_ones(0) {};
    ~DynamicBitvectorBTree() {};

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
        return leaf_payloads[node_headers[idx].idx_payload].bits.get_bit(pos_loc);
    }

    bool get_bit_rank(ui32 pos, ui32 *rank1) const {
        if (pos >= size())
            throw std::out_of_range("get: pos out of range");

        ui32 pos_loc;
        ui32 idx = find_pos(pos, &pos_loc, rank1);
        return leaf_payloads[node_headers[idx].idx_payload].bits.get_bit(pos_loc);
    }

    void set_bit(ui32 pos, bool bit) {
        if (pos >= size())
            throw std::out_of_range("get: pos out of range");

        ui32 pos_loc;
        ui32 idx = find_pos(pos, &pos_loc);

        LeafPayload &leaf = leaf_payloads[node_headers[idx].idx_payload];

        if (leaf.test_set_bit(pos_loc, bit)) {

            if (bit) {
                fixup_size_ones<0, 1>(idx);
                sub_ones++;
            } else {
                fixup_size_ones<0, -1>(idx);
                sub_ones--;
            }
        }
    }

    // Compute the number of 1's in [0..pos)
    ui32 rank1(ui32 pos) const {
        if (pos >= size()) {
            return sub_ones;
        }

        ui32 pos_loc;
        ui32 count;
        ui32 idx = find_pos(pos, &pos_loc, &count);

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
        ui32 idx = find_pos(pos, &pos_loc);

        NodeHeader &node_cur = node_headers[idx];
        LeafPayload &leaf_cur = leaf_payloads[node_cur.idx_payload];

        if (leaf_cur.bits.sz + 1 <= MAX_BITS) {
            // If there isn't overflow
            // Also handles when there's an empty node
            leaf_cur.bits.insert_at(pos_loc, bit);
            if (bit) {
                fixup_size_ones<1, 1>(idx);
                sub_ones++;
            } else {
                fixup_size_ones<1, 0>(idx);
            }
            sub_sz++;
            return;
        }

        // If there is overflow.
        // Never balance with neighboring blocks, just split whenever
        // because if we balance with neighbor, then we need to update
        // the count for the neighbor which is annoying
        if (bit) {
            insert_split<true>(idx, pos_loc);
            sub_ones++;
        } else
            insert_split<false>(idx, pos_loc);
        sub_sz++;
    }

    // erase bit at pos in [0..size())
    void erase(ui32 pos) {
        ui32 pos_loc;
        ui32 idx = find_pos(pos, &pos_loc);

        NodeHeader &header_cur = node_headers[idx];
        LeafPayload &leaf_cur = leaf_payloads[header_cur.idx_payload];
        const bool bit = leaf_cur.bits.get_bit(pos_loc);

        if (leaf_cur.bits.sz > MIN_BITS) {
            // if the current node has enough bits to delete one bit
            
            leaf_cur.bits.remove_at(pos_loc);
            if (bit) {
                fixup_size_ones<-1, -1>(idx);
                sub_ones--;
            }
            else
                fixup_size_ones<-1, 0>(idx);
            sub_sz--;
            return;
        }

        ui32 sz_cur = leaf_cur.bits.sz;
        ui32 ones_cur = leaf_cur.bits.ones;

        NodeHeader &header_par = node_headers[header_cur.idx_parent];
        NodePayload &node_par = node_payloads[header_par.idx_payload];
        ui32 next = idx_null, prev = idx_null;

        if (header_cur.idx_in_parent + 1 < node_par.sz_child)
            next = node_par.children[header_cur.idx_in_parent + 1];
        if (header_cur.idx_in_parent > 0)
            prev = node_par.children[header_cur.idx_in_parent - 1];

        //bool can_borrow = false;
        //ui32 idx_borrow_other = idx_null;
        //Datablock<MAX_WORDS> *bits_left = nullptr, *bits_right = nullptr;
        //Datablock<MAX_WORDS> *bits_other = nullptr;
        //ui32 sz_borrow_other = 0;
        //ui32 ones_borrow_other = 0;

        //if (next != idx_null) {
        //    LeafPayload &leaf_next = leaf_payloads[node_headers[next].idx_payload];
        //    if (leaf_next.bits.sz + sz_cur > 2 * MIN_BITS) {
        //        can_borrow = true;
        //        idx_borrow_other = next;
        //        bits_left = &leaf_cur.bits;
        //        bits_right = &leaf_next.bits;
        //        bits_other = bits_right;
        //        sz_borrow_other = leaf_next.bits.sz;
        //        ones_borrow_other = leaf_next.bits.ones;
        //    }
        //}
        //if (!can_borrow && prev != idx_null) {
        //    LeafPayload &leaf_prev = leaf_payloads[node_headers[prev].idx_payload];
        //    if (leaf_prev.bits.sz + sz_cur > 2 * MIN_BITS) {
        //        can_borrow = true;
        //        idx_borrow_other = prev;
        //        bits_left = &leaf_prev.bits;
        //        bits_right = &leaf_cur.bits;
        //        bits_other = bits_left;
        //        pos_loc += leaf_prev.bits.sz;
        //        sz_borrow_other = leaf_prev.bits.sz;
        //        ones_borrow_other = leaf_prev.bits.ones;
        //    }
        //}

        //if (can_borrow) {
        //    Datablock<MAX_WORDS>::delete_balance(bits_left, bits_right, pos_loc);

        //    if (bit) {
        //        fixup_bal_sub<true>(
        //            idx, leaf_cur.bits.sz - sz_cur, leaf_cur.bits.ones + 1 - ones_cur,
        //            idx_borrow_other, sz_borrow_other - bits_other->sz, ones_borrow_other - bits_other->ones);
        //    } else {
        //        fixup_bal_sub<false>(
        //            idx, leaf_cur.bits.sz - sz_cur, leaf_cur.bits.ones + 1 - ones_cur,
        //            idx_borrow_other, sz_borrow_other - bits_other->sz, ones_borrow_other - bits_other->ones);
        //    }
        //    return;
        //}

        if (next != idx_null) {
            LeafPayload &leaf_next = leaf_payloads[node_headers[next].idx_payload];
            if (leaf_next.bits.sz + sz_cur > 2 * MIN_BITS) {
                ui32 sz_next = leaf_next.bits.sz;
                ui32 ones_next = leaf_next.bits.ones;

                Datablock<MAX_WORDS>::delete_balance(&leaf_cur.bits, &leaf_next.bits, pos_loc);

                if (bit) {
                    fixup_bal_sub<true>(
                        idx, leaf_cur.bits.sz - sz_cur, leaf_cur.bits.ones + 1 - ones_cur,
                        next, sz_next - leaf_next.bits.sz, ones_next - leaf_next.bits.ones);
                } else {
                    fixup_bal_sub<false>(
                        idx, leaf_cur.bits.sz - sz_cur, leaf_cur.bits.ones + 1 - ones_cur,
                        next, sz_next - leaf_next.bits.sz, ones_next - leaf_next.bits.ones);
                }
                return;
            }
        }

        if (prev != idx_null) {
            LeafPayload &leaf_prev = leaf_payloads[node_headers[prev].idx_payload];
            if (leaf_prev.bits.sz + sz_cur > 2 * MIN_BITS) {

                ui32 sz_prev = leaf_prev.bits.sz;
                ui32 ones_prev = leaf_prev.bits.ones;

                Datablock<MAX_WORDS>::delete_balance(&leaf_prev.bits, &leaf_cur.bits, pos_loc + leaf_prev.bits.sz);

                if (bit) {
                    fixup_bal_sub<true>(
                        idx, leaf_cur.bits.sz - sz_cur, leaf_cur.bits.ones + 1 - ones_cur,
                        prev, sz_prev - leaf_prev.bits.sz, ones_prev - leaf_prev.bits.ones);
                } else {
                    fixup_bal_sub<false>(
                        idx, leaf_cur.bits.sz - sz_cur, leaf_cur.bits.ones + 1 - ones_cur,
                        prev, sz_prev - leaf_prev.bits.sz, sz_prev - leaf_prev.bits.ones);
                }
                return;
            }
        }

        if (next != idx_null || prev != idx_null) {
            LeafPayload *left, *right;
            auto func = Datablock<MAX_WORDS>::delete_merge_left;
            ui32 idx_in_other;
            ui32 idx_delete;
            if (next != idx_null) {
                left = &leaf_cur;
                right = &leaf_payloads[node_headers[next].idx_payload];
                idx_in_other = header_cur.idx_in_parent + 1;
                idx_delete = next;
            } else {
                left = &leaf_payloads[node_headers[prev].idx_payload];
                right = &leaf_cur;
                pos_loc += left->bits.sz;
                func = Datablock<MAX_WORDS>::delete_merge_right;
                idx_in_other = header_cur.idx_in_parent - 1;
                idx_delete = idx;
            }

            func(&left->bits, &right->bits, pos_loc);

            node_par.sub_sz[header_cur.idx_in_parent] += node_par.sub_sz[idx_other];
            node_par.sub_sz[header_cur.idx_in_parent]--;
            node_par.sub_ones[header_cur.idx_in_parent] += node_par.sub_ones[idx_in_other];
            if (bit)
                node_par.sub_ones[header_cur.idx_in_parent]--;
            node_par.remove_at(idx_in_other);
            delete_node(idx_delete);

            if (bit) {
                borrow_merge<true>(header_cur.idx_parent);
                sub_ones--;
            } else {
                borrow_merge<false>(header_cur.idx_parent);
            }
            sub_sz--;
            return;
        }

        leaf_cur.bits.remove_at(pos_loc);
        if (bit)
            sub_ones--;
        sub_sz--;
    }
};




#endif //__DYNAMIC_BITVECTOR_BTREE_H__
