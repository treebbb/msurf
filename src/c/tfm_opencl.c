#ifdef __OPENCL_VERSION__
// find some equivalent to our imports
#else
// normal C imports
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "tfm_opencl.h"
#endif

/* tfm.c */

/* ISO C code for COMBA macros */
#define COMBA_START
#define COMBA_CLEAR \
   c0 = c1 = c2 = 0;
#define COMBA_FORWARD \
   do { c0 = c1; c1 = c2; c2 = 0; } while (0);
#define COMBA_STORE(x) \
   x = c0;
#define COMBA_STORE2(x) \
   x = c1;
#define COMBA_FINI
#define MULADD(i, j)                                    \
   do { fp_word t;                                      \
   t = (fp_word)c0 + ((fp_word)i) * ((fp_word)j);       \
   c0 = t;                                              \
   t = (fp_word)c1 + (t >> DIGIT_BIT);                  \
   c1 = t;                                              \
   c2 += t >> DIGIT_BIT;                                \
   } while (0);

/* generic PxQ multiplier */
/* was fp_mul_comba */
void fp_mul(const fp_int *A, const fp_int *B, fp_int *C)
{
   int             ix, iy, iz, tx, ty, pa;
   const fp_digit *tmpx, *tmpy;
   fp_digit        c0, c1, c2;
   fp_int          tmp, *dst;

   COMBA_START;
   COMBA_CLEAR;

   /* get size of output and trim */
   pa = A->used + B->used;
   if (pa >= FP_SIZE) {
      pa = FP_SIZE-1;
   }

   if (A == C || B == C) {
      fp_zero(&tmp);
      dst = &tmp;
   } else {
      fp_zero(C);
      dst = C;
   }

   for (ix = 0; ix < pa; ix++) {
      /* get offsets into the two bignums */
      ty = MIN(ix, B->used-1);
      tx = ix - ty;

      /* setup temp aliases */
      tmpx = A->dp + tx;
      tmpy = B->dp + ty;

      /* this is the number of times the loop will iterrate, essentially its
         while (tx++ < a->used && ty-- >= 0) { ... }
       */
      iy = MIN(A->used-tx, ty+1);

      /* execute loop */
      COMBA_FORWARD;
      for (iz = 0; iz < iy; ++iz) {
          fp_digit _tmpx = *tmpx++;
          fp_digit _tmpy = *tmpy--;
          MULADD(_tmpx, _tmpy);
      }

      /* store term */
      COMBA_STORE(dst->dp[ix]);
  }
  COMBA_FINI;

  dst->used = pa;
  dst->sign = A->sign ^ B->sign;
  fp_clamp(dst);
  fp_copy(dst, C);
}

/* multiply and then scale to keep float scale consistent */
void fp_mul_scaled(const fp_int *a, const fp_int *b, fp_int *c) {
    fp_mul(a, b, c);
    fp_rshd(c, FP_SCALE_SHIFT_FP_DIGITS);
}

/* fp_cmp */
int fp_cmp(const fp_int *a, const fp_int *b)
{
   if (a->sign == FP_NEG && b->sign == FP_ZPOS) {
      return FP_LT;
   } else if (a->sign == FP_ZPOS && b->sign == FP_NEG) {
      return FP_GT;
   } else {
      /* compare digits */
      if (a->sign == FP_NEG) {
         /* if negative compare opposite direction */
         return fp_cmp_mag(b, a);
      } else {
         return fp_cmp_mag(a, b);
      }
   }
}


/* fp_cmp_mag */
int fp_cmp_mag(const fp_int *a, const fp_int *b)
{
   int x;

   if (a->used > b->used) {
      return FP_GT;
   } else if (a->used < b->used) {
      return FP_LT;
   } else {
      for (x = a->used - 1; x >= 0; x--) {
          if (a->dp[x] > b->dp[x]) {
             return FP_GT;
          } else if (a->dp[x] < b->dp[x]) {
             return FP_LT;
          }
      }
   }
   return FP_EQ;
}


