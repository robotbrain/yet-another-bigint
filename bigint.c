#include "bigint.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

// get the most significant n bits
#define HI_N_BITS(n, bits) (n >> (YABI_WORD_BIT_SIZE - bits))
// get the most significant bit (sign bit)
#define HI_BIT(n) (n >> (YABI_WORD_BIT_SIZE - 1))
// get the three most significant bits (for carry)
#define HI_3_BITS(n) (n >> (YABI_WORD_BIT_SIZE - 3))

// static helpers
static int addAndCarry(WordType a, WordType b, WordType c, WordType* d);
static size_t addBuffers(
    size_t alen, const WordType* a,
    size_t blen, const WordType* b,
    int negateb,
    size_t len, WordType* buffer);
// end static helpers

/** Adds three unsigned ints and returns the carry out. */
static int addAndCarry(WordType a, WordType b, WordType c, WordType* d) {
    WordType ab = a + b;
    WordType abc = ab + c;
    *d = abc;
    return (ab < a) + (abc < ab);
}

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

//helper macros for defining the bitwise operations
#define BITWISE_TOBUF_IMPL(op) \
    size_t stop = max(a->len, b->len); \
    stop = min(stop, len); \
    size_t alen = min(a->len, stop); \
    size_t blen = min(b->len, stop); \
    size_t upTo = min(alen, blen); \
    WordType asign = -HI_BIT(a->data[alen - 1]); \
    WordType bsign = -HI_BIT(b->data[blen - 1]); \
    size_t i; \
    /* fold a and b */ \
    for(i = 0; i < upTo; i++) { \
        buffer[i] = a->data[i] op b->data[i]; \
    } \
    /* fold a with sign extension of b */ \
    while(i < alen) { \
        buffer[i] = a->data[i] op bsign; \
        i++; \
    } \
    /* fold sign extension of a with b */ \
    while(i < blen) { \
        buffer[i] = asign op b->data[i]; \
        i++; \
    } \
    /* fold sign extensions of a and b */ \
    while(i < len) { \
        buffer[i++] = asign op bsign; \
    } \
    /* adjust to minimum buffer size */ \
    int signc = HI_BIT(buffer[stop - 1]); \
    while(stop > 1 && buffer[stop - 1] == (WordType)-signc) { \
        stop--; \
    } \
    if(HI_BIT(buffer[stop - 1]) != signc) { \
        stop++; \
    } \
    return stop;

size_t yabi_andToBuf(const BigInt* a, const BigInt* b, size_t len, WordType* buffer) {
    BITWISE_TOBUF_IMPL(&);
}

size_t yabi_orToBuf(const BigInt* a, const BigInt* b, size_t len, WordType* buffer) {
    BITWISE_TOBUF_IMPL(|);
}

size_t yabi_xorToBuf(const BigInt* a, const BigInt* b, size_t len, WordType* buffer) {
    BITWISE_TOBUF_IMPL(^);
}

BigInt* yabi_and(const BigInt* a, const BigInt* b) {
    size_t len = max(a->len, b->len);
    BigInt* res = YABI_NEW_BIGINT(len);
    res->refCount = 0;
    res->len = len;
    len = yabi_andToBuf(a, b, len, res->data);
    if(len != res->len) {
        YABI_RESIZE_BIGINT(res, len);
    }
    return res;
}

BigInt* yabi_or(const BigInt* a, const BigInt* b) {
    size_t len = max(a->len, b->len);
    BigInt* res = YABI_NEW_BIGINT(len);
    res->refCount = 0;
    res->len = len;
    len = yabi_orToBuf(a, b, len, res->data);
    if(len != res->len) {
        YABI_RESIZE_BIGINT(res, len);
    }
    return res;
}

BigInt* yabi_xor(const BigInt* a, const BigInt* b) {
    size_t len = max(a->len, b->len);
    BigInt* res = YABI_NEW_BIGINT(len);
    res->refCount = 0;
    res->len = len;
    len = yabi_xorToBuf(a, b, len, res->data);
    if(len != res->len) {
        YABI_RESIZE_BIGINT(res, len);
    }
    return res;
}

size_t yabi_complToBuf(const BigInt* a, size_t len, WordType* buffer) {
    size_t stop = min(a->len, len);
    size_t i;
    for(i = 0; i < stop; i++) {
        buffer[i] = ~a->data[i];
    }
    //sign extend the buffer
    int signb = HI_BIT(buffer[stop - 1]);
    while(i < len) {
        buffer[i++] = (WordType)-signb;
    }
    return stop;
}

BigInt* yabi_compl(const BigInt* a) {
    BigInt* res = YABI_NEW_BIGINT(a->len);
    res->refCount = 0;
    res->len = a->len;
    size_t s = yabi_complToBuf(a, a->len, res->data);
    assert(s == a->len);
    return res;
}

