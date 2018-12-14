#include "bigint_internal.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

size_t yabi_lshiftToBuf(const BigInt* a, size_t amt, size_t len, WordType* buffer) {
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
    //the number of *words* to shift left
    size_t shiftWords = (amt / YABI_WORD_BIT_SIZE);
    //special case logic
    if((a->len == 1 && a->data[0] == 0) || (shiftWords > len)) {
        //0 << x == 0
        memset(buffer, 0, len * sizeof(WordType));
        return 1;
    }
    //the remaining number of *bits* to shift left within a word
    WordType shiftRem = amt & (YABI_WORD_BIT_SIZE - 1);
    size_t stop = min(a->len + shiftWords, len);
    WordType asign = -HI_BIT(a->data[stop - shiftWords - 1]);
    //fill lower order words with 0
    memset(buffer, 0, shiftWords);
    size_t i;
    //carry bits shifted off
    WordType carry = 0;
    for(i = shiftWords; i < stop; i++) {
        buffer[i] = (WordType)(a->data[i - shiftWords] << shiftRem) | carry;
        carry = HI_N_BITS(a->data[i - shiftWords], shiftRem);
    }
    //add last carry in if we have room
    if(stop < len && carry != HI_N_BITS(asign, shiftRem)) {
        buffer[i++] = (WordType)(asign << shiftRem) | carry;
        stop++;
    }
    //shift with the sign extension of a
    memset(buffer + i, asign, (len - i) * sizeof(WordType));
    //fix potential overflow of the sign bit
    if(stop < len && (WordType)-HI_BIT(buffer[stop - 1]) != asign) {
        stop++;
    }
    return stop;
}

size_t yabi_rshiftToBuf(const BigInt* a, size_t amt, size_t len, WordType* buffer) {
    if(amt == 0) {
        //x >> 0 == x
        size_t stop = min(len, a->len);
        unsigned char sign = -HI_BIT(a->data[stop - 1]);
        memcpy(buffer, a->data, stop * sizeof(WordType));
        if(stop < len) {
            memset(buffer + stop, sign, (len - stop) * sizeof(WordType));
        }
        return stop;
    }
    //the number of words to shift right
    size_t shiftWords = (amt / YABI_WORD_BIT_SIZE);
    size_t stop = min(a->len, len);
    WordType asign = -HI_BIT(a->data[stop - 1]);
    //special case logic
    if((a->len == 1 && a->data[0] == 0) || (shiftWords > a->len)) {
        //0 >> x == 0 and a >> huge == sign
        memset(buffer, asign, len * sizeof(WordType));
        return 1;
    }
    #define SHIFT_HI(word) ((word & (((WordType)1 << shiftRem) - 1)) << (YABI_WORD_BIT_SIZE - shiftRem))
    //the remaining number of *bits* to shift right within a word
    WordType shiftRem = amt & (YABI_WORD_BIT_SIZE - 1);
    //initial carry, get the sign bit if data at idx `len` does not exist
    WordType carry = (stop + shiftWords >= a->len) ? (asign << (YABI_WORD_BIT_SIZE - shiftRem))
        : SHIFT_HI(a->data[stop + shiftWords]);
    //fill sign extension
    size_t st = a->len - shiftWords;
    memset(buffer + st, asign, (len - st) * sizeof(WordType));
    //fill shifted data from a->data
    for(size_t i = st; i > 0; i--) {
        buffer[i - 1] = (WordType)((a->data[i - 1 + shiftWords] >> shiftRem) | carry);
        carry = SHIFT_HI(a->data[i - 1 + shiftWords]);
    }
    //calculate actual buffer length
    while(stop > 1 && buffer[stop - 1] == asign) {
        stop--;
    }
    //fix for overcorrection
    if(HI_BIT(buffer[stop - 1]) != (asign & 1)) {
        stop++;
    }
    return stop;
    #undef SHIFT_HI
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

BigInt* yabi_rshift(const BigInt* a, size_t amt) {
    size_t len = a->len;
    BigInt* res = YABI_NEW_BIGINT(len);
    res->refCount = 0;
    res->len = len;
    len = yabi_rshiftToBuf(a, amt, len, res->data);
    if(len != res->len) {
        YABI_RESIZE_BIGINT(res, len);
    }
    return res;
}
