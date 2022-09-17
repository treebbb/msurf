#! /usr/bin/env python
"""
===================================
Shaded & power normalized rendering
===================================

The Mandelbrot set rendering can be improved by using a normalized recount
associated with a power normalized colormap (gamma=0.3). Rendering can be
further enhanced thanks to shading.

The ``maxiter`` gives the precision of the computation. ``maxiter=200`` should
take a few seconds on most modern laptops.
"""

from math import floor, sin, cos, pi
import numpy as np
from MP4Writer import MP4Writer


class MBrotProcessor:
    traceArray = None

    def __init__(self):
        pass

    def mandelbrot_set(self, xmin, xmax, ymin, ymax, xn, yn, maxiter, horizon=2.0, num_frames=200):
        DONE = [0] * num_frames
        def point_to_index(x, y):
            '''
            x,y are points on the real plane
            returns indexes into the pixel array
            '''
            x_inc = (xmax - xmin) / xn
            y_inc = (ymax - ymin) / yn
            x_index = floor((x - xmin) / x_inc)
            y_index = floor((y - ymin) / y_inc)
            # assert(x_index >= 0 and x_index < xn)
            # assert(y_index >= 0 and y_index < yn)
            x_index = max(0, x_index)
            x_index = min(xn - 1, x_index)
            y_index = max(0, y_index)
            y_index = min(yn - 1, y_index)
            return (x_index, y_index)
        # C is a 2d array of points on the real plane
        X = np.linspace(xmin, xmax, xn).astype(np.float32)
        Y = np.linspace(ymin, ymax, yn).astype(np.float32)
        C = X + Y[:, None] * 1j
        # trace array is array of pixels showing the progression of the selected point
        # towards the attractor
        # trace array is [num_frames, xn, yn, 3]
        # last position is blue=0 | green=1 | red=2
        self.traceArray = np.zeros((num_frames,)+C.shape+(3,), dtype=np.uint8)
        #
        print(C.shape)
        # N is the count of repetitions before reaching the horizon
        # shape [xn, yn]
        N = np.zeros_like(C, dtype=int)
        print('N:', N.shape)
        # Z is the calculation in the real plane, updated each repetition
        Z = np.zeros_like(C)
        def color_shift(n):
            '''
            'n' is the iteration number
            return (blue, green, red), pixel_size_of_dot
            '''
            DIVS = 120
            i = n % DIVS
            size = 5 - ((n // DIVS) % 5)
            degrees = i * (360./DIVS)
            rad = i * (2*pi) / DIVS
            green_shift = 2 * pi / 3  # 240 degrees
            blue_shift = 4 * pi / 3  # 120 degrees
            red_index = max(0, int(cos(rad) * 255))
            green_index = max(0, int(cos(rad + green_shift) * 255))
            blue_index = max(0, int(cos(rad + blue_shift) * 255))
            print('{:.2f}  {}  {}'.format(rad, (blue_index, green_index, red_index), size))
            return (blue_index, green_index, red_index), size
        # the main iteration loop
        for n in range(maxiter):
            color, size = color_shift(n)
            I = abs(Z) < horizon
            N[I] = n
            Z[I] = Z[I]**2 + C[I]
            # the point to sample to create the trace array
            point_of_interest = (-0.61, 0.642)
            x_index_start, y_index_start = point_to_index(*point_of_interest)
            # loop over all points of interest, adding them frame-by-frame to trace
            # one frame == a single point of interest
            for frame in range(num_frames):
                if not DONE[frame]:
                    x_index = x_index_start + frame
                    y_index = y_index_start
                    v = Z[y_index, x_index]  # x,y reversed
                    x, y = point_to_index(v.real, v.imag)
                    # print(n, x_index, y_index, v, x, y)
                    # print('{:.2f},{:.2f}  {},{}'.format(v.real, v.imag, x, y))
                    if x == 0 or x == xn-1:
                        DONE[frame] = 1
                    elif y == 0 or y == yn-1:
                        DONE[frame] = 1
                    else:
                        x1 = max(0, x - size)
                        x2 = min(xn-1, x + size)
                        y1 = max(0, y - size)
                        y2 = min(yn-1, y + size)
                        for i in range(x1, x2):
                            for j in range(y1, y2):
                                self.traceArray[frame, j, i] = color
        N[N == maxiter-1] = 0
        return Z, N


def main():
    # retina display 2880 x 1800
    #xmin, xmax, xn = -2.25, +1.75, 2880
    #ymin, ymax, yn = -1.25, +1.25, 1800
    xmin, xmax, xn = -2.25, +1.75, 600
    ymin, ymax, yn = -1.25, +1.25, 400
    fps = 25
    duration_seconds = 2
    num_frames = fps * duration_seconds
    maxiter = 200
    horizon = 2.0 ** 40
    log_horizon = np.log2(np.log(horizon))
    mb = MBrotProcessor()
    Z, N = mb.mandelbrot_set(xmin, xmax, ymin, ymax, xn, yn, maxiter, horizon, num_frames)

    # Normalized recount as explained in:
    # https://linas.org/art-gallery/escape/smooth.html
    # https://web.archive.org/web/20160331171238/https://www.ibm.com/developerworks/community/blogs/jfp/entry/My_Christmas_Gift?lang=en

    # This line will generate warnings for null values but it is faster to
    # process them afterwards using the nan_to_num
    # M is array of float64 from 0 to maxiter
#     with np.errstate(invalid='ignore'):
#        M = np.nan_to_num(N + 1 - np.log2(np.log(abs(Z))) + log_horizon)
    M = np.zeros((N.shape[0], N.shape[1], 3), dtype=np.uint8)
    M[:, :, 0] = N
    M[:, :, 1] = N
    M[:, :, 2] = N
    print(M.shape)
    print(M.dtype, np.max(M), np.min(M))
    M = M.astype(np.uint8)
    xinterval = (xmax - xmin) / xn
    x0 = int((0 - xmin) / xinterval)
    print(x0)
    M[:, x0, 2] = 255
    #
    print(M.shape, M.dtype)
    w = MP4Writer('x.mp4', fps, xn, yn, is_color=True)
    '''
    for frame in range(num_frames):
        trace = mb.traceArray[frame]
        # xdim, ydim = np.where(trace > 0)
        # has_values = np.column_stack((xdim, ydim))
        # has_values = list(zip(xdim, ydim))
        # print(n, has_values)
        # convert 3D to 4D with first axis as time/frames
        x = np.copy(np.expand_dims(M, axis=0))
        x[:, 0:yn, 0:xn] = np.maximum(x[:, 0:yn, 0:xn], trace)
        if frame == 0:
            print(x.shape)
        w.write_frames(x)
    '''
    w.write_frames(mb.traceArray)
    w.commit()

#mb = MBrotProcessor()
#mb.mbrot2(-1.76, 0.01, do_print=True)
main()
