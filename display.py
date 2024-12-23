from copy import copy
from MandelbrotFuncs import mandelbrot_set
from MandelbrotParams import MandelbrotParams
from PIL import Image, ImageTk
import tkinter as tk
from tkinter import filedialog, simpledialog
import numpy as np


class CurPointState:
    orig = 0 + 0j
    steps = 0
    history = None
    self._count = None  # cache for calc_escape
    def __init__(self, xpos, ypos):
        self.orig = xpos + ypos * 1j
        self.history = [self.orig]

    def go_left(self):
        '''
        pop off history (go back to previous iter)
        '''
        if self.steps > 0:
            self.history.pop()
            self.steps -= 1

    def go_right(self):
        '''
        calc next iter
        '''
        c = self.orig
        z = self.z()
        z = z * z + c
        self.steps += 1
        self.history.append(z)

    def z(self):
        return self.history[-1]

    def calc_escape(self, maxiter):
        '''
        return number of reps for escape (limited by maxiter)
        '''
        if self._count is None:
            c = self.orig
            z = c
            self._count = 0
            while abs(z) < 2 and count < maxiter:
                z = z * z + c
                self._count += 1
        return self._count


class InteractiveImageDisplay:
    image_on_canvas = None
    cur_point_rect = None
    cur_point_state = None
    
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

        # Create canvas
        self.canvas = tk.Canvas(master, width=width, height=height)
        self.canvas.pack()

        # draw image
        self.reset_image()
        
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

        # status dialog
        #self.status_dialog = tk.Label(self.master, text="", bd=1, relief=tk.SUNKEN, anchor=tk.W, height=2, width=50, bg='black', fg='red')
        #self.status_dialog.place(relx=1.0, rely=1.0, x=-10, y=-50, anchor=tk.SE)
        self.status_text = self.canvas.create_text(self.width - 10, self.height - 10,
                                                   anchor=tk.SE,
                                                   text='',
                                                   fill='red',
                                                   font=('Arial', 10))

    def set_initial_params(self):
        # Initial parameters for the Mandelbrot set
        initial_xmin, initial_xmax = -2.0, 1.0
        initial_ymin, initial_ymax = -1.5, 1.5
        maxiter = 128
        self.params = MandelbrotParams(initial_xmin, initial_xmax, initial_ymin, initial_ymax, self.width, self.height, maxiter)

    def reset_image(self):
        self.set_initial_params()
        self.mandelbrot = mandelbrot_set(self.params)
        self.reload_image()        

    def reload_image(self):
        # Normalize and convert to image
        normalized = (self.mandelbrot / self.params.maxiter * 255).astype(np.uint8)
        normalized = np.rot90(normalized, k=1)
        print(f'normalized.shape: {normalized.shape}')
        self.image = Image.fromarray(normalized, 'L')
        self.photo = ImageTk.PhotoImage(self.image)
        if self.image_on_canvas is None:
            # create the image
            self.image_on_canvas = self.canvas.create_image(0, 0, anchor=tk.NW, image=self.photo)
        else:
            # Update the image on the canvas
            self.canvas.itemconfig(self.image_on_canvas, image=self.photo)        

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
        print(f'on_button_release: x1: {x1}  x2: {x2}  y1: {y1}  y2: {y2}')
        if (x1 == x2) or (y1 == y2):
            # clear current rect
            self.canvas.delete(self.rect)
            xpos, ypos = self.params.image_to_complex(x1, y1)
            self.cur_point_state = CurPointState(xpos, ypos)
            self.show_cur_point()
            return

        self.params.zoom_by_bbox(x1, x2, y1, y2)
        
        # Recalculate the Mandelbrot set for the new region
        self.mandelbrot = mandelbrot_set(self.params)
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
        print(f'key_handler: event: {event}  event.state: {event.state}')
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
        print(f'iter_cur_point: orig=({z})  new=({x}, {y})')
        if self.cur_point_rect is None:
            self.cur_point_rect = self.canvas.create_rectangle(x, y, x+3, y+3, fill='blue', outline='blue')
        else:
            self.canvas.coords(self.cur_point_rect, x, y, x+3, y+3)
        self.update_status(f'steps: {self.cur_point_state.steps}  Z={self.cur_point_state.cur}')
        

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
            self.mandelbrot = mandelbrot_set(self.params)
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
    display = InteractiveImageDisplay(root, 800, 600)
    display.add_set_maxiter_button()
    display.update_status('Ready to explore the Mandelbrot set')
    root.mainloop()
