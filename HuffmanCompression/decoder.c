#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "tree.h"
#include "encoder.h"
#include "decoder.h"

Decoder* DecoderConstructor() {
	Decoder* d = malloc(sizeof(Decoder));
	d->bits = 0;
	d->bytes = 0;
	d->n = 0;
	d->code = NULL;
	d->mask = NULL;
	return d;
}

void DecoderDestructor(Decoder* d) {
	if (d->code != NULL)
		free(d->code);
	if (d->mask != NULL)
		free(d->mask);
	free(d);
	return;
}

//returns NULL if no match
Decoder* decode(Decoder* d, Decoder* c, int n) {
	int i, j;
	bool isMatch;
	Decoder* match = NULL;
	for (i = 0; i < n; i++) {
		if (c->bits >= d[i].bits) {
			isMatch = true;
			for (j = 0; j < c->bytes; j++)
				isMatch = isMatch && ((d[i].mask[j] & c->code[j]) == d[i].code[j]);
			if (isMatch) {
				match = &(d[i]);
				break;
			}
		} else if (c->bytes < d[i].bytes) {
			break;
		}
	}
	return match;
}

void removeBits(Decoder* t, Decoder* d) {
	int i;
	int n = d->bits / 8;
	int shift = d->bits % 8;
	for (i = n; i < t->bytes + n; i++) {
		if (i < t->bytes) {
			t->code[i - n] = (t->code[i] << shift) + (t->code[i + 1] >> (8 - shift));
		} else {
			t->code[i - n] = 0;
		}
	}
	t->bits -= d->bits;
	t->bytes = t->bits / 8 + (t->bits % 8 ? 1 : 0);
	return;
}

void addByte(Decoder* t, uint8_t byte) {
	uint16_t shifted = byte << (8 - (t->bits % 8));
	t->code[t->bits / 8] = (t->code[t->bits / 8] & (255 << (t->bits % 8))) + (shifted >> 8);
	if (t->bits % 8 > 0)
		t->code[t->bits / 8 + 1] = shifted & 255;
	t->bits += 8;
	t->bytes = t->bits / 8 + (t->bits % 8 ? 1 : 0);
	return;
}

Decoder* makeDecoder(Encoder** e, int n) {
	int i, j, k;
	Decoder* d = malloc(n * sizeof(Decoder));
	j = 0;
	i = 0;
	for (i = 0; i < (int)UNIQUE_BYTES; i++) {
		if (e[i] != NULL) {
			d[j].bits = e[i]->bits;
			d[j].bytes = e[i]->bytes;
			d[j].code = e[i]->code;
			d[j].n = i;
			d[j].mask = malloc(d[j].bytes * sizeof(uint8_t));
			for (k = 0; k < d[j].bits / 8; i++)
				d[j].mask[k] = 255;
			d[j].mask[d[j].bytes - 1] = 255 << ((8 - d[j].bits % 8) % 8);
			++j;
		}
	}
	/*for (i = 0; i < n; i++) {
		while (e[j++] != NULL);
		printf("%d\n", j - 1);
		d[i]->bits = e[j - 1]->bits;
		d[i]->bytes = e[j - 1]->bytes;
		d[i]->code = e[j - 1]->code;
		d[i]->n = j - 1;
		d[i]->mask = malloc(d[i]->bytes * sizeof(uint8_t));
		for (k = 0; k < d[i]->bits / 8; i++)
			d[i]->mask[k] = 255;
		d[i]->mask[d[i]->bytes - 1] = 255 << ((8 - d[i]->bits % 8) % 8);
	}*/
	return d;
}
