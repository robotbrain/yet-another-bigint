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
    size_t i;
    //fill sign extension
    for(i = len; i > a->len - shiftWords; i--) {
        buffer[i - 1] = asign;
        stop--; //sign extension is not strictly required in repr
    }
    //fill shifted data from a->data
    for( ; i > 0; i--) {
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

size_t yabi_fromStrToBuf(const char* restrict str, size_t len, WordType* data) {
    const char* c = str;
    //get the sign
    int negative = 0;
    if(*c == '-') {
        c++;
        negative = 1;
    }
    size_t stop = len;
    memset(data, 0, len * sizeof(WordType));
    while(*c) {
        assert(*c >= '0' && *c <= '9');
        //calculate 10*x + digit
        //save the carry from the bits shifted out
        unsigned carry = HI_3_BITS(data[0]) + HI_BIT(data[0]);
        //10*x = 8*x + 2*x = (x << 3) + (x << 1)
        carry += addAndCarry(data[0] << 3, data[0] << 1, (*c - '0'), &data[0]);
        //calculate 10*x + carry for each higher word
        for(size_t i = 1; i < stop; i++) {
            unsigned newCarry = HI_3_BITS(data[i]) + HI_BIT(data[i]);
            newCarry += addAndCarry(data[i] << 3, data[i] << 1, carry, &data[i]);
            carry = newCarry;
        }
        c++;
    }
    //if negative, flip the bits and add one
    if(negative) {
        unsigned carry = 1;
        for(size_t i = 0; i < stop; i++) {
            data[i] = ~data[i];
            carry = addAndCarry(data[i], carry, 0, &data[i]);
        }
        //throw away last carry
    }
    //calculate true number of words. i.e. get rid of the extra sign extension.
    while(stop > 1 && data[stop - 1] == (WordType)-negative) {
        stop--;
    }
    //if the sign bit is not the same as `negative`, we need to add it back
    if(stop < len && HI_BIT(data[stop - 1]) != negative) {
        stop++;
    }
    return stop;
}

BigInt* yabi_fromStr(const char* str) {
    //overestimates required space
    //space = 1 + (log2(num) / log2(WordBase))
    #define TO_LOGB2(a) ((a) * 7 / 2)
    size_t cap = 1 + TO_LOGB2(strlen(str)) / YABI_WORD_BIT_SIZE;
    #undef TO_LOGB2
    BigInt* res = YABI_NEW_BIGINT(cap);
    res->refCount = 0;
    res->len = cap;
    cap = yabi_fromStrToBuf(str, cap, res->data);
    if(cap != res->len) {
        YABI_RESIZE_BIGINT(res, cap);
    }
    return res;
}

size_t yabi_toBuf(const BigInt* a, size_t len, char* restrict _buffer) {
    //nil buffer case
    if(len == 1) {
        *_buffer = '\0';
        return 0;
    }
    #define NTH_BIT(a, n) (((a) & ((WordType)1 << (n))) >> (n))
    const int negative = HI_BIT(a->data[a->len - 1]);
    unsigned char* buffer = (unsigned char*) _buffer; //unsigned for definedness :)
    //check for string length 1 with negative number
    if(negative) {
        buffer[0] = '-';
        buffer[1] = '\0';
        if(len == 2) {
            return 1;
        }
    } else {
        buffer[0] = 0;
    }
    size_t bufferLen = 1 + negative;
    //"double dabble" to convert binary to binary coded decimal
    //this takes time proportional to the number of bits in a->data
    for(size_t aWordIdx = a->len; aWordIdx > 0; aWordIdx--) {
        WordType word = a->data[aWordIdx - 1];
        if(negative) {
            word = ~word;
        }
        for(size_t aBitIdx = (sizeof(WordType) << 3); aBitIdx > 0; aBitIdx--) {
            //dabble
            for(size_t i = negative; i < bufferLen; i++) {
                unsigned char tmp = buffer[i];
                if(tmp >= 5) {
                    tmp += 3;
                }
                //overflow of nibble is not possible in double-dabble
                assert(tmp < 16);
                buffer[i] = tmp;
            }
            //double
            int carry = NTH_BIT(word, aBitIdx - 1);
            for(size_t i = negative; i < bufferLen; i++) {
                unsigned char tmp = buffer[i];
                assert(carry == 0 || carry == 1);
                tmp = (tmp << 1) | carry;
                carry = (tmp & 0x10) >> 4; //overflow bit (0b1_xxxx)
                tmp &= 0xf;
                buffer[i] = tmp;
            }
            if(carry && bufferLen < len - 1) {
                buffer[bufferLen++] = carry;
                buffer[bufferLen] = 0;
            }
        }
    }
    if(negative) {
        //add one (in base 10)
        int carry = 1;
        for(size_t i = 1; i < bufferLen; i++) {
            unsigned char tmp = buffer[i];
            tmp += carry;
            if(tmp > 9) {
                tmp -= 10;
                carry = 1;
            } else {
                carry = 0;
            }
            buffer[i] = tmp;
        }
        if(carry && bufferLen < len - 1) {
            buffer[bufferLen++] = carry;
        }
    }
    buffer[bufferLen] = '\0'; //NUL-terminate
    //put into ASCII digit range and in correct endian order
    for(size_t i = 0; i < (bufferLen - negative) / 2; i++) {
        unsigned char tmp = buffer[bufferLen - 1 - i];
        buffer[bufferLen - 1 - i] = buffer[negative + i] + '0';
        buffer[negative + i] = tmp + '0';
    }
    if((bufferLen - negative) & 1) {
        buffer[bufferLen / 2] += '0';
    }
    return bufferLen;
    #undef NTH_BIT
}

char* yabi_toStr(const BigInt* a) {
    //len = 1 + log10(a) = 1 + logWT(a) / logWT(10)
    //logWT(a) = a->len
    //logWT(10) ~= 3.32 / WORD_BIT_SIZE
    //len = 1 + a->len * (WORD_BIT_SIZE / 3.32)
    size_t len = 1 + a->len * ((YABI_WORD_BIT_SIZE + 1) / 3)
        + HI_BIT(a->data[a->len - 1]); //minus sign takes up one extra
    char* res = YABI_MALLOC(len + 1);
    size_t newLen = yabi_toBuf(a, len + 1, res);
    if(newLen != len) {
        res = YABI_REALLOC(res, newLen + 1);
    }
    return res;
}