/* fp_add.c */
void fp_add(const fp_int *a, const fp_int *b, fp_int *c)
{
  int     sa, sb;

  /* get sign of both inputs */
  sa = a->sign;
  sb = b->sign;

  /* handle two cases, not four */
  if (sa == sb) {
    /* both positive or both negative */
    /* add their magnitudes, copy the sign */
    c->sign = sa;
    s_fp_add (a, b, c);
  } else {
    /* one positive, the other negative */
    /* subtract the one with the greater magnitude from */
    /* the one of the lesser magnitude.  The result gets */
    /* the sign of the one with the greater magnitude. */
    if (fp_cmp_mag (a, b) == FP_LT) {
      c->sign = sb;
      s_fp_sub (b, a, c);
    } else {
      c->sign = sa;
      s_fp_sub (a, b, c);
    }
  }
}

/* fp_sub.c */
/* c = a - b */
void fp_sub(const fp_int *a, const fp_int *b, fp_int *c)
{
  int     sa, sb;

  sa = a->sign;
  sb = b->sign;

  if (sa != sb) {
    /* subtract a negative from a positive, OR */
    /* subtract a positive from a negative. */
    /* In either case, ADD their magnitudes, */
    /* and use the sign of the first number. */
    c->sign = sa;
    s_fp_add (a, b, c);
  } else {
    /* subtract a positive from a positive, OR */
    /* subtract a negative from a negative. */
    /* First, take the difference between their */
    /* magnitudes, then... */
    if (fp_cmp_mag (a, b) != FP_LT) {
      /* Copy the sign from the first */
      c->sign = sa;
      /* The first has a larger or equal magnitude */
      s_fp_sub (a, b, c);
    } else {
      /* The result has the *opposite* sign from */
      /* the first number. */
      c->sign = (sa == FP_ZPOS) ? FP_NEG : FP_ZPOS;
      /* The second has a larger magnitude */
      s_fp_sub (b, a, c);
    }
  }
}

/* s_fp_add.c */
/* unsigned addition */
void s_fp_add(const fp_int *a, const fp_int *b, fp_int *c)
{
  int      x, y, oldused;
  REGISTER fp_word  t;

  y       = MAX(a->used, b->used);
  oldused = MIN(c->used, FP_SIZE);
  c->used = y;

  t = 0;
  for (x = 0; x < y; x++) {
      t         += ((fp_word)a->dp[x]) + ((fp_word)b->dp[x]);
      c->dp[x]   = (fp_digit)t;
      t        >>= DIGIT_BIT;
  }
  if (t != 0 && x < FP_SIZE) {
     c->dp[c->used++] = (fp_digit)t;
     ++x;
  }

  c->used = x;
  for (; x < oldused; x++) {
     c->dp[x] = 0;
  }
  fp_clamp(c);
}

/* s_fp_sub.c */
/* unsigned subtraction ||a|| >= ||b|| ALWAYS! */
void s_fp_sub(const fp_int *a, const fp_int *b, fp_int *c)
{
  int      x, oldbused, oldused;
  fp_word  t;

  oldused  = c->used;
  oldbused = b->used;
  c->used  = a->used;
  t       = 0;
  for (x = 0; x < oldbused; x++) {
     t         = ((fp_word)a->dp[x]) - (((fp_word)b->dp[x]) + t);
     c->dp[x]  = (fp_digit)t;
     t         = (t >> DIGIT_BIT)&1;
  }
  for (; x < a->used; x++) {
     t         = ((fp_word)a->dp[x]) - t;
     c->dp[x]  = (fp_digit)t;
     t         = (t >> DIGIT_BIT)&1;
   }
  for (; x < oldused; x++) {
     c->dp[x] = 0;
  }
  fp_clamp(c);
}

// Function to set an fp_int from a base-10 string
void fp_from_radix(fp_int *a, const char *str) {
    // Initialize the fp_int structure
    fp_zero(a);

    // Determine if the number is negative
    int is_negative = (str[0] == '-');
    if (is_negative) {
        str++; // Skip the negative sign for further processing
    }

    // Iterate over each character in the string
    for (size_t i = 0; i < strlen(str); ++i) {
        // Ensure the character is a digit
        if (!isdigit(str[i])) {
            continue;
        }

        // Multiply the current value by 10
        fp_mul_d(a, 10, a);

        // Add the new digit
        fp_add_d(a, str[i] - '0', a);
    }

    // Set the sign of the result
    if (is_negative) {
        a->sign = FP_NEG;
    } else {
        a->sign = FP_ZPOS;
    }
}

