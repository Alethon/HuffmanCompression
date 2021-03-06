#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "tree.h"
#include "encoder.h"

#include "huff.h"
#include "unhuff.h"

int main() {
	//huff("test.txt", "tested.huff");
	//printf("compressed\n");
	/*FILE* fpo;
	fopen_s(&fpo, "tested.huff", "r");
	if (fpo == NULL) {
		return 0;
	}
	uint8_t c;
	int d = 0;
	for (int i = 0; i < 17; i++) {
		fread(&c, 1, 1, fpo);
		printf("read %02x\n", c);
		++d;
	}*/
	//printf("%d bytes read\n", d);
	unhuff("tested.huff", "tested.txt");
	return 0;
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

int getPaddingBits(long long* count, Encoder** table) {
	int paddingBits = 0;
	for (int i = 0; i < (int)UNIQUE_BYTES; i++) if (table[i]) {
		paddingBits += (count[i] % 8) * table[i]->bits;
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
	} else {
		//write branch
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
	//for (int i = 0; i < 40; i++)
	//	printf("%02x", bitTree[i]);
	//printf("\n\n");

	//write padding and tree specs
	d = (paddingBits << 12) + treeBits;
	fwrite(&d, 2, 1, fp);

	//write bitTree (zero-padded)
	int n = (treeBits / 8) + (treeBits % 8 ? 1 : 0);//number of bytes to write from bitTree
	fwrite(bitTree, 1, n, fp);
	printf("%d\n", n + 2);
	/*fwrite(bitTree, 8, n / 8, fp);//writing the fully-populated unsigned longs
	for (int i = 7; i > 7 - n % 8; i--) {
		c = (bitTree[n / 8] >> (8 * i)) & 255;
		fwrite(&c, 1, 1, fp);
	}*/
	return;
}

//CRT doesn't like this function in debug mode, but it functions correctly without memory errors in Release mode
int huffmanCompress(Encoder** table, FILE* fpi, FILE* fpo) {
	uint8_t c, code;
	Encoder* e = EncoderConstructor();
	e->bits = 0;
	e->bytes = 0;
	uint8_t* n = (uint8_t*)malloc(34 * sizeof(uint8_t));
	for (int i = 0; i < 33; i++)
		n[i] = 0;
	n[33] = '\0';
	e->code = n;
	while (fread(&c, 1, 1, fpi)) {
		//printf("read %c\n", (char)c);
		addToEncoder(e, table[c]);
		while (e->bits > 8) {
			c = removeFirstByte(e);
			printf("wrote %02x\n", c);
			fwrite(&c, 1, 1, fpo);
		}
	}
	if (e->bits > 0) {
		c = removeFirstByte(e);
		fwrite(&c, 1, 1, fpo);
		printf("wrote %02x\n", c);
	}
	c = -1 * (e->bits);
	//EncoderDestructor(e);
	return c;
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
	Encoder** table = makeEncoder(uni);
	for (int i = 0; i < 256; i++) if (table[i] != NULL) {
		printf("%c : %02x, %d\n", (char)i, (table[i]->code)[0], table[i]->bits);
	}
	//printf("%d, %d\n", table['o'][0], table['o'][1]);

	//create the header from the tree and encoding, then write it to the output file
	int treeBits = 0;
	int paddingBits = getPaddingBits(count, table);
	uint8_t* bitTree = (uint8_t*)calloc(320, sizeof(uint8_t));
	getPreorderFromTree(*uni, bitTree, &treeBits);
	//bitTree[treeBits / 8] = bitTree[treeBits / 8] << ((8 - treeBits % 8) % 8);
	writeHeader(fpo, paddingBits, treeBits, bitTree);
	//printf("header written\n");

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

