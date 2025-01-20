
/* math operators */
#define FP_FMAF(mul1, mul2, add1) mad(mul1, mul2, add1)
#define FP_SQR(mul1) mad(mul1, mul1, 0.0f)
#define FP_SUB(add1, sub1) add1 - sub1
#define FP_ADD(add1, sub1) add1 + sub1
#define FP_CMP(num1, num2) (num1 == num2) ? FP_EQ : (num1 > num2) ? FP_GT : FP_LT
#define FP_MUL2(num1) (num1 * 2.0f)  // multiply by 2


__kernel void mandelbrot(__global char *output,
                         __global char *palette,
                         const int maxiter, const float horizon, const int width, const int height,
                         const float xmin, const float ymin, const float step_size) {
    const int x = get_global_id(0);
    const int y = get_global_id(1);

    // fmaf(a, b, c) is equivalent to (a * b) + c, but with better rounding
    const float c_real = FP_FMAF(step_size, x, xmin);
    const float c_imag = FP_FMAF(step_size, y, ymin);

    float z_real = 0.0f;
    float z_imag = 0.0f;
    float z_real_squared = 0.0f;
    float z_imag_squared = 0.0f;
    int i;

    for(i = 0; i < maxiter; i++) {
        if(FP_CMP(FP_ADD(z_real_squared, z_imag_squared), FP_SQR(horizon * horizon)) == FP_GT)  break;

        //z_imag = 2.0f * z_real * z_imag + c_imag;
        z_imag = FP_FMAF(FP_MUL2(z_real), z_imag, c_imag);
        z_real = FP_ADD(FP_SUB(z_real_squared, z_imag_squared), c_real);

        z_real_squared = FP_SQR(z_real);
        z_imag_squared = FP_SQR(z_imag);
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
