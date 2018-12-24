#include "bigint_internal.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

// calculates the largest multiple of b that fits into a, returns the quotient
// and subtracts the appropriate amount from the remainder.
static WordType simpleDiv(size_t rlen, WordType* rbuf, size_t blen, const WordType* bbuf) {
    WordType q = 0;
    size_t shf = rlen;
    while(rbuf[shf - 1] == 0) {
        shf--;
    }
    shf *= YABI_WORD_BIT_SIZE;
    while(cmpBuffers(rlen, rbuf, blen, bbuf, 0) >= 0) {
        size_t shfWords, shfAmt;
        WordType holder;
        // calculate the maximum power of two we can divide by
        while(1) {
            // number of words to shift right
            shfWords = shf / YABI_WORD_BIT_SIZE;
            shfAmt = shf % YABI_WORD_BIT_SIZE;
            while(shfWords >= rlen) {
                shfWords--;
            }
            // temporary variable for shifted out bits
            holder = rbuf[shfWords] & (((WordType)1 << shfAmt) - 1);
            // can we divide by this number
            rshiftBuffers(rlen - shfWords, rbuf + shfWords, shfAmt, rlen - shfWords, rbuf + shfWords, 0);
            if(cmpBuffers(rlen - shfWords, rbuf + shfWords, blen, bbuf, 0) >= 0) {
                break;
            }
            // no, reset and try the next power of two
            assert(shfWords != 0 || shfAmt != 0);
            lshiftBuffers(rlen - shfWords, rbuf + shfWords, shfAmt, rlen - shfWords, rbuf + shfWords);
            rbuf[shfWords] |= holder;
            shf = shfWords * YABI_WORD_BIT_SIZE + shfAmt - 1;
        }
        // calculate (2^shf * (r / 2^shf - d))
        // rbuf[shfWords:] has already been divided out
        addBuffers(rlen - shfWords, rbuf + shfWords, blen, bbuf, 1, rlen - shfWords, rbuf + shfWords);
        lshiftBuffers(rlen - shfWords, rbuf + shfWords, shfAmt, rlen - shfWords, rbuf + shfWords);
        rbuf[shfWords] |= holder;
        q += (WordType)1 << shf;
        shf--;
    }
    return q;
}

/**
 * Divides two unsigned BigInts. b != 0.
 */
static ydiv_t divUnsigned(const BigInt* a, const BigInt* b, size_t qlen, WordType* qbuffer, size_t rlen, WordType* rbuffer) {
    // long division
    memset(qbuffer, 0, qlen * sizeof(WordType));
    memset(rbuffer, 0, rlen * sizeof(WordType));
    for(size_t i = a->data[a->len - 1] ? a->len : a->len - 1; i > 0; i--) {
        // shift r up by 1 word
        memmove(rbuffer + 1, rbuffer, (rlen - 1) * sizeof(WordType));
        // set LSW to current word of a
        rbuffer[0] = a->data[i - 1];
        // set next digit of q to r / d and subtract qd from r
        // if r < d, sets the current word of q to 0 and leaves r unchanged
        WordType w = simpleDiv(rlen, rbuffer, b->len, b->data);
        if(i <= qlen) {
            qbuffer[i - 1] = w;
        }
    }
    ydiv_t res;
    // find quotient and remainder length
    res.qlen = qlen;
    res.rlen = rlen;
    while(res.qlen > 1 && qbuffer[res.qlen - 1] == 0) {
        res.qlen--;
    }
    while(res.rlen > 1 && rbuffer[res.rlen - 1] == 0) {
        res.rlen--;
    }
    return res;
}

static void negateInPlace(size_t len, WordType* buf) {
    // quick negation algorithm:
    // find the lowest 1 bit. Flip all the bits
    // above the lowest 1 bit.
    // e.g. -0b110100 = 0b001100
    //      -   (-12) =       12
    size_t i;
    for(i = 0; i < (len - 1) && buf[i] == 0; i++);
    buf[i] = ~buf[i] + 1;
    for(i++; i < len; i++) {
        buf[i] = ~buf[i];
    }
}

ydiv_t yabi_divToBuf(const BigInt* a, const BigInt* b, size_t qlen, WordType* qbuffer, size_t rlen, WordType* rbuffer) {
    // cannot divide by zero
    if(b->len == 1 && b->data[0] == 0) {
        return (ydiv_t) {
            .qlen = 0,
            .rlen = 0
        };
    }
    const BigInt* num;
    const BigInt* denom;
    size_t rrlen;
    WordType* rbuf;
    int qnegative = 0;
    int rnegative = 0;
    // change negative numbers to positive
    if(HI_BIT(a->data[a->len - 1])) {
        num = yabi_negate(a);
        qnegative ^= 1;
        rnegative = 1;
    } else {
        num = a;
    }
    if(HI_BIT(b->data[b->len - 1])) {
        denom = yabi_negate(b);
        qnegative ^= 1;
    } else {
        denom = b;
    }
    // make sure the remainder can hold partial results
    // (rlen >= b->len)
    if(rlen < b->len) {
        rrlen = b->len;
        rbuf = YABI_MALLOC(rrlen * sizeof(WordType));
    } else {
        rrlen = rlen;
        rbuf = rbuffer;
    }
    // quotient does not have to be expanded in long division
    // ;
    // do unsigned division
    ydiv_t result = divUnsigned(num, denom, qlen, qbuffer, rrlen, rbuf);
    // negate the quotient if either operand was negative
    if(qnegative) {
        negateInPlace(qlen, qbuffer);
    }
    // negate the remainder if the numerator was negative
    if(rnegative) {
        negateInPlace(rrlen, rbuf);
    }
    // copy and free dynamic storage
    if(num != a) {
        YABI_FREE((void*)num);
    }
    if(denom != b) {
        YABI_FREE((void*)denom);
    }
    if(rbuf != rbuffer) {
        memcpy(rbuffer, rbuf, rlen * sizeof(WordType));
        YABI_FREE(rbuf);
    }
    // return
    return result;
}
