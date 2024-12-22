from numba import jit, prange
import numpy as np


def mandelbrot_set(xmin, xmax, ymin, ymax, xn, yn, maxiter, horizon=2.0):
    # C is a 2d array of points on the real plane
    real = np.linspace(xmin, xmax, xn).astype(np.float32)
    imaginary = np.linspace(ymin, ymax, yn).astype(np.float32)
    complex = real[:, np.newaxis] + imaginary[np.newaxis, :] * 1j
    # N is the count of repetitions before reaching the horizon
    # shape [xn, yn]
    mandlebrot = np.zeros(complex.shape, dtype=np.int32)
    # Z is the calculation in the real plane, updated each repetition
    z = np.zeros_like(complex)
    # the point to sample to create the trace array
    for n in range(maxiter):
        points_in_bounds = np.where(np.abs(z) < horizon)
        z[points_in_bounds] = z[points_in_bounds]**2 + complex[points_in_bounds]
        mandlebrot[points_in_bounds] = n
    mandlebrot[mandlebrot == maxiter-1] = 0  # inside black
    # mandlebrot[np.abs(z) <= horizon] = maxiter  # inside white
    return mandlebrot


@jit(nopython=True, cache=True)
def mandelbrot_set_jit(xmin, xmax, ymin, ymax, xn, yn, maxiter, horizon=2.0):
    # C is a 2d array of points on the real plane
    real = np.linspace(xmin, xmax, xn).astype(np.float32)
    imaginary = np.linspace(ymin, ymax, yn).astype(np.float32)
    complex = real[:, np.newaxis] + imaginary[np.newaxis, :] * 1j
    
    # N is the count of repetitions before reaching the horizon
    # shape [xn, yn]
    mandelbrot = np.zeros((yn, xn), dtype=np.int32)
    
    # Z is the calculation in the real plane, updated each repetition
    z = np.zeros_like(complex)
    
    # Iterate over each point
    for n in range(maxiter):
        # Flatten the array for easier indexing
        flat_z = z.ravel()
        flat_complex = complex.ravel()
        flat_mandelbrot = mandelbrot.ravel()
        # Use a loop to go through each element
        for i in range(flat_z.shape[0]):
            if np.abs(flat_z[i]) < horizon:
                flat_z[i] = flat_z[i]**2 + flat_complex[i]
                flat_mandelbrot[i] = n
        # Reshape back to 2D
        z = flat_z.reshape(z.shape)
        mandelbrot = flat_mandelbrot.reshape(mandelbrot.shape)
    # Set points that reached maxiter to 0
    # Instead of boolean indexing, we'll use a loop
    flat_mandelbrot = mandelbrot.ravel()
    for i in range(flat_mandelbrot.shape[0]):
        if flat_mandelbrot[i] == maxiter - 1:
            flat_mandelbrot[i] = 0
    
    mandelbrot = flat_mandelbrot.reshape(mandelbrot.shape)
    return mandelbrot


class MandlebrotFuncs:

    def __init__(self):
        pass

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


    def mandlebrot_image(self, image_dims: (int,int), mbrot_center: (float, float) = (-0.5,0), mbrot_width: float = 2.0):
        mbrot_height = mbrot_width * image_dims[1] / image_dims[0]
        xmin, xmax, xn = mbrot_center[0] - (mbrot_width / 2), mbrot_center[0] + (mbrot_width / 2), image_dims[0]
        ymin, ymax, yn = mbrot_center[1] - (mbrot_height / 2), mbrot_center[1] + (mbrot_height / 2), image_dims[1]
        maxiter = 200
        mandlebrot = mandelbrot_set(xmin, xmax, ymin, ymax, xn, yn, maxiter)
        normalized = (mandlebrot / maxiter * 255).astype(np.uint8)
        normalized = np.rot90(normalized, k=1)
        grayscale_as_rgb = np.repeat(normalized[:, :, np.newaxis], 3, axis=2)
        return grayscale_as_rgb

def generate_sample(filename):
    m = MandlebrotFuncs()
    image_array = m.mandlebrot_image((1200,800))
    from PIL import Image, ImageTk
    image = Image.fromarray(image_array, 'RGB')
    image.save(filename, format='JPEG', quality=95)
    
def test(profile=True):
    import os
    import cProfile
    from pstats import Stats
    sample_filename = 'x.jpg'
    if os.path.exists(sample_filename):
        os.unlink(sample_filename)
    if profile:
        profile_stats_file = 'mandlebrot_profile'
        cProfile.run(f'generate_sample({sample_filename!r})', profile_stats_file)
        p = Stats(profile_stats_file)
        p.sort_stats('cumulative').print_stats(10)
    else:
        generate_sample(sample_filename)


if __name__ == '__main__':
    test(profile=True)
