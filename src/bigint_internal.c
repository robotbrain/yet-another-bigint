#include "bigint_internal.h"

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
