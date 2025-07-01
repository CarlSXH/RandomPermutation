


#include "Testing.h"
#include <random>
#include <bitset>
#include <iostream>
#include <vector>

#include <cassert>
#include "RandPerm.h"
#include <chrono>





#include "WaveletTree.h"

using namespace std;

// 1825102
#define _RANDOM_SEED 109182

void test_sizing_out_of_bound() {
    Datablock<10> db;

    assert(db.sz == 0);

    std::minstd_rand rng(_RANDOM_SEED);

    while (db.sz < 400) {
        std::uniform_int_distribution<int> dist(1, 400 - db.sz);
        ui32 orig_size = db.sz;
        ui32 add_size = dist(rng);
        db.expand(add_size);
        assert(db.sz == orig_size + add_size);
        assert(db.ones == 0);
    }

    // so now the size is 400
    for (int i = 0; i < 400; i++)
        db.set_bit(i, true);
    assert(db.ones == 400);


    while (db.sz > 0) {
        std::uniform_int_distribution<int> dist(1, db.sz);
        ui32 orig_size = db.sz;
        ui32 remove_size = dist(rng);
        db.shrink(remove_size);
        assert(db.sz == orig_size - remove_size);
        assert(db.ones == orig_size - remove_size);
    }

    db.expand(400);

    for (int i = 0; i < 400; i++)
        db.set_bit(i, true);
    assert(db.ones == 400);

    while (db.sz > 0) {
        std::uniform_int_distribution<int> dist(1, db.sz);
        ui32 orig_size = db.sz;
        ui32 remove_size = dist(rng);
        db.shrink_no_count(remove_size);
        assert(db.sz == orig_size - remove_size);
        assert(db.ones == 400);
    }
}
void test_insert_end() {
    Datablock<3> db;



}
void test_get_set_1_bit() {
    ui32 n = 5 * 64 - 100;

    Datablock<5> db;
    // empty on construction
    assert(db.sz == 0 && db.ones == 0);

    // expand and ensure zeros
    db.expand(n);
    assert(db.sz == n);
    for (ui32 i = 0; i < n; i++)
        assert(db.get_bit(i) == false);
    assert(db.ones == 0);

    // setting one bit at a time
    for (ui32 i = 0; i < n; i++) {
        db.set_bit(i, true);
        assert(db.ones == 1);
        assert(db.sz == n);
        for (ui32 j = 0; j < n; j++) {
            if (j == i)
                assert(db.get_bit(j) == true);
            else
                assert(db.get_bit(j) == false);
        }
        db.set_bit(i, false);
        assert(db.get_bit(i) == false);
        assert(db.ones == 0);
        assert(db.sz == n);
    }

    // test set_bit_no_count
    for (ui32 i = 0; i < n; i++) {
        db.set_bit_no_count(i, true);
        assert(db.ones == 0);
        assert(db.sz == n);
        for (ui32 j = 0; j < n; j++) {
            if (j == i)
                assert(db.get_bit(j) == true);
            else
                assert(db.get_bit(j) == false);
        }
        db.set_bit_no_count(i, false);
        assert(db.get_bit(i) == false);
        assert(db.ones == 0);
        assert(db.sz == n);
    }
}
void test_get_set_2_bit() {
    ui32 n = 5 * 64 - 100;

    Datablock<5> db;
    // empty on construction
    assert(db.sz == 0 && db.ones == 0);

    // expand and ensure zeros
    db.expand(n);
    assert(db.sz == n);
    for (ui32 i = 0; i < n; i++)
        assert(db.get_bit(i) == false);
    assert(db.ones == 0);

    // setting two bits
    for (ui32 i = 0; i < n; i++) {
        for (ui32 j = 0; j < n; j++) {
            if (i == j)
                continue;

            db.set_bit(i, true);
            db.set_bit(j, true);
            assert(db.ones == 2);
            assert(db.sz == n);
            for (ui32 k = 0; k < n; k++) {
                if (k == i || k == j)
                    assert(db.get_bit(k) == true);
                else
                    assert(db.get_bit(k) == false);
            }
            db.set_bit(i, false);
            db.set_bit(j, false);
            assert(db.ones == 0);
            assert(db.sz == n);
        }
    }
}
void test_get_set_random_bit() {
    ui32 n = 5 * 64 - 100;

    std::vector<bool> vec;
    Datablock<5> db;
    // empty on construction
    assert(db.sz == 0 && db.ones == 0);

    // expand and ensure zeros
    db.expand(n);
    vec.reserve(n);
    assert(db.sz == n);

    std::minstd_rand rng(_RANDOM_SEED);
    ui32 ones = 0;

    for (ui32 i = 0; i < n; i++) {
        bool bit = (rng() % 2) == 0;
        db.set_bit(i, bit);
        vec.push_back(bit);
        if (bit)
            ones++;

        assert(bit == db.get_bit(i));
    }

    assert(db.ones == ones);

    for (ui32 i = 0; i < 10000; i++) {
        ui32 index = rng() % n;
        if (vec[index]) {
            vec[index] = false;
            db.set_bit(index, false);
        } else {
            vec[index] = true;
            db.set_bit(index, true);
        }
    }

    for (ui32 i = 0; i < n; i++)
        assert(db.get_bit(i) == vec[i]);
}
void test_bits64() {
    Datablock<10> db;
    db.clear();

    ui32 n = 10 * 64 - 100;
    db.expand(n);


    std::minstd_rand rng(_RANDOM_SEED);
    std::vector<bool> vec;
    vec.reserve(n);

    for (ui32 i = 0; i < n; i++) {
        std::uniform_int_distribution<uint64_t> dist(0, 1);
        bool bit = (dist(rng) == 0);
        db.set_bit(i, bit);
        vec.push_back(bit);
    }

    uint64_t bits = 0;
    for (int i = 0; i < 64; i++) {
        if (db.get_bit(n - i - 1))
            bits = (bits << 1) | 1ULL;
        else
            bits = (bits << 1);
    }
        

    for (int i = n - 64; i >= 0; i--) {
        assert(db.get_bits64(i) == bits);
        if (i > 0 && db.get_bit(i-1))
            bits = (bits << 1) | 1ULL;
        else
            bits = (bits << 1);
    }


    for (ui32 i = 0; i < n - 64; i++) {
        std::uniform_int_distribution<uint64_t> dist;
        uint64_t rand_bits = dist(rng);
        db.set_bits64(i, rand_bits);

        for (ui32 j = 0; j < 64; j++) {
            vec[i + j] = rand_bits % 2 == 1;
            rand_bits = rand_bits >> 1;
        }

        ui32 ones = 0;
        for (ui32 j = 0; j < n; j++)
            if (vec[j])
                ones++;
        assert(db.ones == ones);

        for (ui32 j = 0; j < n; j++)
            assert(vec[j] == db.get_bit(j));
    }



}
void test_bits64_no_count() {
    Datablock<10> db;
    db.clear();

    ui32 n = 10 * 64 - 100;
    db.expand(n);


    std::minstd_rand rng(_RANDOM_SEED);
    std::vector<bool> vec;
    vec.reserve(n);

    for (ui32 i = 0; i < n; i++) {
        std::uniform_int_distribution<uint64_t> dist(0, 1);
        bool bit = (dist(rng) == 0);
        db.set_bit(i, bit);
        vec.push_back(bit);
    }

    ui32 orig_ones = db.ones;

    for (ui32 i = 0; i < n - 64; i++) {
        std::uniform_int_distribution<uint64_t> dist;
        uint64_t rand_bits = dist(rng);
        db.set_bits64_no_count(i, rand_bits);
        assert(db.ones == orig_ones);

        for (ui32 j = 0; j < 64; j++) {
            vec[i + j] = rand_bits % 2 == 1;
            rand_bits = rand_bits >> 1;
        }

        for (ui32 j = 0; j < n; j++)
            assert(vec[j] == db.get_bit(j));
    }

}
void test_bits_s() {

    ui32 n = 10 * 64 - 100;
    std::minstd_rand rng(_RANDOM_SEED);
    std::vector<bool> vec;
    vec.reserve(n);

    Datablock<10> db;

    db.clear();
    db.expand(n);

    for (ui32 i = 0; i < n; i++) {
        bool bit = (rng() % 2) == 0;
        db.set_bit(i, bit);
        vec.push_back(bit);
    }

    for (ui32 s = 1; s < 64; s++) {

        uint64_t bits = 0;
        for (ui32 i = 0; i < s; i++) {
            if (db.get_bit(n - i - 1))
                bits = (bits << 1) | 1ULL;
            else
                bits = (bits << 1);
        }

        for (int i = n - s; i >= 0; i--) {
            assert(db.get_bits(i, s) == bits);
            if (i > 0 && db.get_bit(i - 1))
                bits = (bits << 1) | 1ULL;
            else
                bits = (bits << 1);
            bits = bits & ((1ULL << s) - 1);
        }

        for (ui32 i = 0; i < n - s; i++) {
            std::uniform_int_distribution<uint64_t> dist;
            uint64_t rand_bits = dist(rng);
            db.set_bits(i, s, rand_bits);

            for (ui32 j = 0; j < s; j++) {
                vec[i + j] = rand_bits % 2 == 1;
                rand_bits = rand_bits >> 1;
            }

            ui32 ones = 0;
            for (ui32 j = 0; j < n; j++)
                if (vec[j])
                    ones++;
            assert(db.ones == ones);

            for (ui32 j = 0; j < n; j++)
                assert(vec[j] == db.get_bit(j));
        }
    }
}
void test_bits_s_no_count() {

    ui32 n = 10 * 64 - 100;
    std::minstd_rand rng(_RANDOM_SEED);
    std::vector<bool> vec;
    vec.reserve(n);

    Datablock<10> db;

    db.clear();
    db.expand(n);

    for (ui32 i = 0; i < n; i++) {
        bool bit = (rng() % 2) == 0;
        db.set_bit(i, bit);
        vec.push_back(bit);
    }

    ui32 orig_ones = db.ones;

    for (ui32 s = 1; s < 64; s++) {

        for (ui32 i = 0; i < n - s; i++) {
            std::uniform_int_distribution<uint64_t> dist;
            uint64_t rand_bits = dist(rng);
            db.set_bits_no_count(i, s, rand_bits);
            assert(db.ones == orig_ones);

            for (ui32 j = 0; j < s; j++) {
                vec[i + j] = rand_bits % 2 == 1;
                rand_bits = rand_bits >> 1;
            }


            for (ui32 j = 0; j < n; j++)
                assert(vec[j] == db.get_bit(j));
        }

    }
}
void test_expand_shrink_clear() {
    Datablock<10> db;
    db.clear();

    // expand
    db.expand(512);
    assert(db.sz == 512);

    for (ui32 i = 0; i < 512; i++)
        db.set_bit(i, true);

    db.shrink(100);
    db.expand(100);

    assert(db.ones == 512 - 100);
    for (ui32 i = 0; i < 512 - 100; i++)
        assert(db.get_bit(i) == true);
    for (ui32 i = 512-100; i < 512; i++)
        assert(db.get_bit(i) == false);
}
void test_block_ops() {
    Datablock<1> db;
    db.clear();

    db.expand(64);

    db.set_block(0, ~0ULL);
    assert(db.get_block(0) == ~0ULL);
    assert(db.ones == 64);

    db.set_block_no_count(0, 0ULL);
    assert(db.get_block(0) == 0ULL);
    assert(db.ones == 64);
}
void test_rank_and_pull() {

    Datablock<10> db;
    std::vector<bool> vec;
    ui32 n = 10 * 64 - 10;

    std::minstd_rand rng(_RANDOM_SEED);

    db.expand(n);
    vec.reserve(n);

    for (ui32 i = 0; i < n; i++) {
        bool bit = rng() % 2 == 0;
        vec.push_back(bit);
        db.set_bit(i, bit);
    }

    ui32 partial_count = 0;

    for (ui32 i = 0; i < n; i++) {
        assert(db.rank1(i) == partial_count);
        if (vec[i])
            partial_count++;
    }
}
void test_select1() {
    for (int i = 0; i < 100; i++) {
        ui32 n = 300;
        vector<bool> vec(n);
        Datablock<10> db;
        db.expand(n);
        std::minstd_rand rng(_RANDOM_SEED);

        ui32 ones = 0;
        for (ui32 j = 0; j < n; j++) {
            uniform_int_distribution<ui32> dist(0, 1);
            bool bit = dist(rng) == 1;
            db.set_bit(j, bit);
            vec[j] = bit;
            if (bit)
                ones++;
        }

        ui32 last_loc = -1;
        for (ui32 j = 0; j < ones; j++) {
            last_loc++;
            while (!vec[last_loc])
                last_loc++;

            assert(db.select1(j) == last_loc);
        }
    }
}
void test_insert_remove() {
    ui32 n = 10 * 64;
    std::vector<bool> vec;
    Datablock<10> db;
    db.clear();

    std::minstd_rand rng(_RANDOM_SEED);

    for (ui32 i = 0; i < n; i++) {
        std::uniform_int_distribution<ui32> dist(0, i);
        ui32 index = dist(rng);
        bool bit = (rng() % 2 == 1);
        vec.insert(vec.begin() + index, bit);
        db.insert_at(index, bit);

        assert(db.sz == i + 1);
        ui32 ones = 0;
        for (ui32 j = 0; j < i + 1; j++)
            if (vec[j])
                ones++;
        assert(db.ones == ones);

        for (ui32 j = 0; j < i + 1; j++)
            assert(db.get_bit(j) == vec[j]);
    }

    for (ui32 i = 0; i < n; i++) {
        std::uniform_int_distribution<ui32> dist(0, n-i-1);
        ui32 index = dist(rng);
        vec.erase(vec.begin() + index);
        db.remove_at(index);

        assert(db.sz == n-i-1);
        ui32 ones = 0;
        for (ui32 j = 0; j < n-i-1; j++)
            if (vec[j])
                ones++;
        assert(db.ones == ones);
        
        for (ui32 j = 0; j < n-i-1; j++)
            assert(db.get_bit(j) == vec[j]);
    }

    for (ui32 i = 0; i < n; i++) {
        std::uniform_int_distribution<ui32> dist(0, i);
        ui32 index = dist(rng);
        bool bit = (rng() % 2 == 1);
        vec.insert(vec.begin() + index, bit);
        db.insert_at_no_count(index, bit);

        assert(db.sz == i + 1);
        assert(db.ones == 0);

        for (ui32 j = 0; j < i + 1; j++)
            assert(db.get_bit(j) == vec[j]);
    }

    for (ui32 i = 0; i < n; i++) {
        std::uniform_int_distribution<ui32> dist(0, n - i - 1);
        ui32 index = dist(rng);
        vec.erase(vec.begin() + index);
        db.remove_at_no_count(index);

        assert(db.sz == n - i - 1);
        assert(db.ones == 0);

        for (ui32 j = 0; j < n - i - 1; j++)
            assert(db.get_bit(j) == vec[j]);
    }

    n = 400;
    db.expand(n);
    vec.reserve(n);
    ui32 ones = 0;
    for (ui32 i = 0; i < n; i++) {
        std::uniform_int_distribution<ui32> dist(0, 1);
        bool bit = (rng() == 1);
        vec.push_back(bit);
        db.set_bit(i, bit);
        if (bit)
            ones++;
    }


    for (ui32 b = 0; b < 2; b++) {
        for (ui32 i = 0; i <= n; i++) {
            vec.insert(vec.begin() + i, b%2==0);
            db.insert_at(i, b%2==0);

            assert(db.sz == n + 1);
            if (b % 2 == 0)
                assert(db.ones == ones + 1);
            else
                assert(db.ones == ones);

            for (ui32 j = 0; j < n + 1; j++)
                assert(db.get_bit(j) == vec[j]);

            db.remove_at(i);
            vec.erase(vec.begin() + i);

            assert(db.sz == n);
            assert(db.ones == ones);

            for (ui32 j = 0; j < n; j++)
                assert(db.get_bit(j) == vec[j]);
        }
    }
}
void test_copy_to() {
    ui32 n = 100;
    Datablock<3> src, dest, dest_2;
    src.clear();
    dest.clear();
    src.expand(n);
    dest.expand(n);
    dest_2.expand(n);


    std::minstd_rand rng(_RANDOM_SEED);

    for (ui32 s = 1; s < n - 1; s++) {
        for (ui32 i = 0; i < n - s; i++) {
            for (ui32 j = 0; j < n - s; j++) {
                ui32 ones1 = 0, ones2 = 0;
                for (ui32 x = 0; x < n; x++) {
                    bool bit1 = rng() % 2 == 0;
                    src.set_bit(x, bit1);
                    if (bit1)
                        ones1++;

                    bool bit2 = rng() % 2 == 0;
                    dest.set_bit(x, bit2);
                    dest_2.set_bit(x, bit2);
                    if (bit2)
                        ones2++;
                }

                assert(src.sz == n);
                assert(dest.sz == n);
                assert(src.ones == ones1);
                assert(dest.ones == ones2);

                ui32 orig_ones = 0;
                ui32 final_ones = 0;
                for (ui32 x = 0; x < s; x++) {
                    if (dest.get_bit(j + x))
                        orig_ones++;
                }
                for (ui32 x = 0; x < s; x++) {
                    if (src.get_bit(i + x))
                        final_ones++;
                }

                src.copy_to(&dest_2, i, j, s);
                assert(dest_2.ones == ones2 - orig_ones + final_ones);

                for (ui32 x = 0; x < n; x++) {
                    if (x >= j && x - j < s)
                        assert(dest_2.get_bit(x) == src.get_bit(x - j + i));
                    else
                        assert(dest_2.get_bit(x) == dest.get_bit(x));

                }
            }
        }
    }
}
void test_copy_to_no_count() {
    ui32 n = 200;
    Datablock<5> src, dest, dest_2;
    src.clear();
    dest.clear();
    src.expand(n);
    dest.expand(n);
    dest_2.expand(n);


    std::minstd_rand rng(_RANDOM_SEED);

    for (ui32 s = 1; s < n - 1; s++) {
        for (ui32 i = 0; i < n - s; i++) {
            for (ui32 j = 0; j < n - s; j++) {

                src.clear();
                dest.clear();
                dest_2.clear();
                src.expand(n);
                dest.expand(n);
                dest_2.expand(n);

                ui32 ones1 = 0, ones2 = 0;
                for (ui32 x = 0; x < n; x++) {
                    bool bit1 = rng() % 2 == 0;
                    src.set_bit(x, bit1);
                    if (bit1)
                        ones1++;

                    bool bit2 = rng() % 2 == 0;
                    dest.set_bit(x, bit2);
                    dest_2.set_bit(x, bit2);
                    if (bit2)
                        ones2++;
                }

                assert(src.sz == n);
                assert(dest.sz == n);
                assert(src.ones == ones1);
                assert(dest.ones == ones2);

                src.copy_to_no_count(&dest_2, i, j, s);
                assert(dest_2.ones == ones2);

                for (ui32 x = 0; x < n; x++) {
                    if (x >= j && x - j < s)
                        assert(dest_2.get_bit(x) == src.get_bit(x - j + i));
                    else
                        assert(dest_2.get_bit(x) == dest.get_bit(x));

                }
            }
        }
    }
}
void test_pad_shift() {
    Datablock<10> db;
    db.clear();

    ui32 n = 300;
    vector<bool> vec;
    std::minstd_rand rng(_RANDOM_SEED);


    db.expand(n);
    ui32 ones = 0;
    for (ui32 i = 0; i < n; i++) {
        uniform_int_distribution<ui32> dist(0, 1);
        bool bit = dist(rng) == 1;
        vec.push_back(bit);
        db.set_bit(i, bit);
        if (bit)
            ones++;
    }

    assert(db.sz == n);
    assert(db.ones == ones);

    for (ui32 i = 1; i < 10 * 64 - n; i++) {
        if (i == 64) {
            int l = 0;
        }
        db.pad_zeros_front(i);
        assert(db.sz == i + n);
        assert(db.ones == ones);
        for (ui32 j = 0; j < i; j++)
            assert(db.get_bit(j) == false);
        for (ui32 j = 0; j < n; j++)
            assert(db.get_bit(j+i) == vec[j]);
        db.shift_backward(i);
        assert(db.sz == n);
        assert(db.ones == ones);
        for (ui32 j = 0; j < n; j++)
            assert(db.get_bit(j) == vec[j]);
    }

    for (ui32 i = 0; i < n; i++) {
        db.shift_backward(i);
        assert(db.sz == n - i);
        assert(db.ones == ones);
        for (ui32 j = i; j < n; j++)
            assert(db.get_bit(j - i) == vec[j]);
        db.pad_zeros_front(i);
        assert(db.sz == n);
        assert(db.ones == ones);
        for (ui32 j = 0; j < i; j++)
            assert(db.get_bit(j) == false);
        for (ui32 j = i; j < n; j++)
            assert(db.get_bit(j) == vec[j]);

        if (db.get_bit(i))
            ones--;
    }


    for (ui32 i = 0; i < n; i++) {
        db.set_bit(i, vec[i]);
        if (vec[i])
            ones++;
    }

    for (ui32 i = 0; i < n; i++) {
        db.shift_backward_no_count(i);
        assert(db.sz == n - i);
        assert(db.ones == ones);
        for (ui32 j = i; j < n; j++)
            assert(db.get_bit(j - i) == vec[j]);
        db.pad_zeros_front(i);
        assert(db.sz == n);
        assert(db.ones == ones);
        for (ui32 j = 0; j < i; j++)
            assert(db.get_bit(j) == false);
        for (ui32 j = i; j < n; j++)
            assert(db.get_bit(j) == vec[j]);
    }
}
void test_insert_balance() {
    // Prepare small patterns
    Datablock<10> b1, b2;

    ui32 n = 500;
    vector<bool> vec(n);


    std::minstd_rand rng(_RANDOM_SEED);

    for (ui32 i = 0; i < n; i++) {
        for (ui32 at = 0; at <= i; at++) {
            b1.clear(); b2.clear();

            b1.expand(i);
            for (ui32 j = 0; j < i; j++) {
                bool bit = rng() % 2 == 1;
                b1.set_bit_no_count(j, bit);
                vec[j] = bit;
            }
            b1.pull_ones();

            ui32 ones_sum = b1.ones;

            if (i == 128 && at == 4) {
                int l = 0;
            }

            Datablock<10>::insert_balance(&b1, &b2, at, true);

            assert(b1.sz + b2.sz == i + 1);
            assert(b1.sz >= b2.sz);
            assert(b1.sz - b2.sz <= 1);
            assert(b1.ones + b2.ones == ones_sum + 1);

            for (ui32 j = 0; j < i; j++) {
                if (j < b1.sz) {
                    if (at < j)
                        assert(b1.get_bit(j) == vec[j - 1]);
                    else if (at == j)
                        assert(b1.get_bit(j) == true);
                    else
                        assert(b1.get_bit(j) == vec[j]);
                } else {
                    if (at < j)
                        assert(b2.get_bit(j - b1.sz) == vec[j - 1]);
                    else if (at == j)
                        assert(b2.get_bit(j - b1.sz) == true);
                    else
                        assert(b2.get_bit(j - b1.sz) == vec[j]);
                }
            }
        }
    }
}
void test_delete_balance() {

    // Prepare small patterns
    Datablock<10> b1, b2;

    ui32 n = 500;
    vector<bool> vec(n);


    std::minstd_rand rng(_RANDOM_SEED);


    for (ui32 i = 0; i < n; i++) {
        for (ui32 at = 0; at < n; at++) {
            b1.clear(); b2.clear();

            b1.expand(i);
            for (ui32 j = 0; j < i; j++) {
                bool bit = rng() % 2 == 1;
                b1.set_bit(j, bit);
                vec[j] = bit;
            }
            b2.expand(n - i);
            for (ui32 j = 0; j < n - i; j++) {
                bool bit = rng() % 2 == 1;
                b2.set_bit(j, bit);
                vec[i + j] = bit;
            }

            ui32 ones_sum = b1.ones + b2.ones;
            if (at < b1.sz) {
                if (b1.get_bit(at))
                    ones_sum--;
            } else {
                if (b2.get_bit(at - b1.sz))
                    ones_sum--;
            }

            if (i == 252 && at == 251) {
                int l = 0;
            }

            if (i == 250 && at == 250) {
                int l = 0;
            }

            Datablock<10>::delete_balance(&b1, &b2, at);

            assert(b1.sz + b2.sz == n - 1);
            assert(b1.sz >= b2.sz);
            assert(b1.sz - b2.sz <= 1);
            assert(b1.ones + b2.ones == ones_sum);

            for (ui32 j = 0; j < n - 1; j++) {
                if (j < b1.sz) {
                    if (at <= j)
                        assert(b1.get_bit(j) == vec[j + 1]);
                    else
                        assert(b1.get_bit(j) == vec[j]);
                } else {
                    if (at <= j)
                        assert(b2.get_bit(j - b1.sz) == vec[j + 1]);
                    else
                        assert(b2.get_bit(j - b1.sz) == vec[j]);
                }

            }
        }
    }
}
void test_delete_merge_left() {

    // Prepare small patterns
    Datablock<10> b1, b2;

    ui32 n = 500;
    vector<bool> vec(n);


    std::minstd_rand rng(_RANDOM_SEED);


    for (ui32 i = 0; i < n; i++) {
        for (ui32 at = 0; at < n; at++) {
            b1.clear(); b2.clear();
            ui32 ones = 0;

            b1.expand(i);
            for (ui32 j = 0; j < i; j++) {
                bool bit = rng() % 2 == 1;
                b1.set_bit(j, bit);
                vec[j] = bit;
                if (bit)
                    ones++;
            }

            b2.expand(n - i);
            for (ui32 j = 0; j < n - i; j++) {
                bool bit = rng() % 2 == 1;
                b2.set_bit(j, bit);
                vec[i + j] = bit;
                if (bit)
                    ones++;
            }

            if (i == 1 && at == 0) {
                int l = 0;
            }

            Datablock<10>::delete_merge_left(&b1, &b2, at);

            assert(b1.sz == n - 1);
            assert(b2.sz == 0);
            assert(b2.ones == 0);
            if (vec[at])
                assert(b1.ones == ones - 1);
            else
                assert(b1.ones == ones);

            for (ui32 j = 0; j < n - 1; j++) {
                if (at <= j)
                    assert(b1.get_bit(j) == vec[j + 1]);
                else
                    assert(b1.get_bit(j) == vec[j]);
            }
        }
    }
}
void test_delete_merge_right() {
    // Prepare small patterns
    Datablock<10> b1, b2;

    ui32 n = 500;
    vector<bool> vec(n);


    std::minstd_rand rng(_RANDOM_SEED);


    for (ui32 i = 0; i < n; i++) {
        for (ui32 at = 0; at < i; at++) {
            b1.clear(); b2.clear();
            ui32 ones = 0;
            b1.expand(i);
            for (ui32 j = 0; j < i; j++) {
                bool bit = rng() % 2 == 1;
                b1.set_bit(j, bit);
                vec[j] = bit;
                if (bit)
                    ones++;
            }

            b2.expand(n - i);
            for (ui32 j = 0; j < n - i; j++) {
                bool bit = rng() % 2 == 1;
                b2.set_bit(j, bit);
                vec[i + j] = bit;
                if (bit)
                    ones++;
            }

            Datablock<10>::delete_merge_right(&b1, &b2, at);

            assert(b2.sz == n - 1);
            assert(b1.sz == 0);
            assert(b1.ones == 0);
            if (vec[at])
                assert(b2.ones == ones - 1);
            else
                assert(b2.ones == ones);

            for (ui32 j = 0; j < n - 1; j++) {
                if (at <= j)
                    assert(b2.get_bit(j) == vec[j + 1]);
                else
                    assert(b2.get_bit(j) == vec[j]);
            }
        }
    }
}

