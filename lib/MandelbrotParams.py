class MandelbrotParams:
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
        """
        xwidth = xmax - xmin
        yheight = height / width * xwidth
        xcenter = xmin + xwidth / 2
        ycenter = ymin + (ymax - ymin) / 2
        print(f'MandelbrotParams: xcenter: {xcenter}  ycenter: {ycenter}')
        self.xmin = xcenter - xwidth / 2
        self.xmax = xcenter + xwidth / 2
        self.ymin = ycenter - yheight / 2
        self.ymax = ycenter + yheight / 2
        print(f'MandelbrotParams: xmin: {self.xmin}  xmax: {self.xmax}  ymin: {self.ymin}  ymax: {self.ymax}')
        self.width = int(width)
        self.height = int(height)
        self.maxiter = int(maxiter)

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

    def get_params(self):
        """
        Get all parameters in a tuple format suitable for passing to the mandelbrot_set function.
        
        :return: Tuple of (xmin, xmax, ymin, ymax, width, height, maxiter)
        """
        return (self.xmin, self.xmax, self.ymin, self.ymax, self.width, self.height, self.maxiter)
    
