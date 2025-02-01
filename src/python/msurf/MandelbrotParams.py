import numpy as np
from math import pi, cos, log


class MandelbrotParams:
    # used by mandelbrot_set_opencl. (width, height, np.array)
    _mandelbrot_cache = None
    xmin = 0
    xmax = 0
    ymin = 0
    ymax = 0
    width = 0
    height = 0
    maxiter = 255

    def __init__(self, xmin, xmax, ymin, ymax, width, height, maxiter):
        """
        Initialize the MandelbrotParams with the bounding box in the complex plane,
        image dimensions, and the maximum number of iterations.

        :param xmin: Minimum real value of the bounding box
        :param xmax: Maximum real value of the bounding box
        :param ymin: Minimum imaginary value of the bounding box
        :param ymax: Maximum imaginary value of the bounding box
        :param width: Width of the image to be displayed
        :param height: Height of the image to be displayed
        :param maxiter: Maximum number of iterations for the Mandelbrot calculation

        No proportion checks - you must ensure that xwidth/ywidth matches width/height
        """
        self.xmin = xmin
        self.xmax = xmax
        self.ymin = ymin
        self.ymax = ymax
        self.width = int(width)
        self.height = int(height)
        self.maxiter = maxiter

    @classmethod
    def from_bounds(cls, xmin, xmax, ymin, ymax, width, height, maxiter):
        '''
        calculates the center of the given coords\
        adjusts xmin, xmax based on width, height so that proportions match
        '''
        xwidth = xmax - xmin
        yheight = height / width * xwidth
        xcenter = xmin + xwidth / 2
        ycenter = ymin + (ymax - ymin) / 2
        print(f'MandelbrotParams.from_bounds: xcenter: {xcenter}  ycenter: {ycenter}')
        xmin = xcenter - xwidth / 2
        xmax = xcenter + xwidth / 2
        ymin = ycenter - yheight / 2
        ymax = ycenter + yheight / 2
        print(f'MandelbrotParams/from_bounds: xmin: {xmin}  xmax: {xmax}  ymin: {ymin}  ymax: {ymax}')
        mp = MandelbrotParams(xmin, xmax, ymin, ymax, width, height, maxiter)
        return mp

    def __str__(self):
        """
        String representation of the MandelbrotParams for easy debugging or logging.
        """
        return (f"MandelbrotParams(xmin={self.xmin}, xmax={self.xmax}, ymin={self.ymin}, ymax={self.ymax}, "
                f"width={self.width}, height={self.height}, maxiter={self.maxiter})")

    def update_bounds(self, new_xmin, new_xmax, new_ymin, new_ymax):
        """
        Update the complex plane bounds for zooming or panning the Mandelbrot set.

        :param new_xmin: New minimum real value
        :param new_xmax: New maximum real value
        :param new_ymin: New minimum imaginary value
        :param new_ymax: New maximum imaginary value
        """
        self.xmin = float(new_xmin)
        self.xmax = float(new_xmax)
        self.ymin = float(new_ymin)
        self.ymax = float(new_ymax)

    def zoom(self, zoomval=1.0):
        xwidth = self.xmax - self.xmin
        yheight = self.ymax - self.ymin
        xcenter = self.xmin + xwidth / 2
        ycenter = self.ymin + yheight / 2
        new_xwidth = xwidth * zoomval
        new_yheight = yheight * zoomval
        self.xmin = xcenter - new_xwidth / 2
        self.xmax = xcenter + new_xwidth / 2
        self.ymin = ycenter - new_yheight / 2
        self.ymax = ycenter + new_yheight / 2

    def zoom_by_bbox(self, x1, x2, y1, y2):
        """
        Zoom into the Mandelbrot based on a bounding box around the pixel image

        :param x1, x2, y1, y2: the image bounding box
        """
        icenter = x1 + (x2 - x1) / 2, y1 + (y2 - y1) / 2
        xfactor = (x2 - x1) / self.width
        yfactor = (y2 - y1) / self.height
        factor = max(xfactor, yfactor)
        xwidth = self.xmax - self.xmin
        yheight = self.ymax - self.ymin
        xcenter = self.xmin + (icenter[0] / self.width) * xwidth
        ycenter = self.ymax - (icenter[1] / self.height) * yheight
        xwidth = xwidth * factor
        yheight = self.height / self.width * xwidth
        # self.maxiter = int(128 / xwidth)
        print(f'zoom_by_bbox: icenter: {icenter}  factor: {factor}  xcenter: {xcenter}  ycenter: {ycenter}  xwidth: {xwidth}  yheight: {yheight}  maxiter: {self.maxiter}')
        # Update bounds
        self.xmin = xcenter - xwidth / 2
        self.xmax = xcenter + xwidth / 2
        self.ymin = ycenter - yheight / 2
        self.ymax = ycenter + yheight / 2

    def image_to_complex(self, x1, y1):
        '''
        given a point on the image (pixel) space
        return the x,y on the complex space
        '''
        xwidth = self.xmax - self.xmin
        yheight = self.ymax - self.ymin
        xpos = self.xmin + (x1 / self.width) * xwidth
        ypos = self.ymax - (y1 / self.height) * yheight
        return xpos, ypos

    def complex_to_image(self, xpos, ypos):
        '''
        given a point in the complex space
        return the x,y in the image space
        '''
        xwidth = self.xmax - self.xmin
        yheight = self.ymax - self.ymin
        x1 = int( (xpos - self.xmin) / xwidth * self.width )
        y1 = self.height - int((ypos - self.ymin) / yheight * self.height)
        return x1, y1

    def iter_to_color(self):
        '''
        returns an array for the color,size of each iteration
        [((blue, green, red), pixel_size_of_dot) ... ]
        '''
        maxiter = self.maxiter
        num_revolutions = 0.5  # repeat the color wheel this many times over maxiter
        rad_inc = 2 * pi / (maxiter / num_revolutions)
        red_shift = 0 # 0 degrees
        green_shift = 2 * pi / 3  # 120 degrees
        blue_shift = 4 * pi / 3  # 240 degrees
        palette = np.zeros((maxiter, 3), dtype=np.uint8)
        rad = 0
        for i in range(maxiter):
            r = int(log(i + 1) / log(maxiter + 1) * 255)
            g = r
            b = 0
            palette[i][0] = r
            palette[i][1] = g
            palette[i][2] = b
            rad += rad_inc
        return palette

    def tile_params(self, x, y, tile_size):
        '''
        x and y are relative to our width and height
        tile_size is the width and height of the tile

        returns the new params along with tile_x (remaining x pixels) and tile_y (remaining y pixels)
        '''
        # Define the bounds for this tile in the complex plane
        tile_x = min(tile_size, self.width - x)  # remaining tile extent
        tile_y = min(tile_size, self.height - y)
        step_size = (self.xmax - self.xmin) / self.width
        tile_xmin = self.xmin + (x * step_size)
        tile_xmax = tile_xmin + (tile_x * step_size)
        tile_ymin = self.ymin + (y * step_size)
        tile_ymax = tile_ymin + (tile_y * step_size)

        # Create new MandelbrotParams for this tile
        tile_params = MandelbrotParams(tile_xmin, tile_xmax, tile_ymin, tile_ymax, tile_x, tile_y, self.maxiter)
        return tile_params, tile_x, tile_y

    def tile_iter(self, tile_size):
        '''
        return a generator that will loop over tiles in this display
        ordering is [(x0, y0), (x1, y0), (x2, y0) ... (xN-1, yN-1)]
        value is
          (x, y, tile_params, tile_width, tile_height)
        '''
        x = 0
        y = 0
        ITER_STEP = 100  # sync with opencl kernel
        cur_iter = 0
        while cur_iter < self.maxiter:
            while True:
                tile_params, tile_width, tile_height = self.tile_params(x, y, tile_size)
                tile_params.maxiter = cur_iter + ITER_STEP
                yield (x, y, tile_params, tile_width, tile_height)
                x += tile_width
                if x == self.width:
                    x = 0
                    y = y + tile_height
                if y == self.height:
                    break
            x = 0
            y = 0
            cur_iter = cur_iter + ITER_STEP

    def get_params(self):
        """
        Get all parameters in a tuple format suitable for passing to the mandelbrot_set function.

        :return: Tuple of (xmin, xmax, ymin, ymax, width, height, maxiter)
        """
        return (self.xmin, self.xmax, self.ymin, self.ymax, self.width, self.height, self.maxiter)

    def bookmark_string(self):
        d = {}
        d['xcenter'] = (self.xmax + self.xmin) / 2.0
        d['ycenter'] = (self.ymax + self.ymin) / 2.0
        xwidth = self.xmax - self.xmin
        d['step_size'] = xwidth / self.width
        d['maxiter'] = self.maxiter
        return repr(d)

    @classmethod
    def from_bookmark_string(self, bookmark_string, width, height):
        d = eval(bookmark_string)  # !!! UNSAFE !!!
        step_size = d['step_size']
        xcenter = d['xcenter']
        ycenter = d['ycenter']
        maxiter = d['maxiter']
        xwidth = width * step_size
        yheight = height * step_size
        xmin = xcenter - (xwidth / 2.0)
        xmax = xcenter + (xwidth / 2.0)
        ymin = ycenter - (yheight / 2.0)
        ymax = ycenter + (yheight / 2.0)
        return MandelbrotParams(xmin, xmax, ymin, ymax, width, height, maxiter)