void DatablockTest()
{
    test_sizing_out_of_bound();
    cout << "Sizing out of bound test finished" << endl;
    test_insert_end();
    cout << "Insert at the end test finished" << endl;
    test_get_set_1_bit();
    cout << "Set all 1 bit test finished" << endl;
    test_get_set_2_bit();
    cout << "Set all combination of two bits test finished" << endl;
    test_get_set_random_bit();
    cout << "Set random bits test finished" << endl;
    test_bits64();
    cout << "Set and get 64 bit test finished" << endl;
    test_bits64_no_count();
    cout << "Set and get 64 bit no count test finished" << endl;
    test_bits_s();
    cout << "Set and get bits test finished" << endl;
    test_bits_s_no_count();
    cout << "Set and get bits no count test finished" << endl;
    test_expand_shrink_clear();
    cout << "Expand and shrink operations test finished" << endl;
    test_block_ops();
    cout << "Block operations test finished" << endl;
    test_rank_and_pull();
    cout << "Ones count and pull test finished" << endl;
    test_select1();
    cout << "Rank 1 test finished" << endl;
    test_insert_remove();
    cout << "Insert remove test finished" << endl;
    test_copy_to();
    cout << "Copy to test finished" << endl;
    test_copy_to_no_count();
    cout << "Copy to no count test finished" << endl;
    test_pad_shift();
    cout << "Pad zero front and shift left test finished" << endl;
    test_insert_balance();
    cout << "insert balance test finished" << endl;
    test_delete_balance();
    cout << "Delete balance test finished" << endl;
    test_delete_merge_left();
    cout << "Delete merge left test finished" << endl;
    test_delete_merge_right();
    cout << "Delete merge right test finished" << endl;
}


