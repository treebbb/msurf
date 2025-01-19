#include <assert.h>
#include "bignum.h"
#include <string.h>

/* Begin test framework helpers */
void bignum_print_internals(bignum_t *num) {
    printf("bignum(length=%zu, maxlength=%zu)\n", num->length, num->max_length);
    for (size_t i = 0; i < num->length; ++i) {
        printf("  [%zu]: %lu\n", i, num->v[i]);
    }
}

void test_eq(bignum_t* bignum_instance, const char* expected) {
    char* bignum_str = bignum_to_string(bignum_instance);
    if (bignum_str == NULL) {
        fprintf(stderr, "Failed to convert bignum to string\n");
        assert(0 && "Failed to convert bignum to string");
    } else {
        int result = strcmp(bignum_str, expected);
        if (result != 0) {
            printf("Expected %s  Got %s\n", expected, bignum_str);
            bignum_print_internals(bignum_instance);
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

int main() {
    for (int i = 0; i < test_count; ++i) {
        printf("Running %s .... ", test_functions[i].name);
        test_functions[i].func();
        printf("OK\n");
    }
    printf("%d tests completed successfully.\n", test_count);
    return 0;
}
