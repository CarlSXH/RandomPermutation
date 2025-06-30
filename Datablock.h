
#ifndef __DATABLOCK_H__
#define __DATABLOCK_H__

#include <cstdint>

#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(__popcnt64)
#endif

#define _BOUND_CHECKING


using ui32 = uint32_t;



// This class represents a collection of 64*W bits.
// All operations will be linear in W implemented
// using bitwise operations.
template <unsigned int W>
struct Datablock {
    ui32 sz;       // number of bits in this structure
    ui32 ones;     // number of 1-bits in this structure

    // This field holds the actual stream of bits. Each
    // element of the array is a block of data. Within a
    // block, the less significant bits of a word comes
    // before the more significant bits. The blocks with
    // smaller indices will come before the blocks with
    // larger indices.
    uint64_t data[W];

    Datablock() : sz(0), ones(0) {
        std::fill(data, data + W, 0ULL);
    }

private:

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
    // Compute the number of trailing 0's of x.
    static inline ui32 trailing_ones(const uint64_t &x) noexcept {
#if defined(_MSC_VER)
        return static_cast<ui32>(_tzcnt_u64(x));
#else
        return static_cast<ui32>(__builtin_ctz(x));
#endif
    }

#if defined(_BOUND_CHECKING)
    // Returns true if a position index pos is within valid range,
    // which is [0, sz). Otherwise return false.
    inline bool valid_pos(const ui32 &pos) const { return pos < sz; }
    // Returns true if a block index pos is within valid range,
    // which is [0, ceil(sz/64)). Otherwise return false.
    inline bool valid_block_pos(const ui32 &pos) const { return pos < (sz + 63) / 64; }
    // Returns true if a size argument s is within valid range,
    // which is [0, sz]. Otherwise return false.
    inline bool valid_size(const ui32 &s) const { return s <= sz; }
    // Returns true if a size argument s is within valid capacity,
    // which is [0, 64B]. Otherwise return false.
    inline static bool valid_capacity(const ui32 &s) { return s <= 64 * W; }
#endif

public:

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
    inline void set_bit(const ui32 &pos, const bool &bit) {
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
    inline void set_bit_no_count(const ui32 &pos, const bool &bit) {
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
        if (!valid_size(pos + 64))
            throw std::out_of_range("");
#endif
        const ui32 w = block_index(pos);
        const ui32 b = bit_index(pos);

        if (b == 0)
            return data[w];
        return (data[w] >> b) | (data[w + 1] << (64 - b));
    }
    // Set the next 64 bits at the pos'th bit in this stream
    // and update the number of ones
    inline void set_bits64(const ui32 &pos, const uint64_t &bits) {
#if defined(_BOUND_CHECKING)
        if (!valid_size(pos + 64))
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
        const uint64_t bfm = ~ffm; // mask for the fixed bits of data[w+1]

        uint64_t old = 0;
        old = data[w] & (~ffm);
        old = old | (data[w + 1] & (~bfm));
        ones -= popcount64(old);

        data[w] = (data[w] & ffm) | (bits << b);
        data[w + 1] = (data[w + 1] & bfm) | (bits >> (64 - b));

    }
    // Set the next 64 bits at the pos'th bit in this stream
    // without updating the number of ones
    inline void set_bits64_no_count(const ui32 &pos, const uint64_t &bits) {
#if defined(_BOUND_CHECKING)
        if (!valid_size(pos + 64))
            throw std::out_of_range("");
#endif
        const ui32 w = block_index(pos);
        const ui32 b = bit_index(pos);

        if (b == 0) {
            data[w] = bits;
            return;
        }
        const uint64_t ffm = (1ULL << b) - 1; // mask for the fixed bits of data[w]
        const uint64_t bfm = ~ffm; // mask for the fixed bits of data[w+1]
        data[w] = (data[w] & ffm) | (bits << b);
        data[w + 1] = (data[w + 1] & bfm) | (bits >> (64 - b));
    }

    // Return the next s bits at the pos'th bit in this stream.
    inline uint64_t get_bits(const ui32 &pos, const ui32 &s) const {
#if defined(_BOUND_CHECKING)
        if (!valid_size(pos + s))
            throw std::out_of_range("");
#endif
        if (s == 64)
            return get_bits64(pos);

        const ui32 w = block_index(pos);
        const ui32 b = bit_index(pos);

        // if the range is within a block
        if (b + s <= 64) {
            uint64_t mask = (1ULL << s) - 1;
            return (data[w] >> b) & mask;
        }

        // if the range spans two blocks
        const uint64_t front = data[w] >> b;
        const uint64_t size_back = s - (64 - b);
        const uint64_t back = data[w + 1] & ((1ULL << size_back) - 1);
        return front | (back << (64 - b));
    }
    // Set the next s <= 64 bits at the pos'th bit in this stream
    // and update the number of ones
    inline void set_bits(const ui32 &pos, const ui32 &s, const uint64_t &bits) {
#if defined(_BOUND_CHECKING)
        if (!valid_size(pos + s))
            throw std::out_of_range("");
#endif
        if (s == 64) {
            set_bits64(pos, bits);
            return;
        }

        const uint64_t masked_bits = bits & ((1ULL << s) - 1);
        ones += popcount64(masked_bits);

        const ui32 w = block_index(pos);
        const ui32 b = bit_index(pos);

        if (b + s <= 64) {
            const uint64_t fm = ~(((1ULL << s) - 1) << b); // mask for the fixed bits
            uint64_t old = data[w] & (~fm);
            data[w] = (data[w] & fm) | (masked_bits << b);
            ones -= popcount64(old);
            return;
        }

        const uint64_t ffm = (1ULL << b) - 1; // mask for the fixed bits of data[w]
        const uint64_t size_back = s - (64 - b);
        const uint64_t bfm = ~((1ULL << size_back) - 1); // mask for the fixed bits of data[w+1]

        uint64_t old = 0;
        old = data[w] & (~ffm);
        old = old | (data[w + 1] & (~bfm));
        ones -= popcount64(old);

        data[w] = (data[w] & ffm) | (masked_bits << b);
        data[w + 1] = (data[w + 1] & bfm) | (masked_bits >> (64 - b));
    }
    // Set the next s <= 64 bits at the pos'th bit in this stream
    // without updating the number of ones
    inline void set_bits_no_count(const ui32 &pos, const ui32 &s, const uint64_t &bits) {
#if defined(_BOUND_CHECKING)
        if (!valid_size(pos + s))
            throw std::out_of_range("");
#endif
        if (s == 64) {
            set_bits64_no_count(pos, bits);
            return;
        }

        const uint64_t masked_bits = bits & ((1ULL << s) - 1);

        const ui32 w = block_index(pos);
        const ui32 b = bit_index(pos);

        if (b + s <= 64) {
            const uint64_t fm = ~(((1ULL << s) - 1) << b); // mask for the fixed bits
            data[w] = (data[w] & fm) | (masked_bits << b);
            return;
        }

        const uint64_t ffm = (1ULL << b) - 1; // mask for the fixed bits of data[w]
        const uint64_t size_back = s - (64 - b);
        const uint64_t bfm = ~((1ULL << size_back) - 1); // mask for the fixed bits of data[w+1]
        data[w] = (data[w] & ffm) | (masked_bits << b);
        data[w + 1] = (data[w + 1] & bfm) | (masked_bits >> (64 - b));
    }

    // Expand the current stream by s bits and pad the end with 0's.
    inline void expand(const ui32 &s) {
#if defined(_BOUND_CHECKING)
        if (!valid_capacity(sz + s))
            throw std::out_of_range("");
#endif
        const ui32 start_w = block_index(sz);
        const ui32 start_b = bit_index(sz);
        const ui32 end_w = block_index(sz + s);
        const ui32 end_b = bit_index(sz + s);

        sz += s;
        if (start_w == end_w) {
            uint64_t fm = (1ULL << start_b) - 1;
            data[start_w] = data[start_w] & fm;
            return;
        }

        data[start_w] = data[start_w] & ((1ULL << start_b) - 1);
        for (ui32 i = start_w + 1; i < end_w; i++)
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
        const ui32 start_b = bit_index(sz - s);
        const ui32 end_w = block_index(sz);
        const ui32 end_b = bit_index(sz);

        if (start_w == end_w) {
            uint64_t fm = (1ULL << start_b) - 1;
            ones -= popcount64((data[start_w] >> start_b) & ((1ULL << (end_b - start_b)) - 1));
            sz -= s;
            return;
        }

        ones -= popcount64(data[start_w] & ((~0ULL) << start_b));
        for (ui32 i = start_w + 1; i < end_w; i++)
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
        uint64_t masked_bits;
        if (sz - w * 64 < 64) {
            masked_bits = bits & ((1ULL << (sz - w * 64)) - 1);
            data[w] = data[w] & ((1ULL << (sz - w * 64)) - 1);
        } else
            masked_bits = bits;
        ones -= popcount64(data[w]);
        ones += popcount64(masked_bits);
        data[w] = masked_bits;
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
    inline ui32 rank1(const ui32 &s) const {
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
            count += popcount64(data[w] & ((1ULL << b) - 1));

        return count;
    }
    // Return the position of the s'th 1 in the stream.
    inline ui32 select1(const ui32 &s) const {
        if (s >= ones)
            return sz;
        ui32 count = 0;
        ui32 w = 0;
        while (true) {
            const ui32 word_count = popcount64(data[w]);
            if (count + word_count > s)
                break;
            w++;
            count += word_count;
        }

        uint64_t word = data[w];
        while (count < s) {
            word = word & (word - 1);
            count++;
        }
        return 64 * w + trailing_ones(word);
    }
    // Recalculate the number of ones in this stream.
    inline void pull_ones() {
        ones = rank1(sz);
    }

    // Insert the bit in the pos'th position in the stream and
    // update the number of ones.
    inline void insert_at(const ui32 &pos, const bool &bit) {
#if defined(_BOUND_CHECKING)
        if (!valid_size(pos) || !valid_capacity(sz + 1))
            throw std::out_of_range("");
#endif
        // shift all the block after the block that contains the pos'th bit
        const ui32 w = block_index(pos);
        const ui32 b = bit_index(pos);

        const ui32 end_w = block_index(sz);
        const ui32 end_b = bit_index(sz);

        if (end_w > w) {
            data[end_w] = data[end_w] << 1;

            for (ui32 i = end_w - 1; i > w; i--) {
                data[i + 1] = data[i + 1] | (data[i] >> 63);
                data[i] = data[i] << 1;
            }
            data[w + 1] = data[w + 1] | (data[w] >> 63);
        }

        // inserting the bit in the block containing the pos'th bit
        const uint64_t fm = (1ULL << b) - 1; // mask for the fixed bits
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
    // Remove the pos'th bit in the stream and update the number
    // of ones.
    inline void remove_at(const ui32 &pos) {
#if defined(_BOUND_CHECKING)
        if (!valid_pos(pos) || sz <= 0)
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
    // Insert the bit in the pos'th position in the stream without
    // updating the number of ones.
    inline void insert_at_no_count(const ui32 &pos, const bool &bit) {
#if defined(_BOUND_CHECKING)
        if (!valid_size(pos) || !valid_capacity(sz + 1))
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

            for (ui32 i = end_w - 1; i > w; i--) {
                data[i + 1] = data[i + 1] | (data[i] >> 63);
                data[i] = data[i] << 1;
            }
            data[w + 1] = data[w + 1] | (data[w] >> 63);
        }

        // inserting the bit in the block containing the pos'th bit
        const uint64_t fm = (1ULL << b) - 1; // mask for the fixed bits
        const uint64_t sm = ~fm;           // mask for the shift bits

        // shift the portion after the bit and fix the portion
        // before the bit
        data[w] = ((data[w] & sm) << 1) | (data[w] & fm);

        if (bit)
            data[w] = data[w] | (1ULL << b);
        sz++;
    }
    // Remove the pos'th bit in the stream without updating the
    // number of ones.
    inline void remove_at_no_count(const ui32 &pos) {
#if defined(_BOUND_CHECKING)
        if (!valid_pos(pos) || sz <= 0)
            throw std::out_of_range("");
#endif
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
    inline void copy_to(Datablock *dest, const ui32 &src_pos, const ui32 &dest_pos, const ui32 &n) const {
#if defined(_BOUND_CHECKING)
        if (!valid_size(src_pos + n) || !dest->valid_size(dest_pos + n))
            throw std::out_of_range("");
#endif
        ui32 i;
        for (i = 0; i + 64 < n; i += 64) {
            dest->set_bits64(dest_pos + i, get_bits64(src_pos + i));
        }
        dest->set_bits(dest_pos + i, n - i, get_bits(src_pos + i, n - i));
    }
    // Copy bits in this stream from the range [src_pos, src_pos+n)
    // to the range [dest_pos, dest_pos+n) in dest without updating
    // the number of ones in dest.
    inline void copy_to_no_count(Datablock *dest, const ui32 &src_pos, const ui32 &dest_pos, const ui32 &n) const {
#if defined(_BOUND_CHECKING)
        if (!valid_size(src_pos + n) || !dest->valid_size(dest_pos + n))
            throw std::out_of_range("");
#endif
        const ui32 start_w = block_index(src_pos);
        const ui32 start_b = bit_index(src_pos);
        const ui32 end_w = block_index(src_pos + n);
        const ui32 end_b = bit_index(src_pos + n);

        if (start_w == end_w) {
            dest->set_bits_no_count(dest_pos, n, (data[start_w] >> start_b) & ((1ULL << n) - 1));
            return;
        }

        dest->set_bits_no_count(dest_pos, 64 - start_b, data[start_w] >> start_b);

        for (ui32 i = start_w + 1; i < end_w; i++) {
            dest->set_bits64_no_count(dest_pos + 64 - start_b + 64 * (i - start_w - 1), data[i]);
        }
        if (end_b > 0)
            dest->set_bits_no_count(dest_pos + n - end_b, end_b, data[end_w]);
    }

    // Pad s zeros in the beginning of the bits.
    inline void pad_zeros_front(const ui32 &s) {
#if defined(_BOUND_CHECKING)
        if (!valid_capacity(sz + s))
            throw std::out_of_range("");
#endif
        // block index of the last bit
        const ui32 w = block_index(sz - 1);
        const ui32 w_shift = s / 64;
        const ui32 b_shift = s - w_shift * 64;

        if (b_shift > 0) {
            ui32 end_b = w + w_shift + 1;
            if (end_b >= W)
                end_b = W - 1;

            for (ui32 i = end_b; i > w_shift; i--)
                data[i] = (data[i - w_shift] << b_shift) | (data[i - w_shift - 1] >> (64 - b_shift));

            data[w_shift] = data[0] << b_shift;

            for (ui32 i = 0; i < w_shift; i++)
                data[i] = 0;
        } else {
            ui32 end_b = w + w_shift;
            if (end_b >= W)
                end_b = W - 1;

            const ui32 len = end_b - w_shift;

            for (ui32 i = 0; i <= len; i++)
                data[end_b - i] = data[end_b - i - w_shift];
            for (ui32 i = 0; i < w_shift; i++)
                data[i] = 0;
        }

        sz += s;
    }
    // Shift the bits s bits towards the beginning and
    // update the number of ones.
    inline void shift_backward(const ui32 &s) {
        if (s >= sz) {
            clear();
            return;
        }

        const ui32 new_w = block_index(s);
        const ui32 new_s = bit_index(s);
        for (ui32 i = 0; i < new_w; i++)
            ones -= popcount64(data[i]);
        ones -= popcount64(((1ULL << new_s) - 1) & data[new_w]);

        // round to the smallest multiple of 64 greater than sz
        const ui32 effective_sz = ((sz + 63) / 64) * 64;
        const ui32 w = block_index(effective_sz - s - 1);
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

        sz -= s;
    }
    // Shift the bits s bits towards the beginning without
    // updating the number of ones.
    inline void shift_backward_no_count(const ui32 &s) {
        if (s >= sz) {
            clear();
            return;
        }

        // round to the smallest multiple of 64 greater than sz
        const ui32 effective_sz = ((sz + 63) / 64) * 64;
        const ui32 w = block_index(effective_sz - s - 1);
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

        sz -= s;
    }

    // Combine the data in b1 and b2, insert "bit" at position "at", and
    // split the data into two equally sized datablock in b1 and b2. If
    // the total size is odd, the first block will have one more bit.
    static inline void insert_balance(Datablock *b1, Datablock *b2, const ui32 &at, const bool &bit) {
        const ui32 sz = b1->sz + 1;
        const ui32 sz2 = sz / 2;   // size of the second block
        const ui32 sz1 = sz - sz2; // size of the first block
        ui32 sz_orig = b1->sz;
        ui32 ones_orig = b1->ones;

        b2->clear();

        if (sz2 <= 0) {
            b1->expand(1);
            b1->set_bit(at, bit);
            return;
        }

        if (at == sz1 - 1) {
            b2->expand(sz2);
            b1->copy_to(b2, sz1 - 1, 0, sz2);
            b1->shrink_no_count(sz2 - 1);
            b1->ones -= b2->ones;
            if (b1->get_bit(sz1 - 1))
                b1->ones++;
            else
                b1->set_bit(sz1 - 1, bit);
            return;
        }

        if (at == sz1) {
            b2->expand(sz2);
            b1->copy_to(b2, sz1, 1, sz2 - 1);
            b1->shrink_no_count(sz2 - 1);
            b1->ones -= b2->ones;
            b2->set_bit(0, bit);
            return;
        }

        if (at > sz1) {
            b2->expand(sz2);
            b1->copy_to(b2, sz1, 0, at - sz1);
            b1->copy_to(b2, at, at - sz1 + 1, sz_orig - at);
            b1->shrink_no_count(sz2 - 1);
            b1->ones -= b2->ones;
            b2->set_bit(at - sz1, bit);
        } else {
            b2->expand(sz2);
            b1->copy_to(b2, sz1 - 1, 0, sz2);
            b1->shrink_no_count(sz2);
            b1->ones -= b2->ones;
            b1->insert_at(at, bit);
        }
    }

    // Combine the data in b1 and b2, delete the bit at position "at", and
    // split the data into two equally sized datablock in b1 and b2. If
    // the total size is odd, the first block will have one more bit.
    static void delete_balance(Datablock *b1, Datablock *b2, const ui32 &at) {
        if (b1->sz + b2->sz <= 0)
            return;

        const ui32 sz = b1->sz + b2->sz - 1;
        const ui32 sz2 = sz / 2; // size of the second block
        const ui32 sz1 = sz - sz2; // size of the first block
        ui32 sz1_orig = b1->sz;
        ui32 sz2_orig = b2->sz;
        ui32 ones1_orig = b1->ones;
        ui32 ones2_orig = b2->ones;

        if (at < sz1_orig && sz1 == sz1_orig - 1) {
            b1->remove_at(at);
            return;
        }
        if (at >= sz1_orig && sz1 == sz1_orig) {
            b2->remove_at(at - sz1_orig);
            return;
        }

        if (at == sz1_orig - 1) {
            b1->set_bit(at, false);
            ones1_orig = b1->ones;
            if (sz1 > sz1_orig - 1) {
                b1->expand(sz1 - sz1_orig);
                b2->copy_to(b1, 0, sz1_orig - 1, sz1 - sz1_orig + 1);
                b2->shift_backward_no_count(sz1 - sz1_orig + 1);
                b2->ones -= b1->ones - ones1_orig;
            } else {
                b2->pad_zeros_front(sz2 - sz2_orig);
                b1->copy_to(b2, sz1, 0, sz2 - sz2_orig);
                b1->shrink_no_count(sz1_orig - sz1);
                b1->ones -= b2->ones - ones2_orig;
            }
            return;
        }

        if (at == sz1_orig) {
            b2->set_bit(0, false);
            ones2_orig = b2->ones;
            if (sz1 > sz1_orig) {
                b1->expand(sz1 - sz1_orig);
                b2->copy_to(b1, 1, sz1_orig, sz1 - sz1_orig);
                b2->shift_backward_no_count(sz2_orig - sz2);
                b2->ones -= b1->ones - ones1_orig;
            } else {
                b2->pad_zeros_front(sz2 - sz2_orig);
                b1->copy_to(b2, sz1, 0, sz2 - sz2_orig + 1);
                b1->shrink_no_count(sz1_orig - sz1);
                b1->ones -= b2->ones - ones2_orig;
            }
            return;
        }

        if (at < sz1_orig) {
            // the bit to remove is in the first block
            if (sz1 > sz1_orig - 1) {
                // if we need to move bits from b2 to b1
                b1->remove_at(at);
                sz1_orig = b1->sz;
                ones1_orig = b1->ones;

                b1->expand(sz1 - sz1_orig);
                b2->copy_to(b1, 0, sz1_orig, sz1 - sz1_orig);
                b2->shift_backward_no_count(sz1 - sz1_orig);
                b2->ones -= b1->ones - ones1_orig;
            } else {
                // if we need to move bits from b1 to b2
                if (at < sz1) {
                    b2->pad_zeros_front(sz2 - sz2_orig);
                    b1->copy_to(b2, sz1 + 1, 0, sz2 - sz2_orig);
                    b1->shrink_no_count(sz1_orig - sz1 - 1);
                    b1->ones -= b2->ones - ones2_orig;
                    b1->remove_at(at);
                } else {
                    b2->pad_zeros_front(sz2 - sz2_orig);
                    b1->copy_to(b2, sz1, 0, at - sz1);
                    b1->copy_to(b2, at + 1, at - sz1, sz1_orig - at - 1);
                    if (b1->get_bit(at))
                        b1->ones--;
                    b1->shrink_no_count(b1->sz - sz1);
                    b1->ones -= b2->ones - ones2_orig;
                }
            }
        } else {
            if (sz1 > sz1_orig) {
                // if we need to move bits from b2 to b1
                if (at < sz1) {
                    b1->expand(sz1 - sz1_orig);
                    b2->copy_to(b1, 0, sz1_orig, at - sz1_orig);
                    b2->copy_to(b1, at - sz1_orig + 1, at, sz1 - at);
                    bool removed_bit = b2->get_bit(at - sz1_orig);
                    b2->shift_backward_no_count(sz2_orig - sz2);
                    b2->ones -= b1->ones - ones1_orig;
                    if (removed_bit)
                        b2->ones--;
                } else {
                    b1->expand(sz1 - sz1_orig);
                    b2->copy_to(b1, 0, sz1_orig, sz1 - sz1_orig);
                    b2->shift_backward_no_count(sz1 - sz1_orig);
                    b2->ones -= b1->ones - ones1_orig;
                    b2->remove_at(at - sz1);
                }
            } else {
                // if we need to move bits from b1 to b2
                b2->remove_at(at - b1->sz);
                sz2_orig = b2->sz;
                ones2_orig = b2->ones;

                b2->pad_zeros_front(sz2 - sz2_orig);
                b1->copy_to(b2, sz1, 0, b1->sz - sz1);
                b1->shrink_no_count(sz1_orig - sz1);
                b1->ones -= b2->ones - ones2_orig;
            }
        }
    }
    // Combine the data in b1 and b2, delete the bit at position "at", and
    // put the output in b1.
    static inline void delete_merge_left(Datablock *b1, Datablock *b2, const ui32 &pos) {
        const ui32 sz1_orig = b1->sz;
        if (pos < sz1_orig) {
            b1->remove_at(pos);
            b1->expand(b2->sz);
            b2->copy_to_no_count(b1, 0, sz1_orig - 1, b2->sz);
            b1->ones += b2->ones;
            b2->clear();
        } else {
            ui32 pos_loc = pos - sz1_orig;
            b1->expand(b2->sz - 1);
            b2->copy_to_no_count(b1, 0, sz1_orig, pos_loc);
            b2->copy_to_no_count(b1, pos_loc + 1, sz1_orig + pos_loc, b2->sz - pos_loc - 1);
            b1->ones += b2->ones;
            if (b2->get_bit(pos_loc))
                b1->ones--;
            b2->clear();
        }
    }
    // Combine the data in b1 and b2, delete the bit at position "at", and
    // put the output in b2.
    static inline void delete_merge_right(Datablock *b1, Datablock *b2, const ui32 &at) {
        if (at < b1->sz) {
            b2->pad_zeros_front(b1->sz - 1);
            b1->copy_to_no_count(b2, 0, 0, at);
            b1->copy_to_no_count(b2, at + 1, at, b1->sz - 1 - at);
            b2->ones += b1->ones;
            if (b1->get_bit(at))
                b2->ones--;
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



#endif // __DATABLOCK_H__