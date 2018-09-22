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
    if(argc < 4) {
        return 1;
    }
    char* digits = argv[1];
    char* digits2 = argv[2];
    int subtract = *argv[3] == '1';
    BigInt* a = yabi_fromStr(digits);
    BigInt* b = yabi_fromStr(digits2);
    printHex("a", a);
    printHex("b", b);
    BigInt* c = subtract ? yabi_sub(a, b) : yabi_add(a, b);
    printHex("a+b", c);
    free(c);
    c = yabi_negate(a);
    printHex("-a", c);
    free(c);
    c = yabi_compl(a);
    printHex("~a", c);
    free(c);
    free(b);
    free(a);
    return 0;
}
