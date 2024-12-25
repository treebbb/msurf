from copy import copy
from MandelbrotFuncs import MandelbrotFuncs
from MandelbrotParams import MandelbrotParams
from PIL import Image, ImageTk
import tkinter as tk
from tkinter import filedialog, simpledialog
import numpy as np


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

        # status dialog
        #self.status_dialog = tk.Label(self.master, text="", bd=1, relief=tk.SUNKEN, anchor=tk.W, height=2, width=50, bg='black', fg='red')
        #self.status_dialog.place(relx=1.0, rely=1.0, x=-10, y=-50, anchor=tk.SE)
        self.status_text = self.canvas.create_text(self.width - 10, self.height - 10,
                                                   anchor=tk.SE,
                                                   text='',
                                                   fill='red',
                                                   font=('Arial', 10))

        # Bind the configure event to handle window resizing
        self.master.bind('<Configure>', self.on_resize)

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
        # Normalize and convert to image
        mandelbrot = self.mandelbrot_funcs.mandelbrot_set_opencl(self.params)
        normalized = (mandelbrot / self.params.maxiter * 255).astype(np.uint8)
        normalized = np.rot90(normalized, k=1)
        #print(f'normalized.shape: {normalized.shape}')
        # clear cur_point
        self.cur_point_state = None  # remove point
        if self.cur_point_rect:
            self.canvas.delete(self.cur_point_rect)
            self.cur_point_rect = None
        # draw image
        self.image = Image.fromarray(normalized, 'L')
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

    # If you want to trigger this dialog from a button or menu
    def add_set_maxiter_button(self):
        """Add a button to the display to open the maxiter setting dialog."""
        button = tk.Button(self.master, text="Set Max Iterations", command=self.set_maxiter_dialog)
        button.pack()

    def update_status(self, message):
        #self.status_dialog.config(text=message)
        self.canvas.itemconfig(self.status_text, text=message)
        # ensure the status dialog is updated immediately
        self.master.update_idletasks()


if __name__ == "__main__":
    root = tk.Tk()
    root.title("Interactive Mandelbrot Set Display")
    root.resizable(True, True)
    display = InteractiveImageDisplay(root, 800, 600)
    display.add_set_maxiter_button()
    display.update_status('Ready to explore the Mandelbrot set')
    root.mainloop()
