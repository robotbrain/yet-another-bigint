#ifndef YET_ANOTHER_BIGINT_H
#define YET_ANOTHER_BIGINT_H
#include <stdint.h>
#include <stddef.h>

// redefine this macro to change the size of words
#define YABI_WORD_BIT_SIZE 8

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

// override these with calls to your memory management scheme
#define YABI_MALLOC(siz) (malloc(siz))
#define YABI_CALLOC(n, siz) (calloc(n, siz))
#define YABI_REALLOC(p, siz) (realloc(p, siz))
#define YABI_FREE(p) (free(p))
#define YABI_NEW_BIGINT(len) (YABI_MALLOC(sizeof(BigInt) + len * sizeof(WordType)))
#define YABI_RESIZE_BIGINT(p, len) do { p = YABI_REALLOC(p, sizeof(BigInt) + len * sizeof(WordType)); (p)->len = len; } while(0)

typedef struct BigInt {
   size_t refCount;
   size_t len;
   WordType data[];
} BigInt;

BigInt* yabi_add(const BigInt* a, const BigInt* b);
BigInt* yabi_sub(const BigInt* a, const BigInt* b);
BigInt* yabi_mul(const BigInt* a, const BigInt* b);
BigInt* yabi_div(const BigInt* a, const BigInt* b);
BigInt* yabi_rem(const BigInt* a, const BigInt* b);
BigInt* yabi_negate(const BigInt* a);

int yabi_equal(const BigInt* a, const BigInt* b);
int yabi_cmp(const BigInt* a, const BigInt* b);

BigInt* yabi_lshift(const BigInt* a, WordType amt);
BigInt* yabi_rshift(const BigInt* a, WordType amt);

BigInt* yabi_and(const BigInt* a, const BigInt* b);
BigInt* yabi_or(const BigInt* a, const BigInt* b);
BigInt* yabi_xor(const BigInt* a, const BigInt* b);
BigInt* yabi_compl(const BigInt* a);

/*
 * The following functions let you provide your own buffer (that need not
 * even be a BigInt). These functions return the number of words the
 * result takes up in the buffer. The remaining words in the buffer are
 * sign-extended. In the case of overflow, the result is silently truncated
 * and the buffer length is returned. Useful for fixed-length arithmetic,
 * but may be surprising.
 */

size_t yabi_addToBuf(const BigInt* a, const BigInt* b, size_t len, WordType* buffer);
size_t yabi_subToBuf(const BigInt* a, const BigInt* b, size_t len, WordType* buffer);
size_t yabi_mulToBuf(const BigInt* a, const BigInt* b, size_t len, WordType* buffer);
size_t yabi_divToBuf(const BigInt* a, const BigInt* b, size_t len, WordType* buffer);
size_t yabi_negateToBuf(const BigInt* a, size_t len, WordType* buffer);

size_t yabi_lshiftToBuf(const BigInt* a, WordType amt, size_t len, WordType* buffer);
size_t yabi_rshiftToBuf(const BigInt* a, WordType amt, size_t len, WordType* buffer);

size_t yabi_andToBuf(const BigInt* a, const BigInt* b, size_t len, WordType* buffer);
size_t yabi_orToBuf(const BigInt* a, const BigInt* b, size_t len, WordType* buffer);
size_t yabi_xorToBuf(const BigInt* a, const BigInt* b, size_t len, WordType* buffer);
size_t yabi_complToBuf(const BigInt* a, size_t len, WordType* buffer);

/**
 * Creates a BigInt from the number given in the string. The string must be
 * numeric (base 10), optionally starting with an ASCII minus sign.
 */
BigInt* yabi_fromStr(const char* str);

char* yabi_toStr(const BigInt* a);
size_t yabi_toBuf(const BigInt* a, size_t len, char* restrict buffer);

#endif
