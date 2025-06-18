#include "AVLTree.h"
#include <iostream>

typedef AVLTree::index_t index_t;

AVLTree::AVLTree() : N(0), nodes(nullptr), root(0) {}

void AVLTree::Init(size_t size)
{
    N = size;
    nodes = new AVLTreeNode[N];
    root = 0;
}
void AVLTree::Init(int *values, size_t size)
{
    Init(size);
    for (int i = 0; i < size; i++) {
        root = InternalInsert(root, values[i], i + 1);
    }
}
void AVLTree::Delete()
{
    N = 0;
    if (nodes != nullptr)
    {
        delete[] nodes;
        nodes = nullptr;
    }
}



void AVLTree::Insert(index_t index, int value)
{
    root = InternalInsert(root, value, index+1);
}
void AVLTree::Remove(index_t index)
{
    root = InternalDelete(root, index+1);
}
void AVLTree::Change(index_t index, int value)
{
    // Remove then reinsert the element to change value.
    root = InternalDelete(root, index+1);
    root = InternalInsert(root, value, index+1);
}
size_t AVLTree::Count(int Bound)
{
    index_t current = root;
    size_t count = 0;
    while (current > 0)
    {
        if (nodes[current - 1].value < Bound)
        {
            // If a node has value less than the bound, then
            // the entire left subtree can be included in
            // the count. The plus one is for the current node
            count += NodeSize(nodes[current - 1].left) + 1;

            // The remaining nodes are all in the right subtree.
            current = nodes[current - 1].right;
        }
        else // if (nodes[current - 1].value >= Bound)
        {
            // If a node has value greater than the bound, then
            // the entire right subtree can be eliminated from
            // the count.
            // So we only need to consider the left subtree.
            current = nodes[current - 1].left;
        }
    }

    return count;
}
bool AVLTree::Find(int value, index_t *outIndex)
{
    index_t currentNode = root;
    while (currentNode > 0)
    {
        if (nodes[currentNode-1].value == value)
        {
            *outIndex = currentNode;
            return true;
        }

        if (nodes[currentNode-1].value < value)
            currentNode = nodes[currentNode-1].right;
        else // if (nodes[currentNode-1].value > value)
            currentNode = nodes[currentNode-1].left;
    }
    return false;
}


size_t AVLTree::NodeSize(index_t index)
{
    if (index == 0)
        return 0;
    return nodes[index - 1].size;
}
size_t AVLTree::NodeHeight(index_t index)
{
    if (index == 0)
        return 0;
    return nodes[index - 1].height;
}
void AVLTree::RecalcHeightSize(index_t index)
{
    if (index == 0)
        return;

    size_t leftSize = NodeSize(nodes[index-1].left);
    size_t rightSize = NodeSize(nodes[index-1].right);

    size_t leftHeight = NodeHeight(nodes[index-1].left);
    size_t rightHeight = NodeHeight(nodes[index-1].right);

    nodes[index-1].height = (leftHeight>rightHeight ? leftHeight : rightHeight) + 1;
    nodes[index-1].size = leftSize + rightSize + 1;
}


index_t AVLTree::RotateLeft(index_t index)
{
    index_t newRoot = nodes[index-1].right;

    nodes[index-1].right = nodes[newRoot-1].left;
    nodes[newRoot-1].left = index;

    // order matters
    RecalcHeightSize(index);
    RecalcHeightSize(newRoot);

    return newRoot;
}
index_t AVLTree::RotateRight(index_t index)
{
    index_t newRoot = nodes[index-1].left;

    nodes[index-1].left = nodes[newRoot-1].right;
    nodes[newRoot-1].right = index;

    // order matters
    RecalcHeightSize(index);
    RecalcHeightSize(newRoot);

    return newRoot;
}
index_t AVLTree::BalanceCurrent(index_t index)
{
    if (index == 0)
        return 0;

    size_t leftHeight = NodeHeight(nodes[index-1].left);
    size_t rightHeight = NodeHeight(nodes[index-1].right);

    // The AVL property is true here
    if ((rightHeight <= 1 + leftHeight) && (leftHeight <= 1 + rightHeight))
        return index;

    // Rotation based on which subtree is taller
    if (rightHeight > leftHeight)
    {
        // If the right subtree is left heavy, we need another
        // right rotation on the right subtree
        size_t rightLeftHeight = NodeHeight(nodes[nodes[index-1].right-1].left);
        size_t rightRightHeight = NodeHeight(nodes[nodes[index-1].right-1].right);
        if (rightLeftHeight > rightRightHeight)
            nodes[index-1].right = RotateRight(nodes[index-1].right);
        
        return RotateLeft(index);
    }
    else
    {
        // Similarly, if the left subtree is right heavy, we need another
        // right rotation on the right subtree
        size_t leftLeftHeight = NodeHeight(nodes[nodes[index-1].left-1].left);
        size_t leftRightHeight = NodeHeight(nodes[nodes[index-1].left-1].right);
        if (leftRightHeight > leftLeftHeight)
            nodes[index-1].left = RotateLeft(nodes[index-1].left);

        return RotateRight(index);
    }
}



