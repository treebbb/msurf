from copy import copy
import cv2
from decimal import Decimal
import itertools
from MandelbrotFuncs import MandelbrotFuncs
from MandelbrotParams import MandelbrotParams
import math
import numpy as np
from PIL import Image, ImageTk
from scipy.ndimage import label, find_objects
import tkinter as tk
from tkinter import filedialog, simpledialog

CLEAR_EVENT = 'CLEAR_EVENT'

class CurPointState:
    '''
    used to track a single point and allow display of position
    at each iteration

    orig is the starting point
    steps is the number of iterations since the starting point
    history is a pre-calculated list of iteration positions

    escape_count is length(history) and is the number of iterations
    before abs(z) > 2.0 -- or maxiter if it doesn't escape
    '''
    orig = 0 + 0j
    steps = 0
    history = None
    escape_count = None  # cache for calc_escape
    maxiter = 256
    def __init__(self, xpos, ypos, maxiter=256):
        self.orig = xpos + ypos * 1j
        self.maxiter = maxiter
        self.history = []
        self._calc_escape()

    def go_left(self):
        '''
        pop off history (go back to previous iter)
        '''
        if self.steps > 0:
            self.steps -= 1

    def go_right(self):
        '''
        calc next iter
        '''
        if self.steps < self.escape_count:
            self.steps += 1

    def z(self):
        return self.history[self.steps]

    def _calc_escape(self):
        '''
        return number of reps for escape (limited by maxiter)
        '''
        self.escape_count = 0
        c = self.orig
        z = c
        self.history.append(z)
        for i in range(self.maxiter):
            z = z * z + c
            if abs(z) > 2.0:
                break
            self.history.append(z)
            self.escape_count += 1

    def message(self):
        s = 'steps: ' + str(self.steps)
        s += '  escape: ' + str(self.escape_count)
        s += '  cur: (' + str(self.z().real) + ', ' + str(self.z().imag) + ')'
        return s

