from MandelbrotParams import MandelbrotParams
import numpy as np
import pyopencl as cl



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
        self.ctx = cl.create_some_context()
        self.queue = cl.CommandQueue(self.ctx)
        # OpenCL kernel code
        kernel_src = """
        __kernel void mandelbrot(__global const float *c_real, __global const float *c_imag, __global int *output,
                                const int maxiter, const float horizon, const int width, const int height) {
            const int x = get_global_id(0);
            const int y = get_global_id(1);

            float z_real = 0.0f;
            float z_imag = 0.0f;
            float z_real_squared = 0.0f;
            float z_imag_squared = 0.0f;
            int i;

            for(i = 0; i < maxiter; i++) {
                if(z_real_squared + z_imag_squared > horizon * horizon) break;

                z_imag = 2.0f * z_real * z_imag + c_imag[y * width + x];
                z_real = z_real_squared - z_imag_squared + c_real[y * width + x];

                z_real_squared = z_real * z_real;
                z_imag_squared = z_imag * z_imag;
            }

            if (i == maxiter) {
                output[y * width + x] = 0;
            } else {
                output[y * width + x] = i;
            }
        }
        """

        # Compile the kernel
        self.prg = cl.Program(self.ctx, kernel_src).build()


    def iter_to_color(self, maxiter):
        '''
        returns an array for the color,size of each iteration
        [((blue, green, red), pixel_size_of_dot) ... ]
        '''
        num_revolutions = 5  # repeat the color wheel this many times over maxiter
        total_divs = maxiter // num_revolutions  # number of divisions within color wheel
        red_shift = 0 # 0 degrees
        green_shift = 2 * pi / 3  # 120 degrees
        blue_shift = 4 * pi / 3  # 240 degrees
        def rgb_for_div_index(div_index):
            rad = 2 * pi * (div_index / total_divs)
            red_index = max(0, int(cos(rad + red_shift) * 255))
            green_index = max(0, int(cos(rad + green_shift) * 255))
            blue_index = max(0, int(cos(rad + blue_shift) * 255))
            return (red_index, green_index, blue_index)
        #print('{:.2f}  {}  {}'.format(rad, (blue_index, green_index, red_index), size))
        def color_size_for_div_index(n):
            rgb = rgb_for_div_index(n)
            div_count = n // total_divs  # which revolution we're on
            size = num_revolutions - (div_count)  # shrink by one pixel each rotation
            return (rgb, size)
        return [color_size_for_div_index(n) for n in range(maxiter)]

    # export PYOPENCL_CTX='0:1'
    def mandelbrot_set_opencl(self, params: MandelbrotParams, horizon=2.0):
        xmin, xmax, ymin, ymax, xn, yn, maxiter = \
            params.xmin, params.xmax, params.ymin, params.ymax, params.width, params.height, params.maxiter
        # Create OpenCL context and command queue

        # Prepare data
        real = np.linspace(xmin, xmax, xn, dtype=np.float32)
        imaginary = np.linspace(ymin, ymax, yn, dtype=np.float32)
        c = real[:, np.newaxis] + imaginary[np.newaxis, :] * 1j
        c_real = np.real(c).astype(np.float32)
        c_imag = np.imag(c).astype(np.float32)

        # Allocate memory on the GPU
        c_real_buf = cl.Buffer(self.ctx, cl.mem_flags.READ_ONLY | cl.mem_flags.COPY_HOST_PTR, hostbuf=c_real)
        c_imag_buf = cl.Buffer(self.ctx, cl.mem_flags.READ_ONLY | cl.mem_flags.COPY_HOST_PTR, hostbuf=c_imag)
        output_buf = cl.Buffer(self.ctx, cl.mem_flags.WRITE_ONLY, c_real.nbytes)


        # Execute the kernel
        self.prg.mandelbrot(self.queue, c_real.shape, None, c_real_buf, c_imag_buf, output_buf,
                       np.int32(maxiter), np.float32(horizon), np.int32(xn), np.int32(yn))

        # Read results back to host
        #mandelbrot = np.empty_like(c_real, dtype=np.int32)
        mandelbrot = np.zeros_like(c_real, dtype=np.int32)
        print('pre: sum: ', sum(sum(mandelbrot)))
        print('pre: shape: ', mandelbrot.shape)
        print('pre: num == 128', np.sum(mandelbrot == 128))
        cl.enqueue_copy(self.queue, mandelbrot, output_buf)
        print('post: sum: ', sum(sum(mandelbrot)))
        print('post: shape: ', mandelbrot.shape)
        print('post: num == 128', np.sum(mandelbrot == 128))
        # Cleanup
        c_real_buf.release()
        c_imag_buf.release()
        output_buf.release()
        # Return
        return mandelbrot

    def mandelbrot_image(self, params):
        #mandelbrot = mandelbrot_set(params)
        mandelbrot = self.mandelbrot_set_opencl(params)
        normalized = (mandelbrot / params.maxiter * 255).astype(np.uint8)
        normalized = np.rot90(normalized, k=1)
        grayscale_as_rgb = np.repeat(normalized[:, :, np.newaxis], 3, axis=2)
        return grayscale_as_rgb

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
