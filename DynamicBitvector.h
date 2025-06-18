

#ifndef __DYNAMIC_BITVECTOR_H__
#define __DYNAMIC_BITVECTOR_H__

#include <random>
#include <vector>


template <unsigned int B>
class DynamicBitvector {
public:

    // Data block: stores between B/2 and 2B bits
    struct Datablock {
        size_t sz;       // number of bits in this block
        size_t ones;     // number of 1-bits in this block

        static constexpr size_t MAX_BITS = (4 * B + 1) / 3; // this is ceil( (4B-1)/3 )
        static constexpr size_t MIN_BITS = (2 * B + 1) / 3; // this is floor( (2B+1)/3 )
        static constexpr size_t MAX_WORDS = (MAX_BITS + 63) / 64; // ceil(MAX_BITS / 64)
        static constexpr size_t MIN_WORDS = (MIN_BITS + 63) / 64; // ceil(MIN_BITS / 64)
        uint64_t data[MAX_WORDS];

        Datablock();

        bool get_bit(size_t pos);

        static size_t block_index(size_t pos);
        static size_t bit_index(size_t pos);

        static void split_block(const Datablock *src, Datablock *b1, Datablock *b2, size_t at);
        static void merge_block(const Datablock *src1, const Datablock *src2, Datablock *dest);
        static void balance_block(Datablock *b1, Datablock *b2);
        static void insert_block(Datablock *b, size_t at, bool bit);
        static void delete_block(Datablock *b, size_t at);

        static bool try_insert_balance(Datablock *b1, Datablock *b2, size_t at, bool bit);
        static bool try_delete_balance(Datablock *b1, Datablock *b2, size_t at);
    };

private:
    static constexpr size_t null_idx = SIZE_MAX;

    struct Node {
        size_t left;
        size_t right;
        size_t prev;  // doubly-linked list of data blocks
        size_t next;
        size_t sub_sz;       // total bits in subtree, including this block
        size_t sub_ones;     // total 1-bits in subtree, including this block
        uint32_t prio;       // random heap priority for treap
        Datablock bits;
        Node();
    };

    std::vector<Node> nodes;
    std::vector<size_t> free_list;
    size_t root;
    std::mt19937_64 rng;

    // allocate or recycle a node, initialize with one bit
    size_t alloc_node(bool bit);

    void dealloc_node(size_t idx);

    // helper: get bit at position pos within bits
    static bool get_bit(const Datablock &L, size_t pos);

    // helper: set bit
    static void set_bit(Datablock &L, size_t pos, bool v);

    // recompute subtree aggregates
    void pull(size_t t);

    // rotate right at t -> returns new root
    size_t rotate_right(size_t t);
    // rotate left at t
    size_t rotate_left(size_t t);

    // recursive insert bit at global position pos
    size_t insertRec(size_t t, size_t pos, bool bit);

    // recursive erase bit at global position pos
    size_t eraseRec(size_t t, size_t pos);

    // merge two treaps a < b
    size_t merge(size_t a, size_t b);

public:
    DynamicBitvector();
    ~DynamicBitvector();

    // total number of bits
    size_t size() const;

    // access bit
    bool get(size_t pos) const;

    // rank = # of 1's in [0..pos)
    size_t rank1(size_t pos) const;

    // insert bit at pos in [0..size()]
    void insert(size_t pos, bool bit);

    // erase bit at pos in [0..size())
    void erase(size_t pos);
};



#endif // __DYNAMIC_BITVECTOR_H__