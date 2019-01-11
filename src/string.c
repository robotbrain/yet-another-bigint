#include "bigint_internal.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
    while(*c >= '0' && *c <= '9') {
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
