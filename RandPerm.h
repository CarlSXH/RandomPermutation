
#ifndef __RAND_PERM_H__
#define __RAND_PERM_H__


#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>
#include <crtdbg.h>



#ifdef _DEBUG
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
// allocations to be of _CLIENT_BLOCK type
#define new DBG_NEW
#else
#define DBG_NEW new
#endif



#include "MyRandom.h"





struct MonoPermData {
    // number of blocks in a row
    unsigned int dim;

    // the widths of each block
    // X[0], ..., X[k-1]
    unsigned int *X;

    // the heights of each block
    // Y[0], ..., Y[l-1]
    unsigned int *Y;

    bool IsIn(int N, int a, int b);
};


struct PermData {
    // number of blocks in a direction
    unsigned int k;

    // the restriction matrix
    // I[u*l + v] for 0 <= u < l
    //                0 <= v < k
    bool *I;

    bool Get(unsigned int u, unsigned int v);
    void Set(unsigned int u, unsigned int v, bool b);

    bool IsMonotone();

    bool IsIn(int N, int a, int b);

    void FillMonoRestrict(MonoPermData *);
};

// this is entirely 1 indexed
struct FenwickTree {
    int n;
    int bitMask;
    int *bit;

    FenwickTree(int _n = 0);

    ~FenwickTree();

    void initEmpty(int _n);

    void init(int _n);

    void update(int idx, int delta);

    int sum(int idx) const;

    int findKth(int k) const;

    int removeIth(int k);
};

// the root of the tree
struct WaveletTreeSquare {


    struct WaveletTreeNode {
        FenwickTree bitVector;

        // contain elements in [a,b)
        int a, b;
        int m;

        // the left contains elements in [a,m)
        // the right contains elements in [m,b)
        WaveletTreeNode *left, *right;

        WaveletTreeNode(int _N);

        // queue number of elements in position [L,R) that are within [_a,_b)
        int Range(int L, int R, int _a, int _b);

        void build(int _N, int _a, int _b);
    };

    WaveletTreeNode *root;

    int N;
    unsigned int *perm;

    WaveletTreeSquare(int _N);

    void SetPerm(unsigned int *_perm);

    void Update(int i, int newVal);

    int Range(int L, int R, int a, int b);

private:

    void Set(int i, int val);

};

struct UniformTranspositionSampler {

};


void MonotoneSampling(MonoPermData d, unsigned int N, float q, unsigned int *perm, RandDevice device, FenwickTree *ft, float *t1 = NULL, float *t2 = NULL);



#endif //__RAND_PERM_H__