// Function to multiply an fp_int by a single digit
void fp_mul_d(fp_int *a, fp_digit b, fp_int *c) {
    fp_int temp;
    int sign = a->sign;
    fp_zero(&temp);
    fp_digit carry = 0;
    for (int i = 0; i < a->used; ++i) {
        fp_word prod = (fp_word)a->dp[i] * b + carry;
        temp.dp[i] = (fp_digit)(prod & FP_MASK);
        carry = (fp_digit)(prod >> DIGIT_BIT);
    }
    if (carry != 0) {
        temp.dp[a->used] = carry;
        temp.used = a->used + 1;
    } else {
        temp.used = a->used;
    }
    temp.sign = sign;
    fp_copy(&temp, c);
}

// Function to add a single digit to an fp_int
void fp_add_d(fp_int *a, fp_digit b, fp_int *c) {
    int i;
    fp_word sum = b;

    for (i = 0; i < a->used; ++i) {
        sum = (fp_word)a->dp[i] + sum;
        c->dp[i] = (fp_digit) sum;
        sum >>= DIGIT_BIT;
    }
    if (sum) {
        c->dp[i++] = (fp_digit) sum;
    }
    c->used = i;
    fp_clamp(c);
}

// shift digits left
void fp_lshd(fp_int *a, int x)
{
   int y;

   /* move up and truncate as required */
   y = MIN(a->used + x - 1, (int)(FP_SIZE-1));

   /* store new size */
   a->used = y + 1;

   /* move digits */
   for (; y >= x; y--) {
       a->dp[y] = a->dp[y-x];
   }

   /* zero lower digits */
   for (; y >= 0; y--) {
       a->dp[y] = 0;
   }

   /* clamp digits */
   fp_clamp(a);
}

// shift digits right
void fp_rshd(fp_int *a, int x)
{
  int y;

  /* too many digits just zero and return */
  if (x >= a->used) {
     fp_zero(a);
     return;
  }

   /* shift */
   for (y = 0; y < a->used - x; y++) {
      a->dp[y] = a->dp[y+x];
   }

   /* zero rest */
   for (; y < a->used; y++) {
      a->dp[y] = 0;
   }

   /* decrement count */
   a->used -= x;
   fp_clamp(a);
}

/* c = a * 2**d */
void fp_mul_2d(const fp_int *a, int b, fp_int *c)
{
   fp_digit carry, carrytmp, shift;
   int x;

   /* copy it */
   fp_copy(a, c);

   /* handle whole digits */
   if (b >= DIGIT_BIT) {
      fp_lshd(c, b/DIGIT_BIT);
   }
   b %= DIGIT_BIT;

   /* shift the digits */
   if (b != 0) {
      carry = 0;
      shift = DIGIT_BIT - b;
      for (x = 0; x < c->used; x++) {
          carrytmp = c->dp[x] >> shift;
          c->dp[x] = (c->dp[x] << b) + carry;
          carry = carrytmp;
      }
      /* store last carry if room */
      if (carry && x < FP_SIZE) {
         c->dp[c->used++] = carry;
      }
   }
   fp_clamp(c);
}

/* c = a / 2**b */
void fp_div_2d(const fp_int *a, int b, fp_int *c, fp_int *d)
{
  fp_digit D, r, rr;
  int      x;
  fp_int   t;

  /* if the shift count is <= 0 then we do no work */
  if (b <= 0) {
    fp_copy (a, c);
    if (d != NULL) {
      fp_zero (d);
    }
    return;
  }

  fp_zero(&t);

  /* get the remainder */
  if (d != NULL) {
    fp_mod_2d (a, b, &t);
  }

  /* copy */
  fp_copy(a, c);

  /* shift by as many digits in the bit count */
  if (b >= (int)DIGIT_BIT) {
    fp_rshd (c, b / DIGIT_BIT);
  }

  /* shift any bit count < DIGIT_BIT */
  D = (fp_digit) (b % DIGIT_BIT);
  if (D != 0) {
    REGISTER fp_digit *tmpc, mask, shift;

    /* mask */
    mask = (((fp_digit)1) << D) - 1;

    /* shift for lsb */
    shift = DIGIT_BIT - D;

    /* alias */
    tmpc = c->dp + (c->used - 1);

    /* carry */
    r = 0;
    for (x = c->used - 1; x >= 0; x--) {
      /* get the lower  bits of this word in a temp */
      rr = *tmpc & mask;

      /* shift the current word and mix in the carry bits from the previous word */
      *tmpc = (*tmpc >> D) | (r << shift);
      --tmpc;

      /* set the carry to the carry bits of the current word found above */
      r = rr;
    }
  }
  fp_clamp (c);
  if (d != NULL) {
    fp_copy (&t, d);
  }
}

