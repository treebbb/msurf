#include <stdio.h>
#include "tfm_opencl.h"
#include "bignum_test_framework.h"

DEFINE_TEST(tfm_multiply1) {
    fp_int a, b, c;
    fp_zero(&a);
    fp_zero(&b);
    fp_add(&a, &b, &c);
    assert(fp_cmp(&a, &c) == FP_EQ && "equals zero");
    
}

int main() {
    printf("HELLO from test_tfm.c\n");
    btl_run_tests();
    test_bignum_main();
}
