#include "bigint_internal.h"

WordType yabi_toUnsigned(const BigInt* a) {
    return a->data[0];
}

SWordType yabi_toSigned(const BigInt* a) {
    return a->data[0];
}

size_t yabi_toSize(const BigInt* a) {
    #define SIZE_IN_WORDS (1 + sizeof(size_t) / sizeof(WordType))
    size_t res = 0;
    size_t shiftDist = 0;
    size_t i;
    //move each word to its appropriate spot
    for(i = 0; i < a->len && i < SIZE_IN_WORDS; i++) {
        res |= ((size_t)a->data[i]) << shiftDist;
        shiftDist += YABI_WORD_BIT_SIZE;
    }
    //sign extend to fill the remaining slots
    WordType asign = -HI_BIT(a->data[i - 1]);
    while(i < SIZE_IN_WORDS) {
        res |= ((size_t)asign) << shiftDist;
        i++;
        shiftDist += YABI_WORD_BIT_SIZE;
    }
    return res;
    #undef SIZE_IN_WORDS
}
