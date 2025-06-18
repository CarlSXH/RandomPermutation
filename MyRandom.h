
#ifndef __MY_RANDOM_H__
#define __MY_RANDOM_H__

#include <random>


struct RandDevice
{
    std::mt19937 *gen;

    static RandDevice SetSeed(unsigned int seed);
    static RandDevice RandomSeed();
    static void DeleteDevice(RandDevice d);

    void SetQ(float _q);

    float q;
    float lq;
    std::random_device *rd;

    // inclusive on both ends
    int UniformN(int a, int b);

    float UniformF(float a, float b);

    // sample an element of {1, ..., n} such that
    // mu(i) is proportional to q^i
    int TrunGeom(float q, int n);

    // sample true with probability pass, false with probability 1-pass
    bool Bernoulli(float pass);
};







#endif //__MY_RANDOM_H__