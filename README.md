# Yet Another BigInt
Yet Another BigInt is a vector-based BigInteger implementation in C. The first goal is to support basic mathematical operations.

### Why Another BigInt?
A quick trawl through GitHub reveals a surprising lack of pure C BigInt implementations that have the qualities described below. This project aims to fix that.
* Yet Another BigInt uses C99 flexible array members to allocate a contiguous chunk of memory for the entire value.
* Yet Another BigInt represents its big integers as numbers in signed two's complement, unlike other libraries which use base 10 strings or sign-magnitude.
* Yet Another BigInt allocates only as much memory as is necessary to store a given value.

### Considerations
Some things to consider before using Yet Another BigInt
* Yet Another BigInt relies on dynamic allocations for storing big integer values.
* Because BigInt values contain flexible arrays, they must always be accessed through pointers and may never be allocated on the stack.
