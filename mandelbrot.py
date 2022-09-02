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

import math
import numpy as np
import random
from MP4Writer import MP4Writer

class MBrotProcessor:
    traceArray = None
    def __init__(self):
        pass        
    
    def mandelbrot_set(self, xmin, xmax, ymin, ymax, xn, yn, maxiter, horizon=2.0):
        DONE = 0
        blue_index = 255
        def point_to_index(x, y):
            x_inc = (xmax - xmin) / xn
            y_inc = (ymax - ymin) / yn
            x_index = math.floor((x - xmin) / x_inc)
            y_index = math.floor((y - ymin) / y_inc)
            # assert(x_index >= 0 and x_index < xn)
            # assert(y_index >= 0 and y_index < yn)
            x_index = max(0, x_index)
            x_index = min(xn - 1, x_index)
            y_index = max(0, y_index)
            y_index = min(yn - 1, y_index)
            return (x_index, y_index)
        X = np.linspace(xmin, xmax, xn).astype(np.float32)
        Y = np.linspace(ymin, ymax, yn).astype(np.float32)
        C = X + Y[:, None] * 1j
        #
        point_of_interest = (-0.51, 0.51)
        x_index, y_index = point_to_index(*point_of_interest)
        print(x_index, y_index, C[y_index, x_index])
        self.traceArray = np.zeros(C.shape, dtype=np.uint8)
        #
        print(C.shape)
        N = np.zeros_like(C, dtype=int)
        Z = np.zeros_like(C)
        for n in range(maxiter):
            I = abs(Z) < horizon
            N[I] = n
            Z[I] = Z[I]**2 + C[I]
            #
            if not DONE:
                v = Z[y_index, x_index]  # x,y reversed
                x, y = point_to_index(v.real, v.imag)
                # print(n, x_index, y_index, v, x, y)
                print('{:.2f},{:.2f}  {},{}'.format(v.real, v.imag, x, y))
                if x == 0 or x == xn-1:
                    DONE = 1
                elif y == 0 or y == yn-1:
                    DONE = 1
                else:
                    x1 = max(0, x-5)
                    x2 = min(xn-1, x+5)
                    y1 = max(0, y-5)
                    y2 = min(yn-1, y+5)
                    for i in range(x1,x2):
                        for j in range(y1,y2):
                            self.traceArray[j, i] = blue_index
                    blue_index -= 10
            #
        N[N == maxiter-1] = 0
        return Z, N


def main():
    import time
    import matplotlib
    from matplotlib import colors
    # retina display 2880 x 1800
    xmin, xmax, xn = -2.25, +1.75, 2880
    ymin, ymax, yn = -1.25, +1.25, 1800
    #xmin, xmax, xn = -2.25, +1.75, 600
    #ymin, ymax, yn = -1.25, +1.25, 400
    maxiter = 200
    horizon = 2.0 ** 40
    log_horizon = np.log2(np.log(horizon))
    mb = MBrotProcessor()
    Z, N = mb.mandelbrot_set(xmin, xmax, ymin, ymax, xn, yn, maxiter, horizon)

    # Normalized recount as explained in:
    # https://linas.org/art-gallery/escape/smooth.html
    # https://web.archive.org/web/20160331171238/https://www.ibm.com/developerworks/community/blogs/jfp/entry/My_Christmas_Gift?lang=en

    # This line will generate warnings for null values but it is faster to
    # process them afterwards using the nan_to_num
    # M is array of float64 from 0 to maxiter
    with np.errstate(invalid='ignore'):
        M = np.nan_to_num(N + 1 - np.log2(np.log(abs(Z))) + log_horizon)
    print(M.shape)
    print(M.dtype, np.max(M), np.min(M))
    M = M.astype(np.uint8)
    fps = 25
    duration = maxiter // fps
    # convert 2D arrray to 3 channel grey
    M2 = np.zeros((M.shape[0], M.shape[1], 3), dtype=np.uint8)
    M2[:,:,0] = M
    M2[:,:,1] = M
    M2[:,:,2] = M
    xinterval = (xmax - xmin) / xn
    x0 = int((0 - xmin) / xinterval)
    print(x0)
    M2[:,x0,2] = 255
    # 
    print(M2.shape, M2.dtype)
    w = MP4Writer('x.mp4', fps, xn, yn, is_color=True)
    for n in range(duration * fps):
        trace = mb.traceArray
        # xdim, ydim = np.where(trace > 0)
        # has_values = np.column_stack((xdim, ydim))
        # has_values = list(zip(xdim, ydim))
        # print(n, has_values)
        # convert 3D to 4D with first axis as time/frames
        x = np.expand_dims(M2, axis=0)
        x[:,0:yn,0:xn,0] = np.maximum(x[:,0:yn,0:xn,0], trace)
        if n == 0:
            print(x.shape)
        w.write_frames(x)
    w.commit()

#mb = MBrotProcessor()
#mb.mbrot2(-1.76, 0.01, do_print=True)
main()
