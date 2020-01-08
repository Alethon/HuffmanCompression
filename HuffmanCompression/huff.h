/*this is huff.h*/

#include "tree.h"
#include "encoder.h"

#ifndef HUFF
#define HUFF

Tree** initializeUnicodeFrequencies(void);
void getUnicodeFrequencies(Tree**, long long*, FILE*);
int getPaddingBits(long long*, Encoder**);
void getPreorderFromTree(Tree*, uint8_t*, int*);
void writeHeader(FILE*, int, int, uint8_t*);
int huffmanCompress(Encoder**, FILE*, FILE*);
void huff(char*, char*);

#endif /*HUFF*/

/*end of huff.h*/
