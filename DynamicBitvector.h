

#ifndef __DYNAMIC_BITVECTOR_H__
#define __DYNAMIC_BITVECTOR_H__

#include <random>
#include <vector>

#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(__popcnt64)
#endif

#define _BOUND_CHECKING


using ui32 = uint32_t;



// This class represents a collection of 64*B bits.
// All operations will be linear in B implemented
// using bitwise operations.
template <unsigned int B>
struct Datablock {
    ui32 sz;       // number of bits in this structure
    ui32 ones;     // number of 1-bits in this structure

    // This field holds the actual stream of bits. Each
    // element of the array is a block of data. Within a
    // block, the less significant bits of a word comes
    // before the more significant bits. The blocks with
    // smaller indices will come before the blocks with
    // larger indices.
    uint64_t data[B];

    Datablock() : sz(0), ones(0) {
        std::fill(data, data + B, 0ULL);
    }

    // The index of the block in which contains the
    // pos'th bit.
    static inline constexpr ui32 block_index(const ui32 &pos) {
        return pos >> 6;
    }
    // The index of the bit within block that contains the
    // pos'th bit of this stream.
    static inline constexpr ui32 bit_index(const ui32 &pos) {
        return pos & 63;
    }

    // Count the number of 1's in a 64 bit number x.
    // This is the cross-platform implementation.
    static inline ui32 popcount64(const uint64_t &x) noexcept {
#if defined(_MSC_VER)
        return static_cast<ui32>(__popcnt64(x));
#else
        return static_cast<ui32>(__builtin_popcountll(x));
#endif
    }

#if defined(_BOUND_CHECKING)
    // Returns true if a position index pos is within valid range,
    // which is [0, sz). Otherwise return false.
    inline bool valid_pos(const ui32 &pos) const { return pos < sz; }
    // Returns true if a block index pos is within valid range,
    // which is [0, floor(sz/64)). Otherwise return false.
    inline bool valid_block_pos(const ui32 &pos) const { return pos < sz / 64; }
    // Returns true if a size argument s is within valid range,
    // which is [0, sz]. Otherwise return false.
    inline bool valid_size(const ui32 &s) const { return s <= sz; }
    // Returns true if a size argument s is within valid capacity,
    // which is [0, 64B]. Otherwise return false.
    inline static bool valid_capacity(const ui32 &s) { return s <= 64 * B; }
#endif


    // Return the pos'th bit in this stream.
    inline bool get_bit(const ui32 &pos) const {
#if defined(_BOUND_CHECKING)
        if (!valid_pos(pos))
            throw std::out_of_range("");
#endif

        const ui32 w = block_index(pos);
        const ui32 b = bit_index(pos);
        return (data[w] >> b) & 1U;
    }
    // Set the pos'th bit in this stream and update the
    // number of ones
    inline void set_bit(const ui32 &pos, bool &bit) {
#if defined(_BOUND_CHECKING)
        if (!valid_pos(pos))
            throw std::out_of_range("");
#endif
        const ui32 w = block_index(pos);
        const ui32 b = bit_index(pos);
        const bool old = (data[w] >> b) & 1U;
        if (old == bit)
            return;

        if (bit) {
            ones++;
            data[w] |= (1ULL << b);
        } else {
            ones--;
            data[w] &= ~(1ULL << b);
        }
    }
    // Set the pos'th bit in this stream without updating 
    // the number of ones
    inline void set_bit_no_count(const ui32 &pos, bool &bit) {
#if defined(_BOUND_CHECKING)
        if (!valid_pos(pos))
            throw std::out_of_range("");
#endif
        const ui32 w = block_index(pos);
        const ui32 b = bit_index(pos);
        const bool old = (data[w] >> b) & 1U;
        if (old == bit)
            return;

        if (bit)
            data[w] |= (1ULL << b);
        else
            data[w] &= ~(1ULL << b);
    }
    
