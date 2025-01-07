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
