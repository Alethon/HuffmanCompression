#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "tree.h"
#include "encoder.h"

Encoder* EncoderConstructor() {
	Encoder* e = (Encoder*)calloc(1, sizeof(Encoder));
	return e;
}

void EncoderDestructor(Encoder* e) {
	free(e->code);
	free(e);
	return;
}

void exploreTree(Tree* t, Encoder* pos, Encoder** table) {
	if (t->left == NULL) {
		Encoder* n = EncoderConstructor();
		n->bits = pos->bits;
		n->bytes = (pos->bits - 1) / 8 + ((pos->bits - 1) % 8 ? 1 : 0);
		n->code = (uint8_t*)calloc(n->bytes, sizeof(uint8_t));
		if (n->code != 0)
			memcpy(n->code, pos->code, n->bytes);
		table[t->n] = n;
	} else {
		pos->bits += 1;
		exploreTree(t->left, pos, table);
		(pos->code)[pos->bits / 8] = (pos->code)[pos->bits / 8] | (128 >> (pos->bits % 8));
		exploreTree(t->right, pos, table);
		(pos->code)[pos->bits / 8] = (pos->code)[pos->bits / 8] & ~(128 >> (pos->bits % 8));
		pos->bits -= 1;
	}
	return;
}

Encoder** makeEncoder(Tree** uni) {
	Encoder* pos = EncoderConstructor();
	pos->code = (uint8_t*)calloc(32, sizeof(uint8_t));
	Encoder** table = (Encoder**)calloc((int)UNIQUE_BYTES, sizeof(Encoder*));
	exploreTree(uni[0], pos, table);
	EncoderDestructor(pos);
	return table;
}

void addToEncoder(Encoder* e0, Encoder* e1) {
	int n = e0->bits % 8;
	for (int i = 0; i < e1->bytes; i++) {
		e0->code[e0->bytes + i - 1] += e1->code[i] >> n;
		e0->code[e0->bytes + i] += e1->code[i + 1] << (8 - n);
	}
	e0->bits += e1->bits;
	e0->bytes = e0->bits / 8 + (e0->bits % 8 ? 1 : 0);
	return;
}

uint8_t removeFirstByte(Encoder* e) {
	uint8_t b = *(e->code);
	for (int i = 1; i < e->bytes; i++)
		*(e->code + i - 1) = *(e->code + i);
	e->bytes -= 1;
	e->bits -= 8;
	*(e->code + e->bytes) = 0;
	return b;
}
