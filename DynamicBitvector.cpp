
#include "DynamicBitvector.h"


template<unsigned int B>
DynamicBitvector<B>::Datablock::Datablock() : sz(0), ones(0)
{
    std::fill(data, data + MAX_WORDS, 0);
};


template<unsigned int B>
size_t DynamicBitvector<B>::Datablock::block_index(size_t pos)
{
    return pos >> 6;
}
template<unsigned int B>
size_t DynamicBitvector<B>::Datablock::bit_index(size_t pos)
{
    return pos & 63;
}


template<unsigned int B>
static void DynamicBitvector<B>::Datablock::split_block(const Datablock *src, Datablock *b1, Datablock *b2, size_t at)
{

}
template<unsigned int B>
static void DynamicBitvector<B>::Datablock::merge_block(const Datablock *src1, const Datablock *src2, Datablock *dest)
{

}
template<unsigned int B>
static void DynamicBitvector<B>::Datablock::balance_block(Datablock *b1, Datablock *b2)
{

}
template<unsigned int B>
static void DynamicBitvector<B>::Datablock::insert_block(Datablock *b, size_t at, bool bit)
{

}

template<unsigned int B>
static bool DynamicBitvector<B>::Datablock::try_insert_balance(Datablock *b1, Datablock *b2, size_t at, bool bit)
{

}
template<unsigned int B>
static bool DynamicBitvector<B>::Datablock::try_delete_balance(Datablock *b1, Datablock *b2, size_t at)
{

}








template<unsigned int B>
DynamicBitvector<B>::Node::Node() : left(null_idx), right(null_idx), prev(null_idx), next(null_idx), sub_sz(0), sub_ones(0), prio(0)
{
};

template<unsigned int B>
DynamicBitvector<B>::DynamicBitvector() : root(null_idx), rng(std::random_device{}())
{
};

template<unsigned int B>
size_t DynamicBitvector<B>::alloc_node(bool bit) {
    size_t idx;
    if (!free_list.empty()) {
        idx = free_list.back(); free_list.pop_back();
        nodes[idx] = Node();
    }
    else {
        idx = nodes.size();
        nodes.emplace_back();
    }
    Node &n = nodes[idx];
    n.prio = rng();
    // init bits
    n.bits.sz = 1;
    n.bits.ones = bit ? 1 : 0;
    n.bits.data[0] = bit ? 1ULL : 0ULL;
    // subtree stats
    n.sub_sz = 1;
    n.sub_ones = n.bits.ones;
    return idx;
};

template<unsigned int B>
void DynamicBitvector<B>::dealloc_node(size_t idx) {
    free_list.push_back(idx);
};

template<unsigned int B>
bool DynamicBitvector<B>::get_bit(const Datablock &L, size_t pos) {
    size_t w = Datablock::block_index(pos);
    size_t b = Datablock::bit_index(pos);
    return (L.data[w] >> b) & 1U;
}

template<unsigned int B>
void DynamicBitvector<B>::set_bit(Datablock &L, size_t pos, bool v) {
    size_t w = Datablock::block_index(pos);
    size_t b = Datablock::bit_index(pos);
    if (v) L.data[w] |= (1ULL << b);
    else     L.data[w] &= ~(1ULL << b);
}

template<unsigned int B>
void DynamicBitvector<B>::pull(size_t t) {
    auto &n = nodes[t];
    n.sub_sz = n.bits.sz;
    n.sub_ones = n.bits.ones;
    if (n.left != null_idx) { n.sub_sz += nodes[n.left].sub_sz;   n.sub_ones += nodes[n.left].sub_ones; }
    if (n.right != null_idx) { n.sub_sz += nodes[n.right].sub_sz;  n.sub_ones += nodes[n.right].sub_ones; }
}
    
template<unsigned int B>
size_t DynamicBitvector<B>::rotate_right(size_t t) {
    size_t L = nodes[t].left;
    nodes[t].left = nodes[L].right;
    nodes[L].right = t;
    pull(t);
    pull(L);
    return L;
}

