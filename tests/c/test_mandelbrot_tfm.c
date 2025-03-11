/*
 * Tests for (a copy of) the OpenCL kernel code
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "tfm_opencl.h"
#include "bignum_test_framework.h"

#define __kernel static
#define __global

static int global_x = 0;
static int global_y = 0;
int get_global_id(int dimension) {
    if (dimension == 0) {
        return global_x;
    } else if (dimension == 1) {
        return global_y;
    } else {
        return 0;
    }
}

void double_to_fp_int_array(double val, uint64_t *val_hi, uint64_t *val_lo) {
    int sign = (val < 0);
    val = fabs(val);
    fp_int val_fp;
    fp_from_double(&val_fp, val);
    *val_hi = val_fp.dp[2];
    *val_lo = ((uint64_t) val_fp.dp[1] << 32) + val_fp.dp[0];
    if (sign) {
        *val_hi = *val_hi | 0x8000000000000000;
    } else {
        *val_hi = *val_hi & ~0x8000000000000000;
    }
}

void fp_from_hi_lo(fp_int *dest, uint64_t val_hi, uint64_t val_lo);
void fp_from_hi_lo(fp_int *dest, uint64_t val_hi, uint64_t val_lo) {
    dest->used = 3;
    dest->sign = (val_hi >> 63) ? FP_NEG : FP_ZPOS;
    dest->dp[2] = val_hi & FP_MASK;
    dest->dp[1] = val_lo >> 32;
    dest->dp[0] = val_lo & FP_MASK;
    fp_clamp(dest);
}    

/* output is an array of uint8 with size width * height * 3
 * palette is an array of uint8 with size maxiter * 3
 */
__kernel void mandelbrot(__global char *output,
                         __global char *palette,
                         const int maxiter, const float horizon_squared, const int width, const int height,
                         const uint64_t xmin_hi, const uint64_t xmin_lo,
                         const uint64_t ymin_hi, const uint64_t ymin_lo,
                         const uint64_t step_size_hi, const uint64_t step_size_lo
    ) {
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    int i = 0;
    // fmaf(a, b, c) is equivalent to (a * b) + c, but with better rounding
    fp_int z_real, z_imag, z_real_squared, z_imag_squared, c_real, c_imag, temp_fp, horizon_squared_fp;
    //
    fp_from_float(&horizon_squared_fp, horizon_squared);
    // set c_real to step_size
    fp_from_hi_lo(&c_real, step_size_hi, step_size_lo);
    
    // copy step_size to c_imag (faster than re-parse float)
    fp_copy(&c_real, &c_imag);
    // c_real = step_size * x
    fp_mul_d(&c_real, x, &c_real);
    // c_real = step_size * x + xmin
    fp_from_hi_lo(&temp_fp, xmin_hi, xmin_lo);
    fp_add(&c_real, &temp_fp, &c_real);
    // c_imag = step_size * y
    fp_mul_d(&c_imag, y, &c_imag);
    // c_imag = (step_size * y) + ymin
    fp_from_hi_lo(&temp_fp, ymin_hi, ymin_lo);
    fp_add(&c_imag, &temp_fp, &c_imag);

    fp_zero(&z_real);
    fp_zero(&z_imag);
    fp_zero(&z_real_squared);
    fp_zero(&z_imag_squared);

    for(i = 0; i < maxiter; i++) {
        // check (z_real_squared + z_imag_squared) < horizon_squared
        fp_add(&z_real_squared, &z_imag_squared, &temp_fp);
        if (fp_cmp(&temp_fp, &horizon_squared_fp) == FP_GT) {  break; }

        // z_imag = 2.0 * z_real * z_imag + c_imag
        fp_mul_2d(&z_real, 1, &temp_fp);  // temp = z_real * 2
        fp_mul_scaled(&temp_fp, &z_imag, &temp_fp); // temp = temp * z_imag
        fp_add(&temp_fp, &c_imag, &z_imag);  // z_imag = temp + c_imag

        // z_real = z_real_squared - z_imag_squared + c_real
        fp_sub(&z_real_squared, &z_imag_squared, &temp_fp);  // temp = z_real_squared - z_imag_squared
        fp_add(&temp_fp, &c_real, &z_real);  // z_real = temp + c_real

        // z_real_squared = z_real * z_real
        fp_mul_scaled(&z_real, &z_real, &z_real_squared);
        // z_imag_squared = z_imag * z_imag
        fp_mul_scaled(&z_imag, &z_imag, &z_imag_squared);
    }
    // debug
    double real_fp64 = fp_to_double(&c_real);
    double imag_fp64 = fp_to_double(&c_imag);
    printf("x: %d  y: %d  i: %d  real: %lf  imag: %lf\n", x, y, i, real_fp64, imag_fp64);
    
     // 8-bit rgb output
    // the output buffer is displayed buffer[0] = top
    // so we use (height - y - 1) for indexing
    int row = height - y - 1;
    int row_width = width * 3;  // 3 channels, one byte each
    int col_pos = x * 3;
    int o_ix = row * row_width + col_pos;
    //int value = (i * 255) / maxiter;
    if (i == maxiter) {
        output[o_ix] = 0;
        output[o_ix + 1] = 0;
        output[o_ix + 2] = 0;
    } else {
        output[o_ix] = palette[i*3 + 0];
        output[o_ix + 1] = palette[i*3 + 1];
        output[o_ix + 2] = palette[i*3 + 2];
    }
}

