#include "bignum_test_framework.h"

/* Begin tests */
DEFINE_TEST(test_bignum_create) {
    bignum_t *a = bignum_new();
    TESTEQ(a, "0");
    bignum_free(a);
}
DEFINE_TEST(test_bignum_new_from_string) {
    bignum_t *a = bignum_new_from_string("18446744073709551616");
    TESTEQ(a, "18446744073709551616");
    bignum_free(a);
}
DEFINE_TEST(test_bignum_add_elem1) {
    bignum_t *a;
    a = bignum_new();
    bignum_add_elem(a, 0);
    TESTEQ(a, "0");
    bignum_add_elem(a, 1);
    TESTEQ(a, "1");
    bignum_free(a);
}
DEFINE_TEST(test_bignum_add_elem2) {
    bignum_t *a;
    a = bignum_new();
    bignum_add_elem(a, 18446744073709551615UL);
    TESTEQ(a, "18446744073709551615");
    bignum_add_elem(a, 1);
    TESTEQ(a, "18446744073709551616");
    bignum_free(a);
}
DEFINE_TEST(test_bignum_add_elem3) {
    bignum_t *a = bignum_new_from_string("0");
    TESTEQ(a, "0");
    for (size_t i = 0; i < 100; ++i) {
        bignum_add_elem(a, 7UL);
    }
    TESTEQ(a, "700");
    bignum_free(a);
}
DEFINE_TEST(test_bignum_add_elem4) {
    bignum_t *a = bignum_new_from_string("340282366920938463463374607431768211455"); // 2^128 - 1
    TESTEQ(a, "340282366920938463463374607431768211455");
    for (size_t i = 0; i < 100; ++i) {
        bignum_add_elem(a, 18446744073709551615UL);  // 2^64 - 1
    }
    TESTEQ(a, "340282366920938465308049014802723372955");
    bignum_free(a);
}
DEFINE_TEST(test_bignum_multiply_elem1) {
    bignum_t *a;
    a = bignum_new();
    bignum_multiply_elem(a, 2UL);
    TESTEQ(a, "0");
    bignum_free(a);
    a = bignum_new();
    bignum_add_elem(a, 1UL);
    bignum_multiply_elem(a, 1UL);
    TESTEQ(a, "1");
    bignum_multiply_elem(a, 1000UL);
    TESTEQ(a, "1000");
    bignum_multiply_elem(a, 0UL);
    TESTEQ(a, "0");
    bignum_free(a);
}
DEFINE_TEST(test_bignum_multiply_elem2) {
    bignum_t *a = bignum_new_from_string("1");
    bignum_elem_t max_unsigned_long_minus_one = 18446744073709551615UL; // 2^64 - 1
    bignum_multiply_elem(a, max_unsigned_long_minus_one);
    TESTEQ(a, "18446744073709551615");
    bignum_multiply_elem(a, max_unsigned_long_minus_one);
    TESTEQ(a, "340282366920938463426481119284349108225");
    bignum_free(a);
}
DEFINE_TEST(test_bignum_multiply_elem3) {
    bignum_t *a = bignum_new_from_string("18446744073709551616");  // 2^64
    TESTEQ(a, "18446744073709551616");
    bignum_elem_t max_unsigned_long_minus_one = 18446744073709551615UL; // 2^64 - 1
    bignum_multiply_elem(a, max_unsigned_long_minus_one);
    TESTEQ(a, "340282366920938463444927863358058659840");
    bignum_multiply_elem(a, max_unsigned_long_minus_one);
    TESTEQ(a, "6277101735386680763155224689365789489194052973674207641600");
    bignum_multiply_elem(a, 0L);
    TESTEQ(a, "0");
    assert(a->length == 1 && "length 1");
    assert(a->v[0] == 0 && "v[0] == 0");
    bignum_free(a);
}

DEFINE_TEST(test_bignum_add_inplace1) {
    bignum_t *a = bignum_new();
    bignum_t *b = bignum_new_from_string("0");
    bignum_add_inplace(a, b);
    TESTEQ(a, "0");
    TESTEQ(b, "0");
    bignum_free(b);
    b = bignum_new_from_string("2");
    bignum_add_inplace(a, b);
    TESTEQ(a, "2");
    TESTEQ(b, "2");
    bignum_add_inplace(a, b);
    TESTEQ(a, "4");
    TESTEQ(b, "2");
    bignum_free(a);
    bignum_free(b);
}

DEFINE_TEST(test_bignum_add_inplace2) {
    bignum_t *a = bignum_new();
    bignum_t *b = bignum_new_from_string("9223372036854775808");  // 2^63
    bignum_add_inplace(a, b);
    TESTEQ(a, "9223372036854775808");
    bignum_add_inplace(a, b);
    TESTEQ(a, "18446744073709551616");
    bignum_add_inplace(a, b);
    TESTEQ(a, "27670116110564327424");
    bignum_free(a);
    bignum_free(b);
}


void print_and_free(char *message) {
    if (message != NULL) {
        printf("%s\n", message);
        free(message);
    }
}

int test_bignum_main() {
    return btl_run_tests();
}
