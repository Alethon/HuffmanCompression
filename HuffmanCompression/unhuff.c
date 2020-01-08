#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "unhuff.h"
#include "huff.h"

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
	}
	else {
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
	if (t == NULL) {
		printf("");
	}
	else if (t->n != 255) {
		printf("%c: %d\n", (char)t->n, (int)t->f);
	}
	else {
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
		printf("%c: %d\n", (char)uni[i]->n, (int)uni[i]->f);
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
