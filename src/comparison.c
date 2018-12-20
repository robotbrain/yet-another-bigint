#include "bigint_internal.h"

int yabi_equal(const BigInt* a, const BigInt* b) {
    return eqBuffers(a->len, a->data, b->len, b->data);
}

int yabi_cmp(const BigInt* a, const BigInt* b) {
    return cmpBuffers(a->len, a->data, b->len, b->data, 1);
}
