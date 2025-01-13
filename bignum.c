#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PRINTERR(msg) fprintf(stderr, "%s\n", msg)
#define BIGNUM_MAX_LENGTH 8  // 512 bit / sizeof(bignum_elem_t)
#define BIGNUM_ELEM_TYPE unsigned long
#define BIGNUM_ELEM_SIZE sizeof(BIGNUM_ELEM_TYPE)
#define BIGNUM_ELEM_MAX ((BIGNUM_ELEM_TYPE) 0 - 1)
/** @brief Bitmask for the lower half of a bignum_elem_t */
#define BIGNUM_ELEM_LO (BIGNUM_ELEM_MAX >> BIGNUM_ELEM_SIZE*4)

/** @brief Bitmask for the upper half of a bignum_elem_t */
#define BIGNUM_ELEM_HI (BIGNUM_ELEM_MAX << BIGNUM_ELEM_SIZE*4)

typedef unsigned long bignum_elem_t;

typedef struct bignum {
    size_t length;
    size_t max_length;
    bignum_elem_t *v;
} bignum_t;

// allocate
bignum_t *bignum_new() {
    bignum_t *num = malloc(sizeof(bignum_t));
    if (num == NULL) {
        PRINTERR("bignum_new error allocating memory");
    }
    num->length = 1;
    num->max_length = BIGNUM_MAX_LENGTH;
    size_t v_size = BIGNUM_MAX_LENGTH * sizeof(bignum_elem_t);
    num->v = malloc(v_size);
    if (num->v == NULL) {
        PRINTERR("bignum_zero error allocating buffer");
        return NULL;
    }
    memset(num->v, 0, v_size);
    return num;
}
void bignum_free(bignum_t *num) {
    free(num->v);
    free(num);
}
// set to zero
void bignum_zero(bignum_t *num) {
    num->v[0] = 0;
    num->length = 1;
}
// print internals
void bignum_print_internals(bignum_t *num) {
    printf("bignum(length=%zu, maxlength=%zu)\n", num->length, num->max_length);
    for (size_t i = 0; i < num->length; ++i) {
        printf("  [%zu]: %lu\n", i, num->v[i]);
    }
}

// Function to add two bignum values
void bignum_add(bignum_t *result, const bignum_t *a, const bignum_t *b) {
    // Determine the max length for the result
    size_t max_length = (a->length > b->length) ? a->length : b->length;
    if (max_length > result->max_length) {
        // Reallocate if necessary, not handling memory management here
        fprintf(stderr, "Error: result bignum too small for sum.\n");
        return;
    }

    // Reset result's length to zero
    result->length = 0;

    bignum_elem_t carry = 0;
    for (size_t i = 0; i < max_length || carry; ++i) {
        // Get digits from 'a' and 'b', assuming 0 for out-of-length digits
        bignum_elem_t digit_a = (i < a->length) ? a->v[i] : 0;
        bignum_elem_t digit_b = (i < b->length) ? b->v[i] : 0;

        // Perform addition with carry
        bignum_elem_t sum = digit_a + digit_b + carry;
        carry = (sum < digit_a || sum < digit_b) ? 1 : 0;  // If overflow, carry = 1

        // Store result
        if (i >= result->max_length) {
            fprintf(stderr, "Error: result bignum overflow.\n");
            return;
        }
        result->v[i] = sum;
        result->length = i + 1;
    }

    // Trim leading zeros from the result
    while (result->length > 1 && result->v[result->length - 1] == 0) {
        result->length--;
    }
}

// adds val to a (in-place)
void bignum_add_elem(bignum_t *a, bignum_elem_t val) {
    if (a == NULL) {
        return; // Handle NULL pointer
    }

    bignum_elem_t carry = val;
    size_t i = 0;
    while (carry) {
        if (i >= a->max_length) {
            PRINTERR("bignum_add_elem overflow beyond max_length");
        }
        bignum_elem_t sum = a->v[i] + carry;
        carry = (sum < a->v[i]) ? 1 : 0;  // Check for overflow
        a->v[i] = sum;
        
        if (i >= a->length) {
            a->length = i + 1;
        }
        i++;
    }
}

/*
void bignum_multiply_elem(bignum_t *a, bignum_elem_t val) {
    if (a == NULL) {
        return; // Handle NULL pointer
    }

    if (val == 0) {
        // Multiplying by 0 results in 0
        a->v[0] = 0;
        a->length = 1;
        return;
    }

    size_t original_length = a->length;
    bignum_elem_t carry = 0;
    size_t i = 0;

    while (i < original_length || carry) {
        // If we need to extend beyond current length, reallocate
        if (i >= a->max_length) {
            PRINTERR("bignum_multiply_elem overflow beyond max_length");
            return;
        }

        // Multiply and handle carry
        bignum_elem_t product = (i < original_length ? a->v[i] : 0) * val + carry;
        carry = product / (bignum_elem_t)(-1);  // Use -1 for max unsigned long + 1 for division
        a->v[i] = product % (bignum_elem_t)(-1); // Modulo for remainder

        // Update length if we've added significant digits
        if (i >= a->length && a->v[i] != 0) {
            a->length = i + 1;
        }
        i++;
    }

    // Trim leading zeros if multiplication resulted in extra zeros
    while (a->length > 1 && a->v[a->length - 1] == 0) {
        a->length--;
    }

}
*/