class ImageProcessor:
    def find_largest_black_region(self, image):
        # Detect black regions
        black_pixels = np.all(image == [0, 0, 0], axis=-1)

        # Label connected components
        labeled_array, num_features = label(black_pixels)

        # Find bounding boxes of all connected components
        objects_slices = find_objects(labeled_array)

        # Find the largest black region
        max_area = 0
        largest_region_slice = None
        for region_slice in objects_slices:
            area = (region_slice[0].stop - region_slice[0].start) * (region_slice[1].stop - region_slice[1].start)
            if area > max_area:
                max_area = area
                largest_region_slice = region_slice

        if largest_region_slice is None:
            return None  # No black region found

        # Calculate bounding box coordinates
        y1, x1 = largest_region_slice[0].start, largest_region_slice[1].start
        y2, x2 = largest_region_slice[0].stop, largest_region_slice[1].stop
        largest_black_region = black_pixels[y1:y2, x1:x2]

        return largest_black_region, (x1, y1, x2, y2)

    def find_min_area_rect(self, image):
        '''
        input is a numpy array of booleans indicating black pixels (from find_largest_black_region)
        return 4 points defining a rotated boundingbox around the image
        '''

        #gray_image = np.average(image, axis=-1).astype(np.uint8)
        #contours, _ = cv2.findContours(gray_image, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        contours, _ = cv2.findContours(image.astype(np.uint8), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

        if not contours:
            return None  # No contours found

        # Find the contour with the largest area
        largest_contour = max(contours, key=cv2.contourArea)

        # Find the minimum area rectangle
        min_area_rect = cv2.minAreaRect(largest_contour)
        print(f'min_area_rect: {min_area_rect}')

        # Get the bounding box points
        box = cv2.boxPoints(min_area_rect)
        box = np.int0(box)

        return box

    def furthest_black_point(self, black_pixels, x, y, use_min=False):
        height, width = black_pixels.shape
        y_coords, x_coords = np.where(black_pixels)
        if len(x_coords) == 0:
            return None
        points = np.column_stack((x_coords, y_coords))
        center_point = np.array([x, y])
        distances = np.sum((points - center_point) ** 2, axis=1)
        if use_min:
            furthest_idx = np.argmin(distances)
        else:
            furthest_idx = np.argmax(distances)
        ax = x_coords[furthest_idx]
        ay = y_coords[furthest_idx]
        return ax, ay

    def find_black_shape_center(self, black_pixels):
        '''
        Find the center of a black shape in an RGB image by averaging all black pixel coordinates

        Args:
            image: numpy array with shape (height, width) where the value is True if black pixel, False otherwise

        Returns:
            tuple (x, y) representing the center coordinates of the black shape
            or None if no black pixels are found
        '''
        debug_points = []  # (x, y, color)

        # Get the coordinates of black pixels
        height, width = black_pixels.shape
        y_coords, x_coords = np.where(black_pixels)

        # Check if any black pixels were found
        if len(x_coords) == 0:
            return None

        # Calculate the center by averaging x and y coordinates
        center_x = int(np.mean(x_coords))
        center_y = int(np.mean(y_coords))
        '''

        # Create arrays of all black pixel coordinates
        points = np.column_stack((x_coords, y_coords))
        center_point = np.array([center_x, center_y])

        # Calculate distances from center to all black pixels
        # Using broadcasting to subtract center from all points
        distances = np.sum((points - center_point) ** 2, axis=1)

        # Find the index of the maximum distance
        furthest_idx = np.argmax(distances)

        # Get the coordinates of the furthest point
        ax = x_coords[furthest_idx]
        ay = y_coords[furthest_idx]
        '''
        ax, ay = self.furthest_black_point(black_pixels, center_x, center_y)
        print(f'ax: {ax}  ay: {ay}')

        # Calculate the angle of point1 from vertical
        dx = float(center_x - ax)
        dy = float(center_y - ay)
        angle_rad = np.arctan2(dx, -dy)

        # Normalize the vector
        length = np.sqrt(dx*dx + dy*dy)
        print(f'  A: ({ax}, {ay})  center: ({center_x}, {center_y})  length: {length}')
        if length == 0:
            return (center_x, center_y, ax, ay, center_x, center_y), debug_points

        dx = dx / length
        dy = dy / length
        print(f' dx: {dx}  dy: {dy}')
        # the center isn't calculated well, possibly because it's averaging
        # black pixels outside of the main bulbs. Draw perpendicular lines
        # to the edge of the bulbs and adjust center
        # ldx, ldy is counterclockwise and rdx, rdy is clockwise
        ldx, ldy = dy, -dx
        lbx, lby = self.find_furthest_color_point(black_pixels, center_x, center_y, ldx, ldy)
        rdx, rdy = -dy, dx
        rbx, rby = self.find_furthest_color_point(black_pixels, center_x, center_y, rdx, rdy)
        debug_points.append((lbx, lby, 'red'))
        debug_points.append((rbx, rby, 'red'))
        ldist = np.sqrt((lbx - center_x) ** 2 + (lby - center_y) ** 2)
        rdist = np.sqrt((rbx - center_x) ** 2 + (rby - center_y) ** 2)
        print(f' lb: ({lbx}, {lby})  rb: ({rbx}, {rby})  ldist: {ldist}  rdist: {rdist}')
        bx, by = self.find_furthest_color_point(black_pixels, center_x, center_y, dx, dy)
        head_to_center_dist = np.sqrt((ax - center_x) ** 2 + (ay - center_y) ** 2)
        non_black = int(head_to_center_dist / 10)
        lbx, lby = self.find_furthest_color_point(black_pixels, bx, by, ldx, ldy, allow_non_black=non_black)
        rbx, rby = self.find_furthest_color_point(black_pixels, bx, by, rdx, rdy, allow_non_black=non_black)
        debug_points.append((lbx, lby, 'blue'))
        debug_points.append((rbx, rby, 'blue'))
        ldist = np.sqrt((lbx - bx) ** 2 + (lby - by) ** 2)
        rdist = np.sqrt((rbx - bx) ** 2 + (rby - by) ** 2)
        print(f' (2)lb: ({lbx}, {lby})  rb: ({rbx}, {rby})  ldist: {ldist}  rdist: {rdist}  non_black: {non_black}')
        #
        head_center_to_center_tail_ratio = 1.095 / 0.53
        move_length = head_to_center_dist * 0.4  # it's actually ~0.54 from center to tail
        center_x2 = int(center_x + (move_length * dx))
        center_y2 = int(center_y + (move_length * dy))
        white_pixels = ~black_pixels
        tailx, taily = self.furthest_black_point(white_pixels, center_x2, center_y2, use_min=True)
        debug_points.append((tailx, taily, 'green'))
        debug_points.append((center_x, center_y, 'red'))
        debug_points.append((center_x2, center_y2, 'purple'))
        print(f' tailx: {tailx}  taily: {taily}')
        #
        return (ax, ay, tailx, taily), debug_points


    def find_furthest_color_point(self, black_pixels, ax, ay, dx, dy, allow_non_black=0):
        """
        Find the furthest point from B along the vector AB that matches target_color

        Args:
            black_pixels: 2D numpy array (height, width) of Booleans
            point_a: tuple (x,y) of first point
            dx, dy: floats representing the normalized run and rise

        Returns:
            tuple (x,y) of furthest matching point, or point_a if no match found
        """
        height, width = black_pixels.shape[:2]

        # Start from point A
        exact_x, exact_y = ax, ay
        furthest_point = (int(ax), int(ay))

        # Continue until we hit image boundaries
        while True:
            # Update exact position
            exact_x += dx
            exact_y += dy

            # Convert to integer coordinates for array indexing
            x = int(round(exact_x))
            y = int(round(exact_y))

            # Check if we're outside image bounds
            if x < 0 or x >= width or y < 0 or y >= height:
                break

            # Check if color matches
            if black_pixels[y, x]:
                furthest_point = (x, y)
            elif allow_non_black:
                allow_non_black -= 1
            else:
                break

        return furthest_point