    // Return the next 64 bits at the pos'th bit in this stream.
    inline uint64_t get_bits64(const ui32 &pos) const {
#if defined(_BOUND_CHECKING)
        if (!valid_pos(pos) || !valid_size(pos + 64))
            throw std::out_of_range("");
#endif
        const ui32 w = block_index(pos);
        const ui32 b = bit_index(pos);

        if (b == 0)
            return data[w];
        return (data[w] >> b) | (data[w+1] << (64-b));
    }
    // Set the next 64 bits at the pos'th bit in this stream
    // and update the number of ones
    inline void set_bits64(const ui32 &pos, const uint64_t &bits) {
#if defined(_BOUND_CHECKING)
        if (!valid_pos(pos) || !valid_size(pos + 64))
            throw std::out_of_range("");
#endif
        const ui32 w = block_index(pos);
        const ui32 b = bit_index(pos);
        ones += popcount64(bits);

        if (b == 0) {
            ones -= popcount64(data[w]);
            data[w] = bits;
            return;
        }
        const uint64_t ffm = (1ULL << b) - 1; // mask for the fixed bits of data[w]
        const uint64_t bfm = ~((1ULL << (64-b)) - 1); // mask for the fixed bits of data[w+1]

        uint64_t old = 0;
        old = data[w] & (~ffm);
        old = old | (data[w+1] & (~bfm));
        ones -= popcount64(old);

        data[w] = (data[w] & ffm) | (bits << b);
        data[w+1] = (data[w+1] & bfm) | (bits >> (64-b));

    }
    // Set the next 64 bits at the pos'th bit in this stream
    // without updating the number of ones
    inline void set_bits64_no_count(const ui32 &pos, const uint64_t &bits) {
#if defined(_BOUND_CHECKING)
        if (!valid_pos(pos) || !valid_size(pos + 64))
            throw std::out_of_range("");
#endif
        const ui32 w = block_index(pos);
        const ui32 b = bit_index(pos);

        if (b == 0) {
            data[w] = bits;
            return;
        }
        const uint64_t ffm = (1ULL << b) - 1; // mask for the fixed bits of data[w]
        const uint64_t bfm = ~((1ULL << (64-b)) - 1); // mask for the fixed bits of data[w+1]
        data[w] = (data[w] & ffm) | (bits << b);
        data[w+1] = (data[w+1] & bfm) | (bits >> (64-b));
    }

    // Return the next s bits at the pos'th bit in this stream.
    inline uint64_t get_bits(const ui32 &pos, const ui32 &s) const {
#if defined(_BOUND_CHECKING)
        if (!valid_pos(pos) || !valid_size(pos + s))
            throw std::out_of_range("");
#endif
        if (s == 64)
            return get_bits64(pos);

        const ui32 w = block_index(pos);
        const ui32 b = bit_index(pos);

        // if the range is within a block
        if (b + s <= 64) {
            uint64_t mask = (1ULL<<s) - 1;
            return (data[w] >> b) & mask;
        }

        // if the range spans two blocks
        const uint64_t front = data[w] >> b;
        const uint64_t size_back = s - (64 - b);
        const uint64_t back = data[w+1] & ((1ULL << size_back) - 1);
        return front | (back << (64-b));
    }
    // Set the next s bits at the pos'th bit in this stream
    // and update the number of ones
    inline void set_bits(const ui32 &pos, const ui32 &s, const uint64_t &bits) {
#if defined(_BOUND_CHECKING)
        if (!valid_pos(pos) || !valid_size(pos + s))
            throw std::out_of_range("");
#endif
        if (s == 64) {
            set_bits64(pos, bits);
            return;
        }

        bits = bits & ((1ULL<<s)-1);
        ones += popcount64(bits);

        const ui32 w = block_index(pos);
        const ui32 b = bit_index(pos);

        if (b + s <= 64) {
            const uint64_t fm = ~(((1ULL<<s)-1) << b); // mask for the fixed bits
            uint64_t old = data[w] & (~fm);
            data[w] = (data[w] & fm) | (bits << b);
            ones -= popcount64(old);
            return;
        }

        const uint64_t ffm = (1ULL << b) - 1; // mask for the fixed bits of data[w]
        const uint64_t size_back = s - (64 - b);
        const uint64_t bfm = ~((1ULL << size_back) - 1); // mask for the fixed bits of data[w+1]

        uint64_t old = 0;
        old = data[w] & (~ffm);
        old = old | (data[w+1] & (~bfm));
        ones -= popcount64(old);

        data[w] = (data[w] & ffm) | (bits << b);
        data[w+1] = (data[w+1] & bfm) | (bits >> (64-b));
    }
    // Set the next s bits at the pos'th bit in this stream
    // without updating the number of ones
    inline void set_bits_no_count(const ui32 &pos, const ui32 &s, const uint64_t &bits) {
#if defined(_BOUND_CHECKING)
        if (!valid_pos(pos) || !valid_size(pos + s))
            throw std::out_of_range("");
#endif
        if (s == 64) {
            set_bits64_no_count(pos, bits);
            return;
        }

        const uint64_t masked_bits = bits & ((1ULL<<s)-1);

        const ui32 w = block_index(pos);
        const ui32 b = bit_index(pos);

        if (b + s <= 64) {
            const uint64_t fm = ~(((1ULL<<s)-1) << b); // mask for the fixed bits
            data[w] = (data[w] & fm) | (masked_bits << b);
            return;
        }

        const uint64_t ffm = (1ULL << b) - 1; // mask for the fixed bits of data[w]
        const uint64_t size_back = s - (64 - b);
        const uint64_t bfm = ~((1ULL << size_back) - 1); // mask for the fixed bits of data[w+1]
        data[w] = (data[w] & ffm) | (masked_bits << b);
        data[w+1] = (data[w+1] & bfm) | (masked_bits >> (64-b));
    }