// shifts modulo the buffer len
size_t yabi_lshiftToBuf(const BigInt* a, size_t amt, size_t len, WordType* buffer) {
    //special case logic
    if(a->len == 1 && a->data[0] == 0) {
        //0 << x == 0
        memset(buffer, 0, len * sizeof(WordType));
        return 1;
    }
    if(amt == 0) {
        //x << 0 == x
        size_t stop = min(len, a->len);
        unsigned char sign = -HI_BIT(a->data[stop - 1]);
        memcpy(buffer, a->data, stop * sizeof(WordType));
        if(stop < len) {
            memset(buffer + stop, sign, (len - stop) * sizeof(WordType));
        }
        return stop;
    }
    //the number of *words* to shift left, modulo len
    size_t shiftWords = (amt / YABI_WORD_BIT_SIZE) % len;
    //the remaining number of *bits* to shift left within a word
    WordType shiftRem = amt & (YABI_WORD_BIT_SIZE - 1);
    size_t stop = min(a->len + shiftWords, len);
    WordType asign = -HI_BIT(a->data[stop - shiftWords - 1]);
    size_t i;
    //fill lower order words with 0
    for(i = 0; i < shiftWords; i++) {
        buffer[i] = 0;
    }
    //carry bits shifted off
    WordType carry = 0;
    for( ; i < stop; i++) {
        buffer[i] = (WordType)(a->data[i - shiftWords] << shiftRem) | carry;
        carry = HI_N_BITS(a->data[i - shiftWords], shiftRem);
    }
    //add last carry in if we have room
    if(stop < len && carry != HI_N_BITS(asign, shiftRem)) {
        buffer[i++] = (WordType)(asign << shiftRem) | carry;
        stop++;
    }
    //shift with the sign extension of a
    for( ; i < len; i++) {
        buffer[i] = asign;
    }
    //fix potential overflow of the sign bit
    if(stop < len && (WordType)-HI_BIT(buffer[stop - 1]) != asign) {
        stop++;
    }
    return stop;
}

BigInt* yabi_lshift(const BigInt* a, size_t amt) {
    size_t len = a->len + (amt / YABI_WORD_BIT_SIZE) + 1;
    BigInt* res = YABI_NEW_BIGINT(len);
    res->refCount = 0;
    res->len = len;
    len = yabi_lshiftToBuf(a, amt, len, res->data);
    if(len != res->len) {
        YABI_RESIZE_BIGINT(res, len);
    }
    return res;
}

size_t yabi_negateToBuf(const BigInt* a, size_t len, WordType* buffer) {
    // -a = ~a + 1
    size_t stop = yabi_complToBuf(a, len, buffer);
    WordType toAdd = 1;
    stop = addBuffers(len, buffer, 1, &toAdd, 0, len, buffer);
    return stop;
}

BigInt* yabi_negate(const BigInt* a) {
    // -a = ~a + 1
    BigInt* res = YABI_NEW_BIGINT(a->len + 1);
    res->refCount = 0;
    res->len = a->len + 1;
    size_t len = yabi_negateToBuf(a, res->len, res->data);
    if(len != res->len) {
        YABI_RESIZE_BIGINT(res, len);
    }
    return res;
}

// implements two's complement addition on two buffers
static size_t addBuffers(
        size_t alen, const WordType* adata,
        size_t blen, const WordType* bdata,
        int negateb,
        size_t len, WordType* buffer) {
    //when to stop storing the result, assuming no overflow
    size_t stop = max(alen, blen);
    stop = min(stop, len);
    //when to start sign extending
    size_t upTo = min(alen, blen);
    upTo = min(upTo, len);
    alen = min(stop, alen);
    blen = min(stop, blen);
    int signa = HI_BIT(adata[alen - 1]);
    int signb = HI_BIT(bdata[blen - 1]);
    unsigned carry;
    size_t i;
    int overflowed;
    if(negateb) {
        //add twos complement negation of b (-b = ~b + 1)
        carry = 1;
        for(i = 0; i < upTo; i++) {
            carry = addAndCarry(adata[i], ~bdata[i], carry, &buffer[i]);
        }
        //add leftovers from a with sign extension of b
        while(i < alen) {
            carry = addAndCarry(adata[i], (WordType)-!signb, carry, &buffer[i]);
            i++;
        }
        //add leftovers from b with sign extension of a
        while(i < blen) {
            carry = addAndCarry((WordType)-signa, ~bdata[i], carry, &buffer[i]);
            i++;
        }
        //overflow would occur iff a and b have the same sign, AND the sign
        //of the result is different from the sign of a and b. Except b
        //is negated, so we flip the sign of b.
        overflowed = signa != signb && signa != HI_BIT(buffer[stop - 1]);
        if(overflowed && stop < len) {
            //add the last carry (normally we drop the last carry in
            //two's complement arithmetic)
            addAndCarry((WordType)-signa, (WordType)-!signb, carry, &buffer[i]);
            stop++;
            i++;
        }
    } else {
        //add and and b normally
        carry = 0;
        for(i = 0; i < upTo; i++) {
            carry = addAndCarry(adata[i], bdata[i], carry, &buffer[i]);
        }
        //add leftovers from a with sign extension of b
        while(i < alen) {
            carry = addAndCarry(adata[i], (WordType)-signb, carry, &buffer[i]);
            i++;
        }
        //add leftovers from b with sign extension of a
        while(i < blen) {
            carry = addAndCarry((WordType)-signa, bdata[i], carry, &buffer[i]);
            i++;
        }
        //overflow would occur iff a and b have the same sign, AND the sign
        //of the result is different from the sign of a and b.
        overflowed = signa == signb && signa != HI_BIT(buffer[stop - 1]);
        if(overflowed && stop < len) {
            //add the last carry (normally we drop the last carry in
            //two's complement arithmetic)
            addAndCarry((WordType)-signa, (WordType)-signb, carry, &buffer[i]);
            stop++;
            i++;
        }
    }
    assert(stop == i);
    //sign extend sign of result to the rest of the buffer
    int signc = HI_BIT(buffer[i - 1]);
    while(i < len) {
        buffer[i++] = (WordType)-signc;
    }
    //the result may take up less space than is indicated by `stop` because
    //of extra words taken up by unnecessary sign extensions
    // e.g. 128 + -1 = 0x00_80 + 0xff = 0x00_7f = 0x7f
    while(stop > 1 && buffer[stop - 1] == (WordType)-signc) {
        stop--;
    }
    //but don't go too far
    if(HI_BIT(buffer[stop - 1]) != signc) {
        stop++;
    }
    //and that's it! You just survived two's complement addition
    return stop;
}

