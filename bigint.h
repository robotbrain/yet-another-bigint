#ifndef YET_ANOTHER_BIGINT_H
#define YET_ANOTHER_BIGINT_H
#include <stdint.h>
#include <stddef.h>

// redefine this macro to change the size of words
#define YABI_WORD_BIT_SIZE 8

#if YABI_WORD_BIT_SIZE == 8
    typedef uint8_t WordType;
    typedef int8_t SWordType;
#elif YABI_WORD_BIT_SIZE == 16
    typedef uint16_t WordType;
    typedef int16_t SWordType;
#elif YABI_WORD_BIT_SIZE == 32
    typedef uint32_t WordType;
    typedef int32_t SWordType;
#elif YABI_WORD_BIT_SIZE == 64
    typedef uint64_t WordType;
    typedef int64_t SWordType;
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

BigInt* yabi_lshift(const BigInt* a, size_t amt);
BigInt* yabi_rshift(const BigInt* a, size_t amt);

BigInt* yabi_and(const BigInt* a, const BigInt* b);
BigInt* yabi_or(const BigInt* a, const BigInt* b);
BigInt* yabi_xor(const BigInt* a, const BigInt* b);
BigInt* yabi_compl(const BigInt* a);

/*
 * The following functions let you provide your own buffer (that need not
 * even be a BigInt). These functions behave as if `a` and `b` were sign
 * extended or truncated to `len` words, and then the given operation applied
 * and the result stored in `buffer`. The minimum buffer length required to
 * represent the result, up to `len`, is returned. Note that the case of truncation
 * may result in two's complement integer overflow. Useful for fixed-length
 * arithmetic, but may be surprising. The parameter `buffer` in all of these
 * functions is explicitly allowed to refer to the same data as one or both of the
 * arguments, but it may not alias them in any other way (for example, by
 * an offset).
 */

size_t yabi_addToBuf(const BigInt* a, const BigInt* b, size_t len, WordType* buffer);
size_t yabi_subToBuf(const BigInt* a, const BigInt* b, size_t len, WordType* buffer);
size_t yabi_mulToBuf(const BigInt* a, const BigInt* b, size_t len, WordType* buffer);
size_t yabi_divToBuf(const BigInt* a, const BigInt* b, size_t len, WordType* buffer);
size_t yabi_negateToBuf(const BigInt* a, size_t len, WordType* buffer);

size_t yabi_lshiftToBuf(const BigInt* a, size_t amt, size_t len, WordType* buffer);
size_t yabi_rshiftToBuf(const BigInt* a, size_t amt, size_t len, WordType* buffer);

size_t yabi_andToBuf(const BigInt* a, const BigInt* b, size_t len, WordType* buffer);
size_t yabi_orToBuf(const BigInt* a, const BigInt* b, size_t len, WordType* buffer);
size_t yabi_xorToBuf(const BigInt* a, const BigInt* b, size_t len, WordType* buffer);
size_t yabi_complToBuf(const BigInt* a, size_t len, WordType* buffer);

WordType yabi_toUnsigned(const BigInt* a);
SWordType yabi_toSigned(const BigInt* a);
size_t yabi_toSize(const BigInt* a);

/**
 * Creates a BigInt from the number given in the string. The string must be
 * numeric (base 10), optionally starting with an ASCII minus sign.
 */
BigInt* yabi_fromStr(const char* str);

char* yabi_toStr(const BigInt* a);
size_t yabi_toBuf(const BigInt* a, size_t len, char* restrict buffer);

#endif
