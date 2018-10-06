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
    BigInt* c;          //dyn-allocated result
    WordType word;      //truncated buffer
    WordType buf[4];    //overlong buffer
    char charBuf[10];  //character buffer
    printHex("a", a);
    printHex("b", b);
    yabi_toBuf(a, 10, charBuf);
    char* s1 = yabi_toStr(a);
    printf("str(a): %s\n", s1);
    free(s1); s1 = NULL;
    printf("[str(a)]: %s\n", charBuf);
    //print truncated buffer
    #define PTB(desc) printf(desc": "PRIxWT"\n", word)
    //print overlong buffer
    #define POB(desc) printf(desc": "PRIxWT" "PRIxWT" "PRIxWT" "PRIxWT"\n", buf[3], buf[2], buf[1], buf[0])

    //addition
    c = yabi_add(a, b);
    printHex("a+b", c);
    free(c);
    yabi_addToBuf(a, b, 1, &word);
    yabi_addToBuf(a, b, 4, buf);
    PTB("[a+b]");
    POB("{a+b}");

    //negation
    c = yabi_negate(a);
    printHex("-a", c);
    free(c);
    yabi_negateToBuf(a, 1, &word);
    yabi_negateToBuf(a, 4, buf);
    PTB("[-a]");
    POB("{-a}");

    //complement
    c = yabi_compl(a);
    printHex("~a", c);
    free(c);
    yabi_complToBuf(a, 1, &word);
    yabi_complToBuf(a, 4, buf);
    PTB("[~a]");
    POB("{~a}");

    //bitwise and
    c = yabi_and(a, b);
    printHex("a&b", c);
    free(c);
    yabi_andToBuf(a, b, 1, &word);
    yabi_andToBuf(a, b, 4, buf);
    PTB("[a&b]");
    POB("{a&b}");

    //bitwise or
    c = yabi_or(a, b);
    printHex("a|b", c);
    free(c);
    yabi_orToBuf(a, b, 1, &word);
    yabi_orToBuf(a, b, 4, buf);
    PTB("[a|b]");
    POB("{a|b}");

    //bitwise xor
    c = yabi_xor(a, b);
    printHex("a^b", c);
    free(c);
    yabi_xorToBuf(a, b, 1, &word);
    yabi_xorToBuf(a, b, 4, buf);
    PTB("[a^b]");
    POB("{a^b}");

    size_t s = yabi_toSize(b);
    printf("toSize(b): %lu\n", s);

    //left shift
    // c = yabi_lshift(a, s);
    // printHex("a<<b", c);
    // free(c);
    // yabi_lshiftToBuf(a, s, 1, &word);
    // yabi_lshiftToBuf(a, s, 4, buf);
    // PTB("[a<<b]");
    // POB("{a<<b}");
    //
    // //left shift
    // c = yabi_rshift(a, s);
    // printHex("a>>b", c);
    // free(c);
    // yabi_rshiftToBuf(a, s, 1, &word);
    // yabi_rshiftToBuf(a, s, 4, buf);
    // PTB("[a>>b]");
    // POB("{a>>b}");

    yabi_mulToBuf(a, b, 4, buf);
    POB("{a*b}");

    free(b);
    free(a);
    return 0;
}
