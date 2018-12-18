#ifndef YET_ANOTHER_BIGINT_INTERNAL_H
#define YET_ANOTHER_BIGINT_INTERNAL_H
#include "bigint.h"

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

// get the most significant n bits
#define HI_N_BITS(n, bits) ((n) >> (YABI_WORD_BIT_SIZE - (bits)))
// get the most significant bit (sign bit)
#define HI_BIT(n) ((n) >> (YABI_WORD_BIT_SIZE - 1))
// get the three most significant bits (for carry)
#define HI_3_BITS(n) ((n) >> (YABI_WORD_BIT_SIZE - 3))

// helpers
int addAndCarry(WordType a, WordType b, WordType c, WordType* d);
WordType mulAndCarry(WordType a, WordType b, WordType* c);
size_t addBuffers(
    size_t alen, const WordType* a,
    size_t blen, const WordType* b,
    int negateb,
    size_t len, WordType* buffer);
size_t mulBuffers(
    size_t alen, const WordType* adata,
    size_t blen, const WordType* bdata,
    size_t len, WordType* buffer);
int eqBuffers(size_t alen, const WordType* a, size_t blen, const WordType* b);
int cmpBuffers(size_t alen, const WordType* a, size_t blen, const WordType* b);

#endif