template<unsigned int B>
size_t DynamicBitvector<B>::rotate_left(size_t t) {
    size_t R = nodes[t].right;
    nodes[t].right = nodes[R].left;
    nodes[R].left = t;
    pull(t);
    pull(R);
    return R;
}

template<unsigned int B>
size_t DynamicBitvector<B>::insertRec(size_t t, size_t pos, bool bit) {
    if (t == null_idx) {
        return alloc_node(bit);
    }
    auto &n = nodes[t];
    size_t ls = (n.left == null_idx ? 0 : nodes[n.left].sub_sz);
    size_t ms = n.bits.sz;

    if (pos < ls) {
        n.left = insertRec(n.left, pos, bit);
        if (nodes[n.left].prio > n.prio)
            t = rotate_right(t);

        pull(t);
        return t;
    }

    if (pos > ls + ms) {
        n.right = insertRec(n.right, pos - ls - ms, bit);
        if (nodes[n.right].prio > n.prio)
            t = rotate_left(t);

        pull(t);
        return t;
    }

    // insert inside this bits
    size_t off = pos - ls;

    // there isn't overflow
    if (n.bits.sz + 1 <= 2 * B) {
        size_t w = Datablock::block_index(off);
        for (size_t i = 1; (n.bits.sz+1 >> 6) - i > w; i++)
        {
            size_t curIndex = (n.bits.sz+1 >> 6) - i;
            n.bits.data[curIndex] = (n.bits.data[curIndex] << 1) | (n.bits.data[curIndex-1] >> 63);
        }
        size_t b = Datablock::bit_index(off);
        size_t fixed_mask = (1 << (b + 1)) - 1;
        size_t shift_mask = ~fixed_mask;
        auto word = n.bits.data[w];
        n.bits.data[w] = ((word & shift_mask) << 1) | (word & fixed_mask);
        set_bit(n.bits, off, bit);
        n.bits.sz++;
        if (bit)
            n.bits.ones++;

        pull(t);
        return t;
    }

    // split bits if overflow
    size_t half = n.bits.sz / 2;
    // create right-half node
    size_t R = alloc_node(false);
    auto &nr = nodes[R];
    // copy bits [half .. end)
    nr.bits.sz = n.bits.sz - half;
    nr.bits.ones = 0;
    for (size_t i = 0; i < nr.bits.sz; ++i) {
        bool v = get_bit(n.bits, half + i);
        set_bit(nr.bits, i, v);
        if (v) nr.bits.ones++;
    }
    // fix linked list
    nr.next = n.next;
    nr.prev = t;
    if (n.next != null_idx)
        nodes[n.next].prev = R;
    n.next = R;
    // shrink left bits
    n.bits.sz = half;
    n.bits.ones = 0;
    for (size_t i = 0; i < half; ++i)
        if (get_bit(n.bits, i)) n.bits.ones++;

    // detach R subtree
    nr.right = n.right;
    n.right = null_idx;
    pull(R);
    pull(t);

    // re-heapify
    if (n.prio > nr.prio) {
        n.right = R;
    }
    else {
        // make R the new root of this subtree
        nr.left = t;
        t = R;
    }

    pull(t);
    return t;
}

