#include "bigint_internal.h"
#include <string.h>
#include <assert.h>

// calculates the largest multiple of b that fits into a, returns the quotient
// and subtracts the appropriate amount from the remainder.
// TODO: make faster cause this is slow as a snail!
static WordType simpleDiv(size_t rlen, WordType* rbuf, size_t blen, const WordType* bbuf) {
    WordType q = 0;
    size_t shf = rlen * YABI_WORD_BIT_SIZE;
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

ydiv_t yabi_divToBuf(const BigInt* a, const BigInt* b, size_t qlen, WordType* qbuffer, size_t rlen, WordType* rbuffer) {
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
