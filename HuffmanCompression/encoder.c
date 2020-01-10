#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "tree.h"
#include "encoder.h"

Encoder* EncoderConstructor() {
	Encoder* e = (Encoder*)malloc(sizeof(Encoder));
	e->bits = 0;
	e->bytes = 0;
	e->code = NULL;
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
		pos->bits = pos->bits + 1;
		exploreTree(t->left, pos, table);
		(pos->code)[pos->bits / 8] = (pos->code)[pos->bits / 8] | (1 << ((8 - pos->bits % 8) % 8));
		exploreTree(t->right, pos, table);
		pos->bits = pos->bits - 1;
		if (pos->bits % 8 > 0) {
			(pos->code)[pos->bits / 8] = (pos->code)[pos->bits / 8] & (255 << (8 - pos->bits % 8));
		} else {
			(pos->code)[pos->bits / 8] = 0;// (pos->code)[pos->bits / 8] & 254;
		}
	}
	return;
}

Encoder** makeEncoder(Tree** uni) {
	Encoder* pos = EncoderConstructor();
	pos->code = (uint8_t*)malloc(34 * sizeof(uint8_t));
	for (int i = 0; i < 34; i++)
		pos->code[i] = 0;
	Encoder** table = (Encoder**)malloc((int)UNIQUE_BYTES * sizeof(Encoder*));
	for (int i = 0; i < (int)UNIQUE_BYTES; i++)
		table[i] = NULL;
	exploreTree(uni[0], pos, table);
	//EncoderDestructor(pos);
	return table;
}

void addToEncoder(Encoder* e0, Encoder* e1) {
	int n = e0->bits % 8;
	//printf("before %02x, after ", e0->code[0]);
	e0->code[e0->bits / 8] = (e0->code[e0->bits / 8] & (255 << (8 - n))) + (e1->code[0] >> n);
	for (int i = 1; i < e1->bytes; i++) {
		//e0->code[e0->bits / 8 + i - 1] = (e0->code[e0->bits / 8 + i - 1]) | e1->code[i] >> n;
		//e0->code[e0->bits / 8 + i] = e1->code[i] << (8 - n);
		e0->code[e0->bits / 8 + i] = (e1->code[i - 1] << (8 - n)) + (e1->code[i] >> n);
	}
	e0->code[e0->bits / 8 + e1->bytes] = e1->code[e1->bytes - 1] << (8 - n);
	//printf("%02x, x %02x\n", e0->code[0], e1->code[0]);
	e0->bits += e1->bits;
	e0->bytes = e0->bits / 8 + (e0->bits % 8 ? 1 : 0);
	return;
}

uint8_t removeFirstByte(Encoder* e) {
	uint8_t b = e->code[0];
	//printf("removed %02x\n", b);
	for (int i = 1; i <= e->bytes; i++)
		e->code[i - 1] = e->code[i];
	e->bytes -= 1;
	e->bits -= 8;
	//*(e->code + e->bytes) = 0;
	return b;
}

int getCodeCount(Encoder** e) {
	int count = 0;
	for (int i = 0; i < (int)UNIQUE_BYTES; i++) if (e[i] != NULL)
		++count;
	return count;
}