template<unsigned int B>
size_t DynamicBitvector<B>::eraseRec(size_t t, size_t pos) {
    if (t == null_idx)
        throw std::out_of_range("erase: pos out of range");
    auto &n = nodes[t];
    size_t ls = (n.left == null_idx ? 0 : nodes[n.left].sub_sz);
    size_t ms = n.bits.sz;
    if (pos < ls) {
        n.left = eraseRec(n.left, pos);
    }
    else if (pos >= ls + ms) {
        n.right = eraseRec(n.right, pos - ls - ms);
    }
    else {
        // delete inside this leaf
        size_t off = pos - ls;
        bool old = get_bit(n.bits, off);
        for (size_t i = off; i + 1 < n.bits.sz; ++i) {
            bool v = get_bit(n.bits, i + 1);
            set_bit(n.bits, i, v);
        }
        n.bits.sz--;
        if (old) n.bits.ones--;
        // if leaf becomes empty, remove node
        if (n.bits.sz == 0) {
            // fix linked list
            if (n.prev != null_idx) nodes[n.prev].next = n.next;
            if (n.next != null_idx) nodes[n.next].prev = n.prev;
            size_t merged = merge(n.left, n.right);
            dealloc_node(t);
            return merged;
        }
        // underflow merging not implemented here
    }
    pull(t);
    return t;
}

template<unsigned int B>
size_t DynamicBitvector<B>::merge(size_t a, size_t b) {
    if (a == null_idx) return b;
    if (b == null_idx) return a;
    if (nodes[a].prio > nodes[b].prio) {
        nodes[a].right = merge(nodes[a].right, b);
        pull(a);
        return a;
    }
    else {
        nodes[b].left = merge(a, nodes[b].left);
        pull(b);
        return b;
    }
}


template<unsigned int B>
size_t DynamicBitvector<B>::size() const
{
    return (root == null_idx ? 0 : nodes[root].sub_sz);
}

template<unsigned int B>
bool DynamicBitvector<B>::get(size_t pos) const {
    if (!root || pos >= size())
        throw std::out_of_range("get: pos out of range");

    size_t t = root;
    while (true)
    {
        const auto &n = nodes[t];
        size_t ls = (n.left == null_idx ? 0 : nodes[n.left].sub_sz);
        if (pos < ls) {
            t = n.left;
            continue;
        }
        pos -= ls;
        if (pos >= n.bits.sz) {
            pos -= n.bits.sz;
            t = n.right;
            continue;
        }
        break;
    }
    return get_bit(nodes[t].bits, pos);
}

template<unsigned int B>
size_t DynamicBitvector<B>::rank1(size_t pos) const {
    if (pos > size())
        throw std::out_of_range("rank: pos out of range");

    size_t t = root;
    size_t count = 0;

    if (t == null_idx || pos == 0) return 0;

    while (true)
    {
        const auto &n = nodes[t];
        size_t ls = (n.left == null_idx ? 0 : nodes[n.left].sub_sz);
        size_t lo = (n.left == null_idx ? 0 : nodes[n.left].sub_ones);
        if (pos <= ls) {
            t = n.left;
            continue;
        }
        pos -= ls;
        count += lo;
        if (pos > n.bits.sz) {
            pos -= n.bits.sz;
            count += n.bits.ones;
            t = n.right;
            continue;

        }
        break;
    }

    // pos is within this block
    // Now count the first 'pos' bits in this block using 4-bit bithack
    static const unsigned short bithack[16] = {
        0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4
    };
    const auto &data = nodes[t].bits.data;
    size_t ones = 0;
    // count full 4-bit chunks
    size_t full_nibbles = pos / 4;
    for (size_t i = 0; i < full_nibbles; ++i) {
        size_t bitpos = i * 4;
        size_t w = bitpos >> 6;
        size_t b = bitpos & 63;
        unsigned short nib = (data[w] >> b) & 0xF;
        ones += bithack[nib];
    }

    // count remaining bits
    for (size_t j = full_nibbles * 4; j < pos; ++j) {
        if (get_bit(nodes[t].bits, j))
            ones++;
    }

    return count + ones;

}

template<unsigned int B>
void DynamicBitvector<B>::insert(size_t pos, bool bit) {
    if (pos > size())
        throw std::out_of_range("insert: pos out of range");

    root = insertRec(root, pos, bit);
}

template<unsigned int B>
void DynamicBitvector<B>::erase(size_t pos) {
    if (pos >= size())
        throw std::out_of_range("erase: pos out of range");

    root = eraseRec(root, pos);
}