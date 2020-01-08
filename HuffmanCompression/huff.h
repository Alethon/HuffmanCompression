/*this is huff.h*/

#include "huffTree.h"

#ifndef HUFF
#define HUFF

Tree** initializeUnicodeFrequencies(void);
void getUnicodeFrequencies(Tree**, long long*, FILE*);
int getPaddingBits(long long*, int**);
void getPreorderFromTree(Tree*, uint8_t*, int*);
void writeHeader(FILE*, int, int, uint8_t*);
int huffmanCompress(int**, FILE*, FILE*);
void huff(char*, char*);

#endif /*HUFF*/

/*end of huff.h*/