class InteractiveImageDisplay:
    # state
    width = None
    height = None
    # widgets
    image_on_canvas = None
    canvas = None
    # point iterator
    cur_point_state = None
    cur_point_rect = None
    # resize state
    _is_resizing = None
    _master_dims_vs_image_dims = None
    _last_master_dims = None
    # color palette
    showing_palette = False
    palette_window = None
    # bounding box
    black_bounding_box_axis_line = None
    # debug rectangles
    debug_points = None

    def __init__(self, master, width, height):
        """
        Initialize the interactive image display for the Mandelbrot set.

        :param master: Tkinter root or frame where the image will be displayed
        :param width: pixel width of canvas
        :param height: pixel height of canvas
        """
        self.master = master
        self.width = width
        self.height = height
        self.mandelbrot_funcs = MandelbrotFuncs()

        # Create canvas
        self.canvas = tk.Canvas(master, width=width, height=height)
        self.canvas.pack()

        # init params
        self.set_initial_params()

        # Variables for drag selection
        self.start_x = None
        self.start_y = None
        self.rect = None

        # Bind mouse events
        self.canvas.bind("<ButtonPress-1>", self.on_button_press)
        self.canvas.bind("<B1-Motion>", self.on_move_press)
        self.canvas.bind("<ButtonRelease-1>", self.on_button_release)

        # Bind key events
        self.master.bind('<Command-r>', self.key_handler)
        self.master.bind("<Right>", self.key_handler)
        self.master.bind("<Left>", self.key_handler)
        self.master.bind("<Command-equal>", self.key_handler)
        self.master.bind("<Command-minus>", self.key_handler)

        # status dialog
        #self.status_dialog = tk.Label(self.master, text="", bd=1, relief=tk.SUNKEN, anchor=tk.W, height=2, width=50, bg='black', fg='red')
        #self.status_dialog.place(relx=1.0, rely=1.0, x=-10, y=-50, anchor=tk.SE)
        self.status_text = self.canvas.create_text(self.width - 10, self.height - 10,
                                                   anchor=tk.SE,
                                                   text='',
                                                   fill='red',
                                                   font=('Arial', 12))
        # display buttons side-by-side
        self.button_frame = tk.Frame(master)
        self.button_frame.pack(fill=tk.X)  # Fill horizontally

        # Create a button to toggle the color palette display
        self.color_palette_button = tk.Button(self.button_frame, text="Show Color Palette", command=self.toggle_color_palette)
        self.color_palette_button.pack(side=tk.LEFT, padx=(0, 5))  # left side with some padding

        # Add a button to the display to open the maxiter setting dialog
        button = tk.Button(self.button_frame, text="Set Max Iterations", command=self.set_maxiter_dialog)
        button.pack(side=tk.LEFT)

        # Add a button to draw bounding box around largest black region
        button = tk.Button(self.button_frame, text="Draw Bounding Box", command=self.toggle_draw_bounding_box)
        button.pack(side=tk.LEFT)

        # Bind the configure event to handle window resizing
        self.master.bind('<Configure>', self.on_resize)

        # initialize Image Processor
        self.image_processor = ImageProcessor()

    def set_initial_params(self):
        # Initial parameters for the Mandelbrot set
        initial_xmin, initial_xmax = -2.0, 1.0
        initial_ymin, initial_ymax = -1.5, 1.5
        maxiter = 128
        self.params = MandelbrotParams(initial_xmin, initial_xmax, initial_ymin, initial_ymax, self.width, self.height, maxiter)

    def reset_image(self, event=None):
        self.set_initial_params()
        self.reload_image()

    def reload_image(self):
        # clear cur_point
        self.cur_point_state = None  # remove point
        self.toggle_draw_bounding_box(event=CLEAR_EVENT)
        if self.cur_point_rect:
            self.canvas.delete(self.cur_point_rect)
            self.cur_point_rect = None
        # show dimensions in message
        zoom_factor = Decimal(3.0 / (self.params.xmax - self.params.xmin))
        self.update_status(f'Zoom: {zoom_factor:.2e}  MaxIter: {self.params.maxiter}')
        # Generate, normalize and convert to image
        self.mandelbrot_array = self.mandelbrot_funcs.mandelbrot_set_opencl(self.params)
        print(f'MB shape: {self.mandelbrot_array.shape}')
        #self.normalized_mandelbrot = normalized = np.rot90(mandelbrot, k=1)
        # draw image
        self.image = Image.fromarray(self.mandelbrot_array, 'RGB')
        self.photo = ImageTk.PhotoImage(self.image)
        self.canvas.config(width=self.width, height=self.height)
        if self.image_on_canvas is None:
            # create the image
            self.image_on_canvas = self.canvas.create_image(0, 0, anchor=tk.NW, image=self.photo)
        else:
            # Update the image on the canvas
            self.canvas.itemconfig(self.image_on_canvas, image=self.photo)
        # Update status text position
        self.canvas.coords(self.status_text, self.width - 10, self.height - 10)
        self.canvas.tag_raise(self.status_text)
        self._master_dims_vs_image_dims = (
            self.master.winfo_width() - self.width,
            self.master.winfo_height() - self.height,
        )
        #print(f'reload_image(): canvas after complete: {self.canvas.winfo_width()} {self.canvas.winfo_height()}')
        #print(f'reload_image(): master after complete: {self.master.winfo_width()} {self.master.winfo_height()}')
        #print(f'reload_image: _master_dims_vs_image_dims: {self._master_dims_vs_image_dims}')

    def on_resize(self, event):
        #print(f'on_resize({event})')
        if self._last_master_dims is None:
            self._last_master_dims = (self.master.winfo_width(), self.master.winfo_height())
            self.master.after(200, self.check_resize_finished, event)

    def check_resize_finished(self, event):
        #print(f'check_resize_finished(1): {event}')
        #print(f'check_resize_finished(1): canvas: {self.canvas.winfo_width()} {self.canvas.winfo_height()}')
        #print(f'check_resize_finished(1): master: {self.master.winfo_width()} {self.master.winfo_height()}')
        master_dims = (self.master.winfo_width(), self.master.winfo_height())
        if master_dims == self._last_master_dims:
            # master dimensions are stable. update canvas size
            if self._master_dims_vs_image_dims is not None:
                self.width = master_dims[0] - self._master_dims_vs_image_dims[0]
                self.height = master_dims[1] - self._master_dims_vs_image_dims[1]
                self.params.width = self.width
                self.params.height = self.height
                self.params.zoom_by_bbox(0, self.width, 0, self.height)  # reset complex plane bbox
            #self.master.config(width=event.width, height=event.height)
            self.reload_image()
            self._last_master_dims = None
        else:
            self._last_master_dims = master_dims
            # wait and then check again
            self.master.after(500, self.check_resize_finished, event)

    def on_button_press(self, event):
        """Start of the drag selection"""
        self.start_x = self.canvas.canvasx(event.x)
        self.start_y = self.canvas.canvasy(event.y)
        # Create a rectangle for highlighting
        self.rect = self.canvas.create_rectangle(self.start_x, self.start_y, self.start_x, self.start_y, outline='red')

    def on_move_press(self, event):
        """Update the rectangle as the mouse moves"""
        current_x = self.canvas.canvasx(event.x)
        current_y = self.canvas.canvasy(event.y)

        # Update the rectangle's coordinates
        self.canvas.coords(self.rect, self.start_x, self.start_y, current_x, current_y)

    def on_button_release(self, event):
        """End of the drag selection, recalculate Mandelbrot set"""
        current_x = self.canvas.canvasx(event.x)
        current_y = self.canvas.canvasy(event.y)

        # Ensure start and end coordinates are in the correct order
        x1, x2 = min(self.start_x, current_x), max(self.start_x, current_x)
        y1, y2 = min(self.start_y, current_y), max(self.start_y, current_y)
        #print(f'on_button_release: x1: {x1}  x2: {x2}  y1: {y1}  y2: {y2}')
        if (x1 == x2) or (y1 == y2):
            # clear current rect
            self.canvas.delete(self.rect)
            xpos, ypos = self.params.image_to_complex(x1, y1)
            self.cur_point_state = CurPointState(xpos, ypos, maxiter=self.params.maxiter)
            self.show_cur_point()
            return

        self.params.zoom_by_bbox(x1, x2, y1, y2)

        # Recalculate the Mandelbrot set for the new region
        self.reload_image()

        # Clear the selection rectangle
        self.canvas.delete(self.rect)

    def on_move_press(self, event):
        """Update the rectangle as the mouse moves"""
        current_x = self.canvas.canvasx(event.x)
        current_y = self.canvas.canvasy(event.y)

        # Update the rectangle's coordinates
        self.canvas.coords(self.rect, self.start_x, self.start_y, current_x, current_y)

    def key_handler(self, event):
        """
        Handle key press events. This method is designed to be overridden
        with specific functionality, like resetting the view with Command-R.

        :param event: Tkinter event object containing key press information
        """
        # print(f'key_handler: event: {event}  event.state: {event.state}')
        # Check if the pressed key is 'r' and if the Command key (on Mac) or Control key (on Windows/Linux) is pressed
        if event.keysym.lower() == 'r' and (event.state & 0x8):  # 0x10 is the bitmask for Command on Mac
            # You can implement the reset functionality here
            # For example, you might reset the view to the initial state or perform some other action
            print("Command-R was pressed. Implement reset functionality here.")
            self.reset_image()
        elif event.keysym == 'Right':
            self.cur_point_state.go_right()
            self.show_cur_point()
        elif event.keysym == 'Left':
            self.cur_point_state.go_left()
            self.show_cur_point()
        elif event.keysym == 'equal':  # !!! also check Command bitmask
            self.params.zoom(0.80)
            self.reload_image()
        elif event.keysym == 'minus':
            self.params.zoom(1.25)
            self.reload_image()

    def show_cur_point(self):
        z = self.cur_point_state.z()
        x, y = self.params.complex_to_image(z.real, z.imag)
        message = self.cur_point_state.message()
        # print(f'iter_cur_point: orig=({z})  new=({x}, {y})')
        if x < 0 or x >= self.params.width or y < 0 or y >= self.params.height:
            message = 'OUT-OF-BOUNDS  ' + message
        elif self.cur_point_rect is None:
            self.cur_point_rect = self.canvas.create_rectangle(x, y, x+3, y+3, fill='blue', outline='blue')
        else:
            self.canvas.coords(self.cur_point_rect, x, y, x+3, y+3)
        self.update_status(message)

    def set_maxiter_dialog(self):
        """
        Open a dialog box to set the maxiter value for the Mandelbrot calculation.
        Updates self.params.maxiter with the new value if provided.
        """
        # Ask for a new maxiter value
        new_maxiter = simpledialog.askinteger("Set Max Iterations",
                                              "Enter new maxiter value:",
                                              initialvalue=self.params.maxiter,
                                              minvalue=1,  # Ensure a positive integer
                                              parent=self.master)

        # Check if a value was entered and update if so
        if new_maxiter is not None:
            self.params.maxiter = new_maxiter
            print(f"Max iterations set to: {self.params.maxiter}")
            self.reload_image()

    def update_status(self, message):
        #self.status_dialog.config(text=message)
        self.canvas.itemconfig(self.status_text, text=message)
        # ensure the status dialog is updated immediately
        self.master.update_idletasks()

    def toggle_color_palette(self):
        image_height = 400
        if not self.showing_palette:
            # Generate the color palette
            color_palette = self.params.iter_to_color()
            # flip top-to-bottom
            color_palette = np.flipud(color_palette)
            # Create an array of indices to broadcast along the new axis
            new_axis = np.arange(image_height)[:, np.newaxis]
            # Use broadcasting to expand the array
            expanded_array = color_palette[:, np.newaxis, :] + np.zeros((1, image_height, 1), dtype=color_palette.dtype)

            print(f'color_palette.shape: {color_palette.shape}')
            print(f'expanded_array.shape: {expanded_array.shape}')

            # Convert numpy array to PIL Image
            img = Image.fromarray(expanded_array, 'RGB')

            # Resize the image for visibility (optional, adjust as needed)
            # img = img.resize((400, self.params.maxiter), Image.NEAREST)

            # Convert PIL Image to Tkinter PhotoImage
            palette_image = ImageTk.PhotoImage(img)\

            # Create a new window for the palette
            self.palette_window = tk.Toplevel(self.master)
            self.palette_window.title("Color Palette")

            # Display the image in the new window
            self.palette_label = tk.Label(self.palette_window, image=palette_image)
            self.palette_label.pack()

            # Store reference to prevent garbage collection
            self.palette_label.image = palette_image

            # Update button text
            self.color_palette_button.config(text="Hide Color Palette")
            self.showing_palette = True
        else:
            # Close the palette window
            if self.palette_window:
                self.palette_window.destroy()
                self.palette_window = None
            self.color_palette_button.config(text="Show Color Palette")
            self.showing_palette = False

    def clear_debug_points(self):
        if self.debug_points is not None:
            for point in self.debug_points:
                self.canvas.delete(point)
        self.debug_points = []


    def toggle_draw_bounding_box(self, event=None):
        self.clear_debug_points()
        if event != CLEAR_EVENT and self.black_bounding_box_axis_line is None:
            largest_black_region, (x1, y1, x2, y2) = self.image_processor.find_largest_black_region(self.mandelbrot_array)
            (ax, ay, bx, by), debug_points = self.image_processor.find_black_shape_center(largest_black_region)
            for (x, y, color) in debug_points:
                x += x1
                y += y1
                rect = self.canvas.create_rectangle(x - 2, y - 2, x + 2, y + 2, fill=color)
                self.debug_points.append(rect)

            # add offset back
            ax += x1
            bx += x1
            ay += y1
            by += y1
            print(f'A: ({ax}, {ay})  B: ({bx}, {by})')

            w = x2 - x1
            h = y2 - y1
            print(f'toggle_draw_bounding_box: ({x1}, {y1}, {x2}, {y2}) w={w}  h={h}')
            self.black_bounding_box_axis_line = self.canvas.create_line(ax, ay, bx, by, fill='yellow', width=3)
        else:
            if self.black_bounding_box_axis_line:
                self.canvas.delete(self.black_bounding_box_axis_line)
                self.black_bounding_box_axis_line = None


if __name__ == "__main__":
    root = tk.Tk()
    root.title("Interactive Mandelbrot Set Display")
    root.resizable(True, True)
    display = InteractiveImageDisplay(root, 800, 600)
    display.update_status('Ready to explore the Mandelbrot set')
    root.mainloop()
