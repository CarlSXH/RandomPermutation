
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

public:
    static constexpr ui32 idx_null = UINT32_MAX;

    static constexpr ui32 MAX_BITS = 2 * LeafBits;
    static constexpr ui32 MIN_BITS = LeafBits / 2;
    static constexpr ui32 MAX_WORDS = (MAX_BITS + 63) / 64; // ceil(MAX_BITS / 64)
    static constexpr ui32 MIN_WORDS = (MIN_BITS + 63) / 64; // ceil(MIN_BITS / 64)

    static constexpr ui32 MAX_CHILD = B;
    static constexpr ui32 MIN_CHILD = (B+1) / 2;

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
        struct NodeStats {
            ui32 idx, sz, ones;
        };
        NodeStats children[B];

        ui32 sz_child;

        NodePayload() : sz_child(0) {};

        inline ui32 total_size() const {
            ui32 count = 0;
            for (ui32 i = 0; i < sz_child; i++)
                count += children[i].sz;
            return count;
        }
        inline ui32 total_ones() const {
            ui32 count = 0;
            for (ui32 i = 0; i < sz_child; i++)
                count += children[i].ones;
            return count;
        }

        inline void copy_to(NodePayload *other, ui32 src_pos, ui32 dest_pos, ui32 s) const {
            for (ui32 i = 0; i < s; i++) {
                other->children[dest_pos + i] = children[src_pos + i];
            }
        }
        inline void set_child_vals(ui32 index, ui32 idx, ui32 sz, ui32 ones) {

            children[index].idx = idx;
            children[index].sz = sz;
            children[index].ones = ones;
        }
        inline void insert_at(ui32 index, ui32 idx, ui32 sz, ui32 ones) {
            for (ui32 i = sz_child; i > index; i--) {
                children[i] = children[i - 1];
            }
            set_child_vals(index, idx, sz, ones);
            sz_child++;
        }
        inline void remove_at(ui32 index) {
            for (ui32 i = index; i < sz_child - 1; i++) {
                children[i] = children[i + 1];
            }
            sz_child--;
        }
        inline void shift_right(ui32 s) {
            for (ui32 i = sz_child + s; i > s; i--) {
                children[i - 1] = children[i - 1 - s];
            }
        }
        inline void shift_left(ui32 s) {
            for (ui32 i = 0; i < sz_child - s; i++) {
                children[i] = children[i + s];
            }
        }
        static void split_balance(NodePayload *n1, NodePayload *n2, ui32 at, ui32 idx, ui32 sz, ui32 ones) {
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


    ui32 find_pos(ui32 pos, ui32 *out_pos_loc) const {
        if (pos > size())
            throw std::out_of_range("");

        ui32 idx = root;
        NodeHeader const * header_cur = &node_headers[idx];
        while (!header_cur->is_leaf) {
            const NodePayload &np = node_payloads[header_cur->idx_payload];
            ui32 i;
            // shift one to make sure the end is included
            for (i = 0; i+1 < np.sz_child; i++) {
                if (pos < np.children[i].sz)
                    break;
                pos -= np.children[i].sz;
            }
            idx = np.children[i].idx;
            header_cur = &node_headers[idx];
        }

        *out_pos_loc = pos;
        return idx;
    }

    ui32 find_pos(ui32 pos, ui32 *out_pos_loc, ui32 *out_ones) const {
        if (pos >= size())
            throw std::out_of_range("");

        ui32 ones = 0;
        ui32 idx = root;
        while (!node_headers[idx].is_leaf) {
            const NodePayload &np = node_payloads[node_headers[idx].idx_payload];
            ui32 i;
            for (i = 0; i < np.sz_child; i++) {
                if (pos < np.children[i].sz)
                    break;
                pos -= np.children[i].sz;
                ones += np.children[i].ones;
            }
            idx = np.children[i].idx;
        }

        *out_pos_loc = pos;
        *out_ones = ones + leaf_payloads[node_headers[idx].idx_payload].bits.rank1(pos);
        return idx;
    }

    bool internal_check(ui32 idx, ui32 *out_sz, ui32 *out_ones) const {
        if (idx == idx_null)
            return true;

        const NodeHeader &header_cur = node_headers[idx];
        if (header_cur.is_leaf) {
            const LeafPayload &leaf_cur = leaf_payloads[header_cur.idx_payload];
            *out_sz = leaf_cur.bits.sz;
            *out_ones = leaf_cur.bits.ones;
            return true;
        }

        const NodePayload &node_cur = node_payloads[header_cur.idx_payload];
        ui32 total_sz = 0, total_ones = 0;
        for (ui32 i = 0; i < node_cur.sz_child; i++) {
            ui32 cur_sz, cur_ones;
            if (!internal_check(node_cur.children[i].idx, &cur_sz, &cur_ones)) {
                return false;
            }
            if (cur_sz != node_cur.children[i].sz || cur_ones != node_cur.children[i].ones) {
                return false;
            }
            const NodeHeader &header_child = node_headers[node_cur.children[i].idx];
            if (header_child.idx_parent != idx)
                return false;
            if (header_child.idx_in_parent != i)
                return false;
            total_sz += cur_sz;
            total_ones += cur_ones;
        }
        *out_sz = total_sz;
        *out_ones = total_ones;
        return true;
    }
    bool check() const {
        ui32 out_sz, out_ones;
        bool result = internal_check(root, &out_sz, &out_ones);
        return out_sz == sub_sz && out_ones == sub_ones;
    }

    template<int sz_change, int bit_change>
    void fixup_size_ones(ui32 idx) {
        const NodeHeader &header_orig = node_headers[idx];
        ui32 idx_parent = header_orig.idx_parent;
        ui32 idx_in = header_orig.idx_in_parent;
        while (idx_parent != idx_null) {
            const NodeHeader &header_cur = node_headers[idx_parent];
            NodePayload &node_cur = node_payloads[header_cur.idx_payload];
            node_cur.children[idx_in].sz += sz_change;
            node_cur.children[idx_in].ones += bit_change;
            idx_parent = header_cur.idx_parent;
            idx_in = header_cur.idx_in_parent;
        }
    }

    template<bool bit>
    void fixup_bal_sub(ui32 idx_leaf_more, ui32 sz_more_by, ui32 ones_more_by, ui32 idx_leaf_less, ui32 sz_less_by, ui32 ones_less_by) {
        while (idx_leaf_more != idx_leaf_less) {
            const NodeHeader &header_more = node_headers[idx_leaf_more];
            const NodeHeader &header_less = node_headers[idx_leaf_less];
            const NodeHeader &header_par_more = node_headers[header_more.idx_parent];
            const NodeHeader &header_par_less = node_headers[header_less.idx_parent];
            NodePayload &par_more = node_payloads[header_par_more.idx_payload];
            NodePayload &par_less = node_payloads[header_par_less.idx_payload];

            par_more.children[header_more.idx_in_parent].sz += sz_more_by;
            par_more.children[header_more.idx_in_parent].ones += ones_more_by;
            par_less.children[header_less.idx_in_parent].sz -= sz_less_by;
            par_less.children[header_less.idx_in_parent].ones -= ones_less_by;

            idx_leaf_more = header_more.idx_parent;
            idx_leaf_less = header_less.idx_parent;
        }
        
        if (bit)
            fixup_size_ones<-1, -1>(idx_leaf_more);
        else
            fixup_size_ones<-1, 0>(idx_leaf_more);
    }

    inline void insert_balance(ui32 idx_left, ui32 idx_right, ui32 idx_payload_left, ui32 idx_payload_right, ui32 pos_insert, ui32 idx_other, ui32 sz_other, ui32 ones_other) {
        NodePayload &node_left = node_payloads[idx_payload_left];
        NodePayload &node_right = node_payloads[idx_payload_right];

        NodePayload::split_balance(&node_left, &node_right, pos_insert, idx_other, sz_other, ones_other);
        if (pos_insert < node_left.sz_child) {
            for (ui32 i = pos_insert; i < node_left.sz_child; i++) {
                NodeHeader &header_child = node_headers[node_left.children[i].idx];
                header_child.idx_in_parent = i;
            }
        }
        for (ui32 i = 0; i < node_right.sz_child; i++) {
            NodeHeader &header_child = node_headers[node_right.children[i].idx];
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

            sz_other = leaf_right.bits.sz;
            ones_other = leaf_right.bits.ones;
        }

        while (node_headers[idx_cur].idx_parent != idx_null &&
               node_payloads[node_headers[node_headers[idx_cur].idx_parent].idx_payload].sz_child >= MAX_CHILD) {

            ui32 idx_header_new = node_headers.alloc();
            ui32 idx_node_new = node_payloads.alloc();
            NodeHeader &header_new = node_headers[idx_header_new];
            NodePayload &node_new = node_payloads[idx_node_new];

            NodeHeader &header_cur = node_headers[idx_cur];
            NodeHeader &header_par = node_headers[header_cur.idx_parent];
            NodePayload &node_par = node_payloads[header_par.idx_payload];

            ui32 orig_parent = header_cur.idx_parent;


            node_par.children[header_cur.idx_in_parent].sz++;
            node_par.children[header_cur.idx_in_parent].sz -= sz_other;
            if (bit)
                node_par.children[header_cur.idx_in_parent].ones++;
            node_par.children[header_cur.idx_in_parent].ones -= ones_other;

            header_new.is_leaf = false;
            header_new.idx_parent = header_par.idx_parent;
            header_new.idx_payload = idx_node_new;

            insert_balance(orig_parent, idx_header_new, header_par.idx_payload, idx_node_new, header_cur.idx_in_parent + 1, idx_other, sz_other, ones_other);

            idx_cur = orig_parent;
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
            node_new.children[0].idx = idx_cur;
            node_new.children[1].idx = idx_other;
            node_new.children[0].sz = sub_sz + 1 - sz_other;
            node_new.children[1].sz = sz_other;
            if (bit)
                node_new.children[0].ones = sub_ones + 1 - ones_other;
            else
                node_new.children[0].ones = sub_ones - ones_other;
            node_new.children[1].ones = ones_other;
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
            node_parent.children[idx_in_parent].sz++;
            node_parent.children[idx_in_parent].sz -= sz_other;
            if (bit)
                node_parent.children[idx_in_parent].ones++;
            node_parent.children[idx_in_parent].ones -= ones_other;
            node_parent.insert_at(idx_in_parent + 1, idx_other, sz_other, ones_other);
            for (ui32 i = idx_in_parent + 1; i < node_parent.sz_child; i++)
                node_headers[node_parent.children[i].idx].idx_in_parent = i;
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

        if (right_sz > right->sz_child) {
            ui32 s = right_sz - right->sz_child;
            right->shift_right(s);
            left->copy_to(right, left_sz, 0, s);
            left->sz_child = left_sz;
            right->sz_child = right_sz;

            for (ui32 i = 0; i < right->sz_child; i++) {
                NodeHeader &header_child = node_headers[right->children[i].idx];
                header_child.idx_parent = idx_right;
                header_child.idx_in_parent = i;
            }
        } else if (right_sz < right->sz_child) {
            ui32 s = right->sz_child - right_sz;
            ui32 left_new_index = left->sz_child;
            right->copy_to(left, 0, left_new_index, s);
            right->shift_left(s);

            left->sz_child = left_sz;
            right->sz_child = right_sz;

            for (ui32 i = left_new_index; i < left->sz_child; i++) {
                NodeHeader &header_child = node_headers[left->children[i].idx];
                header_child.idx_parent = idx_left;
                header_child.idx_in_parent = i;
            }

            for (ui32 i = 0; i < right->sz_child; i++) {
                NodeHeader &header_child = node_headers[right->children[i].idx];
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
            NodeHeader &header_child = node_headers[left->children[i].idx];
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

    void remove_at(NodePayload &node, ui32 pos_removed) {
        node.remove_at(pos_removed);
        for (ui32 i = pos_removed; i < node.sz_child; i++) {
            NodeHeader &header_child = node_headers[node.children[i].idx];
            header_child.idx_in_parent = i;
        }
    }

    template<int sz_change, int bit_change>
    void borrow_merge(ui32 idx_underflow) {

        while (node_payloads[node_headers[idx_underflow].idx_payload].sz_child < MIN_CHILD) {
            NodeHeader &header_cur = node_headers[idx_underflow];
            ui32 idx_next = header_cur.idx_parent;

            if (idx_next == idx_null) {
                // it's fine for root node to underflow
                break;
            }

            NodeHeader &header_par = node_headers[header_cur.idx_parent];


            NodePayload &node_par = node_payloads[header_par.idx_payload];
            NodePayload &node_cur = node_payloads[header_cur.idx_payload];

            bool has_right = header_cur.idx_in_parent + 1 < node_par.sz_child;
            bool has_left = header_cur.idx_in_parent > 0;
            NodeHeader *header_right = nullptr, *header_left = nullptr;
            NodePayload *node_right = nullptr, *node_left = nullptr;

            if (has_right) {
                header_right = &node_headers[node_par.children[header_cur.idx_in_parent + 1].idx];
                node_right = &node_payloads[header_right->idx_payload];
            }
            if (has_left) {
                header_left = &node_headers[node_par.children[header_cur.idx_in_parent - 1].idx];
                node_left = &node_payloads[header_left->idx_payload];
            }

            bool can_borrow_right = has_right && node_right->sz_child + node_cur.sz_child >= 2 * MIN_CHILD;
            bool can_borrow_left = has_left && node_left->sz_child + node_cur.sz_child >= 2 * MIN_CHILD;
            if (can_borrow_right || can_borrow_left) {
                NodePayload *left, *right;
                ui32 idx_left, idx_right;
                ui32 idx_in_left;
                if (can_borrow_right) {
                    left = &node_cur;
                    right = node_right;
                    idx_left = idx_underflow;
                    idx_right = node_par.children[header_cur.idx_in_parent + 1].idx;
                    idx_in_left = header_cur.idx_in_parent;
                } else {
                    left = node_left;
                    right = &node_cur;
                    idx_left = node_par.children[header_cur.idx_in_parent - 1].idx;
                    idx_right = idx_underflow;
                    idx_in_left = header_cur.idx_in_parent - 1;
                }
                balance_payload(left, right, idx_left, idx_right);

                ui32 total_sz = node_par.children[idx_in_left].sz + node_par.children[idx_in_left + 1].sz + sz_change;
                ui32 total_ones = node_par.children[idx_in_left].ones + node_par.children[idx_in_left + 1].ones + bit_change;

                ui32 right_sz = right->total_size();
                ui32 right_ones = right->total_ones();
                node_par.children[idx_in_left].sz = total_sz - right_sz;
                node_par.children[idx_in_left + 1].sz = right_sz;
                node_par.children[idx_in_left].ones = total_ones - right_ones;
                node_par.children[idx_in_left + 1].ones = right_ones;
                idx_underflow = idx_next;
                break;
            }

            NodePayload *left, *right;
            ui32 idx_left, idx_delete;
            ui32 pos_left;

            if (has_left) {
                left = node_left;
                right = &node_cur;
                idx_left = node_par.children[header_cur.idx_in_parent - 1].idx;
                idx_delete = idx_underflow;
                pos_left = header_cur.idx_in_parent - 1;
            } else if (has_right) {
                left = &node_cur;
                right = node_right;
                idx_left = idx_underflow;
                idx_delete = node_par.children[header_cur.idx_in_parent + 1].idx;
                pos_left = header_cur.idx_in_parent;
            } else {
                header_cur.idx_parent = idx_null;
                delete_node(root);
                root = idx_underflow;
                break;
            }
            node_par.children[pos_left].sz += sz_change;
            node_par.children[pos_left].sz += node_par.children[pos_left + 1].sz;
            node_par.children[pos_left].ones += bit_change;
            node_par.children[pos_left].ones += node_par.children[pos_left + 1].ones;

            merge_left(left, right, idx_left);
            delete_node(idx_delete);

            remove_at(node_par, pos_left + 1);
            idx_underflow = idx_next;
        }

        fixup_size_ones<sz_change, bit_change>(idx_underflow);
    }

    static void calc_size(ui32 sz, ui32 MAX, ui32 *out_count, ui32 *out_sz, ui32 *out_more) {
        ui32 k = (sz + MAX - 1) / MAX;
        if (k < 1)
            k = 1;
        ui32 q, r;
        q = sz / k;
        r = sz % k;
        *out_sz = q;
        *out_count = k;
        *out_more = r;
    }

public:
    DynamicBitvectorBTree() : root(idx_null), sub_sz(0), sub_ones(0) {
        ui32 idx_header_root = node_headers.alloc();
        ui32 idx_node_root = leaf_payloads.alloc();

        NodeHeader &header_root = node_headers[idx_header_root];
        LeafPayload &node_root = leaf_payloads[idx_node_root];

        header_root.idx_payload = idx_node_root;
        header_root.idx_in_parent = 0;
        header_root.idx_parent = idx_null;
        header_root.is_leaf = true;

        root = idx_header_root;
    };
    ~DynamicBitvectorBTree() {};

    // total number of bits
    ui32 size() const {
        return sub_sz;
    }
    ui32 ones() const {
        return sub_ones;
    }

    void reserve(ui32 bit_sz) {
        ui32 leaf_count = (bit_sz + MIN_BITS - 1) / MIN_BITS;
        leaf_payloads.reserve(leaf_count);

        // the minimum number of nodes is ceil(nt/(t-1)^2),
        // where n is the number of leafs and t is B/2.

        const ui32 demon = (MIN_CHILD - 1) * (MIN_CHILD - 1);
        ui32 node_count = (leaf_count * MIN_CHILD + demon - 1) / demon;

        node_payloads.reserve(node_count);
        node_headers.reserve(node_count + leaf_count);
    }


    void init_size(ui32 bit_sz) {
        if (sub_sz > 0)
            throw std::logic_error("You can't initialize when there's already values.");

        node_headers.clear();
        node_payloads.clear();
        leaf_payloads.clear();

        reserve(bit_sz);

        std::vector<ui32> nodes;
        std::vector<ui32> sizes;
        
        ui32 leaf_count, leaf_size, leaf_mores;
        calc_size(bit_sz, MAX_BITS, &leaf_count, &leaf_size, &leaf_mores);

        nodes.reserve(leaf_count);
        sizes.reserve(leaf_count);

        for (ui32 i = 0; i < leaf_count; i++) {
            ui32 idx_leaf = leaf_payloads.alloc();
            LeafPayload &leaf = leaf_payloads[idx_leaf];
            if (i < leaf_mores) {
                leaf.bits.expand(leaf_size + 1);
                sizes.push_back(leaf_size + 1);
            } else {
                leaf.bits.expand(leaf_size);
                sizes.push_back(leaf_size);
            }

            ui32 header_new = node_headers.alloc();
            NodeHeader &leaf_header = node_headers[header_new];
            leaf_header.idx_payload = idx_leaf;
            leaf_header.is_leaf = true;

            nodes.push_back(header_new);
        }

        while (nodes.size() > 1) {
            ui32 node_count, node_size, node_mores;
            calc_size(nodes.size(), B, &node_count, &node_size, &node_mores);

            std::vector<ui32> old_nodes(nodes.begin(), nodes.end());
            std::vector<ui32> old_sizes(sizes.begin(), sizes.end());
            nodes.clear();
            nodes.reserve(node_count);
            sizes.clear();
            sizes.reserve(node_count);
            ui32 last_index = 0;
            for (ui32 i = 0; i < node_count; i++) {
                ui32 idx_header_new = node_headers.alloc();
                NodeHeader &header_new = node_headers[idx_header_new];
                ui32 idx_payload_new = node_payloads.alloc();
                NodePayload &node_new = node_payloads[idx_payload_new];

                header_new.is_leaf = false;
                header_new.idx_payload = idx_payload_new;

                if (i < node_mores)
                    node_new.sz_child = node_size + 1;
                else
                    node_new.sz_child = node_size;
                ui32 total_sz = 0;
                for (ui32 j = 0; j < node_new.sz_child; j++) {
                    node_new.set_child_vals(j, old_nodes[last_index + j], old_sizes[last_index + j], 0);
                    NodeHeader &header_child = node_headers[old_nodes[last_index + j]];
                    header_child.idx_parent = idx_header_new;
                    header_child.idx_in_parent = j;

                    total_sz += old_sizes[last_index + j];
                }

                last_index += node_new.sz_child;

                nodes.push_back(idx_header_new);
                sizes.push_back(total_sz);
            }
        }

        root = nodes[0];
        sub_sz = sizes[0];
        sub_ones = 0;
    }

    // access bit
    bool get_bit(ui32 pos) const {
        if (pos >= size())
            throw std::out_of_range("get: pos out of range");

        ui32 pos_loc;
        ui32 idx = find_pos(pos, &pos_loc);
        const NodeHeader &header_cur = node_headers[idx];
        const LeafPayload &leaf_cur = leaf_payloads[header_cur.idx_payload];
        return leaf_cur.bits.get_bit(pos_loc);
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

        if (leaf.bits.test_set_bit(pos_loc, bit)) {

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
    ui32 rank0(ui32 pos) const  {
        return pos - rank1(pos);
    }
    // Compute the number bits in [0..pos) that is equal to bit.
    ui32 rank(ui32 pos, bool bit) const {
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

        if (leaf_cur.bits.sz > MIN_BITS || header_cur.idx_parent == idx_null) {
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
            next = node_par.children[header_cur.idx_in_parent + 1].idx;
        if (header_cur.idx_in_parent > 0)
            prev = node_par.children[header_cur.idx_in_parent - 1].idx;

        bool can_borrow = false;
        ui32 idx_borrow_other = idx_null;
        Datablock<MAX_WORDS> *bits_left = nullptr, *bits_right = nullptr;
        Datablock<MAX_WORDS> *bits_other = nullptr;
        ui32 sz_borrow_other = 0;
        ui32 ones_borrow_other = 0;

        if (next != idx_null) {
            LeafPayload &leaf_next = leaf_payloads[node_headers[next].idx_payload];
            if (leaf_next.bits.sz + sz_cur > 2 * MIN_BITS) {
                can_borrow = true;
                idx_borrow_other = next;
                bits_left = &leaf_cur.bits;
                bits_right = &leaf_next.bits;
                bits_other = bits_right;
                sz_borrow_other = leaf_next.bits.sz;
                ones_borrow_other = leaf_next.bits.ones;
            }
        }
        if (!can_borrow && prev != idx_null) {
            LeafPayload &leaf_prev = leaf_payloads[node_headers[prev].idx_payload];
            if (leaf_prev.bits.sz + sz_cur > 2 * MIN_BITS) {
                can_borrow = true;
                idx_borrow_other = prev;
                bits_left = &leaf_prev.bits;
                bits_right = &leaf_cur.bits;
                bits_other = bits_left;
                pos_loc += leaf_prev.bits.sz;
                sz_borrow_other = leaf_prev.bits.sz;
                ones_borrow_other = leaf_prev.bits.ones;
            }
        }

        if (can_borrow) {
            Datablock<MAX_WORDS>::delete_balance(bits_left, bits_right, pos_loc);

            if (bit) {
                fixup_bal_sub<true>(
                    idx, leaf_cur.bits.sz - sz_cur, leaf_cur.bits.ones - ones_cur,
                    idx_borrow_other, sz_borrow_other - bits_other->sz, ones_borrow_other - bits_other->ones);
                sub_ones--;
            } else {
                fixup_bal_sub<false>(
                    idx, leaf_cur.bits.sz - sz_cur, leaf_cur.bits.ones - ones_cur,
                    idx_borrow_other, sz_borrow_other - bits_other->sz, ones_borrow_other - bits_other->ones);
            }
            sub_sz--;
            return;
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
                idx_delete = prev;
            }

            func(&left->bits, &right->bits, pos_loc);

            node_par.children[header_cur.idx_in_parent].sz += node_par.children[idx_in_other].sz;
            node_par.children[header_cur.idx_in_parent].sz--;
            node_par.children[header_cur.idx_in_parent].ones += node_par.children[idx_in_other].ones;
            if (bit)
                node_par.children[header_cur.idx_in_parent].ones--;
            remove_at(node_par, idx_in_other);
            delete_node(idx_delete);

            if (bit) {
                borrow_merge<-1, -1>(header_cur.idx_parent);
                sub_ones--;
            } else {
                borrow_merge<-1, 0>(header_cur.idx_parent);
            }
            sub_sz--;
            return;
        }

        leaf_cur.bits.remove_at(pos_loc);
        if (bit) {
            sub_ones--;
            fixup_size_ones<-1, -1>(idx);
        } else {
            fixup_size_ones<-1, 0>(idx);
        }
        sub_sz--;
    }
};




#endif //__DYNAMIC_BITVECTOR_BTREE_H__
