#include <stdio.h>
#include "tfm_opencl.h"
#include "bignum_test_framework.h"

DEFINE_TEST(tfm_mul_2d_1) {
    fp_int a, b, c, expected;
    fp_zero(&a);
    fp_zero(&b);
    fp_zero(&c);
    fp_from_radix(&expected, "2147483648");
    fp_add_d(&a, 1, &a);
    fp_mul_2d(&a, 31, &a); // 2^31
    //printf("\na:\n");
    //btl_tfm_print_internals(&a);
    assert(fp_cmp(&a, &expected) == FP_EQ && "equals zero");    
}
DEFINE_TEST(tfm_add_d_1) {
    fp_int a, b, c, expected;
    fp_zero(&a);
    fp_zero(&b);
    fp_zero(&expected);
    fp_add_d(&a, 2, &c);
    //printf("tfm_add_d1: c:");
    //btl_tfm_print_internals(&c);
    fp_from_radix(&expected, "2");
    assert(fp_cmp(&c, &expected) == FP_EQ && "equals zero");
    assert(c.dp[0] == 2 && "internal array correct");
}
DEFINE_TEST(tfm_add_d_2) {
    fp_int a, b, c, expected;
    fp_zero(&a);
    fp_zero(&b);
    fp_zero(&c);
    fp_add_d(&a, FP_MASK, &a);
    //printf("\na (1):\n");
    //btl_tfm_print_internals(&a);
    fp_from_radix(&expected, "4294967295");  // 2^32 - 1
    assert(fp_cmp(&a, &expected) == FP_EQ && "equals zero");
    fp_add_d(&a, FP_MASK, &a);
    //printf("\na (2):\n");
    //btl_tfm_print_internals(&a);
    fp_from_radix(&expected, "8589934590");  // 2^33 - 2
    assert(fp_cmp(&a, &expected) == FP_EQ && "equals zero");
}
DEFINE_TEST(tfm_mul_d_1) {
    fp_int a, b, c, expected;
    fp_zero(&a);
    fp_zero(&b);
    fp_zero(&c);
    fp_zero(&expected);
    fp_add_d(&a, 1, &a);
    fp_mul_d(&a, FP_MASK, &a);
    fp_from_radix(&expected, "4294967295");  // 2^32 - 1
    assert(fp_cmp(&a, &expected) == FP_EQ && "equals zero");
    fp_mul_d(&a, FP_MASK, &a);
    fp_from_radix(&expected, "18446744065119617025");  // (2^32 - 1) ^ 2
    assert(fp_cmp(&a, &expected) == FP_EQ && "equals zero");
}
DEFINE_TEST(tfm_mul_d_2) {
    fp_int a, b, c, expected;
    fp_zero(&a);
    fp_zero(&b);
    fp_zero(&c);
    fp_zero(&expected);
    fp_add_d(&a, 1, &a);
    fp_neg(&a, &a);
    //printf("\na (1):\n");
    //btl_tfm_print_internals(&a);
    fp_mul_d(&a, FP_MASK, &a);
    //printf("\na (2):\n");
    //btl_tfm_print_internals(&a);
    fp_from_radix(&expected, "-4294967295");  // 2^32 - 1
    assert(fp_cmp(&a, &expected) == FP_EQ && "equals zero");
    fp_mul_d(&a, FP_MASK, &a);
    fp_from_radix(&expected, "-18446744065119617025");  // (2^32 - 1) ^ 2
    assert(fp_cmp(&a, &expected) == FP_EQ && "equals zero");
}
DEFINE_TEST(tfm_add1) {
    fp_int a, b, c, expected;
    fp_zero(&a);
    fp_zero(&b);
    fp_from_radix(&b, "123");
    fp_add(&a, &b, &c);
    assert(fp_cmp(&b, &c) == FP_EQ && "equals zero");
    fp_add(&b, &b, &c);
    fp_from_radix(&expected, "246");
    assert(fp_cmp(&c, &expected) == FP_EQ && "equals zero");    
}
DEFINE_TEST(tfm_add2) {
    fp_int a, b, c, expected;
    fp_zero(&a);
    fp_from_radix(&b, "2147483647");
    fp_add(&b, &b, &c);
    fp_from_radix(&expected, "4294967294");
    assert(fp_cmp(&c, &expected) == FP_EQ && "equals zero");    
}
DEFINE_TEST(tfm_add3) {
    fp_int a, b, c, expected;
    fp_zero(&expected);
    fp_from_radix(&a, "-123");
    fp_from_radix(&b, "123");
    fp_add(&a, &b, &c);
    assert(fp_cmp(&c, &expected) == FP_EQ && "equals zero");    
}
DEFINE_TEST(tfm_add4) {
    fp_int a, b, c, expected;
    fp_zero(&a);
    fp_zero(&c);
    fp_from_radix(&b, "-2147483647");
    for (size_t i = 0; i < 10; ++i) {
        fp_add(&a, &b, &a);
    }
    fp_from_radix(&expected, "-21474836470");
    assert(fp_cmp(&a, &expected) == FP_EQ && "equals zero");    
}

DEFINE_TEST(tfm_multiply1) {
    int result_code;
    fp_int a, b, c, expected;
    fp_from_radix(&a, "-1");
    fp_from_radix(&b, "2147483647");
    fp_from_radix(&expected, "-2147483647");
    fp_mul(&a, &b, &c);
    result_code = fp_cmp(&c, &expected);
    assert(result_code == FP_EQ && "result code");    
}
DEFINE_TEST(tfm_multiply2) {
    fp_int a, b, c, expected;
    fp_zero(&c);
    fp_from_radix(&a, "1");
    fp_add_d(&b, FP_MASK, &b);  // 2^32 - 1
    fp_neg(&b, &b);
    for (size_t i = 0; i < 3; ++i) {
        fp_mul(&a, &b, &a);
    }
    fp_from_radix(&expected, "-79228162458924105372415295488"); // neg(FP_MASK ^ 3)
    assert(fp_cmp(&a, &expected) && "result code");    
}

int main() {
    printf("HELLO from test_tfm.c\n");
    btl_run_tests();
    //test_bignum_main();
}
