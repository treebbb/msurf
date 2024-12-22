#! /usr/bin/env python

# convert 2D arrray to 3 channel grey
'''
    M2 = np.zeros((M.shape[0], M.shape[1], 3), dtype=np.uint8)
    M2[:, :, 0] = M
    M2[:, :, 1] = M
    M2[:, :, 2] = M

>>> a = np.linspace(0, 24, 24, False)
>>> a
array([ 0.,  1.,  2.,  3.,  4.,  5.,  6.,  7.,  8.,  9., 10., 11., 12.,
       13., 14., 15., 16., 17., 18., 19., 20., 21., 22., 23.])
>>> I = a > 6
>>> I
array([False, False, False, False, False, False, False,  True,  True,
        True,  True,  True,  True,  True,  True,  True,  True,  True,
        True,  True,  True,  True,  True,  True])
>>> b = a.reshape(3,8)
>>> a
array([ 0.,  1.,  2.,  3.,  4.,  5.,  6.,  7.,  8.,  9., 10., 11., 12.,
       13., 14., 15., 16., 17., 18., 19., 20., 21., 22., 23.])
>>> b
array([[ 0.,  1.,  2.,  3.,  4.,  5.,  6.,  7.],
       [ 8.,  9., 10., 11., 12., 13., 14., 15.],
       [16., 17., 18., 19., 20., 21., 22., 23.]])
>>> I = b > 6
>>> I
array([[False, False, False, False, False, False, False,  True],
       [ True,  True,  True,  True,  True,  True,  True,  True],
       [ True,  True,  True,  True,  True,  True,  True,  True]])
>>> b[I]
array([ 7.,  8.,  9., 10., 11., 12., 13., 14., 15., 16., 17., 18., 19.,
       20., 21., 22., 23.])

'''
