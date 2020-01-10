#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "tree.h"

#ifndef ENCODER
#define ENCODER

typedef struct _Encoder {
	int bits;
	int bytes;
	uint8_t* code;
} Encoder;

Encoder* EncoderConstructor(void);
void EncoderDestructor(Encoder*);
void exploreTree(Tree*, Encoder*, Encoder**);
Encoder** makeEncoder(Tree**);
void addToEncoder(Encoder*, Encoder*);
uint8_t removeFirstByte(Encoder*);
int getCodeCount(Encoder**);

#endif /*ENCODER*/ 