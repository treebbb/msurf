#pragma once
#include <assert.h>
#include "bignum.h"
#include <string.h>

static void btl_bignum_print_internals(bignum_t *num);
static void btl_test_eq(bignum_t* bignum_instance, const char* expected);
static void btl_register_test(void (*func)(void), const char *name);

#define TESTEQ(bignum, str) do { \
    btl_test_eq((bignum), (str)); \
} while(0)

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
static GCC_MARK_USED void btl_bignum_print_internals(bignum_t *num) {
    printf("bignum(length=%zu, maxlength=%zu)\n", num->length, num->max_length);
    for (size_t i = 0; i < num->length; ++i) {
        printf("  [%zu]: %lu\n", i, num->v[i]);
    }
}

static GCC_MARK_USED void btl_test_eq(bignum_t* bignum_instance, const char* expected) {
    char* bignum_str = bignum_to_string(bignum_instance);
    if (bignum_str == NULL) {
        fprintf(stderr, "Failed to convert bignum to string\n");
        assert(0 && "Failed to convert bignum to string");
    } else {
        int result = strcmp(bignum_str, expected);
        if (result != 0) {
            printf("Expected %s  Got %s\n", expected, bignum_str);
            btl_bignum_print_internals(bignum_instance);
        }
        assert(result == 0 && "Assertion failed: expected/actual unequal");
        free(bignum_str); // Free the dynamically allocated string
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
        fprintf(stderr, "Too many tests, increase MAX_TESTS\n");
        exit(1);
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
int test_bignum_main();
