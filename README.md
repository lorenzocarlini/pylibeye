
# pylibeye

`pylibeye` is a high-performance C library designed for acquiring X11 video streams with minimal performance overhead. The library achieves efficient video data processing by directly accessing and processing video data within the X11 video buffer using pointers. This reduces data transfer delays and improves processing speed for real-time applications.

## Overview

- **libeye.c**: The core C library that handles X11 video stream acquisition.
- **pylibeye.py**: A Python wrapper around `libeye.so` for easy integration into Python applications.

### Building the Library

To compile the `libeye.c` file into a shared library, run the following command:

```bash
bash build.sh
```

This will generate the `libeye.so` file, which is required for Python interaction.

### Python Wrapper

The `pylibeye.py` file provides a Python interface for interacting with the C library. It allows users to easily retrieve video streams from X11 windows, update frames, and process images using NumPy arrays.

### Usage

Once the library is compiled, you can use the `pylibeye.py` wrapper to access the X11 video streams. Here's a short guide on how to use it in Python.

1. **Import the necessary modules:**

```python
import ctypes
import numpy as np
from ctypes import c_int, c_void_p, POINTER, c_byte
```

2. **Load the shared C library:**

```python
lib = ctypes.CDLL('./libeye.so')
```

3. **Define the function prototypes and structures:**

```python
class WindowInfo(ctypes.Structure):
    _fields_ = [("window", ctypes.c_ulong),  # Window ID
                ("title", ctypes.c_char * 256)]  # Window title (max 256 characters)
```

4. **Interact with the library through the `XImageAcquisition` class.** You can initialize acquisition for a window, update image data, and retrieve the RGBA frame data as a NumPy array.

### Example Python Code

The following example demonstrates how to list all open windows, acquire video frames, and update the image data.

```python
import ctypes
import numpy as np
import cv2
import sys
from pylibeye import XImageAcquisition  # Import the acquisition module

# Example Usage
if __name__ == "__main__":
    
    if len(sys.argv) != 2:
        print("Please, provide a valid Windows ID")
        exit(1)
        
    
    proposed_win_id = int(sys.argv[1])
    

    # window_id = 6354413  # Example window ID
    window_id = proposed_win_id

    # Initialize the acquisition with the given window ID
    acquisition = XImageAcquisition(window_id)

    # Capture the image data
    image_data_np = acquisition.update()

    # Print the numpy array shape and the first few pixel values (for demo purposes)
    print(image_data_np.shape)  # (dynamic height, dynamic width, 4)
    print(image_data_np[0, 0])  # First pixel (RGB values)

    # Display the image
    cv2.namedWindow("Acquisition", cv2.WINDOW_NORMAL)
    cv2.imshow("Acquisition", image_data_np)

    while True:
        image_data_np = acquisition.update()
        cv2.imshow("Acquisition", image_data_np)
        if cv2.waitKey(33) == ord('a'):
            break

    # Close acquisition
    acquisition.close()
```

This Python script uses OpenCV to display the live video stream from a specified X11 window. You can test the functionality with:

```bash
python3 display_image.py WINDOW_ID
```

Replace `WINDOW_ID` with the ID of a window you wish to capture video from.

### License

This project is licensed under the MIT License.