index_t AVLTree::FindMinDelete(index_t currentNode, index_t *minIndex)
{
    // Repeatedly choosing the left subtree for the minimum element,
    // then use the right children of the removed element.
    if (nodes[currentNode - 1].left > 0)
        nodes[currentNode - 1].left = FindMinDelete(nodes[currentNode - 1].left, minIndex);
    else
    {
        *minIndex = currentNode;
        return nodes[currentNode - 1].right;
    }

    RecalcHeightSize(currentNode);

    return BalanceCurrent(currentNode);
}
index_t AVLTree::FindMaxDelete(index_t currentNode, index_t *maxIndex)
{
    // Repeatedly choosing the right subtree for the maximum element,
    // then use the left children of the removed element.
    if (nodes[currentNode - 1].right > 0)
        nodes[currentNode - 1].right = FindMaxDelete(nodes[currentNode - 1].right, maxIndex);
    else
    {
        *maxIndex = currentNode;
        return nodes[currentNode - 1].left;
    }

    RecalcHeightSize(currentNode);

    return BalanceCurrent(currentNode);
}



index_t AVLTree::InternalDelete(index_t currentNode, index_t removedIndex)
{
    if (currentNode == 0) {
        return currentNode;
    }

    if (currentNode == removedIndex)
    {
        // If the current node is the one we want to remove,
        // based on which children is taller, we remove the minimum element
        // from the right child or the maximum element from the left child
        // and use it to be the new current node, and the child remains the same

        if (nodes[currentNode-1].right == 0)
        {
            index_t newRoot = nodes[currentNode-1].left;
            nodes[currentNode-1].left = 0;
            return newRoot;
        }
        else if (nodes[currentNode-1].left == 0)
        {
            index_t newRoot = nodes[currentNode-1].right;
            nodes[currentNode-1].right = 0;
            return newRoot;
        }

        index_t removedExtreme;
        index_t newLeft = nodes[currentNode-1].left;
        index_t newRight = nodes[currentNode-1].right;

        if (NodeHeight(nodes[currentNode-1].left) > NodeHeight(nodes[currentNode-1].right))
            newLeft = FindMaxDelete(nodes[currentNode-1].left, &removedExtreme);
        else
            newRight = FindMinDelete(nodes[currentNode-1].right, &removedExtreme);
        nodes[removedExtreme-1].left = newLeft;
        nodes[removedExtreme-1].right = newRight;
        RecalcHeightSize(removedExtreme);
        
        nodes[currentNode-1].left = 0;
        nodes[currentNode-1].right = 0;
        nodes[currentNode-1].size = 0;
        nodes[currentNode-1].height = 0;
        return removedExtreme;
    }

    if (nodes[currentNode-1].value < nodes[removedIndex-1].value)
        nodes[currentNode-1].right = InternalDelete(nodes[currentNode-1].right, removedIndex);
    else // if (nodes[currentNode-1].value >= nodes[removedIndex-1].value)
        nodes[currentNode-1].left = InternalDelete(nodes[currentNode-1].left, removedIndex);

    // Standard procedure to recalculate data and rebalance
    RecalcHeightSize(currentNode);
    return BalanceCurrent(currentNode);
}
index_t AVLTree::InternalInsert(index_t currentNode, int a, index_t addedIndex)
{
    if (currentNode == 0)
    {
        // fill the memory with the right data.
        // base case for recursion.
        nodes[addedIndex-1].size = 1;
        nodes[addedIndex-1].height = 1;
        nodes[addedIndex-1].left = 0;
        nodes[addedIndex-1].right = 0;
        nodes[addedIndex-1].value = a;
        return addedIndex;
    }

    if (nodes[currentNode-1].value < a)
        nodes[currentNode-1].right = InternalInsert(nodes[currentNode-1].right, a, addedIndex);
    else // (nodes[currentNode-1].value > a)
        nodes[currentNode-1].left = InternalInsert(nodes[currentNode-1].left, a, addedIndex);

    // Standard procedure to recalculate data and rebalance
    RecalcHeightSize(currentNode);
    return BalanceCurrent(currentNode);
}


