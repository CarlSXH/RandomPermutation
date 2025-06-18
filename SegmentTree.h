

#ifndef __SEGMENT_TREE_H__
#define __SEGMENT_TREE_H__

#include "RandPerm.h"
#include "AVLTree.h"

// Data structure on a permutation viewed as a list of number,
// support swapping the values at two positions and calculate
// the number of points between position L and R that has value
// less than M, both operations in O(logn) time.
// Space complexity is O(nlogn).
//
// At each node of the segment tree, we will have a dynamic bitvector,
// implemented as a AVL tree, storing the positions of some numbers
// in the permutation. The left child stores all values in the node
// less than the median, while the right child stores all values greater
// than or equal to the median. Thus, each node actually stores a portion
// of the inverse of the permutation, viewed as a list of number.
// Then, the range query can be resolved by simply recursively splitting
// [1, M) using the median of each internal node, and query the range count
// of AVL tree Count(R)-Count(L) to obtain the count over positions 
// between L and R.
//
// The permutation is 0-based.
struct SegmentTree {



    int N;
    unsigned int *perm;

    SegmentTree();

    void Create(unsigned int *_perm, int _N, size_t _minSize = 300);
    void Delete();

    void Switch(int i, int j);

    int Range(int L, int R, int M);
    int Range(int L, int R, int a, int b);

    // [0, R) with values in [0, M)
    int RangeOneSide(int R, int M);


public:

    struct SegmentTreeNode {
        AVLTree bitVector;

        // contain elements in [a,b)
        int a, b;
        int m;

        // the left contains elements in [a,m)
        // the right contains elements in [m,b)
        SegmentTreeNode *left, *right;

        SegmentTreeNode();

        void Create(int _a, int _b, size_t minSize);
        void Delete();

        static void FillValue(SegmentTreeNode *currentNode, int position, int value);
    };


    int RangeBruteForce(int L, int R, int a, int b);


    SegmentTreeNode *root;

    size_t minSize;

    // Testing purposes
    //
    //void RecurseTest();
    //void RecurseTest(SegmentTreeNode *curNode);
    //
    //bool CheckTree(int *);

};



#endif //__SEGMENT_TREE_H__