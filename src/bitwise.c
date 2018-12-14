#include "bigint_internal.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
    for(size_t i = 0; i < stop; i++) {
        buffer[i] = ~a->data[i];
    }
    //sign extend the buffer
    int signb = HI_BIT(buffer[stop - 1]);
    memset(buffer + stop, (WordType)-signb, (len - stop) * sizeof(WordType));
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
