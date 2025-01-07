import ctypes
import numpy as np
from ctypes import c_int, c_void_p, POINTER, c_byte

# Load the shared C library
lib = ctypes.CDLL('./libeye.so')

# Define the structure representing window information
class WindowInfo(ctypes.Structure):
    _fields_ = [("window", ctypes.c_ulong),  # Window ID
                ("title", ctypes.c_char * 256)]  # Window title (max 256 characters)

# Define C function prototypes for integration
lib.get_window_titles.argtypes = [ctypes.POINTER(ctypes.POINTER(WindowInfo)), ctypes.POINTER(ctypes.c_int)]
lib.get_window_titles.restype = ctypes.c_int

lib.free_window_info.argtypes = [ctypes.POINTER(WindowInfo)]

lib.create_acquisition.argtypes = [c_int]
lib.create_acquisition.restype = c_int

lib.update_array.argtypes = []
lib.update_array.restype = c_int

lib.close_acquisition.argtypes = []
lib.close_acquisition.restype = None

lib.get_image_data.argtypes = []
lib.get_image_data.restype = c_void_p  # Pointer to image data returned by C function

lib.get_window_size.argtypes = [ctypes.POINTER(c_int), ctypes.POINTER(c_int)]
lib.get_window_size.restype = None

lib.get_stride.argtypes = []
lib.get_stride.restype = c_int

class XImageAcquisition:
    """
    A Python wrapper for a C-based image acquisition system, designed for capturing 
    real-time RGBA image data from a specified window.
    """
    def __init__(self, window_id):
        """
        Initializes the image acquisition for a specified window ID.
        
        Args:
            window_id (int): The ID of the target window for acquisition.
        
        Raises:
            RuntimeError: If initialization fails.
        """
        self.window_id = window_id
        
        # Initialize acquisition for the specified window
        if lib.create_acquisition(self.window_id) != 0:
            raise RuntimeError(f"Failed to initialize image acquisition for window ID {self.window_id}")
        
        # Get window dimensions (width and height)
        self.width = c_int(0)
        self.height = c_int(0)
        lib.get_window_size(ctypes.byref(self.width), ctypes.byref(self.height))

        # Retrieve image data stride (bytes per line)
        self.stride = lib.get_stride()

    def update(self):
        """
        Updates the image data by fetching the latest frame.

        Returns:
            np.ndarray: A NumPy array containing the updated RGBA image data.
        
        Raises:
            RuntimeError: If the update operation fails.
        """
        if lib.update_array() != 0:
            raise RuntimeError("Failed to update image data.")
        
        return self.get_image_data()

    def get_image_data(self):
        """
        Converts the raw image data into a properly formatted NumPy array.

        Returns:
            np.ndarray: A (height, width, 4) NumPy array representing the RGBA image.
        """
        # Retrieve raw image data pointer
        image_data_ptr = ctypes.cast(lib.get_image_data(), POINTER(c_byte))

        # Create a NumPy array with raw data and stride information
        np_array = np.ctypeslib.as_array(image_data_ptr, shape=(self.height.value, self.stride))

        # Remove padding introduced by stride to match actual width
        np_array = np_array[:, :self.width.value * 4]

        # Interpret data as uint8 and reshape to (height, width, 4)
        np_array = np_array.astype(np.uint8).reshape((self.height.value, self.width.value, 4))

        # Ensure alpha channel is fully opaque
        np_array[..., 3] = 255

        return np_array

    def close(self):
        """
        Closes the image acquisition system and releases resources.
        """
        lib.close_acquisition()


def list_open_windows():
    """
    Retrieves a list of open windows with their IDs and titles.

    Returns:
        list[dict]: A list of dictionaries containing 'window_id' and 'title'.
    """
    window_info = ctypes.POINTER(WindowInfo)()
    count = ctypes.c_int(0)

    # Fetch window information
    result = lib.get_window_titles(ctypes.byref(window_info), ctypes.byref(count))
    if result != 0:
        print("Error retrieving window list")
        return []

    # Parse the results into a list of dictionaries
    windows = []
    for i in range(count.value):
        title = window_info[i].title.decode('utf-8', errors='ignore').strip()
        windows.append({"window_id": window_info[i].window, "title": title})

    # Free allocated resources
    lib.free_window_info(window_info)
    return windows


if __name__ == "__main__":
    # Example usage: List all open windows and allow selection for image acquisition
    windows = list_open_windows()
    if not windows:
        print("No windows found.")
    else:
        print("Available windows:")
        for win in windows:
            print(f"Window ID: {win['window_id']} Title: {win['title']}")
