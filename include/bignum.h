#include <stdio.h>
#include <stdlib.h>

#define PRINTERR(msg) fprintf(stderr, "%s\n", msg)
#define BIGNUM_MAX_LENGTH 8  // 512 bit / sizeof(bignum_elem_t)
#define BIGNUM_ELEM_TYPE unsigned long
#define BIGNUM_ELEM_SIZE sizeof(BIGNUM_ELEM_TYPE) // 8 for unsigned long
#define BIGNUM_ELEM_MAX ((BIGNUM_ELEM_TYPE) 0 - 1)
/** @brief Bitmask for the lower half of a bignum_elem_t */
#define BIGNUM_ELEM_LO (BIGNUM_ELEM_MAX >> BIGNUM_ELEM_SIZE*4)

/** @brief Bitmask for the upper half of a bignum_elem_t */
#define BIGNUM_ELEM_HI (BIGNUM_ELEM_MAX << BIGNUM_ELEM_SIZE*4) & BIGNUM_ELEM_MAX

typedef unsigned long bignum_elem_t;

typedef struct bignum {
    size_t length;
    size_t max_length;
    bignum_elem_t *v;
} bignum_t;

bignum_t *bignum_new();
void bignum_free(bignum_t *num);
void bignum_zero(bignum_t *num);
void bignum_print_internals(bignum_t *num);
int bignum_add_inplace(bignum_t *a, const bignum_t *b);
void bignum_add_elem(bignum_t *a, bignum_elem_t val);
int bignum_multiply_elem(bignum_t *op1, bignum_elem_t op2);
int bignum_multiply_inplace(bignum_t *a, const bignum_t *b);
void bignum_to_int_array(const bignum_t *num, int *result, size_t *result_size);
void bignum_from_string(const char *base_10_digits, bignum_t *result);
char *bignum_to_string(const bignum_t *num);
bignum_t *bignum_new_from_string(char *base_10_digits);
