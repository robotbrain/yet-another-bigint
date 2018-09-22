#ifndef YET_ANOTHER_BIGINT_H
#define YET_ANOTHER_BIGINT_H
#include <stdint.h>
#include <stddef.h>

#ifndef YABI_WORD_BIT_SIZE
#define YABI_WORD_BIT_SIZE 8
#endif

#if YABI_WORD_BIT_SIZE == 8
    typedef uint8_t WordType;
#elif YABI_WORD_BIT_SIZE == 16
    typedef uint16_t WordType;
#elif YABI_WORD_BIT_SIZE == 32
    typedef uint32_t WordType;
#elif YABI_WORD_BIT_SIZE == 64
    typedef uint64_t WordType;
#else
    #error WORD_BIT_SIZE must be one of: 8, 16, 32, 64
#endif

typedef struct BigInt {
   size_t refCount;
   size_t len;
   WordType data[];
} BigInt;

BigInt* yabi_add(BigInt* a, BigInt* b);
BigInt* yabi_sub(BigInt* a, BigInt* b);
BigInt* yabi_mul(BigInt* a, BigInt* b);
BigInt* yabi_div(BigInt* a, BigInt* b);
BigInt* yabi_rem(BigInt* a, BigInt* b);

BigInt* yabi_negate(BigInt* a);

int yabi_equal(BigInt* a, BigInt* b);
int yabi_cmp(BigInt* a, BigInt* b);

BigInt* yabi_lshift(BigInt* a, WordType amt);
BigInt* yabi_rshift(BigInt* a, WordType amt);
BigInt* yabi_rshiftl(BigInt* a, WordType amt);

BigInt* yabi_and(BigInt* a, BigInt* b);
BigInt* yabi_or(BigInt* a, BigInt* b);
BigInt* yabi_xor(BigInt* a, BigInt* b);
BigInt* yabi_compl(BigInt* a);

/**
 * Creates a BigInt from the number given in the string. The string must be
 * numeric (base 10), optionally starting with an ASCII minus sign.
 */
BigInt* yabi_fromStr(char* str);

char* yabi_toStr(BigInt* a);
size_t yabi_toBuf(BigInt* a, size_t len, char* restrict buffer);

#endif
