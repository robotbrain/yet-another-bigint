#include "bigint.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

// test main file

#if YABI_WORD_BIT_SIZE == 8
    #define PRIxWT "%02"PRIx8
#elif YABI_WORD_BIT_SIZE == 16
    #define PRIxWT "%04"PRIx16
#elif YABI_WORD_BIT_SIZE == 32
    #define PRIxWT "%08"PRIx32
#elif YABI_WORD_BIT_SIZE == 64
    #define PRIxWT "%016"PRIx64
#endif

static void printHex(char* prefix, BigInt* a) {
    size_t idx = a->len;
    printf("%s: ", prefix);
    while(idx != 0) {
        WordType tmp = a->data[idx - 1];
        printf(PRIxWT" ", tmp);
        idx--;
    }
    printf("\n");
}

// for testing
int main(int argc, char* argv[]) {
    if(argc < 3) {
        puts("Enter two integer arguments");
        return 1;
    }
    char* digits = argv[1];
    char* digits2 = argv[2];
    BigInt* a = yabi_fromStr(digits);
    BigInt* b = yabi_fromStr(digits2);
    BigInt* c;
    WordType word;
    printHex("a", a);
    printHex("b", b);

    c = yabi_add(a, b);
    printHex("a+b", c);
    free(c);
    yabi_addToBuf(a, b, 1, &word);
    printf("[a+b]: "PRIxWT"\n", word);

    c = yabi_negate(a);
    printHex("-a", c);
    free(c);
    yabi_negateToBuf(a, 1, &word);
    printf("[-a]: "PRIxWT"\n", word);

    c = yabi_compl(a);
    printHex("~a", c);
    free(c);
    yabi_complToBuf(a, 1, &word);
    printf("[~a]: "PRIxWT"\n", word);

    c = yabi_and(a, b);
    printHex("a&b", c);
    free(c);
    yabi_andToBuf(a, b, 1, &word);
    printf("[a&b]: "PRIxWT"\n", word);

    c = yabi_or(a, b);
    printHex("a|b", c);
    free(c);
    yabi_orToBuf(a, b, 1, &word);
    printf("[a|b]: "PRIxWT"\n", word);

    c = yabi_xor(a, b);
    printHex("a^b", c);
    free(c);
    yabi_xorToBuf(a, b, 1, &word);
    printf("[a^b]: "PRIxWT"\n", word);

    printf("toSize(b): %lu\n", yabi_toSize(b));

    c = yabi_lshift(a, yabi_toSize(b));
    printHex("a<<b", c);
    free(c);
    yabi_lshiftToBuf(a, yabi_toSize(b), 1, &word);
    printf("[a<<b]: "PRIxWT"\n", word);

    free(b);
    free(a);
    return 0;
}