static inline bignum_elem_t lo(bignum_elem_t elem) {
    // Return the value of the lower bits of elem.
    return elem & BIGNUM_ELEM_LO;
}

static inline bignum_elem_t hi(bignum_elem_t elem) {
    // Return the value of the higher bits of elem.
    return (elem & BIGNUM_ELEM_HI) >> BIGNUM_ELEM_SIZE * 4;
}

int bignum_multiply_elem(bignum_t *op1, bignum_elem_t op2) {
    // rop = op1 * op2
    bignum_elem_t result;
    bignum_elem_t lo1, hi1, lo2, hi2;
    bignum_elem_t carry = 0;
    bignum_elem_t elem1, elem2;

    size_t length = 0;
    int overflow = 0;

    size_t max_length;
    if (op1->length > op1->max_length)
        max_length = op1->max_length;
    else
        max_length = op1->length;

    // Calculation starts here.
    for (int pos=0; pos<max_length; pos++) {
        result = carry;
        carry = 0;

        elem1 = op1->v[pos];
        elem2 = op2;
        lo1 = lo(elem1);
        lo2 = lo(elem2);
        hi1 = hi(elem1);
        hi2 = hi(elem2);

        result += lo1 * lo2;
        result += (hi1 * lo2) << BIGNUM_ELEM_SIZE * 4;
        result += (lo1 * hi2) << BIGNUM_ELEM_SIZE * 4;

        carry += hi1 * hi2;
        carry += hi(hi1 * lo2);
        carry += hi(lo1 * hi2);

        if (result != 0 || carry != 0)
            length = pos+1;

        op1->v[pos] = result;
    }

    if (carry != 0) {
        if (length < (op1->max_length - 1)) {
            op1->v[length] = carry;
            op1->length = length + 1;
        } else {
            overflow = 1;
        }
    } else {
        op1->length = length;
    }
    return overflow;
}


// Helper function to perform a single digit multiplication and add to result
static void multiply_digit(bignum_t *result, const bignum_t *a, bignum_elem_t digit, size_t offset) {
    bignum_elem_t carry = 0;
    for (size_t i = 0; i < a->length || carry; ++i) {
        bignum_elem_t product = (i < a->length ? a->v[i] : 0) * digit + carry;
        if (i + offset >= result->max_length) {
            fprintf(stderr, "Error: result bignum overflow in multiply_digit.\n");
            return;
        }
        carry = product / (bignum_elem_t)(-1);  // Use -1 to get the max value for unsigned long + 1 for division
        product %= (bignum_elem_t)(-1);          // Modulo to get the remainder
        
        bignum_elem_t sum = result->v[i + offset] + product;
        carry += (sum < result->v[i + offset] || sum < product) ? 1 : 0;  // Check for overflow
        result->v[i + offset] = sum;
    }
    if (result->length < a->length + offset + (carry > 0 ? 1 : 0)) {
        result->length = a->length + offset + (carry > 0 ? 1 : 0);
    }
    if (carry > 0) {
        result->v[result->length - 1] = carry;
    }
}

// Function to multiply two bignum values
void bignum_multiply(bignum_t *result, const bignum_t *a, const bignum_t *b) {
    // Reset result
    result->length = 0;

    for (size_t i = 0; i < b->length; ++i) {
        multiply_digit(result, a, b->v[i], i);
    }

    // Trim leading zeros
    while (result->length > 1 && result->v[result->length - 1] == 0) {
        result->length--;
    }
}

// Converts bignum to base 10 string representation
void double_array(int *input, size_t *input_size) {
    int carry = 0;
    size_t i;
    
    for (i = 0; i < *input_size; i++) {
        int digit = input[i];
        int doubled = digit * 2 + carry;
        
        carry = doubled / 10;  // New carry
        input[i] = doubled % 10;  // Store the least significant digit

    }
    if (carry) {
        input[i] = carry;
        (*input_size)++;
    }
}
size_t bignum_max_base10_digits(const bignum_t *num) {
    // log(10) / log(2) ~= 3.32
    return (size_t)ceil((double)(sizeof(bignum_elem_t) * 8 * num->length) / 3.32);
}

