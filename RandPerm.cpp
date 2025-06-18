

#include "RandPerm.h"
#include <random>
#include <vector>
#include <chrono>



bool PermData::Get(unsigned int u, unsigned int v)
{
    return I[u*k+v];
}


void PermData::Set(unsigned int u, unsigned int v, bool b) {
    I[u * k + v] = b;
}

bool PermData::IsMonotone() {
    int lastY = 0;

    for (int i = 0; i < k; i++) {
        int j;
        for (j = 0; j < k && Get(i, j); j++) {}

        if (lastY > j)
            return false;
        lastY = j;

        for (j = j + 1; j < k; j++) {
            if (Get(i, j))
                return false;
        }
    }
    return true;
}

void PermData::FillMonoRestrict(MonoPermData *pMono) {
    int lastY = 0;
    int colCount = 0;
    int curIndex = 0;

    int curY;
    for (curY = 0; curY < k && Get(0, curY); curY++) {}


    for (int i = 0; i < k; i++) {
        int j;
        for (j = 0; j<k && Get(i, j); j++) {}

        if (j > curY) {
            // things happen
            pMono->X[curIndex] = colCount;
            pMono->Y[curIndex] = curY - lastY;

            curIndex++;
            colCount = 1;
            lastY = curY;
            curY = j;
            continue;
        }
        colCount++;
    }
    pMono->X[curIndex] = colCount;
    pMono->Y[curIndex] = curY - lastY;
    curIndex++;
    pMono->dim = curIndex;
}

bool PermData::IsIn(int N, int a, int b) {
    int x = a/N, y = b/N;

    return Get(x,y);
}

bool MonoPermData::IsIn(int N, int a, int b) {
    int x = 0, y = 0;
    while (X[x] * N <= a) {
        a -= X[x] * N;
        x++;
    }
    while (Y[y] * N <= b) {
        b -= Y[y] * N;
        y++;
    }
    return y <= x;
}

unsigned int flp2(unsigned int x) {
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >> 16);
#ifdef _WIN64
    x = x | (x >> 32);
#endif
    return x - (x >> 1);
}

FenwickTree::FenwickTree(int _n) : n(_n), bitMask(flp2(_n)), bit(NULL) {}

FenwickTree::~FenwickTree() {
    delete[] bit;
}

void FenwickTree::initEmpty(int _n) {
    n = _n;
    bitMask = flp2(n);
    bit = new int[n + 1];
    for (int i = 0; i <= n; i++)
        bit[i] = 0;
}

void FenwickTree::init(int _n) {
    initEmpty(_n);
    for (int i = 1; i <= n; i++) {
        update(i, 1);
    }
}

void FenwickTree::update(int idx, int delta) {
    while (idx <= n) {
        bit[idx] += delta;
        idx += idx & -idx;
    }
}

int FenwickTree::sum(int idx) const {
    int res = 0;
    while (idx > 0) {
        res += bit[idx];
        idx -= idx & -idx;
    }
    return res;
}

int FenwickTree::findKth(int k) const {
    int pos = 0;
    // compute largest power of two <= n

    // binary©\lifting search
    for (int step = bitMask; step > 0; step >>= 1) {
        if (pos + step <= n && bit[pos + step] < k) {
            k -= bit[pos + step];
            pos += step;
        }
    }
    return pos + 1;
}

int FenwickTree::removeIth(int k) {
    int pos = findKth(k);
    update(pos, -1);
    return pos;
}




void MonotoneSampling(MonoPermData d, unsigned int N, float q, unsigned int *perm, RandDevice device, FenwickTree *ft, float *t1, float *t2)
{
    auto start = std::chrono::high_resolution_clock::now();

    unsigned int index = 0;

    // number of rows left that are able to be sampled.
    unsigned int curRowLeft = 0;
    for (int i = 0; i < d.dim; i++)
    {
        curRowLeft += d.Y[i] * N;
        for (int j = 0; j < d.X[i]*N; j++)
        {
            perm[index++] = device.TrunGeom(q, curRowLeft);
            curRowLeft--;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<float> elapsed = end - start;
    if (t1 != NULL)
        *t1 = elapsed.count();

    start = std::chrono::high_resolution_clock::now();


    ft->init(index);

    for (int i = 0; i < index; i++)
        perm[i] = ft->removeIth(perm[i])-1;

    end = std::chrono::high_resolution_clock::now();

    elapsed = end - start;
    if (t2 != NULL)
        *t2 = elapsed.count();

}



WaveletTreeSquare::WaveletTreeNode::WaveletTreeNode(int _N) : bitVector(_N), left(NULL), right(NULL) {

};

void WaveletTreeSquare::WaveletTreeNode::build(int _N, int _a, int _b) {
    this->a = _a;
    this->b = _b;
    this->bitVector.initEmpty(_N);
    if (a + 1 == b) {
        m = a;
        left = NULL;
        right = NULL;
        return;
    }

    m = (a + b) / 2;
    left = new WaveletTreeSquare::WaveletTreeNode(_N);
    right = new WaveletTreeSquare::WaveletTreeNode(_N);
    left->build(_N, _a, m);
    right->build(_N, m, _b);

}



WaveletTreeSquare::WaveletTreeSquare(int _N) {
    this->N = _N;
    root = new WaveletTreeNode(N);
    root->build(_N, 0, _N);
}

void WaveletTreeSquare::SetPerm(unsigned int *_perm) {
    perm = _perm;

    for (int i = 0; i < N; i++)
        Set(i+1, _perm[i]);

}

void WaveletTreeSquare::Set(int i, int val) {
    auto cur = root;
    while (cur != NULL) {
        cur->bitVector.update(i, 1);
        if (val < cur->m)
            cur = cur->left;
        else
            cur = cur->right;
    }
}

int WaveletTreeSquare::WaveletTreeNode::Range(int L, int R, int _a, int _b) {
    if (a == _a && b == _b)
        return bitVector.sum(R) - bitVector.sum(L - 1);

    if (_b <= m)
        return left->Range(L, R, _a, _b);
    if (_a >= m)
        return right->Range(L, R, _a, _b);

    return left->Range(L, R, _a, m) + right->Range(L, R, m, _b);
}


int WaveletTreeSquare::Range(int L, int R, int a, int b) {
    return root->Range(L, R, a, b);
}


void WaveletTreeSquare::Update(int i, int val) {
    int origVal = perm[i];
    if (origVal == val)
        return;
    perm[i] = val;

    auto origBranch = root;
    auto newBranch = root;

    if (val < origVal) {
        while (origBranch->m <= val || origVal < origBranch->m) {
            if (val < origBranch->m)
                origBranch = origBranch->left;
            else
                origBranch = origBranch->right;
        }
        newBranch = origBranch->left;
        origBranch = origBranch->right;
    } else {
        while (origBranch->m <= origVal || val < origBranch->m) {
            if (val < origBranch->m)
                origBranch = origBranch->left;
            else
                origBranch = origBranch->right;
        }
        newBranch = origBranch->right;
        origBranch = origBranch->left;
    }

    while (newBranch != NULL) {
        newBranch->bitVector.update(i, 1);
        if (val < newBranch->m)
            newBranch = newBranch->left;
        else
            newBranch = newBranch->right;
    }

    while (origBranch != NULL) {
        origBranch->bitVector.update(i, -1);
        if (origVal < origBranch->m)
            origBranch = origBranch->left;
        else
            origBranch = origBranch->right;
    }
}