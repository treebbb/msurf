// Function to split a float into high and low parts
//void split_float(float value, struct double *result) {
//    // Round to nearest representable float for high part
//    result->high = rintf(value * 1e8f) / 1e8f;
//    
//    // Calculate low part as the difference
//    result->low = value - result->high;
//}
//
//// Function to multiply two 'doubles' (split into high and low parts) using FP32 operations
//float double_multiply(struct double a, struct double b) {

__kernel void mandelbrot(__global char *output,
                         __global char *palette,
                         const int maxiter, const float horizon, const int width, const int height,
                         const float xmin, const float ymin, const float step_size) {
    const int x = get_global_id(0);
    const int y = get_global_id(1);

    const float c_real = xmin + step_size * x;
    const float c_imag = ymin + step_size * y;

    float z_real = 0.0f;
    float z_imag = 0.0f;
    float z_real_squared = 0.0f;
    float z_imag_squared = 0.0f;
    int i;

    for(i = 0; i < maxiter; i++) {
        if(z_real_squared + z_imag_squared > horizon * horizon) break;

        z_imag = 2.0f * z_real * z_imag + c_imag;
        z_real = z_real_squared - z_imag_squared + c_real;

        z_real_squared = z_real * z_real;
        z_imag_squared = z_imag * z_imag;
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
