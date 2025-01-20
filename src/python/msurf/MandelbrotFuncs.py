from MandelbrotParams import MandelbrotParams
import numpy as np
import os
import pyopencl as cl


MAX_MAXITER = (2 << 30) / 256 # 2^31 / 256. limited by 32 bit signed ints in opencl

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

    def __init__(self):
        self.init_opencl()

    def init_opencl(self):
        # Create OpenCL context and command queue
        self.ctx = cl.create_some_context()
        self.queue = cl.CommandQueue(self.ctx)
        # OpenCL kernel code
        dir_path = os.path.dirname(os.path.realpath(__file__))
        tfm_code = open('include/tfm_opencl.h', 'r').read()
        tfm_code += open('src/c/tfm_opencl.c', 'r').read()
        kernel_code_path = os.path.join(dir_path, 'mandelbrot_kernel_tfm.cl')
        kernel_src = open(kernel_code_path, 'r').read()

        # Compile the kernel
        self.prg = cl.Program(self.ctx, tfm_code + kernel_src).build()
            

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
        mbc = params._mandelbrot_cache
        if mbc is None or mbc[0] != xn or mbc[1] != yn:
            mandelbrot = np.empty((yn, xn, 3), dtype=np.uint8)
            params._mandelbrot_cache = (xn, yn, mandelbrot)
        else:
            mandelbrot = mbc[2]

        # Prepare data
        palette = params.iter_to_color()  # shape=(maxiter,3) dtype=np.uint8

        # Allocate memory on the GPU
        output_buf_size = xn * yn * 3  # width * height * 3 channels * sizeof(uint8)
        output_buf = cl.Buffer(self.ctx, cl.mem_flags.WRITE_ONLY, output_buf_size)
        c_palette = cl.Buffer(self.ctx, cl.mem_flags.READ_ONLY | cl.mem_flags.COPY_HOST_PTR, hostbuf=palette)


        # Execute the kernel
        shape = (xn, yn)
        step_size = (xmax - xmin) / xn
        print(f'mandelbrot_set_opencl: step_size: {step_size:.8f}')
        self.prg.mandelbrot(self.queue, shape, None, output_buf, c_palette,
                            np.int32(maxiter), np.float32(horizon), np.int32(xn), np.int32(yn),
                            np.float32(xmin), np.float32(ymin), np.float32(step_size))

        # Read results back to host
        cl.enqueue_copy(self.queue, mandelbrot, output_buf)
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
