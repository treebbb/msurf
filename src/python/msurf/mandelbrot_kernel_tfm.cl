
void fp_from_hi_lo(fp_int *dest, uint64_t val_hi, uint64_t val_lo);
void fp_from_hi_lo(fp_int *dest, uint64_t val_hi, uint64_t val_lo) {
    dest->used = 3;
    dest->sign = (val_hi >> 63) ? FP_NEG : FP_ZPOS;
    dest->dp[2] = val_hi & FP_MASK;
    dest->dp[1] = val_lo >> 32;
    dest->dp[0] = val_lo & FP_MASK;
    fp_clamp(dest);
}

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
    fp_int z_real, z_imag, z_real_squared, z_imag_squared, c_real, c_imag, temp_fp;
    // set c_real to step_size
    fp_from_hi_lo(&c_real, step_size_hi, step_size_lo);
    // copy step_size to c_imag (faster than re-parse float)
    fp_copy(&c_real, &c_imag);
    // c_real * x
    fp_mul_d(&c_real, x, &c_real);
    // c_real + xmin
    fp_from_hi_lo(&temp_fp, xmin_hi, xmin_lo);
    fp_add(&c_real, &temp_fp, &c_real);
    // c_imag * y
    fp_mul_d(&c_imag, y, &c_imag);
    // c_imag + ymin
    fp_from_hi_lo(&temp_fp, ymin_hi, ymin_lo);
    fp_add(&c_imag, &temp_fp, &c_imag);

    fp_zero(&z_real);
    fp_zero(&z_imag);
    fp_zero(&z_real_squared);
    fp_zero(&z_imag_squared);

    for(i = 0; i < maxiter; i++) {
        // this horizon_check may introduce artifacts compared to the float implementation
        fp_digit horizon_check = z_real_squared.dp[FP_SCALE_SHIFT_FP_DIGITS];
        horizon_check += z_imag_squared.dp[FP_SCALE_SHIFT_FP_DIGITS];
        if(horizon_check > horizon_squared)  break;

        //z_imag = 2.0f * z_real * z_imag + c_imag;
        // z_imag = FP_FMAF(FP_MUL2(z_real), z_imag, c_imag);
        fp_mul_d(&z_real, 2, &temp_fp);
        fp_mul_scaled(&temp_fp, &z_imag, &temp_fp);
        fp_add(&temp_fp, &c_imag, &z_imag);
        
        //z_real = FP_ADD(FP_SUB(z_real_squared, z_imag_squared), c_real);
        fp_sub(&z_real_squared, &z_imag_squared, &temp_fp);
        fp_add(&temp_fp, &c_real, &z_real);

        fp_mul_scaled(&z_real, &z_real, &z_real_squared);
        fp_mul_scaled(&z_imag, &z_imag, &z_imag_squared);
    }
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
