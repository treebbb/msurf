from MandelbrotParams import MandelbrotParams
import numpy as np
import os
import pyopencl as cl
import struct


OPENCL_DEBUG = False
MAX_MAXITER = (2 << 30) / 256 # 2^31 / 256. limited by 32 bit signed ints in opencl

DEBUG_INFO_SIZE = 52  # bytes
def parse_debug_info(array_slice):
    # Ensure the input is a numpy array of uint8
    #if not isinstance(array, np.ndarray) or array.dtype != np.uint8:
    #    raise ValueError("Input must be a numpy array of dtype uint8")

    # Convert the relevant part of the array to bytes
    byte_data = array_slice.tobytes()
    if len(byte_data) != DEBUG_INFO_SIZE:
        raise ValueError(f'Input must be a numpy array slice with bytesize {DEBUG_INFO_SIZE}')

    # Unpack the data according to the C struct format
    x, y, c_real, c_imag, i, d1, d2, d3, i1, i2, i3, i4, i5 = struct.unpack('ii ff i fff IIIII', byte_data)

    return (x, y, c_real, c_imag, i, d1, d2, d3, i1, i2, i3, i4, i5)

def double_to_fp_int_array(float_value):
    '''
    opencl doesn't always have a double type, but it has uint64.
    multiply the float_value by 2**64
    return two uint64 values (hi, lo)

    The top bit of hi is one if negative, zero otherwise
    fp_int.dp[3] = (hi >> 32) & 0x7FFFFFFF  // this should be 0 for mandelbrot
    fp_int.dp[2] = hi & FP_MASK
    fp_int.dp[1] = lo >> 32
    fp_int.dp[0] = lo & FP_MASK

    '''
    # Pack the float into bytes in IEEE 754 double-precision format
    sign = (float_value < 0)  # true if negative
    float_value = abs(float_value)
    intval = int(float_value * (1<<64))
    hi = intval >> 64
    lo = intval & ((1<<64) - 1)
    # If negative, set the MSB to 1
    if sign:
        # The mask for the MSB in a 64-bit integer is 0x8000000000000000
        hi |= 0x8000000000000000
    else:
        # If sign is False, clear the MSB
        hi &= ~0x8000000000000000
    return (hi, lo)

def mandelbrot_set(params: MandelbrotParams, horizon=2.0):
    xmin, xmax, ymin, ymax, xn, yn, maxiter = params.xmin, params.xmax, params.ymin, params.ymax, params.width, params.height, params.maxiter
    print(f'mandelbrot_set({params.get_params()})')
    # C is a 2d array of points on the real plane
    real = np.linspace(xmin, xmax, xn).astype(np.float64)
    imaginary = np.linspace(ymin, ymax, yn).astype(np.float64)
    complex = real[:, np.newaxis] + imaginary[np.newaxis, :] * 1j
    # N is the count of repetitions before reaching the horizon
    # shape [xn, yn]
    mandelbrot = np.zeros(complex.shape, dtype=np.int32)
    # Z is the calculation in the real plane, updated each repetition
    z = np.zeros_like(complex)
    # the point to sample to create the trace array
    for n in range(maxiter):
        points_in_bounds = np.where(np.abs(z) < horizon)
        z[points_in_bounds] = z[points_in_bounds]**2 + complex[points_in_bounds]
        mandelbrot[points_in_bounds] = n
    mandelbrot[mandelbrot == maxiter-1] = 0  # inside black
    # mandelbrot[np.abs(z) <= horizon] = maxiter  # inside white
    return mandelbrot

