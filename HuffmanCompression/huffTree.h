#ifndef HUFF_TREE
#define HUFF_TREE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* a binary tree with two members
 * n holds the character to be encoded (or -1 if the Tree is a branch)
 * f holds the character's frequency
 */
typedef struct _Tree {
	struct _Tree* left;
	struct _Tree* right;
	long long f;
	uint8_t n;
} Tree;

Tree* TreeConstructor(void);
void TreeDestructor(Tree*);
void sortTreesDescending(Tree**, int);
int getTreeCount(Tree**);
Tree* branchFromTrees(Tree*, Tree*);
void makeTreeFromSorted(Tree**);
void exploreTree(Tree*, int, int**, int);
int** makeEncodingTable(Tree**);

#endif /*HUFF_TREE*/



#ifndef UNIQUE_BYTES

//2^8, the number of 8-bit numbers
#define UNIQUE_BYTES 256

#endif /*UNIQUE_BITS*/
