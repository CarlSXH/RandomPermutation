

#include "MyRandom.h"
#include <math.h>




RandDevice RandDevice::SetSeed(unsigned int seed)
{
    RandDevice d;
    d.gen = new std::mt19937(seed);
    d.rd = NULL;
    return d;
}
RandDevice RandDevice::RandomSeed()
{
    RandDevice d;
    d.rd = new std::random_device();
    d.gen = new std::mt19937(d.rd->operator()());
    return d;
}
void RandDevice::DeleteDevice(RandDevice d)
{
    if (d.gen != NULL)
    {
        delete d.gen;
        d.gen = NULL;
    }
    if (d.rd != NULL)
    {
        delete d.rd;
        d.rd = NULL;
    }
}
void RandDevice::SetQ(float _q)
{
    q = _q;
    lq = log(q);
}

int RandDevice::UniformN(int a, int b)
{
    std::uniform_int_distribution<> distrib(a, b);
    return distrib(*gen);
}


float RandDevice::UniformF(float a, float b)
{
    std::uniform_real_distribution<> distrib(a, b);
    return distrib(*gen);
}

bool RandDevice::Bernoulli(float pass) {
    return UniformF(0, 1) < pass;
}


int RandDevice::TrunGeom(float q, int n)
{
    if (abs(q - 1) < 0.00000001)
        return UniformN(1, n);
    float u = UniformF(0, 1);

    // use double exponential form to reduce computation

    float a = log(1-u);
    float b = log(u) + n * lq;

    float m = a > b ? a : b;

    return lround(ceil(  (m + log(exp(a - m) + exp(b - m))) / lq));

    // exact formula is
    //return lround(ceil( log(1-u*(1-pow(q, n)))/log(q) ));
}