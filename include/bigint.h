#ifndef YET_ANOTHER_BIGINT_H
#define YET_ANOTHER_BIGINT_H
#include <stdint.h>
#include <stddef.h>

// redefine this macro to change the size of words
#ifndef YABI_WORD_BIT_SIZE
#define YABI_WORD_BIT_SIZE 8
#endif

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
#ifndef YABI_MALLOC
#define YABI_MALLOC(siz) (malloc(siz))
#endif
#ifndef YABI_CALLOC
#define YABI_CALLOC(n, siz) (calloc(n, siz))
#endif
#ifndef YABI_REALLOC
#define YABI_REALLOC(p, siz) (realloc(p, siz))
#endif
#ifndef YABI_FREE
#define YABI_FREE(p) (free(p))
#endif
#ifndef YABI_NEW_BIGINT
#define YABI_NEW_BIGINT(siz) (YABI_MALLOC(sizeof(BigInt) + (siz) * sizeof(WordType)))
#endif
#ifndef YABI_RESIZE_BIGINT
#define YABI_RESIZE_BIGINT(p, siz) do { (p) = YABI_REALLOC(p, sizeof(BigInt) + (siz) * sizeof(WordType)); (p)->len = (siz); } while(0)
#endif

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
 * even be a BigInt). These functions behave as if the given operation
 * is applied using arbitrary precision, and then the result stored in `buffer`,
 * possibly truncated. This behavior only matters for right shift and division,
 * where it ensures that they produce the proper result when the answer can be
 * expressed in fewer words than the arguments. The minimum buffer length required to
 * represent the result, up to `len`, is returned. Note that the case of truncation
 * may result in two's complement integer overflow. Useful for fixed-length
 * arithmetic, but may be surprising. The parameter `buffer` in all of these
 * functions is explicitly allowed to refer to the same data as one or both of the
 * arguments, but it may not alias them in any other way (for example, by
 * an offset).
 */

typedef struct ydiv {
    size_t qlen;
    size_t rlen;
} ydiv_t;

size_t yabi_addToBuf(const BigInt* a, const BigInt* b, size_t len, WordType* buffer);
size_t yabi_subToBuf(const BigInt* a, const BigInt* b, size_t len, WordType* buffer);
size_t yabi_mulToBuf(const BigInt* a, const BigInt* b, size_t len, WordType* buffer);
ydiv_t yabi_divToBuf(const BigInt* a, const BigInt* b, size_t qlen, WordType* qbuffer, size_t rlen, WordType* rbuffer);
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
size_t yabi_fromStrToBuf(const char* restrict str, size_t len, WordType* buffer);

char* yabi_toStr(const BigInt* a);
size_t yabi_toBuf(const BigInt* a, size_t len, char* restrict buffer);

#endif
