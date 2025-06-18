#include "SegmentTree.h"


SegmentTree::SegmentTreeNode::SegmentTreeNode() : bitVector(), left(NULL), right(NULL) {

};


SegmentTree::SegmentTree()
{

}

void SegmentTree::SegmentTreeNode::Create(int _a, int _b, size_t minSize)
{
    a = _a;
    b = _b;
    bitVector.Init(_b - _a);
    if (b - a <= minSize) {
        left = nullptr;
        right = nullptr;
        m = _a;
        return;
    }


    m = (a + b) / 2;
    left = new SegmentTree::SegmentTreeNode();
    right = new SegmentTree::SegmentTreeNode();
    left->Create(a, m, minSize);
    right->Create(m, b, minSize);
}


void SegmentTree::SegmentTreeNode::Delete() {
    if (left != nullptr)
    {
        left->Delete();
        delete left;
        left = nullptr;
    }
    if (right != nullptr)
    {
        right->Delete();
        delete right;
        right = nullptr;
    }
    bitVector.Delete();
}

void SegmentTree::SegmentTreeNode::FillValue(SegmentTreeNode *currentNode, int position, int value)
{
    while (currentNode != nullptr)
    {
        currentNode->bitVector.Insert(position - currentNode->a, value);
        if (position < currentNode->m)
            currentNode = currentNode->left;
        else
            currentNode = currentNode->right;
    }
}


void SegmentTree::Create(unsigned int *_perm, int _N, size_t _minSize) {
    this->N = _N;
    this->minSize = _minSize;
    root = new SegmentTreeNode();
    root->Create(0, N, minSize);
    this->perm = _perm;

    for (int i = 0; i < _N; i++)
        SegmentTreeNode::FillValue(root, i, _perm[i]);
}
void SegmentTree::Delete()
{
    root->Delete();
    delete root;
}

void SegmentTree::Switch(int i, int j) {
    if (i > j)
    {
        int temp = i;
        i = j;
        j = temp;
    }

    int val_i = perm[i];
    int val_j = perm[j];

    perm[i] = val_j;
    perm[j] = val_i;
    root->bitVector.Remove(i);
    root->bitVector.Remove(j);
    root->bitVector.Insert(i, val_j);
    root->bitVector.Insert(j, val_i);

    // lowest common ancestor
    SegmentTreeNode *lca = root;

    int loc_ind_i = i;
    int loc_ind_j = j;

    while (!(i < lca->m && lca->m <= j))
    {
        if (i < lca->m) {
            lca = lca->left;
        }
        else {
            lca = lca->right;
        }

        if (lca == nullptr)
            return;

        int startIndex = lca->a;
        lca->bitVector.Remove(i - startIndex);
        lca->bitVector.Remove(j - startIndex);
        lca->bitVector.Insert(i - startIndex, val_j);
        lca->bitVector.Insert(j - startIndex, val_i);
    }

    SegmentTreeNode *node_i = lca->left;

    while (node_i != NULL)
    {
        node_i->bitVector.Change(i - node_i->a, val_j);
        if (i < node_i->m) {
            node_i = node_i->left;
        }
        else {
            node_i = node_i->right;
        }
    }


    SegmentTreeNode *node_j = lca;
    while (node_j != NULL)
    {
        node_j->bitVector.Change(j - node_j->a, val_i);
        if (j < node_j->m) {
            node_j = node_j->left;
        }
        else {
            node_j = node_j->right;
        }
    }
}


int SegmentTree::RangeBruteForce(int L, int R, int a, int b)
{
    int count = 0;
    for (int i = L; i < R; i++)
    {
        if (a <= perm[i] && perm[i] < b)
            count++;
    }
    return count;
}

int SegmentTree::RangeOneSide(int R, int M) {
    SegmentTreeNode *currentNode = root;

    if (currentNode == nullptr)
        return 0;

    int count = 0;
    while (currentNode->left != nullptr)
    {
        if (currentNode->m < R)
        {
            count += currentNode->left->bitVector.Count(M);
            currentNode = currentNode->right;
        }
        else
        {
            currentNode = currentNode->left;
        }
    }

    return count + RangeBruteForce(currentNode->a, R, 0, M);
}

int SegmentTree::Range(int L, int R, int M) {
    return RangeOneSide(R, M) - RangeOneSide(L, M);
}

int SegmentTree::Range(int L, int R, int a, int b) {
    SegmentTreeNode *lca = root;
    
    if (lca == nullptr)
        return 0;

    if (R <= minSize + L)
        return RangeBruteForce(L, R, a, b);

    int count = 0;
    while (lca->left != nullptr && !(L < lca->m && lca->m <= R))
    {
        if (L < lca->m)
            lca = lca->left;
        else
            lca = lca->right;
    }

    if (lca->left == nullptr)
        return RangeBruteForce(L, R, a, b);


    SegmentTreeNode *leftNode = lca->left;
    SegmentTreeNode *rightNode = lca->right;

    while (leftNode->left != nullptr)
    {
        if (L < leftNode->m)
        {
            count += leftNode->right->bitVector.Count(b) - leftNode->right->bitVector.Count(a);
            leftNode = leftNode->left;
        }
        else
            leftNode = leftNode->right;
    }

    count += RangeBruteForce(L, leftNode->b, a, b);

    while (rightNode->left != nullptr)
    {
        if (R < rightNode->m)
            rightNode = rightNode->left;
        else
        {
            count += rightNode->left->bitVector.Count(b) - rightNode->left->bitVector.Count(a);
            rightNode = rightNode->right;
        }
    }

    count += RangeBruteForce(rightNode->a, R, a, b);

    return count;
}


//void SegmentTree::RecurseTest()
//{
//    RecurseTest(root);
//}
//void SegmentTree::RecurseTest(SegmentTreeNode *curNode)
//{
//    if (curNode->left != nullptr)
//        RecurseTest(curNode->left);
//    if (curNode->right != nullptr)
//        RecurseTest(curNode->right);
//    curNode->bitVector.RecurseTest();
//}