void test_dynamic_bitvector_general() {
    ui32 n = 10000;
    DynamicBitvector<128> b;
    vector<bool> vec(n);

    std::minstd_rand rng(_RANDOM_SEED);

    ui32 ones = 0;

    for (ui32 i = 0; i < n; i++) {
        uniform_int_distribution<ui32> dist(0, 1);
        bool bit = dist(rng) == 1;
        if (i == 171) {
            int l = 0;
        }
        b.insert(i, bit);
        vec[i] = bit;
        assert(b.size() == i + 1);
        if (bit)
            ones++;
        assert(b.ones() == ones);
    }

    for (ui32 i = 0; i < n; i++) {
        assert(b.get_bit(i) == vec[i]);
    }

    for (ui32 i = 0; i < 10000; i++) {
        uniform_int_distribution<ui32> dist(0, n);
        uniform_int_distribution<ui32> bit_dist(0, 1);
        ui32 insert_pos = dist(rng);
        ui32 delete_pos = dist(rng);

        bool bit = bit_dist(rng) == 1;
        if (bit)
            ones++;

        b.insert(insert_pos, bit);
        assert(b.size() == n + 1);
        assert(b.ones() == ones);

        vec.insert(vec.begin() + insert_pos, bit);

        for (ui32 j = 0; j <= n; j++) {
            assert(b.get_bit(j) == vec[j]);
        }

        b.erase(delete_pos);

        assert(b.size() == n);
        if (vec[delete_pos])
            ones--;
        assert(b.ones() == ones);

        vec.erase(vec.begin() + delete_pos);

        for (ui32 j = 0; j < n; j++) {
            assert(b.get_bit(j) == vec[j]);
        }
    }


    for (ui32 i = 0; i < n; i++) {
        assert(b.get_bit(i) == vec[i]);
    }
}
void test_dynamic_bitvector_select() {
    ui32 n = 10000;
    DynamicBitvector<128> b;
    vector<bool> vec(n);

    std::minstd_rand rng(_RANDOM_SEED);

    ui32 ones = 0;

    for (ui32 i = 0; i < n; i++) {
        uniform_int_distribution<ui32> dist(0, 1);
        bool bit = dist(rng) == 1;
        b.insert(i, bit);
        vec[i] = bit;
        if (bit)
            ones++;
    }
    rng.~minstd_rand();

    for (ui32 i = 0; i < n; i++) {
        assert(b.get_bit(i) == vec[i]);
    }

    ui32 last_loc = -1;

    for (ui32 i = 0; i < ones; i++) {
        last_loc++;
        while (!vec[last_loc])
            last_loc++;
        assert(b.select1(i) == last_loc);
    }
}
void test_dynamic_bitvector_rank() {
    ui32 n = 10000;
    DynamicBitvector<128> b;
    vector<bool> vec(n);

    std::minstd_rand rng(_RANDOM_SEED);

    ui32 ones = 0;

    for (ui32 i = 0; i < n; i++) {
        uniform_int_distribution<ui32> dist(0, 1);
        bool bit = dist(rng) == 1;
        b.insert(i, bit);
        vec[i] = bit;
        if (bit)
            ones++;
    }

    for (ui32 i = 0; i < n; i++) {
        assert(b.get_bit(i) == vec[i]);
    }

    ui32 ones_count = 0;
    for (ui32 i = 0; i < n; i++) {

        assert(b.rank1(i) == ones_count);
        assert(b.rank0(i) == i - ones_count);
        if (vec[i])
            ones_count++;
    }
}
void test_dynamic_bitvector_removing() {

    ui32 n = 10000;
    DynamicBitvector<128> b;
    vector<bool> vec(n);

    b.reserve(n);
    b.init_size(n);


    std::minstd_rand rng(_RANDOM_SEED);

    ui32 ones = 0;

    for (ui32 i = 0; i < n; i++) {
        uniform_int_distribution<ui32> dist(0, 1);
        bool bit = dist(rng) == 1;
        b.set_bit(i, bit);
        vec[i] = bit;
        if (bit)
            ones++;
    }

    ui32 nodes_to_delete = 9000;

    for (ui32 i = 0; i < nodes_to_delete; i++) {
        uniform_int_distribution<ui32> dist(0, n - i - 1);
        ui32 delete_pos = dist(rng);

        b.erase(delete_pos);
        vec.erase(vec.begin() + delete_pos);
    }

    for (ui32 j = 0; j < n - nodes_to_delete; j++) {
        assert(b.get_bit(j) == vec[j]);
    }
}