/* c = a mod 2**d */
void fp_mod_2d(const fp_int *a, int b, fp_int *c)
{
   int x;

   /* zero if count less than or equal to zero */
   if (b <= 0) {
      fp_zero(c);
      return;
   }

   /* get copy of input */
   fp_copy(a, c);

   /* if 2**d is larger than we just return */
   if (b >= (DIGIT_BIT * a->used)) {
      return;
   }

  /* zero digits above the last digit of the modulus */
  for (x = (b / DIGIT_BIT) + ((b % DIGIT_BIT) == 0 ? 0 : 1); x < c->used; x++) {
    c->dp[x] = 0;
  }
  /* clear the digit that is not completely outside/inside the modulus */
  c->dp[b / DIGIT_BIT] &= ~((fp_digit)0) >> (DIGIT_BIT - b);
  fp_clamp (c);
}

void fp_from_double_naive(fp_int *result, double value) {
    fp_word shift = 1UL << DIGIT_BIT;
    fp_zero(result);
    if (value < 0) {
        result->sign = FP_NEG;
        value = -value;
    }
    if (value > FP_MASK) {
        fprintf(stderr, "double must be less than 2^32");
    }
    int pos = FP_SCALE_SHIFT_FP_DIGITS;
    result->used = pos + 1;
    while (pos >= 0 && value > 0.0) {
        result->dp[pos] = (fp_digit) value;
        //printf("pos(0): %d  dp[pos]: %d\n", pos, result->dp[pos]);
        value = value - result->dp[pos];
        //printf("pos(1): %d  value: %f\n", pos, value);
        value *= shift;
        //printf("pos(2): %d  value: %f\n", pos, value);
        --pos;
    }
    fp_clamp(result);
}

/* Fraction is in the lowest 52 bits.
   fraction value = 1 + Fraction / (1<<52) */
#define DOUBLE_FRACTION_MASK 0x000FFFFFFFFFFFFF // lowest 52 bits
#define DOUBLE_FRACTION_ONE  0x0010000000000000 // bit 53 (1 + fraction)
#define DOUBLE_FRACTION_VALUE_BITS 52

/* exponent is stored in bits 1-12
 * abs(value) = (fraction val) * 2**(exponent - DOUBLE_BIAS)
 */
#define DOUBLE_BIAS 1023
#define DOUBLE_EXPONENT_MASK 0x7FF0000000000000 // bits 1-12
#define DOUBLE_SIGN_MASK     0x8000000000000000 // sign bit
union DoubleBits {
    double d;
    uint64_t u64;
};
void fp_from_double(fp_int *result, double value) {
    // union to do bit operations
    union DoubleBits db;
    db.d = value;

    // Extract the fraction part
    uint64_t fraction = DOUBLE_FRACTION_ONE + (db.u64 & DOUBLE_FRACTION_MASK);

    // Extract the exponent part and unbias it
    long64 exponent = ((db.u64 & DOUBLE_EXPONENT_MASK) >> DOUBLE_FRACTION_VALUE_BITS) - DOUBLE_BIAS;

    long64 total_shift_left = exponent + (FP_SCALE_BITS - DOUBLE_FRACTION_VALUE_BITS);

    // now set fp_int
    fp_zero(result);
    int highest_bit = DOUBLE_FRACTION_VALUE_BITS + 1 + total_shift_left;
    int max_pos = highest_bit / DIGIT_BIT;
    if (max_pos < 0) {
        // return zero. nothing big enough to set into dp array
        return;
    }
    int shift_right = (max_pos * DIGIT_BIT) - total_shift_left;
    for (int i = max_pos; i >= 0; i--) {
        uint64_t val;
        if (shift_right > 0) {
            val = (fraction >> shift_right) & FP_MASK;
        } else if (shift_right < 0) {
            val = (fraction << (-shift_right)) & FP_MASK;
        } else {
            val = (fraction & FP_MASK);
        }
        result->dp[i] = val;
        //printf("  i: %d  val: %llu  dp[i]: %d  shift_right: %d \n", i, val, result->dp[i], shift_right);
        shift_right -= DIGIT_BIT;
    }
    result->used = max_pos + 1;
    result->sign = (db.u64 & DOUBLE_SIGN_MASK) ? FP_NEG : FP_ZPOS;
    fp_clamp(result);
}