    // Expand the current stream by s bits and pad the end with 0's.
    inline void expand(const ui32 &s) {
#if defined(_BOUND_CHECKING)
        if (!valid_capacity(sz + s))
            throw std::out_of_range("");
#endif
        const ui32 start_w = block_index(sz);
        const ui32 start_b = block_index(sz);
        const ui32 end_w = block_index(sz + s);
        const ui32 end_b = block_index(sz + s);

        sz += s;
        if (start_w == end_w) {
            uint64_t fm = (1ULL << start_b) - 1;
            data[start_w] = data[start_w] & fm;
            return;
        }

        for (int i = start_w+1; i < end_w; i++)
            data[i] = 0ULL;
        // we don't care about what's afterwards anyways
        if (end_b > 0)
            data[end_w] = 0;
    }
    // Remove the last s bits in the current stream and update
    // the number of ones.
    inline void shrink(const ui32 &s) {
#if defined(_BOUND_CHECKING)
        if (!valid_size(s))
            throw std::out_of_range("");
#endif
        const ui32 start_w = block_index(sz - s);
        const ui32 start_b = block_index(sz - s);
        const ui32 end_w = block_index(sz);
        const ui32 end_b = block_index(sz);

        if (start_w == end_w) {
            uint64_t fm = (1ULL << start_b) - 1;
            ones -= popcount64((data[start_w] >> start_b) & ((1ULL<<(end_b-start_b))-1));
            sz -= s;
            return;
        }

        for (int i = start_w+1; i < end_w; i++)
            ones -= popcount64(data[i]);
        if (end_b > 0)
            ones -= popcount64(data[end_w] & ((1ULL << end_b) - 1));

        sz -= s;
    }
    // Remove the last s bits in the current stream without
    // updating the number of ones.
    inline void shrink_no_count(const ui32 &s) {
#if defined(_BOUND_CHECKING)
        if (!valid_size(s))
            throw std::out_of_range("");
#endif
        sz -= s;
    }
    // Remove all the bits in the current stream.
    inline void clear() {
        sz = 0;
        ones = 0;
    }

