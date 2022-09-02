#! /usr/bin/env python

import numpy as np
import cv2


class MP4Writer:
    _writer = None

    def __init__(self, output_filename, fps, width, height, is_color=True):
        self._writer = cv2.VideoWriter(output_filename, cv2.VideoWriter_fourcc(*'mp4v'), fps, (width, height), is_color)

    def write_frames(self, np_array):
        for frame in np_array:
            self._writer.write(frame)

    def commit(self):
        self._writer.release()


def main():
    duration = 2
    fps = 25
    width = 720 * 16//9
    height = 720
    is_color = False
    if is_color:
        shape = (duration*fps, height, width, 3)  # w/h swapped!
    else:
        shape = (duration*fps, height, width)
    print(shape)
    data = np.random.randint(0, 256, shape, dtype='uint8')
    w = MP4Writer('output.mp4', fps, width, height, is_color)
    w.write_frames(data)
    w.commit()


if __name__ == '__main__':
    main()