double fp_to_double(fp_int *num) {
    fp_digit digit;
    double result = 0.0;
    double scale = 1.0;
    //printf("result: %f  scale: %f\n", result, scale);
    int count = 0;
    int max_used = FP_SCALE_SHIFT_FP_DIGITS;
    for (int i = max_used; i >=0; --i) {
        digit = (i >= num->used) ? 0 : num->dp[i];
        result += (digit * scale);
        scale /= (1UL << 32);
        //printf("i: %d  result: %f  scale: %f\n", i, result, scale);
        if (count++ > 20) {
            break;
        }
    }
    if (num->sign == FP_NEG) {
        result = - result;
    }
    return result;
}
#define FLOAT_FRACTION_MASK       0x007FFFFF // lowest 23 bits
#define FLOAT_FRACTION_ONE        0x00800000 // bit 24 (1 + fraction)
#define FLOAT_FRACTION_VALUE_BITS 23

#define FLOAT_BIAS                127
#define FLOAT_EXPONENT_MASK       0x7F800000 // bits 1-8
#define FLOAT_SIGN_MASK           0x80000000 // sign bit
union FloatBits {
    float f;
    uint32_t u32;
};

void fp_from_float(fp_int *result, float value) {
    // Union to do bit operations
    union FloatBits fb;
    fb.f = value;

    // Extract the fraction part
    uint32_t fraction = FLOAT_FRACTION_ONE + (fb.u32 & FLOAT_FRACTION_MASK);

    // Extract the exponent part and unbias it
    int32_t exponent = ((fb.u32 & FLOAT_EXPONENT_MASK) >> FLOAT_FRACTION_VALUE_BITS) - FLOAT_BIAS;

    // Compute the total shift left, adjusting for FP_SCALE_BITS and FLOAT_FRACTION_VALUE_BITS
    int64_t total_shift_left = exponent + (FP_SCALE_BITS - FLOAT_FRACTION_VALUE_BITS);

    // Now set fp_int
    fp_zero(result);
    int highest_bit = FLOAT_FRACTION_VALUE_BITS + 1 + total_shift_left;
    int max_pos = highest_bit / DIGIT_BIT;
    if (max_pos < 0) {
        // Return zero if nothing big enough to set into dp array
        return;
    }
    int shift_right = (max_pos * DIGIT_BIT) - total_shift_left;
    for (int i = max_pos; i >= 0; i--) {
        uint64_t val;
        if (shift_right > 0) {
            val = ((uint64_t)fraction >> shift_right) & FP_MASK;
        } else if (shift_right < 0) {
            val = ((uint64_t)fraction << (-shift_right)) & FP_MASK;
        } else {
            val = ((uint64_t)fraction & FP_MASK);
        }
        result->dp[i] = val;
        shift_right -= DIGIT_BIT;
    }
    result->used = max_pos + 1;
    result->sign = (fb.u32 & FLOAT_SIGN_MASK) ? FP_NEG : FP_ZPOS;
    fp_clamp(result);
}

float fp_to_float(fp_int *num) {
    fp_digit digit;
    float result = 0.0;
    float scale = 1.0;
    //printf("result: %f  scale: %f\n", result, scale);
    int count = 0;
    int max_used = FP_SCALE_SHIFT_FP_DIGITS;
    for (int i = max_used; i >=0; --i) {
        digit = (i >= num->used) ? 0 : num->dp[i];
        result += (digit * scale);
        scale /= (1UL << 32);
        //printf("i: %d  result: %f  scale: %f\n", i, result, scale);
        if (count++ > 20) {
            break;
        }
    }
    if (num->sign == FP_NEG) {
        result = - result;
    }
    return result;
}
