#pragma once
#include "tree.h"

#ifndef UNHUFF
#define UNHUFF

void readPackingData(FILE*, int*, int*);
uint8_t* readBitTree(FILE*, int);
int readBitsFromTree(uint8_t*, int, int);
Tree* makeTreeFromBits(uint8_t*, int*);
int** makeDecodingTable(int**);
void unhuff(char*, char*);

#endif /*UNHUFF*/
