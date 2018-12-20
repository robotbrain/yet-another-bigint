#include "bigint_internal.h"
#include <string.h>

/** Adds three unsigned ints and returns the carry out. */
int addAndCarry(WordType a, WordType b, WordType c, WordType* d) {
    WordType ab = a + b;
    WordType abc = ab + c;
    *d = abc;
    return (ab < a) + (abc < ab);
}

WordType mulAndCarry(WordType a, WordType b, WordType* c) {
    //we use the FOIL method for half-words so as not to
    //lose overflow bits.
    // (ahi + alo)(bhi + blo)
    // = ahi*bhi + ahi*blo + alo*bhi + alo*blo
    WordType ahi = HI_N_BITS(a, YABI_WORD_BIT_SIZE >> 1);
    WordType bhi = HI_N_BITS(b, YABI_WORD_BIT_SIZE >> 1);
    WordType alo = a & (((WordType)1 << (YABI_WORD_BIT_SIZE >> 1)) - 1);
    WordType blo = b & (((WordType)1 << (YABI_WORD_BIT_SIZE >> 1)) - 1);
    WordType tmp; //scratch space
    WordType carry;
    carry = ahi * bhi;
    tmp = ahi * blo;
    carry += HI_N_BITS(tmp, YABI_WORD_BIT_SIZE >> 1);
    carry += addAndCarry(*c, tmp << (YABI_WORD_BIT_SIZE >> 1), 0, c);
    tmp = alo * bhi;
    carry += HI_N_BITS(tmp, YABI_WORD_BIT_SIZE >> 1);
    carry += addAndCarry(*c, tmp << (YABI_WORD_BIT_SIZE >> 1), 0, c);
    carry += addAndCarry(*c, alo * blo, 0, c);
    return carry;
}

int cmpBuffers(size_t alen, const WordType* a, size_t blen, const WordType* b, int useSign) {
    int asign = HI_BIT(a[alen - 1]);
    int bsign = HI_BIT(b[blen - 1]);
    // case of different signs
    if(useSign && (asign ^ bsign)) {
        return bsign - asign;
    }
    // normalize alen and blen (trim leading sign words)
    while(alen > 1 && a[alen - 1] == (useSign && asign)) {
        alen--;
    }
    while(blen > 1 && b[blen - 1] == (useSign && bsign)) {
        blen--;
    }
    if(alen != blen) {
        // one is longer than the other
        // and comparison is based on length
        // this assumes that the buffers are trimmed to minimum size
        int res = (alen > blen) * 2 - 1;
        return (useSign && asign) ? -res : res;
    } else {
        // the first differing words determine the comparison
        for(size_t i = alen; i > 0; i--) {
            if(a[i - 1] != b[i - 1]) {
                int res = (a[i - 1] > b[i - 1]) * 2 - 1;
                return res;
            }
        }
        // a and b are well and truly equal
        return 0;
    }
}

int eqBuffers(size_t alen, const WordType* a, size_t blen, const WordType* b) {
    // assumes that the buffers are trimmed to minimum size
    if(alen != blen) {
        return 0;
    }
    return memcmp(a, b, alen * sizeof(WordType)) == 0;
}
