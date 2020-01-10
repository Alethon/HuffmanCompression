#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "unhuff.h"
#include "huff.h"
#include "encoder.h"
#include "decoder.h"

void readPackingData(FILE* fp, int* paddingBits, int* treeBits) {
	uint16_t head;
	fread(&head, 2, 1, fp);
	*paddingBits = head >> 12;
	*treeBits = head & 4095;
	return;
}

uint8_t* readBitTree(FILE* fp, int treeBits) {
	int treeBytes = treeBits / 8 + (treeBits % 8 ? 1 : 0);
	uint8_t* tree = (uint8_t*)calloc(treeBytes + 1, sizeof(uint8_t));
	fread(tree, 1, treeBytes, fp);
	return tree;
}

//read starting at effective index n
int readBitsFromTree(uint8_t* bitTree, int n, int num) {
	int output = 0;
	int i = n / 8;
	int mask = (255 >> (8 - num)) << (16 - num);
	output = (bitTree[i] << 8) + bitTree[i + 1];
	output = (output & (mask >> (n % 8))) >> (16 - num - n % 8);
	return output;
}

Tree* makeTreeFromBits(uint8_t* bitTree, int* n) {
	Tree* t = TreeConstructor();
	if (readBitsFromTree(bitTree, *n, 1) == 1) {
		t->n = readBitsFromTree(bitTree, *n + 1, 8);
		*n = *n + 9;
	}
	else {
		*n = *n + 1;
		t->left = makeTreeFromBits(bitTree, n);
		t->right = makeTreeFromBits(bitTree, n);
	}
	return t;
}

int huffmanDecode(Encoder** e, FILE* fpi, FILE* fpo, int padding) {
	uint8_t c;
	Decoder* code;
	int n = getCodeCount(e);
	Decoder* d = makeDecoder(e, n);
	Decoder* t = DecoderConstructor();
	t->code = malloc(34);
	for (int i = 0; i < 34; i++)
		t->code[i] = 0;
	while (fread(&c, 1, 1, fpi) || feof(fpi) == 0) {
		printf("%02x", c);
		addByte(t, c);
		while (t->bits > padding && (code = decode(d, t, n)) != NULL) {
			fwrite(&(code->n), 1, 1, fpo);
			removeBits(t, code);
		}
	}
	return t->bits;
}

void unhuff(char* fileIn, char* fileOut) {
	//open files
	FILE* fpi;
	FILE* fpo;
	fopen_s(&fpi, fileIn, "rb");
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
	Encoder** e = makeEncoder(uni);
	printf("\n%d\n", huffmanDecode(e, fpi, fpo, paddingBits));
	return;
}
