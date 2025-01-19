// Define the double structure
typedef struct DoubleStruct {
    float high;
    float low;
} dblfloat;

__constant float FLOAT_SPLIT = 16777216.0f;  // 2^24

// Function to split a float into high and low parts
static dblfloat split_float(float value) {
    dblfloat result;
    // Round to nearest representable float for high part
    result.high = rint(value * FLOAT_SPLIT) / FLOAT_SPLIT;
    
    // Calculate low part as the difference
    result.low = value - result.high;
    return result;
}

// Function to multiply two 'doubles' (split into high and low parts) using FP32 operations
static dblfloat double_multiply(dblfloat a, dblfloat b) {
    // https://stackoverflow.com/questions/29344800/emulating-fp64-with-2-fp32-on-a-gpu
    dblfloat t, z;
    float sum;
    t.high = a.high * b.high;
    t.low = fmaf (a.high, b.high, -t.high);
    t.low = fmaf (a.low, b.low, t.low);
    t.low = fmaf (a.high, b.low, t.low);
    t.low = fmaf (a.low, b.high, t.low);
    /* normalize result */
    sum = t.high + t.low;
    z.low = (t.high - sum) + t.low;
    z.high = sum;
    return z;
}    

static dblfloat double_add(dblfloat a, dblfloat b) {
    dblfloat result;
    
    // Add high parts
    float high_sum = a.high + b.high;
    
    // Add low parts
    float low_sum = a.low + b.low;
    
    // If low_sum overflows or underflows, we need to carry over to high_sum
    float tmp = high_sum;
    high_sum += low_sum;
    // Check if we've overflowed or underflowed in high_sum due to low_sum addition
    if (fabs(high_sum) < fabs(tmp)) { // Overflow or underflow detected
        // Adjust by renormalizing
        float correction = (high_sum - tmp) - low_sum;
        low_sum -= correction;
    }
    
    // Store result
    result.high = high_sum;
    result.low = low_sum;
    return result;
}

static dblfloat double_minus(dblfloat a, dblfloat b) {
    b.high = -b.high;
    b.low = -b.low;
    return double_add(a, b);
}

__kernel void k_dblfloat_multiply(__global float *output, const float a_high, const float a_low,
                                 const float b_high, const float b_low) {
    const int x = get_global_id(0);
    dblfloat a = {a_high, a_low};
    dblfloat b = {b_high, b_low};
    dblfloat result = double_multiply(a, b);
    output[0] = result.high;
    output[1] = result.low;
}

__kernel void k_dblfloat_add(__global float *output, const float a_high, const float a_low,
                                 const float b_high, const float b_low) {
    const int x = get_global_id(0);
    dblfloat a = {a_high, a_low};
    dblfloat b = {b_high, b_low};
    dblfloat result = double_add(a, b);
    output[0] = result.high;
    output[1] = result.low;
}

__kernel void mandelbrot(__global char *output,
                         __global char *palette,
                         const int maxiter, const float horizon, const int width, const int height,
                         const float xmin_high, const float xmin_low,
                         const float ymin_high, const float ymin_low,
                         const float step_size_high, const float step_size_low) {
    const int x = get_global_id(0);
    const int y = get_global_id(1);

    const dblfloat xmin = {xmin_high, xmin_low};
    const dblfloat ymin = {ymin_high, ymin_low};
    const dblfloat step_size = {step_size_high, step_size_low};
    const dblfloat dblfloat_x = {x, 0.0f};
    const dblfloat dblfloat_y = {y, 0.0f};

    const dblfloat c_real = double_add(double_multiply(step_size, dblfloat_x), xmin);
    const dblfloat c_imag = double_add(double_multiply(step_size, dblfloat_y), ymin);

    dblfloat z_real = {0.0f, 0.0f};
    dblfloat z_imag = {0.0f, 0.0f};
    dblfloat z_real_squared = {0.0f, 0.0f};
    dblfloat z_imag_squared = {0.0f, 0.0f};
    const dblfloat two = {2.0f, 0.0f};
    int i;

    for(i = 0; i < maxiter; i++) {
        if(double_add(z_real_squared, z_imag_squared).high > horizon * horizon) break;
        z_imag = double_add(double_multiply(double_multiply(two, z_real), z_imag), c_imag);
        z_real = double_add(double_minus(z_real_squared, z_imag_squared), c_real);

        z_real_squared = double_multiply(z_real, z_real);
        z_imag_squared = double_multiply(z_imag, z_imag);
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
