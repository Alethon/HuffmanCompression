#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "tree.h"
#include "encoder.h"

#ifndef DECODER
#define DECODER

typedef struct _Decoder {
	int bits;
	int bytes;
	uint8_t n;
	uint8_t* code;
	uint8_t* mask;
} Decoder;

Decoder* DecoderConstructor(void);
void DecoderDestructor(Decoder*);
Decoder* decode(Decoder*, Decoder*, int);
void removeBits(Decoder*, Decoder*);
void addByte(Decoder*, uint8_t);
Decoder* makeDecoder(Encoder**, int);

#endif /*DECODER*/