void debug_print_array(const int *array, size_t size, int bit_count) {
    printf("(size: %zu  bit_count: %d  [ ", size, bit_count);
    for (size_t i = 0; i < size; ++i) {
        printf("%d, ", array[i]);
    }
    printf(" ]\n");
}

void bignum_to_int_array(const bignum_t *num, int *result, size_t *result_size) {
    // Step 1: Calculate size for the integer array
    size_t max_digits = bignum_max_base10_digits(num);
    
    // Allocate memory for temporary array
    int *base2_digits = malloc(max_digits * sizeof(int));
    if (!base2_digits) {
        fprintf(stderr, "Memory allocation failed for temporary array.\n");
        return;
    }

    // Step 2: Initialize the temporary array
    memset(base2_digits, 0, max_digits * sizeof(int));
    base2_digits[0] = 1;  // Least significant digit set to 1 for starting multiplication
    size_t bit_count = 1;
    size_t base2_digits_size = 1;  // Start with one digit

    // Step 3 & 4: Loop over bignum_elem_t elements in v, least significant first
    int bits_per_bignum_elem = sizeof(bignum_elem_t) * 8;
    for (int i = 0; i < num->length; ++i) {
        bignum_elem_t val = num->v[i];
        // update the base2_digits to match the bit shift for this num->v element
        while (bit_count < (i * bits_per_bignum_elem) + 1) {
            double_array(base2_digits, &base2_digits_size);
            ++bit_count;
        }
        //
        while (val) {
            if (val % 2) {
                //debug_print_array(base2_digits, base2_digits_size, bit_count);
                // Add the current value of temp to result for this bit position
                for (size_t j = 0; j < base2_digits_size; j++) {
                    result[j] += base2_digits[j];
                }
            }
            // Double the base2_digits array
            double_array(base2_digits, &base2_digits_size);
            // keep track of bit count
            ++bit_count;
            // shift val right one bit
            val >>= 1;
        }
    }
    // we're done with base2_digits
    free(base2_digits);

    // Since we've only added values, each digit might exceed 10, so we normalize here
    //debug_print_array(result, base2_digits_size, -1);

    size_t new_size = 0;
    int carry = 0;
    for (size_t i = 0; i < max_digits; i++) {
        result[i] += carry;
        if (result[i] >= 10) {
            carry = result[i] / 10;
            result[i] %= 10;
            new_size = i + 1;
        } else if (result[i] > 0) {
            new_size = i + 1;
            carry = 0;
        }else {
            carry = 0;
        }
    }
    if (carry > 0) {
        fprintf(stderr, "Error: result array too small for carry.\n");
        return;
    }
    *result_size = new_size;
}

// allocates string. must be freed after
char *bignum_to_string(const bignum_t *num) {
    if (num->length == 1 && num->v[0] == 0) {
        return strdup("0");
    }
    int max_digits = bignum_max_base10_digits(num);
    int *digit_array = malloc(max_digits * sizeof(int));
    if (!digit_array) {
        fprintf(stderr, "Memory allocation failed for bignum_to_string digit_array");
        return NULL;
    }
    memset(digit_array, 0, max_digits * sizeof(int));
    size_t digit_array_size = 0;    
    bignum_to_int_array(num, digit_array, &digit_array_size);
    //printf("bignum_to_string: digit_array_size=%zu\n", digit_array_size);
    //for(size_t i=0; i < digit_array_size; ++i) {
    //    printf("  array[%zu]: %d\n", i, digit_array[i]);
    //}
    char *result = malloc((digit_array_size + 1) * sizeof(char));
    if (result == NULL) {
        fprintf(stderr, "error allocating bignum string result");
        free(digit_array);
    }
    result[digit_array_size] = '\0';
    for (size_t i = 0; i < digit_array_size; ++i) {
        size_t pos = digit_array_size - 1 - i;
        result[pos] = digit_array[i] + '0';
    }
/*
    printf("digit_array_size: %zu\n", digit_array_size);
    for (size_t i = 0; i < digit_array_size; ++i) {
        printf("  %zu: %d\n", i, digit_array[i]);
    }
*/
    free(digit_array);
    return result;
}

void bignum_from_string(const char *base_10_digits, bignum_t *result) {
    char *digits = strdup(base_10_digits);
    if (digits == NULL) {
        PRINTERR("bignum_from_string: error allocating digits");
        return;
    }
    bignum_zero(result);
    for (size_t i = 0; i < strlen(base_10_digits); ++i) {
        bignum_multiply_elem(result, 10);
        bignum_elem_t digit_val = (bignum_elem_t) digits[i] - '0';
        bignum_add_elem(result, digit_val);
    }

}

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
    for (int i = 0; i < sum.length; ++i) {
        printf("sum.v[%d]=%lu\n", i, sum.v[i]);
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
    bignum_t a;
    
    return test2();
}
