#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "huff.h"

#define UNIQUE_BYTES 256

//Tree methods
Tree* TreeConstructor(void);
void sortTreesDescending(Tree**, int);
int getTreeCount(Tree**);
Tree* branchFromTrees(Tree*, Tree*);
void makeTreeFromSorted(Tree**);
void exploreTree(Tree*, int, int**, int);
int** makeEncodingTable(Tree**);

//initialization methods
Tree** initializeUnicodeFrequencies(void);

//data processing
void getUnicodeFrequencies(Tree**, long long*, FILE*);
void huff(char*, char*);
void unhuff(char*, char*);

//testing
void printTree(Tree*);
void testTreeSort(void);
void testTreeAssembly(void);
void testEncodingTable(void);

int main() {
	//testTreeSort();
	//testTreeAssembly();
	//testEncodingTable();
	huff("test.txt", "tested.huff");
	unhuff("tested.huff", "tested.txt");
	return 0;
}

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

//returns a pointer to a UNIQUE_BYTES-length pointer to future Tree structures, which hold the frequency of each unicode character
Tree** initializeUnicodeFrequencies() {
	Tree** uni = (Tree**)malloc((int)UNIQUE_BYTES * sizeof(Tree*));
	Tree* tree;
	for (int c = 0; c < (int)UNIQUE_BYTES; c++) {
		tree = TreeConstructor();
		tree->n = c;
		uni[c] = tree;
	}
	return uni;
}

//parses characters from the open file pointer, populating the unicode frequencies
void getUnicodeFrequencies(Tree** uni, long long* counts, FILE* fp) {
	uint8_t c;
	while (fread(&c, 1, 1, fp)) {
		++((uni[c])->f);
		//fread(&c, 1, 1, fp);
	}
	for (int i = 0; i < (int)UNIQUE_BYTES; i++)
		counts[i] = uni[i]->f;
	return;
}