size_t yabi_addToBuf(const BigInt* a, const BigInt* b, size_t len, WordType* buffer) {
    return addBuffers(a->len, a->data, b->len, b->data, 0, len, buffer);
}

size_t yabi_subToBuf(const BigInt* a, const BigInt* b, size_t len, WordType* buffer) {
    return addBuffers(a->len, a->data, b->len, b->data, 1, len, buffer);
}

BigInt* yabi_add(const BigInt* a, const BigInt* b) {
    size_t len = max(a->len, b->len) + 1;
    BigInt* res = YABI_NEW_BIGINT(len);
    res->refCount = 0;
    res->len = len;
    len = addBuffers(a->len, a->data, b->len, b->data, 0, len, res->data);
    if(len != res->len) {
        YABI_RESIZE_BIGINT(res, len);
    }
    return res;
}

BigInt* yabi_sub(const BigInt* a, const BigInt* b) {
    size_t len = max(a->len, b->len) + 1;
    BigInt* res = YABI_NEW_BIGINT(len);
    res->refCount = 0;
    res->len = len;
    len = addBuffers(a->len, a->data, b->len, b->data, 1, len, res->data);
    if(len != res->len) {
        YABI_RESIZE_BIGINT(res, len);
    }
    return res;
}

BigInt* yabi_fromStr(const char* str) {
    //overestimates required space
    //space = 1 + (log2(num) / log2(WordBase))
    #define TO_LOGB2(a) ((a) * 7 / 2)
    size_t cap = 1 + TO_LOGB2(strlen(str)) / YABI_WORD_BIT_SIZE;
    #undef TO_LOGB2
    WordType* data = YABI_CALLOC(cap, sizeof(WordType));
    const char* c = str;
    //get the sign
    int negative = 0;
    if(*c == '-') {
        c++;
        negative = 1;
    }
    while(*c) {
        assert(*c >= '0' && *c <= '9');
        //calculate 10*x + digit
        //save the carry from the bits shifted out
        unsigned carry = HI_3_BITS(data[0]) + HI_BIT(data[0]);
        //10*x = 8*x + 2*x = (x << 3) + (x << 1)
        carry += addAndCarry(data[0] << 3, data[0] << 1, (*c - '0'), &data[0]);
        //calculate 10*x + carry for each higher word
        for(size_t i = 1; i < cap; i++) {
            unsigned newCarry = HI_3_BITS(data[i]) + HI_BIT(data[i]);
            newCarry += addAndCarry(data[i] << 3, data[i] << 1, carry, &data[i]);
            carry = newCarry;
        }
        //there should be no carry out, since
        //1) we should not have overflow, and
        //2) we are adding positive numbers
        assert(carry == 0);
        c++;
    }
    //if negative, flip the bits and add one
    if(negative) {
        unsigned carry = 1;
        for(size_t i = 0; i < cap; i++) {
            data[i] = ~data[i];
            carry = addAndCarry(data[i], carry, 0, &data[i]);
        }
        //throw away last carry
    }
    //calculate true number of words. i.e. get rid of the extra sign extension.
    size_t len = cap;
    while(len > 1 && data[len - 1] == (WordType)-negative) {
        len--;
    }
    //if the sign bit is not the same as `negative`, we need to add it back
    if(HI_BIT(data[len - 1]) != negative) {
        assert(len < cap);
        data[len++] = (WordType)-negative;
    }
    BigInt* res = YABI_NEW_BIGINT(len);
    res->refCount = 0;
    res->len = len;
    memcpy(res->data, data, len * sizeof(WordType));
    YABI_FREE(data);
    return res;
}
