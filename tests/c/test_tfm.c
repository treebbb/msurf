#include <math.h>
#include <stdio.h>
#include "tfm_opencl.h"
#include "bignum_test_framework.h"

/* use 1e-7 for allowed_test error. */
#define MAX_FLOAT_ROUNDTRIP_ERROR 0.0000001
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
DEFINE_TEST(tfm_fp_mul_d_3) {
    fp_int step_size_fp, ymin_fp, c_imag_fp;
    double step_size = 2.5402189032797118e-12;
    double ymin = -1.2534398210784028;
    for (int y = 0; y < 10; ++y) {
        fp_from_double(&step_size_fp, step_size);
        fp_mul_d(&step_size_fp, y, &c_imag_fp);
        fp_from_double(&ymin_fp, ymin);
        fp_add(&c_imag_fp, &ymin_fp, &c_imag_fp);
        printf("\nc_imag:\n");
        btl_tfm_print_internals(&c_imag_fp);
    }
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
DEFINE_TEST(tfm_add5) {
    fp_int step_size, ymin, c_imag, expected;
    fp_from_double(&step_size, 2.7498992666297184e-12);
    printf("\nstep_size: %lf\n", fp_to_double(&step_size));
    btl_tfm_print_internals(&step_size);
    fp_from_double(&ymin, 0.38469586368246556);
    fp_copy(&ymin, &expected);
    printf("\nymin: %lf\n", fp_to_double(&ymin));
    btl_tfm_print_internals(&ymin);
    fp_mul_d(&step_size, 0, &c_imag);
    printf("\nc_imag: %lf\n", fp_to_double(&c_imag));
    btl_tfm_print_internals(&c_imag);
    fp_add(&c_imag, &ymin, &c_imag);
    printf("\nc_imag(2): %lf\n", fp_to_double(&c_imag));
    btl_tfm_print_internals(&c_imag);
    assert(fp_cmp(&c_imag, &expected) == FP_EQ && "equals zero");
}
DEFINE_TEST(tfm_fp_mul_1) {
    int result_code;
    fp_int a, b, c, expected;
    fp_from_radix(&a, "-1");
    fp_from_radix(&b, "2147483647");
    fp_from_radix(&expected, "-2147483647");
    fp_mul(&a, &b, &c);
    result_code = fp_cmp(&c, &expected);
    assert(result_code == FP_EQ && "result code");
}
DEFINE_TEST(tfm_fp_mul_2) {
    int result_code;
    fp_int a, b, c, expected;
    fp_zero(&b);
    fp_zero(&c);
    fp_from_radix(&a, "1");
    fp_add_d(&b, FP_MASK, &b);  // 2^32 - 1
    fp_neg(&b, &b);
    for (size_t i = 0; i < 3; ++i) {
        fp_mul(&a, &b, &a);
    }
    fp_from_radix(&expected, "-79228162458924105385300197375"); // 0 - (FP_MASK ^ 3)
    result_code = fp_cmp(&a, &expected);
    assert(result_code == FP_EQ && "result code");
}
DEFINE_TEST(tfm_fp_mul_3) {
    int result_code;
    fp_int a, b, c, expected;
    fp_from_radix(&a, "46116860184273879040"); // 2.5 * 2**64
    fp_from_radix(&b, "2049617734029868269568"); // 111.11 * 2**64
    fp_from_radix(&expected, "5124044335074670673920"); // (2.5 * 111.11) * 2**64
    fp_lshd(&expected, 2); // shift left 2 fp_digits (2**64) to account for multiplication
    fp_mul(&a, &b, &c);
    result_code = fp_cmp(&c, &expected);
    assert(result_code == FP_EQ && "result code");
}
DEFINE_TEST(tfm_fp_from_double_1) {
    fp_int a, expected;
    int result_code;
    fp_from_double(&a, 0.0f);
    fp_zero(&expected);
    result_code = fp_cmp(&a, &expected);
    assert(result_code == FP_EQ && "zero from double");
}
DEFINE_TEST(tfm_fp_from_double_2) {
    fp_int a, expected;
    int result_code;
    fp_from_double(&a, 2.5f);
    fp_from_radix(&expected, "46116860184273879040"); // (1<<64) + (1 << 64) + (1 <<63)
    result_code = fp_cmp(&a, &expected);
    assert(result_code == FP_EQ && "zero from double");
}
DEFINE_TEST(tfm_fp_from_double_3) {
    fp_int a, expected;
    int result_code;
    fp_from_double(&a, 3.1415926535897932); // pi(approx)
    //printf("\na: \n");
    //btl_tfm_print_internals(&a);
    fp_from_radix(&expected, "57952155664616980480"); // pi(approx) * (1<<64)
    //printf("\nexpected: \n");
    //btl_tfm_print_internals(&expected);
    result_code = fp_cmp(&a, &expected);
    assert(result_code == FP_EQ && "zero from double");
}
DEFINE_TEST(tfm_fp_from_double_4) {
    union DoubleBits db1, db2;
    db1.d = 3.141592653589793;
    //printf("\ndb.u64: %llu  db.d: %lf\n", db1.u64, db1.d);
    db2.u64 = 4614256656552045848;
    //printf("\ndb.u64: %llu  db.d: %lf\n", db2.u64, db2.d);
    assert(db1.u64 == db2.u64 && "double to uint64 bits correct");
    assert(db2.d == db2.d && "uint64 to double bits correct");
}
DEFINE_TEST(tfm_fp_to_double_1) {
    fp_int a;
    double d;
    fp_from_radix(&a, "57952155664616980480"); // pi(approx) * (1<<64)
    //printf("\na (1):\n");
    //btl_tfm_print_internals(&a);
    d = fp_to_double(&a);
    assert(d == 3.141592653589793 && "fp_to_double");
}
DEFINE_TEST(tfm_fp_double_roundtrip_1) {
    int result_code;
    fp_int a, expected;
    double d, result, diff;
    d = -0.100000;
    fp_from_double(&a, d);
    fp_from_radix(&expected, "-1844674407370955264");
    result_code = fp_cmp(&a, &expected);
    assert(result_code == FP_EQ && "a = 0.1 * 2**64");
    //printf("\na (1):\n");
    //btl_tfm_print_internals(&a);
    result = fp_to_double(&a);
    diff = (d - result);
    assert(diff == 0 && "-0.1 round trip");
}
DEFINE_TEST(tfm_fp_double_roundtrip_2) {
    fp_int a;
    double d, result, diff;
    d = -5.0000000001;
    for (size_t i = 0; i < 100; ++i) {
        fp_from_double(&a, d);
        result = fp_to_double(&a);
        diff = (d - result);
        assert(diff == 0 && "roundtrip 2");
        d += 0.1;
    }
}
DEFINE_TEST(tfm_fp_float_roundtrip_1) {
    fp_int a;
    float d, result, diff;
    d = 3.1415926535897932; // pi(approx)
    fp_from_float(&a, d);
    result = fp_to_float(&a);
    diff = fabs(d - result);
    assert(diff < MAX_FLOAT_ROUNDTRIP_ERROR && "float roundtrip");
}
DEFINE_TEST(tfm_fp_float_roundtrip_2) {
    fp_int a;
    float d, result, diff;
    d = -5.0000000001;
    for (size_t i = 0; i < 100; ++i) {
        fp_from_float(&a, d);
        result = fp_to_float(&a);
        diff = fabs(d - result);
        assert(diff < MAX_FLOAT_ROUNDTRIP_ERROR && "roundtrip 2");
        d += 0.1;
    }
}
DEFINE_TEST(tfm_fp_mul_scaled_1) {
    fp_int a, b, c, c_orig;
    double d1, d2, expected, result, diff;
    d1 = 2.5;
    d2 = 111.11;
    fp_from_double(&a, d1);
    fp_from_double(&b, d2);
    fp_mul(&a, &b, &c_orig);
    fp_mul_scaled(&a, &b, &c);
    result = fp_to_double(&c);
    expected = d1 * d2;
    diff = fabs(result - expected);
    assert(diff == 0 && "fp_mul_scaled_1");
}
int main() {
    printf("HELLO from test_tfm.c\n");
    btl_run_tests();
    //test_mandelbrot_tfm_main();
    return 0;
}
