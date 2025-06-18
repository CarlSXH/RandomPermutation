

#ifndef __AVL_TREE_H__
#define __AVL_TREE_H__

#include <cstddef>
#include <string>
#include <algorithm>


// This data structure represents integer values on a sorted
// index set, which allows for O(log n) time to change a value
// and O(log n) time to calculate the number of indices with
// values < L
//
// The data structure is implemented as an order statistics AVL 
// tree, where the size and height is stored at each node.
//
// The data structure does NOT support duplicate values
class AVLTree
{
public:
    typedef unsigned int index_t;
    
    AVLTree();

    // Create an empty array that has capacity "size."
    void Init(size_t size);
    // Create the array with size "n"
    void Init(int *values, size_t size);
    // Destroy the memory
    void Delete();

    // Insert an element at position "index" with the given "value."
    // This will assume that position index has not been used yet.
    void Insert(index_t index, int value);
    // Remove the element at position "index."
    // This will assume that position index is in the tree.
    void Remove(index_t index);
    // Change the value of the item at position "index"
    // to be "value." Here the index will be 0 based
    void Change(index_t index, int value);
    

    // Count the number of items with value < Bound
    size_t Count(int Bound);

    // Find the position of an element with the given value.
    // If the element is found, then the method returns true
    // and store the value in outIndex. If the method doesn't
    // find it, then it returns false.
    bool Find(int value, index_t *outIndex);

    // Testing purposes
    //std::string JSON();
    //void PRINT();
    //std::string JSON(index_t curIndex);
    //void RecurseTest();
    //void RecurseTest(index_t curIndex);
    //bool CheckTree(int *);
    //bool InternalCheck(index_t curIndex, bool *NodeSeen, size_t *size, size_t *height);

    
    // Change to private soon
private:
    struct AVLTreeNode {
        int value;
        index_t left;
        index_t right;
        size_t height;
        size_t size;
    };

    // Not really useful, can be removed
    int N;

    // The nodes are stored as a contiguous block in memory
    // Indices are all 1-based, where 0 means the null pointer
    // However, the actual index is not changed, so index 1 will
    // corresponds to nodes[0]
    AVLTreeNode *nodes;

    // Index to the root of the tree
    index_t root;


    // Internal methods

    // Return the size of the node at "index".
    // This method is safe when index = 0, otherwise
    // meaning the pointer is empty
    size_t NodeSize(index_t index);

    // Return the height of the node at "index".
    // This method is safe when index = 0, otherwise
    // meaning the pointer is empty
    size_t NodeHeight(index_t index);

    // Assuming the two children of an internal have the
    // correct height and size, recalculate the height and
    // size of the node given by "index".
    void RecalcHeightSize(index_t index);


    // Do a left rotation at "index"
    index_t RotateLeft(index_t index);
    // Do a right rotation at "index"
    index_t RotateRight(index_t index);
    // This method preserves the AVL property at "index" by
    // applying a left or right rotation when the heights of
    // the two children differ by most 2
    index_t BalanceCurrent(index_t index);


    // Delete the item with the minimum value from the subtree based at "currentNode",
    // and the index of the minimum index is outputted in "minIndex."
    // The new index for the subtree is returned.
    index_t FindMinDelete(index_t currentNode, index_t *minIndex);

    // Delete the item with the maximum value from the subtree based at "currentNode",
    // and the index of the minimum index is outputted in "maxIndex."
    // The new index for the subtree is returned.
    index_t FindMaxDelete(index_t currentNode, index_t *maxIndex);


    // Deleting the item at index "removedIndex" from subtree based at "currentNode",
    // and return the new index for the subtree.
    // Should be paired with InternalInsert
    index_t InternalDelete(index_t currentNode, index_t removedIndex);

    // Adding an item at position "addedIndex" from subtree based at "currentNode",
    // and return the new index for the subtree.
    // Should be paired with InternalDelete
    index_t InternalInsert(index_t currentNode, int a, index_t addedIndex);
};



#endif //__AVL_TREE_H__