#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "huffTree.h"

#include "huff.h"
#include "unhuff.h"


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