    // Return the w'th block.
    inline uint64_t get_block(const ui32 &w) const {
#if defined(_BOUND_CHECKING)
        if (!valid_block_pos(w))
            throw std::out_of_range("");
#endif
        
        return data[w];
    }
    // Set the w'th block to be the word bits and update
    // the number of ones.
    inline void set_block(const ui32 &w, const uint64_t &bits) {
#if defined(_BOUND_CHECKING)
        if (!valid_block_pos(w))
            throw std::out_of_range("");
#endif
        ones -= popcount64(data[w]);
        ones += popcount64(bits);
        data[w] = bits;
    }
    // Set the w'th block to be the word bits without updating
    // the number of ones.
    inline void set_block_no_count(const ui32 &w, const uint64_t &bits) {
#if defined(_BOUND_CHECKING)
        if (!valid_block_pos(w))
            throw std::out_of_range("");
#endif
        data[w] = bits;
    }

    // Return the number of 1's in position [0, s)
    inline ui32 ones_count(const ui32 &s) const {
#if defined(_BOUND_CHECKING)
        if (!valid_size(s))
            throw std::out_of_range("");
#endif
        const ui32 w = block_index(s);
        const ui32 b = bit_index(s);
        ui32 count = 0;

        for (ui32 i = 0; i < w; i++)
            count += popcount64(data[i]);
        if (b > 0)
            count += popcount64(data[w] & ((1ULL<<b)-1));

        return count;
    }
    // Recalculate the number of ones in this stream.
    inline void pull_ones() {
        ones = ones_count(sz);
    }