class MandelbrotFuncs:
    use_tfm = 1  # high precision TFM library

    def __init__(self):
        self.init_opencl()

    def init_opencl(self):
        # Create OpenCL context and command queue
        self.ctx = cl.create_some_context()
        self.queue = cl.CommandQueue(self.ctx)
        # OpenCL kernel code
        dir_path = os.path.dirname(os.path.realpath(__file__))
        if self.use_tfm:
            tfm_code = open('include/tfm_opencl.h', 'r').read()
            tfm_code += open('src/c/tfm_opencl.c', 'r').read()
            kernel_code_path = os.path.join(dir_path, 'mandelbrot_kernel_tfm.cl')
            kernel_src = tfm_code + open(kernel_code_path, 'r').read()
        else:
            kernel_code_path = os.path.join(dir_path, 'mandelbrot_kernel.cl')
            kernel_src = open(kernel_code_path, 'r').read()
        # Compile the kernel
        self.prg = cl.Program(self.ctx, kernel_src).build()


    # export PYOPENCL_CTX='0:1'
    def mandelbrot_set_opencl(self, params: MandelbrotParams, horizon=2.0):
        '''
        returns a numpy array with shape=(params.width, params.height, 3) and dtype=np.int8
        '''
        xmin, xmax, ymin, ymax, xn, yn, maxiter = \
            params.xmin, params.xmax, params.ymin, params.ymax, params.width, params.height, params.maxiter
        if maxiter >= MAX_MAXITER:
            print(f'WARNING: maxiter: {maxiter} greater than limit {MAX_MAXITER}. reducing to limit')
            maxiter = MAX_MAXITER
        # cache the mandelbrot array so we don't have to reallocate it unless width/height changes
        #### !!! mbc = params._mandelbrot_cache
        mbc = None
        if mbc is None or mbc[0] != xn or mbc[1] != yn:
            mandelbrot = np.zeros((yn, xn, 3), dtype=np.uint8)
            params._mandelbrot_cache = (xn, yn, mandelbrot)
        else:
            mandelbrot = mbc[2]

        # Prepare data
        palette = params.iter_to_color()  # shape=(maxiter,3) dtype=np.uint8
        image_array = np.full((xn, yn, 3), (50, 50, 255), dtype=np.uint8)

        # Allocate memory on the GPU
        output_buf_size = xn * yn * 3  # width * height * 3 channels * sizeof(uint8)
        if OPENCL_DEBUG:
            debug_size = DEBUG_INFO_SIZE * (xn * yn);
            output_buf_size = output_buf_size + debug_size;
        output_buf = cl.Buffer(self.ctx, cl.mem_flags.WRITE_ONLY, output_buf_size)
        c_palette = cl.Buffer(self.ctx, cl.mem_flags.READ_ONLY | cl.mem_flags.COPY_HOST_PTR, hostbuf=palette)

        # Copy the NumPy array to the OpenCL buffer
        event = cl.enqueue_copy(self.queue, output_buf, image_array)

        # Wait for the copy operation to complete if necessary
        event.wait()


        # Execute the kernel
        shape = (xn, yn)
        step_size = (xmax - xmin) / xn
        if self.use_tfm:
            # call tfm with high-precision xmin, ymin, step_size
            step_size_hi, step_size_lo = double_to_fp_int_array(step_size)
            xmin_hi, xmin_lo = double_to_fp_int_array(xmin)
            ymin_hi, ymin_lo = double_to_fp_int_array(ymin)
            print(step_size, step_size_hi, step_size_lo)
            print(xmin, xmin_hi, xmin_lo)
            print(ymin, ymin_hi, ymin_lo)
            print(f'mandelbrot_set_opencl: step_size: {step_size:.8g}')
            self.prg.mandelbrot(self.queue, shape, None, output_buf, c_palette,
                                np.int32(maxiter), np.float32(horizon*horizon), np.int32(xn), np.int32(yn),
                                np.uint64(xmin_hi), np.uint64(xmin_lo),
                                np.uint64(ymin_hi), np.uint64(ymin_lo),
                                np.uint64(step_size_hi), np.uint64(step_size_lo),
                                )
        else:
            # call non-tfm with float32 for xmin, ymin, step_size
            self.prg.mandelbrot(self.queue, shape, None, output_buf, c_palette,
                                np.int32(maxiter), np.float32(horizon*horizon), np.int32(xn), np.int32(yn),
                                np.float32(xmin), np.float32(ymin), np.float32(step_size)
                                )

        # Read results back to host
        cl.enqueue_copy(self.queue, mandelbrot, output_buf)
        # DEBUG
        if OPENCL_DEBUG:
            debug_array = np.zeros((xn,yn,DEBUG_INFO_SIZE), dtype=np.uint8)
            cl.enqueue_copy(self.queue, debug_array, output_buf, src_offset=(xn * yn * 3))
            for i in range(100):
                x, y, c_real, c_imag, i, d1, d2, d3, i1, i2, i3, i4, i5 = parse_debug_info(debug_array[i][0])
                print(f'({x}, {y}): {c_real}, {c_imag} => {i}  d1: {d1}  d2: {d2}  d3: {d3}')
                print(f'  {i1} {i2} {i3} {i4} {i5}')
        # Cleanup
        output_buf.release()
        # Return
        return mandelbrot

    def mandelbrot_image(self, params):
        #mandelbrot = mandelbrot_set(params)
        mandelbrot = self.mandelbrot_set_opencl(params)
        normalized = (mandelbrot / params.maxiter * 255).astype(np.uint8)
        normalized = np.rot90(normalized, k=1)
        #grayscale_as_rgb = np.repeat(normalized[:, :, np.newaxis], 3, axis=2)
        #return grayscale_as_rgb
        return normalized

def generate_sample(filename):
    m = MandelbrotFuncs()
    #image_dims: (int,int), mbrot_center: (float, float) = (-0.5,0), mbrot_width: float = 2.0):
    mbrot_center = (-0.5, 0)
    mbrot_width = 2.0
    image_dims = (1200, 800)
    mbrot_height = mbrot_width * image_dims[1] / image_dims[0]
    xmin, xmax, xn = mbrot_center[0] - (mbrot_width / 2), mbrot_center[0] + (mbrot_width / 2), image_dims[0]
    ymin, ymax, yn = mbrot_center[1] - (mbrot_height / 2), mbrot_center[1] + (mbrot_height / 2), image_dims[1]
    maxiter = 200
    params = MandelbrotParams(xmin, xmax, ymin, ymax, xn, yn, maxiter)
    image_array = m.mandelbrot_image(params)
    from PIL import Image, ImageTk
    image = Image.fromarray(image_array, 'RGB')
    image.save(filename, format='JPEG', quality=95)
    return params

def test_zoom(params, filename):
    params.zoom_by_bbox(0,600, 0, 400)
    m = MandelbrotFuncs()
    m.mandelbrot_image(params)
    image_array = m.mandelbrot_image(params)
    from PIL import Image, ImageTk
    image = Image.fromarray(image_array, 'RGB')
    image.save(filename, format='JPEG', quality=95)
    return params

def test(profile=True):
    import os
    import cProfile
    from pstats import Stats
    sample_filename = 'x.jpg'
    if os.path.exists(sample_filename):
        os.unlink(sample_filename)
    if profile:
        profile_stats_file = 'mandelbrot_profile'
        cProfile.run(f'generate_sample({sample_filename!r})', profile_stats_file)
        p = Stats(profile_stats_file)
        p.sort_stats('cumulative').print_stats(10)
    else:
        params = generate_sample(sample_filename)
        test_zoom(params, 'x2.jpg')


if __name__ == '__main__':
    test(profile=False)