void DynamicBitvectorTest() {
    //test_dynamic_bitvector_general();
    cout << "General test finished" << endl;
    test_dynamic_bitvector_select();
    cout << "Select test finished" << endl;
    test_dynamic_bitvector_rank();
    cout << "Rank test finished" << endl;
    test_dynamic_bitvector_removing();
    cout << "Removing test finished" << endl;
}

ui32 range_count(const ui32 *permutation, ui32 pos_l, ui32 pos_r, ui32 L, ui32 U) {
    ui32 count = 0;
    for (ui32 i = pos_l; i < pos_r; i++)
        if (L <= permutation[i] && permutation[i] < U)
            count++;
    return count;
}

void WaveletTreeTest() {
    int n = 1000000;
    
    FenwickTree ft(n);
    ft.init(n);

    std::minstd_rand rng(_RANDOM_SEED);

    ui32 *permutation = new ui32[n];

    for (int i = 0; i < n; i++) {
        uniform_int_distribution<ui32> dist(1, n - i);
        permutation[i] = ft.removeIth(dist(rng)) - 1;
        //if (i <= 56)
        //    cout << permutation[i] << " ";
    }



    WaveletTree<ui32, 256> wt;
    wt.set_alph_size(n);
    wt.set_max_depth_leaf(n, 100);
    wt.reserve(n, 2);

    wt.create_array(permutation, n);


    //WaveletTree<ui32, 64> wt2;
    //wt2.set_alph_size(n);
    //wt2.set_max_depth_leaf(n, 100);
    //wt2.reserve(n, 2);
    //wt2.orig_create(permutation, n);

    while (true) {
        float opTime = 0;
        float origTime = 0;
        int sampleCount = 0;
        int sampleCountTotal = 3;

        for (int _ = 0; _ < sampleCountTotal; _++) {

            uniform_int_distribution<int> distL(0, n - 1);
            uniform_int_distribution<int> distU(0, n);
            int L = distL(rng);
            int U = distU(rng);

            if (L > U) {
                int temp = L;
                L = U;
                U = temp;
            }

            int a = 0, b = 0;

            while (a == b) {
                a = distL(rng);
                b = distU(rng);

                if (a > b) {
                    int temp = b;
                    b = a;
                    a = temp;
                }
            }

            auto startOrig = chrono::high_resolution_clock::now();
            int orig = range_count(permutation, a, b, L, U);
            auto endOrig = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> elapsedOrig = endOrig - startOrig;

            if (L == 31158) {
                int oaihwga = 0;
            }
            auto startOp = chrono::high_resolution_clock::now();
            int op = wt.range(a, b, L, U);
            auto endOp = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> elapsedOp = endOp - startOp;

            if (orig != op) {
                int k;
                std::cout << "WRONG OUTPUT" << std::endl;
                cin >> k;
                return;
            }

            origTime = origTime * (_ / (_ + 1.0f)) + elapsedOrig.count() / (_ + 1);
            opTime = opTime * (_ / (_ + 1.0f)) + elapsedOp.count() / (_ + 1);
        }

        std::cout << "With " << sampleCountTotal << " samples, the original time is " << origTime << " and the optimized time is " << opTime << std::endl;
        std::cout << "The speed improvement is about " << origTime / opTime << " times" << std::endl;

        //int changeIndex1 = 0;
        //int changeIndex2 = 0;
        //while (changeIndex1 == changeIndex2) {
        //
        //    changeIndex1 = device.UniformN(0, Num - 1);
        //    changeIndex2 = device.UniformN(0, Num - 1);
        //}
        //
        //std::cout << "Changing index " << changeIndex1 << " and " << changeIndex2 << "!" << std::endl;
        //
        //if (changeIndex2 == 292) {
        //    int k = 0;
        //}
        //
        //if (changeIndex2 == 145) {
        //    int k = 0;
        //}
        //
        //auto startSwitch = std::chrono::high_resolution_clock::now();
        //tree->Switch(changeIndex1, changeIndex2);
        //auto endSwitch = std::chrono::high_resolution_clock::now();
        //std::chrono::duration<float> elapsedSwitch = endSwitch - startSwitch;
        //
        //avgSwitchSpeed = avgSwitchSpeed * (numTrials / (numTrials + 1.0f)) + elapsedSwitch.count() / (numTrials + 1);
        //avgOpSpeed = avgOpSpeed * (numTrials / (numTrials + 1.0f)) + opTime / (numTrials + 1);
        //avgOrigSpeed = avgOrigSpeed * (numTrials / (numTrials + 1.0f)) + origTime / (numTrials + 1);
        //
        //numTrials++;
        //
        //if (numTrials % 1000 == 0) {
        //    cout << endl;
        //    cout << "running average switching speed: " << avgSwitchSpeed << endl;
        //    cout << "running average original speed:  " << avgOrigSpeed << endl;
        //    cout << "running average optimized speed: " << avgOpSpeed << endl;
        //    int k;
        //    cin >> k;
        //}
        //
        //
        //
        //nextSeed = device.UniformN(1, 100000000);
        //RandDevice::DeleteDevice(device);
    }


    int laoiwgw = 0;
}

void GeneralTest()
{
    //DatablockTest();
    //DynamicBitvectorTest();
    WaveletTreeTest();
}