#ifndef TFM_OPENCL_H
#define TFM_OPENCL_H
/* tfm.h */

/* min / max */
#ifndef MIN
   #define MIN(x,y) ((x)<(y)?(x):(y))
#endif
#ifndef MAX
   #define MAX(x,y) ((x)>(y)?(x):(y))
#endif

/* signs */
#define FP_ZPOS     0
#define FP_NEG      1

/* return codes */
#define FP_OKAY     0
#define FP_VAL      1
#define FP_MEM      2

/* equalities */
#define FP_LT        -1   /* less than */
#define FP_EQ         0   /* equal to */
#define FP_GT         1   /* greater than */

/* replies */
#define FP_YES        1   /* yes response */
#define FP_NO         0   /* no response */

#ifndef FP_MAX_SIZE
//   #define FP_MAX_SIZE           (4096+(8*DIGIT_BIT))
#define FP_MAX_SIZE 128
#endif

#define CHAR_BIT 8                   /* bits per unsigned char */
#ifdef __OPENCL_VERSION__
typedef unsigned long ulong64;
#else
typedef unsigned long long ulong64;
#endif
typedef unsigned int       fp_digit; /* storage units */
typedef ulong64            fp_word;  /* calculation units. Must be twice size of fp_digit */

/* # of digits this is */
#define DIGIT_BIT  (int)((CHAR_BIT) * sizeof(fp_digit)) /* bits per fp_digit */
#define FP_MASK    (fp_digit)(-1)                       /* 2^DIGIT_BIT - 1 */
#define FP_SIZE    (FP_MAX_SIZE/DIGIT_BIT)              /* size of "dp" storage array */

/* a FP type */
typedef struct {
    fp_digit dp[FP_SIZE];
    int      used,
             sign;
} fp_int;

/* initialize [or zero] an fp int */
#ifdef __OPENCL_VERSION__
/* OPENCL */
#define NULL 0
#define REGISTER
unsigned long strlen(const char *str) {
    int length = 0;
    while (str[length] != '\0') {
        length++;
    }
    return length;
}
int isdigit(char c);
int isdigit(char c) {
    return (c >= '0' && c <= '9');
}
#define fprintf(stream, format) ((void)0)
#define fp_init(a) { \
    for (int i = 0; i < FP_SIZE; i++) { \
        (a)->dp[i] = 0; \
    } \
    (a)->used = 0; \
    (a)->sign = 0; \
}
void fp_copy(const fp_int *a, fp_int *b);
void fp_copy(const fp_int *a, fp_int *b) {
    if (a != b) {
        for (int i = 0; i < a->used; ++i) {
            b->dp[i] = a->dp[i];
        }
        b->sign = a->sign;
        b->used = a->used;
    }
}
#else
/* REGULAR C */
/* zero out fp_int */
#define REGISTER register
#define fp_init(a)  (void)memset((a), 0, sizeof(fp_int))
/* copy from a to b */
#define fp_copy(a, b)      (void)(((a) != (b)) && memcpy((b), (a), sizeof(fp_int)))
#endif
#define fp_zero(a)  fp_init(a)


/* clamp digits */
#define fp_clamp(a)   { while ((a)->used && (a)->dp[(a)->used-1] == 0) --((a)->used); (a)->sign = (a)->used ? (a)->sign : FP_ZPOS; }

/* negate and absolute */
#define fp_neg(a, b)  { fp_copy(a, b); (b)->sign ^= 1; fp_clamp(b); }
#define fp_abs(a, b)  { fp_copy(a, b); (b)->sign  = 0; }

/* right shift x digits */
void fp_rshd(fp_int *a, int x);

/* left shift x digits */
void fp_lshd(fp_int *a, int x);

/* signed comparison */
int fp_cmp(const fp_int *a, const fp_int *b);

/* c = a + b */
void fp_add(const fp_int *a, const fp_int *b, fp_int *c);

/* c = a - b */
void fp_sub(const fp_int *a, const fp_int *b, fp_int *c);

/* c = a * b */
void fp_mul(const fp_int *a, const fp_int *b, fp_int *c);

/* b = a*a  */
void fp_sqr(const fp_int *a, fp_int *b);

/* unsigned comparison */
int fp_cmp_mag(const fp_int *a, const fp_int *b);

/**** tfm_private functions ****/
/* unsigned addition */
void s_fp_add(const fp_int *a, const fp_int *b, fp_int *c);
/* unsigned subtraction ||a|| >= ||b|| ALWAYS! */
void s_fp_sub(const fp_int *a, const fp_int *b, fp_int *c);

// Function to set an fp_int from a base-10 string
void fp_from_radix(fp_int *a, const char *str);

// Function to multiply an fp_int by a single digit
void fp_mul_d(fp_int *a, fp_digit b, fp_int *c);

// Function to add a single digit to an fp_int
void fp_add_d(fp_int *a, fp_digit b, fp_int *c);

/* c = a / 2**b */
void fp_div_2d(const fp_int *a, int b, fp_int *c, fp_int *d);

/* c = a * 2**d */
void fp_mul_2d(const fp_int *a, int b, fp_int *c);

/* c = a mod 2**d */
void fp_mod_2d(const fp_int *a, int b, fp_int *c);

/**** Begin Floating Point functions *****/
/* FP numbers are represented as int( num * 2**FP_SCALE_BITS )
 */
#define FP_SCALE_BITS 64

/* converting from double to fp
 * take the integer portion and create fp_int
 * subtract that from value, etc.
 */
void fp_from_double(fp_int *result, double value);
double fp_to_double(fp_int *num);
void fp_from_float(fp_int *result, float value);
float fp_to_float(fp_int *num);
#endif
