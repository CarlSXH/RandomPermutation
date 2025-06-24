


#include "Testing.h"
#include <random>
#include <bitset>
#include <iostream>
#include <vector>

using namespace std;

template<unsigned int B>
bool Equal(Datablock<B> *b, const vector<bool> &vec)
{
    for (int i = 0; i < vec.size(); i++)
    {
        if (b->get_bit(i) != vec[i])
            return false;
    }
}


struct Arr
{
    uint64_t data[];
};


void DatablockTest()
{

    using namespace std;
    Datablock<512> b;
    vector<bool> control;
    control.reserve(512);

    b.sz = 512;

    DynamicBitvector<512> d;

    d.insert(0, true);



    sizeof(Arr);


    std::mt19937_64 rng(109859012);

    for (int i = 0; i < 512; i++)
    {
        bool bit = (rng() % 2) == 0;
        b.set_bit(i, bit);
        control.push_back(bit);
    }

    for (int i = 0; i <= Datablock<512>::block_index(b.sz-1); i++)
    {
        bitset<64> w(b.data[i]);
        cout << w << endl;
    }
    cout << endl;










    
}

void GeneralTest()
{
    DatablockTest();
}