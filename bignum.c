#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned long bignum_elem_t;

typedef struct bignum {
    size_t length;
    size_t max_length;
    bignum_elem_t *v;
} bignum_t;

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

// Helper function to convert a single bignum_elem_t to string
static char* ulong_to_string(bignum_elem_t n) {
    char buffer[21]; // Max length for unsigned long in base 10 + null terminator
    int index = 20;
    buffer[index--] = '\0';
    
    do {
        buffer[index--] = (n % 10) + '0';
        n /= 10;
    } while (n > 0);
    
    return strdup(&buffer[index + 1]);  // +1 to skip past the last '\0'
}

// Converts bignum to base 10 string representation
void double_array(const int *input, const size_t input_size, int *result, size_t *result_size) {
    int carry = 0;
    size_t i;
    
    for (i = 0; i < input_size || carry; i++) {
        int digit = (i < input_size) ? input[i] : 0; // Use 0 if we've run out of input digits but carry remains
        int doubled = digit * 2 + carry;
        
        carry = doubled / 10;  // New carry
        result[i] = doubled % 10;  // Store the least significant digit

    }

    *result_size = i;  // Set the size of result to the number of digits we've written
}
size_t bignum_max_base10_digits(const bignum_t *num) {
    return (size_t)ceil((double)(sizeof(bignum_elem_t) * 8 * num->length) / 3.32);
}

void bignum_to_int_array(const bignum_t *num, int *result, size_t *result_size) {
    // Step 1: Calculate size for the integer array
    // log(10) / log(2) ~= 3.32
    size_t max_digits = bignum_max_base10_digits(num);
    
    // Allocate memory for temporary array
    int *temp = malloc(max_digits * sizeof(int));
    if (!temp) {
        fprintf(stderr, "Memory allocation failed for temporary array.\n");
        return;
    }

    // Step 2: Initialize the temporary array
    memset(temp, 0, max_digits * sizeof(int));
    temp[0] = 1;  // Least significant digit set to 1 for starting multiplication
    
    size_t temp_size = 1;  // Start with one digit

    // Step 3 & 4: Loop over bignum_elem_t elements in v, least significant first
    for (int i = 0; i < num->length; ++i) {
        for (int bit = 0; bit < sizeof(bignum_elem_t) * 8; ++bit) {  // Loop over each bit
            if (num->v[i] & (1ULL << bit)) { // If the bit is set
                // Add the current value of temp to result for this bit position
                for (size_t j = 0; j < temp_size; j++) {
                    result[j] += temp[j];
                }
            }
            // If the bit isn't set, we do nothing

            // Double the temp array by shifting left one position in base 10
            double_array(temp, temp_size, temp, &temp_size);
        }
    }
    // we're done with temp
    free(temp);

    // Since we've only added values, each digit might exceed 10, so we normalize here
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
    int max_digits = bignum_max_base10_digits(num);
    int *num_array = malloc(max_digits * sizeof(int));
    size_t num_array_size = 0;
    if (!num_array) {
        fprintf(stderr, "Memory allocation failed for bignum_to_string num_array");
        return NULL;
    }
    bignum_to_int_array(num, num_array, &num_array_size);
    char *result = malloc((num_array_size + 1) * sizeof(char));
    if (result == NULL) {
        fprintf(stderr, "error allocating bignum string result");
        free(num_array);
    }
    result[num_array_size] = '\0';
    for (size_t i = 0; i < num_array_size; ++i) {
        size_t pos = num_array_size - 1 - i;
        result[pos] = num_array[i] + '0';
    }
/*
    printf("num_array_size: %zu\n", num_array_size);
    for (size_t i = 0; i < num_array_size; ++i) {
        printf("  %zu: %d\n", i, num_array[i]);
    }
*/
    free(num_array);
    return result;
}

// Example usage:
int main() {
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
