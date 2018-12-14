#include "bigint_internal.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// implements two's complement addition on two buffers
size_t addBuffers(
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
    memset(buffer + i, (WordType)-signc, (len - i) * sizeof(WordType));
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
