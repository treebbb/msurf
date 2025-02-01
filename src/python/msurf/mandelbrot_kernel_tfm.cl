// debug struct for passing info back to Python
typedef struct {
    int x;
    int y;
    float c_real;
    float c_imag;
    int i;
    float debug1;
    float debug2;
    float debug3;
    fp_digit i1;
    fp_digit i2;
    fp_digit i3;
    fp_digit i4;
    fp_digit i5;
} debug_info;

void fp_from_hi_lo(fp_int *dest, uint64_t val_hi, uint64_t val_lo);
void fp_from_hi_lo(fp_int *dest, uint64_t val_hi, uint64_t val_lo) {
    dest->used = 3;
    dest->sign = (val_hi >> 63) ? FP_NEG : FP_ZPOS;
    dest->dp[2] = val_hi & FP_MASK;
    dest->dp[1] = val_lo >> 32;
    dest->dp[0] = val_lo & FP_MASK;
    fp_clamp(dest);
}

void set_output_color(__global char *output, __global char *palette,
                      const int width, const int height,
                      const int x, const int y,
                      const int iter_count, const int maxiter);
void set_output_color(__global char *output, __global char *palette,
                      const int width, const int height,
                      const int x, const int y,
                      int iter_count, const int maxiter) {
    // 8-bit rgb output
    // the output buffer is displayed buffer[0] = top
    // so we use (height - y - 1) for indexing
    int row = height - y - 1;
    int row_width = width * 3;  // 3 channels, one byte each
    int col_pos = x * 3;
    int o_ix = row * row_width + col_pos;
    iter_count = min(iter_count, maxiter);  // cached values larger
    //int value = (i * 255) / maxiter;
    if (iter_count == maxiter) {
        output[o_ix] = 0;
        output[o_ix + 1] = 0;
        output[o_ix + 2] = 0;
    } else {
        output[o_ix] = palette[iter_count * 3 + 0];
        output[o_ix + 1] = palette[iter_count * 3 + 1];
        output[o_ix + 2] = palette[iter_count * 3 + 2];
    }
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
    int iter_count = 0;
    fp_int z_real, z_imag, z_real_squared, z_imag_squared, c_real, c_imag, temp_fp, horizon_squared_fp;
    // load current state
    size_t image_offset = width * height * 3;  // length of image data
    size_t arrpos = y * width + x;  // my index
    size_t state_offset = image_offset + 68 * arrpos;  // my state offset
    memcpy(&iter_count, output + state_offset, 4);  // restore iterator position
    if (iter_count == maxiter || iter_count < 0) {
        // we've already reached maxiter. No more processing
        set_output_color(output, palette, width, height, x, y, abs(iter_count), maxiter);
        return;
    }
    memcpy(&z_real, output + state_offset + 4, 32);  // fp_digit must be 32 bytes
    memcpy(&z_imag, output + state_offset + 36, 32); // dp[6]*uint32 + used (int32) + sign (int32)
    // end load current state

    // initialize the c_real and c_imag values based on x,y position
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

    // begin
    fp_sqr_scaled(&z_real, &z_real_squared);
    fp_sqr_scaled(&z_imag, &z_imag_squared);

    int found = 0;
    for(; iter_count < maxiter; iter_count++) {
        // check (z_real_squared + z_imag_squared) < horizon_squared
        fp_add(&z_real_squared, &z_imag_squared, &temp_fp);
        if (fp_cmp(&temp_fp, &horizon_squared_fp) == FP_GT) {
            found = 1;
            break;
        }

        // z_imag = 2.0 * z_real * z_imag + c_imag
        fp_mul_2d(&z_real, 1, &temp_fp);  // temp = z_real * 2
        fp_mul_scaled(&temp_fp, &z_imag, &temp_fp); // temp = temp * z_imag
        fp_add(&temp_fp, &c_imag, &z_imag);  // z_imag = temp + c_imag

        // z_real = z_real_squared - z_imag_squared + c_real
        fp_sub(&z_real_squared, &z_imag_squared, &temp_fp);  // temp = z_real_squared - z_imag_squared
        fp_add(&temp_fp, &c_real, &z_real);  // z_real = temp + c_real

        // z_real_squared = z_real * z_real
        fp_sqr_scaled(&z_real, &z_real_squared);
        // z_imag_squared = z_imag * z_imag
        fp_sqr_scaled(&z_imag, &z_imag_squared);
    }
    // debug
    /*
    if (iter_count == 100) {
        // crude debug
        int row2 = height - y - 1;
        int row_width2 = width * 3;  // 3 channels, one byte each
        int col_pos2 = x * 3;
        int o_ix2 = row2 * row_width2 + col_pos2;
        //int value = (i * 255) / maxiter;
        output[o_ix2] = 50;
        output[o_ix2 + 1] = 50;
        output[o_ix2 + 2] = 250;
        //int sentinel = fp_to_float(&z_real) * 1000;
        int sentinel = 99;
        memcpy(output + state_offset, &sentinel, 4);
        return;
    }
    */
    set_output_color(output, palette, width, height, x, y, iter_count, maxiter);

    // store state for next time
    if (found) {
        // mark as done
        iter_count = -iter_count;
    }
    memcpy(output + state_offset, &iter_count, 4);
    memcpy(output + state_offset + 4, &z_real, 32);
    memcpy(output + state_offset + 36, &z_imag, 32);
    // end store state

}
