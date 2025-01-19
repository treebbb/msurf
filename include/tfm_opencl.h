#pragma once
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
   #define FP_MAX_SIZE           (4096+(8*DIGIT_BIT))
#endif

#define CHAR_BIT 8
typedef unsigned long long ulong64;
typedef unsigned int       fp_digit;
typedef ulong64            fp_word;
#define SIZEOF_FP_DIGIT 4
#define DIGIT_SHIFT     5

/* # of digits this is */
#define DIGIT_BIT  ((CHAR_BIT) * SIZEOF_FP_DIGIT)
#define FP_MASK    (fp_digit)(-1)
#define FP_SIZE    (FP_MAX_SIZE/DIGIT_BIT)

/* a FP type */
typedef struct {
    fp_digit dp[FP_SIZE];
    int      used,
             sign;
} fp_int;

/* initialize [or zero] an fp int */
#define fp_init(a)  (void)memset((a), 0, sizeof(fp_int))
#define fp_zero(a)  fp_init(a)

/* copy from a to b */
#define fp_copy(a, b)      (void)(((a) != (b)) && memcpy((b), (a), sizeof(fp_int)))

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
