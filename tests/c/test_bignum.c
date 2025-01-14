#include <assert.h>
#include "bignum.h"
#include <string.h>

/* Begin test framework helpers */
#define CHECKVAL(bignum, strval) fprintf(stderr, "%s\n", msg)
void test_eq(bignum_t* bignum_instance, const char* expected) {
    char* bignum_str = bignum_to_string(bignum_instance);
    if (bignum_str == NULL) {
        fprintf(stderr, "Failed to convert bignum to string\n");
        assert(0 && "Failed to convert bignum to string");
    } else {
        int result = strcmp(bignum_str, expected);
        if (result != 0) {
            fprintf(stderr, "Assertion failed: bignum_to_string returned NULL");
        }
        assert(result == 0 && "Assertion failed: expected/actual unequal");
        free(bignum_str); // Free the dynamically allocated string
    }
}
#define TESTEQ(bignum, str) do { \
    test_eq((bignum), (str)); \
} while(0)

/**** test function macros ****/
// Array to hold pointers to all test functions
#define MAX_TESTS 100 // Adjust based on the maximum number of tests you expect
int test_count = 0;
typedef struct {
    void (*func)(void);
    const char *name;
} TestFunction;
TestFunction test_functions[MAX_TESTS];

// Function to register tests
void register_test(void (*func)(void), const char *name) {
    if (test_count < MAX_TESTS) {
        test_functions[test_count].func = func;
        test_functions[test_count].name = name;
        test_count++;
    } else {
        fprintf(stderr, "Too many tests, increase MAX_TESTS\n");
        exit(1);
    }
}
// Macro to define test function
// this registers the test in the test_functions array used by main()
// and then starts the function
#define DEFINE_TEST(func) \
void func(void); \
static void __attribute__((constructor)) register_##func(void) { register_test(func, #func); } \
static void __attribute__((used)) use_##func(void) { register_##func(); } \
void func(void)

/* End test framework helpers */

/* Begin tests */
DEFINE_TEST(test_bignum_create) {
    bignum_t *a = bignum_new();
    TESTEQ(a, "0");
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

/* End tests */
// Example usage:
int test1() {
    bignum_t a, b, sum;
    bignum_elem_t a_val[8];
    bignum_elem_t b_val[8];
    bignum_elem_t sum_val[8];
    // Initialize 'a'
    a.length = 2; // Assuming 512 bits / 64 bits per unsigned long
    a.max_length = 8;
    a.v = a_val;
    a.v[0] = 1;
    a.v[1] = 1;
    // initialize 'b'
    b.length = 1;
    b.max_length = 8;
    b.v = b_val;
    b.v[0] = 2;
    // initialize sum
    sum.length = 0;
    sum.max_length = 8;
    sum.v = sum_val;
    // perform operation
    bignum_add(&sum, &a, &b);
    // print operation
    char *a_str = bignum_to_string(&a);
    char *b_str = bignum_to_string(&b);
    char *sum_str = bignum_to_string(&sum);
    printf("%s + %s = %s\n", a_str, b_str, sum_str);
    free(a_str);
    free(b_str);
    free(sum_str);
    
    // Print or use 'sum' here
    printf("sum.length: %zu\n", sum.length);
    printf("sum.max_length: %zu\n", sum.max_length);    
    for (size_t i = 0; i < sum.length; ++i) {
        printf("sum.v[%zu]=%lu\n", i, sum.v[i]);
    }
    

    // Free memory...
    return 0;
}



void print_and_free(char *message) {
    if (message != NULL) {
        printf("%s\n", message);
        free(message);
    }
}

int test2() {
    bignum_t *a;
    /*
    // test add and multiply
    a = bignum_new();
    bignum_print_internals(a);
    print_and_free(bignum_to_string(a));

    bignum_add_elem(a, 10);
    bignum_print_internals(a);
    print_and_free(bignum_to_string(a));

    bignum_add_elem(a, 10);
    bignum_print_internals(a);
    print_and_free(bignum_to_string(a));

    bignum_multiply_elem(a, 10);
    bignum_print_internals(a);
    print_and_free(bignum_to_string(a));
    
    bignum_free(a);
    /// try from_string
    a = bignum_new();
    bignum_from_string("123", a);
    bignum_print_internals(a);
    print_and_free(bignum_to_string(a));
    bignum_free(a);
    // try 2^64 - 1
    a = bignum_new();
    bignum_from_string("18446744073709551615", a);
    bignum_print_internals(a);
    print_and_free(bignum_to_string(a));
    bignum_add_elem(a, 1);
    bignum_print_internals(a);
    print_and_free(bignum_to_string(a));    
    bignum_free(a);
    // try 2^64
    a = bignum_new();
    bignum_from_string("18446744073709551616", a);
    bignum_print_internals(a);
    print_and_free(bignum_to_string(a));    
    bignum_free(a);
    */
    // try 2^128
    a = bignum_new();
    bignum_from_string("340282366920938463463374607431768211456", a);
    bignum_print_internals(a);
    print_and_free(bignum_to_string(a));
    bignum_free(a);
    
    /*
    a = bignum_new();
    bignum_add_elem(a, 1);
    for (size_t i = 0; i < 10; ++i) {
        bignum_multiply_elem(a, 256);
        bignum_print_internals(a);
        print_and_free(bignum_to_string(a));    
    }
    bignum_free(a);
    */
    return 0;
}

int main() {
    //return test2();
    //test_bignum_create();
    for (int i = 0; i < test_count; ++i) {
        printf("Running %s .... ", test_functions[i].name);
        test_functions[i].func();
        printf("OK\n");
    }
    printf("%d tests completed successfully.\n", test_count);
    return 0;
}
