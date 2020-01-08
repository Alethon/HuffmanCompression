#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "tree.h"

//allocates and initializes a Tree (branch), returning its pointer
Tree* TreeConstructor() {
	Tree* tree = (Tree*)malloc(sizeof(Tree));
	tree->left = NULL;
	tree->right = NULL;
	tree->n = 255;
	tree->f = 0;
	return tree;
}

void TreeDestructor(Tree* t) {
	if (t == NULL)
		return;
	TreeDestructor(t->left);
	TreeDestructor(t->right);
	free(t);
	return;
}

//bubble sorting here, since the pointer has relatively few elements.
//Given a much larger, sparse pointer, zero freqency elements should be removed first.
void sortTreesDescending(Tree** uni, int n) {
	Tree* temp;
	for (int i = n - 1; i > 0; i--) {
		for (int j = 0; j < i; j++) {
			if ((uni[j])->f < (uni[j + 1])->f) {
				temp = uni[j];
				uni[j] = uni[j + 1];
				uni[j + 1] = temp;
			}
		}
	}
	return;
}

int getTreeCount(Tree** uni) {
	int nonzero;
	for (nonzero = 0; nonzero < (int)UNIQUE_BYTES; nonzero++)
		if (uni[nonzero]->f == 0)
			break;
	return nonzero;
}

//assume both are non-null trees 
Tree* branchFromTrees(Tree* t1, Tree* t2) {
	Tree* branch = TreeConstructor();
	branch->f = t1->f + t2->f;
	if (t2->f < t1->f) {
		branch->left = t2;
		branch->right = t1;
	}
	else {
		branch->left = t1;
		branch->right = t2;
	}
	return branch;
}

void makeTreeFromSorted(Tree** uni) {
	int n = getTreeCount(uni);
	while (n-- > 1) {
		uni[n - 1] = branchFromTrees(uni[n - 1], uni[n]);
		uni[n] = NULL;
		sortTreesDescending(uni, n);
	}
	return;
}

void explore_Tree(Tree* t, int loc, int** table, int n) {
	if (t->left == NULL) {
		table[t->n] = (int*)malloc(2 * sizeof(int));
		table[t->n][0] = n - 1;
		table[t->n][1] = loc;
	}
	else {
		loc <<= 1;
		exploreTree(t->left, loc, table, n + 1);
		exploreTree(t->right, loc + 1, table, n + 1);
	}
	return;
}

int** makeEncoding_Table(Tree** uni) {
	int** table = (int**)calloc((int)UNIQUE_BYTES, sizeof(int*));
	exploreTree(uni[0], 0, table, 1);
	return table;
}