//bubble sorting here, since the pointer has relatively few elements.
//Given a much larger, sparse pointer, zero freqency elements should be removed first.
void sortTreesDescending(Tree** uni, int n) {
	Tree* temp;
	for (int i = n - 1 ; i > 0; i--) {
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
	} else {
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

void exploreTree(Tree* t, int loc, int** table, int n) {
	if (t->left == NULL) {
		table[t->n] = (int*)malloc(2 * sizeof(int));
		table[t->n][0] = n - 1;
		table[t->n][1] = loc;
	} else {
		loc <<= 1;
		exploreTree(t->left, loc, table, n + 1);
		exploreTree(t->right, loc + 1, table, n + 1);
	}
	return;
}

int** makeEncodingTable(Tree** uni) {
	int** table = (int**)calloc((int)UNIQUE_BYTES, sizeof(int*));
	exploreTree(uni[0], 0, table, 1);
	return table;
}

int getPaddingBits(long long* count, int** table) {
	int paddingBits = 0;
	for (int i = 0; i < (int)UNIQUE_BYTES; i++) if (table[i]) {
		paddingBits += (count[i] % 8) * table[i][0];
		paddingBits %= 8;
	}
	return (8 - paddingBits) % 8;
}

void getPreorderFromTree(Tree* t, uint8_t* bitTree, int* bits) {
	int i = *bits / 8;
	if (t->left == NULL) {
		//write leaf
		bitTree[i] = bitTree[i] + (1 << (7 - *bits % 8));
		*bits = *bits + 1;
		i = *bits / 8;
		bitTree[i] = bitTree[i] + ((t->n) >> (*bits % 8));
		if (*bits % 8 > 0)
			bitTree[i + 1] = (t->n) << (8 - *bits % 8);
		*bits = *bits + 8;
		//printf("%d ", (int)t->n);
	} else {
		//write branch
		//printf("branch ");
		*bits = *bits + 1;
		//write left
		getPreorderFromTree(t->left, bitTree, bits);
		//write right
		getPreorderFromTree(t->right, bitTree, bits);
	}
	return;
}

//The first 4 bits contains the # of padding bits in the last byte,
//the next 12 bits contain the # of bits containing the tree data (which has a max size of 2559).
void writeHeader(FILE* fp, int paddingBits, int treeBits, uint8_t* bitTree) {
	uint8_t c;
	uint16_t d;
	for (int i = 0; i < 40; i++)
		printf("%02x", bitTree[i]);
	printf("\n\n");

	//write padding and tree specs
	d = (paddingBits << 12) + treeBits;
	fwrite(&d, 2, 1, fp);

	//write bitTree (zero-padded)
	int n = (treeBits / 8) + (treeBits % 8 ? 1 : 0);//number of bytes to write from bitTree
	fwrite(bitTree, 1, n, fp);
	/*fwrite(bitTree, 8, n / 8, fp);//writing the fully-populated unsigned longs
	for (int i = 7; i > 7 - n % 8; i--) {
		c = (bitTree[n / 8] >> (8 * i)) & 255;
		fwrite(&c, 1, 1, fp);
	}*/


	return;
}

int huffmanCompress(int** table, FILE* fpi, FILE* fpo) {
	int n = -1;
	int acm = 0;//accumulator
	int* encoding;
	uint8_t c, code;
	while (fread(&c, 1, 1, fpi)) if ((encoding = table[c]) != NULL) {
		acm = (acm << encoding[0]) + encoding[1];
		n += encoding[0];
		if (n > 6) {
			n -= 8;
			code = (uint8_t)((acm & (255 << (n + 1))) >> (n + 1));
			fwrite(&code, 1, 1, fpo);
		}
	} else {
		printf("%c's encoding is missing!\n", c);
	}
	int padding = 0;
	if (n > -1) {
		padding = 7 - n;
		code = ((acm << padding) & 255);
		fwrite(&code, 1, 1, fpo);
	}
	return padding;
}

void huff(char* fileIn, char* fileOut) {
	//open files
	FILE* fpi;
	FILE* fpo;
	fopen_s(&fpi, fileIn, "r");
	if (fpi == NULL)
		return;
	fopen_s(&fpo, fileOut, "w+");
	if (fpo == NULL) {
		fclose(fpi);
		return;
	}

	//make the Huffman tree and encoding table 
	Tree** uni = initializeUnicodeFrequencies();
	long long* count = (long long*)malloc((int)UNIQUE_BYTES * sizeof(long long));
	getUnicodeFrequencies(uni, count, fpi);
	fseek(fpi, 0, SEEK_SET);
	sortTreesDescending(uni, (int)UNIQUE_BYTES);
	makeTreeFromSorted(uni);
	int** table = makeEncodingTable(uni);
	//printf("%d, %d\n", table['o'][0], table['o'][1]);

	//create the header from the tree and encoding, then write it to the output file
	int treeBits = 0;
	int paddingBits = getPaddingBits(count, table);
	uint8_t* bitTree = (uint8_t*)calloc(320, sizeof(uint8_t));
	getPreorderFromTree(*uni, bitTree, &treeBits);
	//bitTree[treeBits / 8] = bitTree[treeBits / 8] << ((8 - treeBits % 8) % 8);
	//printf("%d\n", treeBits);
	writeHeader(fpo, paddingBits, treeBits, bitTree);

	//Huffman compress
	//printf("%d\n", huffmanCompress(table, fpi, fpo) - paddingBits);
	huffmanCompress(table, fpi, fpo);

	//clean up
	fclose(fpi);
	fclose(fpo);
	free(count);
	TreeDestructor(uni[0]);
	free(uni);
	free(bitTree);
	for (int i = 0; i < (int)UNIQUE_BYTES; i++) if (table[i] != NULL)
		free(table[i]);
	free(table);

	return;
}

void readPackingData(FILE* fp, int* paddingBits, int* treeBits) {
	uint16_t head;
	fread(&head, 2, 1, fp);
	*paddingBits = head >> 12;
	*treeBits = head & 4095;
	return;
}

uint8_t* readBitTree(FILE* fp, int treeBits) {
	int treeBytes = treeBits / 8 + (treeBits % 8 ? 1 : 0);
	//printf("tb%d\n", treeBytes);
	uint8_t* tree = (uint8_t*)calloc(treeBytes + 1, sizeof(uint8_t));
	fread(tree, 1, treeBytes, fp);
	//for (int i = 0; i < treeBytes; i++)
	//	printf("%02x", tree[i]);
	return tree;
}

//read starting at effective index n
int readBitsFromTree(uint8_t* bitTree, int n, int num) {
	int output = 0;
	int i = n / 8;
	int mask = (255 >> (8 - num)) << (16 - num);
	output = (bitTree[i] << 8) + bitTree[i + 1];
	output = (output & (mask >> (n % 8))) >> (16 - num - n % 8);
	//printf("read %d\n", output);
	return output;
}

Tree* makeTreeFromBits(uint8_t* bitTree, int* n) {
	Tree* t = TreeConstructor();
	if (readBitsFromTree(bitTree, *n, 1) == 1) {
		t->n = readBitsFromTree(bitTree, *n + 1, 8);
		*n = *n + 9;
		//printf("%d\n", t->n);
	} else {
		//printf("0\n");
		*n = *n + 1;
		t->left = makeTreeFromBits(bitTree, n);
		t->right = makeTreeFromBits(bitTree, n);
	}
	return t;
}

int** makeDecodingTable(int** etable) {
	return NULL;
}

void unhuff(char* fileIn, char* fileOut) {
	//open files
	FILE* fpi;
	FILE* fpo;
	fopen_s(&fpi, fileIn, "r");
	if (fpi == NULL)
		return;
	fopen_s(&fpo, fileOut, "w+");
	if (fpo == NULL) {
		fclose(fpi);
		return;
	}

	//get the Huffman tree and encoding table 
	int n = 0;
	int paddingBits, treeBits;
	readPackingData(fpi, &paddingBits, &treeBits);
	uint8_t* bitTree = readBitTree(fpi, treeBits);
	Tree** uni = (Tree**)malloc(sizeof(Tree*));
	*uni = makeTreeFromBits(bitTree, &n);
	int** table = makeEncodingTable(uni);
	/*for (int i = 0; i < (int)UNIQUE_BYTES; i++) {
		if (table[i] != NULL) {
			printf("%c: %d, %d\n", (char)i, table[i][0], table[i][1]);
		}
	}*/


	/*
	Tree** uni = initializeUnicodeFrequencies();
	long long* count = (long long*)malloc((int)UNIQUE_BYTES * sizeof(long long));
	getUnicodeFrequencies(uni, count, fpi);
	fseek(fpi, 0, SEEK_SET);
	sortTreesDescending(uni, (int)UNIQUE_BYTES);
	makeTreeFromSorted(uni);
	int** table = makeEncodingTable(uni);
	*/
	return;
}

void printTree(Tree* t) {
	if (t == NULL){
		printf("");
	} else if (t->n != 255) {
		printf("%c: %d\n", (char)t->n, (int)t->f);
	} else {
		printf("%d\n", (int)t->f);
		printTree(t->left);
		printTree(t->right);
		printf("<-%d\n", (int)t->f);
	}
	return;
}

void testTreeSort() {
	Tree** uni = initializeUnicodeFrequencies();
	FILE* fp;
	fopen_s(&fp, "test.txt", "r");
	long long* count = (long long*)malloc((int)UNIQUE_BYTES * sizeof(long long));
	getUnicodeFrequencies(uni, count, fp);
	sortTreesDescending(uni, (int)UNIQUE_BYTES);
	for (int i = 0; i < (int)UNIQUE_BYTES; i++) {
		if (uni[i]->f == 0)
			break;
		printf("%c: %d\n", (char) uni[i]->n, (int) uni[i]->f);
	}
	if (fp != NULL)
		fclose(fp);
	return;
}

void testTreeAssembly() {
	Tree** uni = initializeUnicodeFrequencies();
	FILE* fp;
	fopen_s(&fp, "test.txt", "r");
	long long* count = (long long*)malloc((int)UNIQUE_BYTES * sizeof(long long));
	getUnicodeFrequencies(uni, count, fp);
	if (fp != NULL)
		fclose(fp);
	sortTreesDescending(uni, (int)UNIQUE_BYTES);
	makeTreeFromSorted(uni);
	printf("%d\n", (int)uni[0]->f);
	printTree(uni[0]);
	return;
}

void testEncodingTable() {
	Tree** uni = initializeUnicodeFrequencies();
	FILE* fp;
	fopen_s(&fp, "test.txt", "r");
	long long* count = (long long*)malloc((int)UNIQUE_BYTES * sizeof(long long));
	getUnicodeFrequencies(uni, count, fp);
	if (fp != NULL)
		fclose(fp);
	sortTreesDescending(uni, (int)UNIQUE_BYTES);
	makeTreeFromSorted(uni);
	int** table = makeEncodingTable(uni);
	for (int i = 0; i < (int)UNIQUE_BYTES; i++) {
		if (table[i] != NULL) {
			printf("%c: %d, %d\n", (char)i, table[i][0], table[i][1]);
		}
	}
	return;
}