//void AVLTree::RecurseTest()
//{
//    RecurseTest(root);
//}
//
//void AVLTree::RecurseTest(index_t curIndex)
//{
//    if (nodes[curIndex - 1].right != 0)
//        RecurseTest(nodes[curIndex - 1].right);
//    if (nodes[curIndex - 1].left != 0)
//        RecurseTest(nodes[curIndex - 1].left);
//}
//std::string AVLTree::JSON()
//{
//    return JSON(root);
//}
//std::string AVLTree::JSON(index_t curIndex)
//{
//    std::string left = "";
//    std::string right ="";
//
//    std::string things = "\"value\":" + std::to_string(nodes[curIndex - 1].value) + ", \"size\":" + std::to_string(nodes[curIndex - 1].size) + ", \"height\":" + std::to_string(nodes[curIndex - 1].height) + ",\"index\":"+std::to_string(curIndex);
//
//
//    if (nodes[curIndex - 1].left != 0)
//    {
//        things = "\"left\":" + JSON(nodes[curIndex - 1].left) + ","+things;
//    }
//    if (nodes[curIndex - 1].right != 0)
//    {
//        things = "\"right\":" + JSON(nodes[curIndex - 1].right) + "," + things;
//    }
//
//    
//    return "{" + things + "}";
//}
//void AVLTree::PRINT()
//{
//    auto thething = JSON();
//
//    std::cout << thething.c_str()<<std::endl<<std::endl;
//}
//bool AVLTree::CheckTree(int *pArray)
//{
//    bool *nodeSeen = new bool[N];
//    bool *valueSeen = new bool[N];
//
//    for (int i = 0; i < N; i++)
//    {
//        nodeSeen[i] = false;
//        valueSeen[i] = false;
//    }
//
//    size_t size, height;
//    bool result = InternalCheck(root, nodeSeen,
//        &size, &height);
//
//    
//
//    return result && nodes[root - 1].size == size && nodes[root - 1].height == height;;
//}
//bool AVLTree::InternalCheck(index_t curIndex, bool *NodeSeen, size_t *size, size_t *height)
//{
//    if (curIndex == 0)
//    {
//        *size = 0;
//        *height = 0;
//        return true;
//    }
//    if (NodeSeen[curIndex - 1])
//        return false;
//
//    NodeSeen[curIndex - 1] = true;
//
//
//    size_t rootRightSize, rootLeftSize, rootRightHeight, rootLeftHeight;
//    bool leftRes = InternalCheck(nodes[curIndex - 1].left, NodeSeen, &rootLeftSize, &rootLeftHeight);
//    bool rightRes = InternalCheck(nodes[curIndex - 1].right, NodeSeen, &rootRightSize, &rootRightHeight);
//    
//    if (!(rootRightHeight <= rootLeftHeight + 1 && rootLeftHeight <= rootRightHeight + 1))
//        return false;
//    
//    size_t maxHeight = rootRightHeight > rootLeftHeight ? rootRightHeight : rootLeftHeight;
//    if (nodes[curIndex - 1].height != maxHeight + 1)
//        return false;
//    if (nodes[curIndex - 1].size != rootLeftSize + rootRightSize + 1)
//        return false;
//    *size = rootLeftSize + rootRightSize + 1;
//    *height = maxHeight + 1;
//    return true;
//}