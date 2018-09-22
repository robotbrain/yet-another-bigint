#include "bigint.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

// get the most significant bit (sign bit)
#define HI_BIT(n) (n >> ((sizeof(WordType) << 3) - 1))
// get the three most significant bits (for carry)
#define HI_3_BITS(n) (n >> ((sizeof(WordType) << 3) - 3))

/** Adds three unsigned ints and returns the carry out. */
static int addAndCarry(WordType a, WordType b, WordType c, WordType* d) {
    WordType ab = a + b;
    WordType abc = ab + c;
    *d = abc;
    return (ab < a) + (abc < ab);
}

BigInt* yabi_compl(BigInt* a) {
    BigInt* res = malloc(sizeof(BigInt) + a->len * sizeof(WordType));
    res->refCount = 0;
    res->len = a->len;
    for(size_t i = 0; i < a->len; i++) {
        res->data[i] = ~a->data[i];
    }
    return res;
}

BigInt* yabi_negate(BigInt* a) {
    BigInt* res = malloc(sizeof(BigInt) + a->len * sizeof(WordType));
    res->refCount = 0;
    res->len = a->len;
    unsigned carry = 1;
    for(size_t i = 0; i < res->len; i++) {
        res->data[i] = ~a->data[i];
        carry = addAndCarry(res->data[i], carry, 0, &res->data[i]);
    }
    //if the sign bit overflowed we need to add one more
    if(HI_BIT(res->data[res->len - 1]) == HI_BIT(a->data[a->len - 1])) {
        // -0 is still 0
        if(res->len != 1 || res->data[0] != 0) {
            res = realloc(res, sizeof(BigInt) + ++res->len * sizeof(WordType));
            res->data[res->len - 1] = carry;
        }
    } else if(res->data[res->len - 1] == (WordType)-HI_BIT(res->data[res->len - 1])) {
        //check if we have an extra sign extension
        if(HI_BIT(res->data[res->len - 2]) != HI_BIT(a->data[a->len - 1])) {
            res = realloc(res, sizeof(BigInt) + --res->len * sizeof(WordType));
        }
    }
    return res;
}

static BigInt* add(BigInt* a, BigInt* b, int negateb) {
    //the result may be one word longer if a + b would otherwise overflow
    size_t len  = max(a->len, b->len);
    BigInt* res = malloc(sizeof(BigInt) + len * sizeof(WordType));
    //add word by word, tracking the carry
    size_t upTo = min(a->len, b->len);
    int signa = HI_BIT(a->data[a->len - 1]);
    int signb = HI_BIT(b->data[b->len - 1]);
    unsigned carry;
    size_t i;
    int overflowed;
    if(negateb) {
        //add twos complement negation of b
        carry = 1;
        for(i = 0; i < upTo; i++) {
            carry = addAndCarry(a->data[i], ~b->data[i], carry, &res->data[i]);
        }
        //add leftovers from a
        while(i < a->len) {
            //sign extend b's representation with b's inverted sign
            carry = addAndCarry(a->data[i], (WordType)-!signb, carry, &res->data[i]);
            i++;
        }
        //add leftovers from b
        while(i < b->len) {
            carry = addAndCarry((WordType)-signa, ~b->data[i], carry, &res->data[i]);
            i++;
        }
        //overflow would occur iff a and b have the same sign, AND the sign
        //of the result is different from the sign of a and b. Except b
        //is negated, so we flip the sign of b.
        overflowed =
            HI_BIT(a->data[a->len - 1]) != HI_BIT(b->data[b->len - 1])
         && HI_BIT(a->data[a->len - 1]) != HI_BIT(res->data[len - 1]);
    } else {
        carry = 0;
        for(i = 0; i < upTo; i++) {
            carry = addAndCarry(a->data[i], b->data[i], carry, &res->data[i]);
        }
        //add leftovers from a
        while(i < a->len) {
            //sign extend b's representation with b's sign
            carry = addAndCarry(a->data[i], (WordType)-signb, carry, &res->data[i]);
            i++;
        }
        //add leftovers from b
        while(i < b->len) {
            carry = addAndCarry((WordType)-signa, b->data[i], carry, &res->data[i]);
            i++;
        }
        //overflow would occur iff a and b have the same sign, AND the sign
        //of the result is different from the sign of a and b.
        overflowed =
            HI_BIT(a->data[a->len - 1]) == HI_BIT(b->data[b->len - 1])
         && HI_BIT(a->data[a->len - 1]) != HI_BIT(res->data[len - 1]);
    }
    if(overflowed) {
        //would have overflowed, we must allocate one extra space
        //we only add the last carry if we would have overflowed
        res = realloc(res, sizeof(BigInt) + ++len * sizeof(WordType));
        res->data[i] = carry;
    } else {
        //calculate true number of words. i.e. get rid of the extra sign extension.
        int negative = HI_BIT(res->data[len - 1]);
        while(len > 1 && res->data[len - 1] == (WordType)-negative) {
            len--;
        }
        //if the sign bit is not the same as `negative`, we need to add it back
        if(HI_BIT(res->data[len - 1]) != negative) {
            res->data[len++] = (WordType)-negative;
        }
        res = realloc(res, sizeof(BigInt) + len * sizeof(WordType));
    }
    res->refCount = 0;
    res->len = len;
    return res;
}

BigInt* yabi_add(BigInt* a, BigInt* b) {
    return add(a, b, 0);
}

BigInt* yabi_sub(BigInt* a, BigInt* b) {
    return add(a, b, 1);
}

BigInt* yabi_fromStr(char* str) {
    //overestimates required space
    //space = 1 + (log2(num) / log2(WordBase))
    #define TO_LOGB2(a) ((a) * 7 / 2)
    size_t cap = 1 + TO_LOGB2(strlen(str)) / (sizeof(WordType) << 3);
    #undef TO_LOGB2
    WordType* data = calloc(cap, sizeof(WordType));
    char* c = str;
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
    BigInt* res = malloc(sizeof(BigInt) + len * sizeof(WordType));
    res->refCount = 0;
    res->len = len;
    memcpy(res->data, data, len * sizeof(WordType));
    free(data);
    return res;
}
