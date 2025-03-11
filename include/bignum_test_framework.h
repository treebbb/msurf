#pragma once
#include <assert.h>
#include "tfm_opencl.h"
#include <string.h>

static void btl_tfm_print_internals(fp_int *num);
static void btl_register_test(void (*func)(void), const char *name);

/* btl_run_tests() may not be used by the file that includes this.
   use gcc extention to mark used and avoid error */
#define GCC_MARK_USED __attribute__((used))

#define MAX_TESTS 100 // Adjust based on the maximum number of tests you expect
// Macro to define test function
// this registers the test in the test_functions array used by main()
// and then starts the function
#define DEFINE_TEST(func) \
void func(void); \
 void __attribute__((constructor)) btl_register_##func(void) { btl_register_test(func, #func); } \
 void __attribute__((used)) use_##func(void) { btl_register_##func(); } \
void func(void)

/* run all tests */
static int btl_run_tests();


/* Begin test framework helpers */
static GCC_MARK_USED void btl_tfm_print_internals(fp_int *num) {
    printf("fp_int(used=%d sign=%d)\n", num->used, num->sign);
    for (int i = 0; i < num->used; ++i) {
        printf("  [%d]: %u\n", i, num->dp[i]);
    }
}

/**** test function macros ****/
// Array to hold pointers to all test functions
static int btl_test_count = 0;
typedef struct {
    void (*func)(void);
    const char *name;
} TestFunction;
TestFunction test_functions[MAX_TESTS];

// Function to register tests
static GCC_MARK_USED void btl_register_test(void (*func)(void), const char *name) {
    if (btl_test_count < MAX_TESTS) {
        test_functions[btl_test_count].func = func;
        test_functions[btl_test_count].name = name;
        btl_test_count++;
    } else {
        assert(0 && "Too many tests, increase MAX_TESTS\n");
    }
}

static GCC_MARK_USED int btl_run_tests() {
    for (int i = 0; i < btl_test_count; ++i) {
        printf("Running %s .... ", test_functions[i].name);
        test_functions[i].func();
        printf("OK\n");
    }
    printf("%d tests completed successfully.\n", btl_test_count);
    return 0;

}

/* End test framework helpers */
int test_mandelbrot_tfm_main();
