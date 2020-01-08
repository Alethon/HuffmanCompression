/*this is huff.h*/

#ifndef HUFF
#define HUFF

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

/* a binary tree with two members
 * n holds the character to be encoded (or -1 if the Tree is a branch)
 * f holds the character's frequency
 */
typedef struct _Tree {
	struct _Tree * left;
	struct _Tree * right;
	long long f;
	uint8_t n;
} Tree;

typedef struct _Node {
	struct _Node * next;
	struct _Node * prev;
	char c;
	char * address;
} Node;

typedef struct _List {
	Node * head;
} List;

#endif /*HUFF*/

/*end of huff.h*/