/*
 * simulate the parallel opencl kernel calls
 * this updates global_x and global_y used by get_global_id()
 */
void call_mandelbrot(
    char *output,
    char *palette,
    const int maxiter,
    const float horizon_squared,
    const int width,
    const int height,
    const uint64_t xmin_hi, const uint64_t xmin_lo,
    const uint64_t ymin_hi, const uint64_t ymin_lo,
    const uint64_t step_size_hi, const uint64_t step_size_lo
    ) {
    for (int y = 0; y < height; ++y) {
        global_y  = y;
        for (int x = 0; x < width; ++x) {
            global_x = x;
            mandelbrot(
                output, palette, maxiter, horizon_squared, width, height,
                xmin_hi, xmin_lo,
                ymin_hi, ymin_lo,
                step_size_hi, step_size_lo);
        }
    }
}


DEFINE_TEST(mbtest_mandelbrot_kernel_1) {
    int width = 3;
    int height = 3;
    int maxiter = 255;
    double xmin = -2.0;
    double ymin = -1.5;
    double step_size = 0.00375;
    float horizon_squared = 4.0f;
    int output_buffer_size = width * height * 3;
    int palette_buffer_size = maxiter * 3;
    char *output = (char*) malloc(output_buffer_size);
    char *palette = (char*) malloc(palette_buffer_size);
    fp_int xmin_fp, ymin_fp, step_size_fp;
    fp_from_double(&xmin_fp, xmin);
    fp_from_double(&ymin_fp, ymin);
    fp_from_double(&step_size_fp, step_size);
    uint64_t xmin_hi, xmin_lo, ymin_hi, ymin_lo, step_size_hi, step_size_lo;
    double_to_fp_int_array(xmin, &xmin_hi, &xmin_lo);
    double_to_fp_int_array(ymin, &ymin_hi, &ymin_lo);
    double_to_fp_int_array(step_size, &step_size_hi, &step_size_lo);


    call_mandelbrot(
        output,
        palette,
        maxiter,
        horizon_squared,
        width,
        height,
        xmin_hi, xmin_lo,
        ymin_hi, ymin_lo,
        step_size_hi, step_size_lo
        );
    int index = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int r = output[index++];
            int g = output[index++];
            int b = output[index++];
            printf("(%d, %d): %d %d %d\n", x, y, r, g, b);
        }
    }
}
DEFINE_TEST(mbtest_mandelbrot_kernel_2) {
    int width = 3;
    int height = 3;
    int maxiter = 5000;
    float horizon_squared = 4.0f;
    int output_buffer_size = width * height * 3;
    int palette_buffer_size = maxiter * 3;
    char *output = (char*) malloc(output_buffer_size);
    char *palette = (char*) malloc(palette_buffer_size);
    for (int i = 0; i < maxiter; ++i) {
        float v = 255.0 / (float) maxiter * i;
        int grey = (int)v;
        // printf("i: %d  v: %f  grey: %d\n", i, v, grey);
        palette[i * 3] = grey;
        palette[i * 3 + 1] = grey;
        palette[i * 3 + 2] = grey;
    }    
    uint64_t xmin_hi, xmin_lo, ymin_hi, ymin_lo, step_size_hi, step_size_lo;
    // 2.7498992666297184e-12
    step_size_hi = 0UL;
    step_size_lo = 50726688UL;
    // -1.2534398211609
    xmin_hi = 9223372036854775809UL;
    xmin_lo = 4675139519041839104UL;
    // 0.38469586368246556
    ymin_hi = 0UL;
    ymin_lo = 7096386143565099008UL;

    call_mandelbrot(
        output,
        palette,
        maxiter,
        horizon_squared,
        width,
        height,
        xmin_hi, xmin_lo,
        ymin_hi, ymin_lo,
        step_size_hi, step_size_lo
        );
    int index = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int r = output[index++];
            int g = output[index++];
            int b = output[index++];
            printf("(%d, %d): %d %d %d\n", x, y, r, g, b);
        }
    }
}

int test_mandelbrot_tfm_main() {
    printf("HELLO from test_mandelbrot_tfm.c\n");
    btl_run_tests();
    return 0;
}