    // Insert the bit in the pos'th position in the stream
    inline void insert_at(const ui32 &pos, const bool &bit) {
#if defined(_BOUND_CHECKING)
        if (!valid_pos(pos) || !valid_capacity(sz + 1))
            throw std::out_of_range("");
#endif
        // shift all the block after the block that contains the pos'th bit
        const ui32 w = block_index(pos);
        const ui32 b = bit_index(pos);

        const ui32 end_w = block_index(sz);
        const ui32 end_b = bit_index(sz);

        if (end_w > w) {
            if (end_b > 0)
                data[end_w] = data[end_w] << 1;
            
            for (ui32 i = end_w-1; i > w; i--) {
                data[i+1] = data[i+1] | (data[i] >> 63);
                data[i] = data[i] << 1;
            }
            data[w+1] = data[w+1] | (data[w] >> 63);
        }

        // inserting the bit in the block containing the pos'th bit
        const uint64_t fm = (1ULL<<b) - 1; // mask for the fixed bits
        const uint64_t sm = ~fm;           // mask for the shift bits
        
        // shift the portion after the bit and fix the portion
        // before the bit
        data[w] = ((data[w] & sm) << 1) | (data[w] & fm);

        if (bit) {
            data[w] = data[w] | (1ULL << b);
            ones++;
        }
        sz++;
    }
    // Remove the pos'th bit in the stream
    inline void remove_at(const ui32 &pos) {
#if defined(_BOUND_CHECKING)
        if (!valid_pos(pos) || sz > 0)
            throw std::out_of_range("");
#endif
        // Update count before it gets destroyed
        if (get_bit(pos))
            ones--;

        // First remove the bit from the block containing it
        const ui32 w = block_index(pos);
        const ui32 b = bit_index(pos);
        const uint64_t fm = (1ULL << b) - 1;    // mask for the fixed bits
        uint64_t sm = ~fm;             // mask for the shift bits
        sm = sm & (sm - 1);            // throw away the bit we remove

        // shift the portion after the bit and fix the portion
        // before the bit
        data[w] = ((data[w] & sm) >> 1) | (data[w] & fm);

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

    // Copy bits in this stream from the range [src_pos, src_pos+n)
    // to the range [dest_pos, dest_pos+n) in dest, and update the
    // number of ones in dest.
    inline void copy_to(Datablock *&dest, const ui32 &src_pos, const ui32 &dest_pos, const ui32 &n) const {
#if defined(_BOUND_CHECKING)
        if (!valid_pos(src_pos) || !dest->valid_pos(dest_pos) ||
            !valid_size(src_pos+n) || !dest->valid_size(dest_pos+n))
            throw std::out_of_range("");
#endif
        ui32 i;
        for (i = 0; i + 64 < n; i += 64) {
            dest->set_bits64(dest_pos + i, get_bits64(src_pos + i));
        }
        dest->set_bits(dest_pos + i, get_bits(src_pos + i, n-i), n-i);
    }
    // Copy bits in this stream from the range [src_pos, src_pos+n)
    // to the range [dest_pos, dest_pos+n) in dest without updating
    // the number of ones in dest.
    inline void copy_to_no_count(Datablock *&dest, const ui32 &src_pos, const ui32 &dest_pos, const ui32 &n) const {
#if defined(_BOUND_CHECKING)
        if (!valid_pos(src_pos) || !dest->valid_pos(dest_pos) ||
            !valid_size(src_pos + n) || !dest->valid_size(dest_pos + n))
            throw std::out_of_range("");
#endif
        const ui32 start_w = block_index(src_pos);
        const ui32 start_b = block_index(src_pos);
        const ui32 end_w = block_index(src_pos + n);
        const ui32 end_b = block_index(src_pos + n);

        if (start_w == end_w) {
            dest->set_bits_no_count(dest_pos, n, (data[start_w]>>start_b) & ((1ULL<<n)-1));
            return;
        }

        dest->set_bits_no_count(dest_pos, 64 - start_b, data[start_w] >> start_b);

        for (ui32 i = start_w+1; i < end_w; i++) {
            dest->set_bits64_no_count(dest_pos + 64 - start_b + 64 * (i - start_w), data[i]);
        }
        if (end_b > 0)
            dest->set_bits_no_count(dest_pos+n-end_b, end_b, data[end_w]);
    }

    
    inline void pad_zeros_front(ui32 s) {
#if defined(_BOUND_CHECKING)
        if (!valid_capacity(sz + s))
            throw std::out_of_range("");
#endif
        // block index of the last bit
        const ui32 w = block_index(sz-1);
        const ui32 w_shift = s / 64;
        const ui32 b_shift = s - w_shift * 64;

        if (b_shift > 0) {

            ui32 end_b = w + w_shift + 1;
            if (end_b >= B)
                end_b = B - 1;

            for (ui32 i = end_b; i > w_shift; i--)
                data[i] = (data[i - w_shift] << b_shift) | (data[i - w_shift - 1] >> (64 - b_shift));

            for (ui32 i = 0; i < w_shift; i++)
                data[i] = 0;
            data[w_shift] = data[w_shift] & ((1 << b_shift) - 1);
        } else {
            ui32 end_b = w + w_shift;
            if (end_b >= B)
                end_b = B - 1;

            for (ui32 i = end_b; i > w_shift; i--)
                data[i] = data[i - w_shift];
            for (ui32 i = 0; i <= w_shift; i++)
                data[i] = 0;
        }

        sz += s;
    }
    inline void shift_backward_no_count(ui32 s) {
        if (s >= sz) {
            clear();
            return;
        }

        const ui32 w = block_index(sz - s - 1);
        const ui32 w_shift = s / 64;
        const ui32 b_shift = s - w_shift * 64;

        if (b_shift > 0) {
            for (ui32 i = 0; i < w; i++)
                data[i] = (data[i + w_shift] >> b_shift) | (data[i + w_shift + 1] << (64 - b_shift));

            data[w] = data[w + w_shift] >> b_shift;

        } else {
            for (ui32 i = 0; i <= w; i++)
                data[i] = data[i + w_shift];
        }
    }

    static inline void split_insert_balance(Datablock *b1, Datablock *b2, ui32 at, bool bit) {
        const ui32 sz = b1->sz + b2->sz + 1;
        if (sz > 2 * B * 64)
            return;
        const ui32 sz2 = sz / 2; // size of the first block
        const ui32 sz1 = sz - sz2; // size of the second block

        b2->clear();
        b2->expand(sz2);

        if (at < sz1) {
            b1->copy_to_no_count(b2, sz1-1, 0, sz2);
            b2->pull_ones();
            b1->shrink_no_count(sz2);
            b1->ones -= b2->ones;
            b1->insert_at(at, bit);
        } else {
            b1->copy_to_no_count(b2, sz1, 0, at - sz1);
            b1->copy_to_no_count(b2, at+1, at - sz1 + 1, sz - 1 - at);
            b2->pull_ones();
            b1->shrink_no_count(sz2-1);
            b1->ones -= b2->ones;
            b2->set_bit(at - sz1, bit);
        }
    }
    static inline void delete_balance(Datablock *b1, Datablock *b2, ui32 at) {
        if (b1->sz + b2->sz <= 0)
            return;
        const ui32 sz = b1->sz + b2->sz - 1;
        const ui32 sz1 = sz / 2;   // size of the first block
        const ui32 sz2 = sz - sz1; // size of the second block

        if (at < b1->sz)
            b1->remove_at(at);
        else
            b2->remove_at(at - b1->sz);

        if (b1->sz > sz1) {
            b2->pad_zeros_front(sz2 - b2->sz);
            b1->copy_to_no_count(b2, sz1, 0, b1->sz - sz1);
            b1->shrink_no_count(b1->sz - sz1);
            b1->pull_ones();
            b2->pull_ones();
        } else {
            const ui32 orig_size = b1->sz;
            const ui32 size_diff = sz1 - b1->sz;
            b1->expand(size_diff);
            b2->copy_to_no_count(b1, 0, orig_size, size_diff);
            b2->shift_backward_no_count(size_diff);
            b1->pull_ones();
            b2->pull_ones();
        }
    }
    static inline void delete_merge_left(Datablock *b1, Datablock *b2, ui32 pos) {
        b1->expand(b2->sz-1);
        if (pos < b1->sz) {
            b1->remove_at(pos);
            b2->copy_to_no_count(b1, 0, b1->sz, b2->sz);
            b1->ones += b2->ones;
            b2->clear();
        } else {
            ui32 pos_loc = pos - b1->sz;
            b2->copy_to_no_count(b1, 0, b1->sz, pos_loc);
            b2->copy_to_no_count(b1, pos_loc+1, b1->sz+pos_loc, b2->sz-pos_loc-1);
            b1->ones += b2->ones;
            if (b2->get_bit(pos - b1->sz))
                b1->ones--;
            b2->clear();
        }
    }
    static inline void delete_merge_right(Datablock *b1, Datablock *b2, ui32 at) {
        if (at < b1->sz) {
            b2->pad_zeros_front(b1->sz - 1);
            b1->copy_to_no_count(b2, 0, 0, at);
            b1->copy_to_no_count(b2, at+1, at, b1->sz-1-at);
            if (b1->get_bit(at))
                b2->ones += b1->ones - 1;
            else
                b2->ones += b1->ones;
            b1->clear();
        } else {
            b2->remove_at(at - b1->sz);
            b2->pad_zeros_front(b1->sz);
            b1->copy_to(b2, 0, 0, b1->sz);
            b2->ones += b1->ones;
            b1->clear();
        }
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
        ui32 prev;     // doubly-linked list of data blocks
        ui32 next;
        ui32 sub_sz;   // total bits in subtree, including this block
        ui32 sub_ones; // total 1-bits in subtree, including this block
        ui32 prio;     // random heap priority for treap
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

        *pos_loc = pos - nodes[idx].bits.sz;
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

        *pos_loc = pos - nodes[idx].sz;
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
    ~DynamicBitvector() {};

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
            pull_propagate(idx);
            return;
        }

        // If there is overflow.
        // Never balance with neighboring blocks, just split whenever
        // because if we balance with neighbor, then we need to update
        // the count for the neighbor which is annoying

        ui32 idx_right = alloc_node();
        auto &node_right = nodes[idx_right];

        Datablock<MAX_WORDS>::split_insert_balance(&node_cur.bits, &node_right.bits, pos_loc, bit);

        // fix linked list
        ll_insert_after(idx, idx_right);

        node_cur.right = insert_min(node_cur.right, idx_right);

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
            idx = rotate_right(idx);
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
                idx_prev = rotate_right(idx_prev);
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