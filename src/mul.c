#include "bigint_internal.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// naive two's complement multiplication for buffers
size_t mulBuffers(
        size_t alen, const WordType* adata,
        size_t blen, const WordType* bdata,
        size_t len, WordType* buffer) {
    WordType asign = -HI_BIT(adata[alen - 1]);
    WordType bsign = -HI_BIT(bdata[blen - 1]);
    size_t stop = max(alen, blen) * 2;
    stop = min(stop, len);
    //we multiply with the following method based on the grade-school
    //multiplication method. This allows `buffer` to alias `adata` or `bdata`
    // for idx in [len..1]:
    //    WordType tmp[2] = 0
    //    for j in [0..idx]:
    //        buffer[idx-1:] += a[j] * b[idx - 1 - j]
    for(size_t idx = stop - 1; idx > 0; idx--) {
        {
            //calculate with the data at `idx - 1` before we
            //overwrite it
            WordType scratch[3] = { 0, 0, 0 };
            WordType scratch2[3] = { 0, 0, 0 };
            WordType awordAtIdx = idx < alen ? adata[idx] : asign;
            WordType bwordAtIdx = idx < blen ? bdata[idx] : bsign;
            scratch[1] = mulAndCarry(awordAtIdx, bdata[0], &scratch[0]);
            scratch2[1] = mulAndCarry(bwordAtIdx, adata[0], &scratch[0]);
            buffer[idx] = 0;
            addBuffers(len - idx, buffer + idx,
                3, scratch,
                0,
                len - idx, buffer + idx);
            addBuffers(len - idx, buffer + idx,
                3, scratch2,
                0,
                len - idx, buffer + idx);
        }
        for(size_t j = 1; j < idx; j++) {
            //we use the FOIL method for half-words so as not to
            //lose overflow bits.
            // (ahi + alo)(bhi + blo)
            // = ahi*bhi + ahi*blo + alo*bhi + alo*blo
            WordType scratch[3] = { 0, 0, 0 };
            WordType aword = j < alen ? adata[j] : asign;
            WordType bword = (idx - j) < blen ? bdata[idx - j] : bsign;
            scratch[1] = mulAndCarry(aword, bword, &scratch[0]);
            addBuffers(len - idx, buffer + idx,
                3, scratch,
                0,
                len - idx, buffer + idx);
        }
    }
    {
        //calculate buffer[0]
        WordType scratch[3] = { 0, 0, 0 };
        scratch[1] = mulAndCarry(adata[0], bdata[0], &scratch[0]);
        buffer[0] = 0;
        addBuffers(len, buffer,
            3, scratch,
            0,
            len, buffer);
    }
    WordType sign = -HI_BIT(buffer[stop - 1]);
    memset(buffer + stop, sign, (len - stop) * sizeof(WordType));
    // shrink away extra sign words
    while(stop > 1 && buffer[stop - 1] == sign) {
        stop--;
    }
    if(HI_BIT(buffer[stop - 1]) != (sign & 1)) {
        stop++;
    }
    assert(stop <= len);
    return stop;
}

size_t yabi_mulToBuf(const BigInt* a, const BigInt* b, size_t len, WordType* buffer) {
    return mulBuffers(a->len, a->data, b->len, b->data, len, buffer);
}

BigInt* yabi_mul(const BigInt* a, const BigInt* b) {
    size_t len = a->len + b->len;
    BigInt* res = YABI_NEW_BIGINT(len);
    res->refCount = 0;
    res->len = len;
    len = mulBuffers(a->len, a->data, b->len, b->data, len, res->data);
    if(len != res->len) {
        YABI_RESIZE_BIGINT(res, len);
    }
    return res;
}
