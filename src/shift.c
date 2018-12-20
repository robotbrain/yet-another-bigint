#include "bigint_internal.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

size_t lshiftBuffers(size_t alen, const WordType* a, size_t amt, size_t len, WordType* buffer) {
    if(amt == 0) {
        //x << 0 == x
        size_t stop = min(len, alen);
        unsigned char sign = -HI_BIT(a[stop - 1]);
        memcpy(buffer, a, stop * sizeof(WordType));
        if(stop < len) {
            memset(buffer + stop, sign, (len - stop) * sizeof(WordType));
        }
        return stop;
    }
    //the number of *words* to shift left
    size_t shiftWords = (amt / YABI_WORD_BIT_SIZE);
    //special case logic
    if((alen == 1 && a[0] == 0) || (shiftWords >= len)) {
        //0 << x == 0
        memset(buffer, 0, len * sizeof(WordType));
        return 1;
    }
    //the remaining number of *bits* to shift left within a word
    WordType shiftRem = amt & (YABI_WORD_BIT_SIZE - 1);
    size_t stop = min(alen + shiftWords, len);
    WordType asign = -HI_BIT(a[alen - 1]);
    // hi carry out
    WordType carry = HI_N_BITS(a[stop - 1 - shiftWords], shiftRem);
    size_t i = stop;
    // set the hi words to the sign extension
    memset(buffer + stop, asign, (len - stop) * sizeof(WordType));
    // add hi carry if there is room
    if(stop < len && carry != HI_N_BITS(asign, shiftRem)) {
        buffer[stop++] = (WordType)(asign << shiftRem) | carry;
    }
    // shift the remaining words from hi to lo
    for( ; i > shiftWords + 1; i--) {
        carry = HI_N_BITS(a[i - 2 - shiftWords], shiftRem);
        buffer[i - 1] = (WordType)(a[i - 1 - shiftWords] << shiftRem) | carry;
    }
    // shift the first word
    buffer[shiftWords] = (WordType)(a[0] << shiftRem);
    // fill lo words with 0
    memset(buffer, 0, shiftWords * sizeof(WordType));
    //fix potential overflow of the sign bit
    if(stop < len && (WordType)-HI_BIT(buffer[stop - 1]) != asign) {
        stop++;
    }
    return stop;
}

size_t yabi_lshiftToBuf(const BigInt* a, size_t amt, size_t len, WordType* buffer) {
    return lshiftBuffers(a->len, a->data, amt, len, buffer);
}

size_t rshiftBuffers(size_t alen, const WordType* a, size_t amt, size_t len, WordType* buffer, int useSign) {
    if(amt == 0) {
        //x >> 0 == x
        size_t stop = min(len, alen);
        unsigned char sign = useSign ? -HI_BIT(a[stop - 1]) : 0;
        memcpy(buffer, a, stop * sizeof(WordType));
        if(stop < len) {
            memset(buffer + stop, sign, (len - stop) * sizeof(WordType));
        }
        return stop;
    }
    //the number of words to shift right
    size_t shiftWords = (amt / YABI_WORD_BIT_SIZE);
    size_t stop = min(alen - shiftWords, len);
    WordType asign = useSign ? -HI_BIT(a[alen - 1]) : 0;
    //special case logic
    if((alen == 1 && a[0] == 0) || (shiftWords >= alen)) {
        //0 >> x == 0 and a >> huge == sign
        memset(buffer, asign, len * sizeof(WordType));
        return 1;
    }
    #define SHIFT_HI(word) (((word) & (((WordType)1 << shiftRem) - 1)) << (YABI_WORD_BIT_SIZE - shiftRem))
    //the remaining number of *bits* to shift right within a word
    WordType shiftRem = amt & (YABI_WORD_BIT_SIZE - 1);
    // lo carry out
    assert(shiftWords < alen);
    WordType carry;
    // shift data in lo to hi order
    for(size_t i = 0; i < stop - 1; i++) {
        carry = SHIFT_HI(a[i + 1 + shiftWords]);
        buffer[i] = (WordType)(a[i + shiftWords] >> shiftRem) | carry;
    }
    // shift hi word into place
    carry = SHIFT_HI(stop + shiftWords < alen ? a[stop + shiftWords] : asign);
    buffer[stop - 1] = (WordType)(a[stop - 1 + shiftWords] >> shiftRem) | carry;
    // fill sign extension
    memset(buffer + stop, asign, (len - stop) * sizeof(WordType));
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

size_t yabi_rshiftToBuf(const BigInt* a, size_t amt, size_t len, WordType* buffer) {
    return rshiftBuffers(a->len, a->data, amt, len, buffer, 1);
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
