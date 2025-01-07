#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/shm.h>
#include <stdint.h>

/* Structure to hold window information */
typedef struct {
    Window window;
    char title[256];
} WindowInfo;

static Display *display = NULL;
static int screen;
static XImage *image = NULL;
static XShmSegmentInfo shminfo;
static Window rootwindow;
static Pixmap pixmap;
static Window xid;
static int width, height;
static void *image_data = NULL;

/**
 * Finds the client window associated with a given window.
 * If no frame window is found, returns the given window.
 */
Window find_client_window(Display *display, Window window) {
    Atom frame_atom = XInternAtom(display, "_NET_WM_FRAME_WINDOW", False);
    Atom type;
    int format;
    unsigned long nitems, bytes_after;
    unsigned char *prop = NULL;

    if (XGetWindowProperty(display, window, frame_atom, 0, (~0L), False,
                           XA_WINDOW, &type, &format, &nitems, &bytes_after, &prop) == Success && prop) {
        Window client_window = *(Window *)prop;
        XFree(prop);
        return client_window;
    }

    return window; // Fallback to the original window if no frame is found
}

/**
 * Retrieves the list of open windows and their titles.
 * Returns 0 on success and fills the window_info array with details of each window.
 */
int get_window_titles(WindowInfo **window_info, int *count) {
    Display *display = XOpenDisplay(NULL);
    if (!display) return -1;

    Atom client_list_atom = XInternAtom(display, "_NET_CLIENT_LIST_STACKING", False);
    if (client_list_atom == None) client_list_atom = XInternAtom(display, "_NET_CLIENT_LIST", False);

    if (client_list_atom == None) {
        XCloseDisplay(display);
        return -1;
    }

    Atom type;
    int format;
    unsigned long nitems, bytes_after;
    unsigned char *prop = NULL;

    if (XGetWindowProperty(display, DefaultRootWindow(display), client_list_atom, 0, (~0L), False,
                           XA_WINDOW, &type, &format, &nitems, &bytes_after, &prop) != Success || !prop) {
        XCloseDisplay(display);
        return -1;
    }

    Window *clients = (Window *)prop;
    *count = (int)nitems;

    *window_info = (WindowInfo *)malloc(sizeof(WindowInfo) * (*count));
    if (!*window_info) {
        XFree(prop);
        XCloseDisplay(display);
        return -1;
    }

    int valid_count = 0;
    for (int i = 0; i < *count; ++i) {
        Window client_window = find_client_window(display, clients[i]);
        XTextProperty window_name;
        if (XGetWMName(display, client_window, &window_name)) {
            XWindowAttributes attrs;
            if (XGetWindowAttributes(display, client_window, &attrs) && attrs.map_state == IsViewable) {
                strncpy((*window_info)[valid_count].title, (char *)window_name.value, 255);
                (*window_info)[valid_count].title[255] = '\0';
                (*window_info)[valid_count].window = client_window;
                valid_count++;
            }
        }
    }

    *count = valid_count;
    XFree(prop);
    XCloseDisplay(display);
    return 0;
}

/**
 * Frees the allocated memory for window information.
 */
void free_window_info(WindowInfo *window_info) {
    free(window_info);
}

/**
 * Initializes the window acquisition process for a given window ID.
 * This includes setting up shared memory and the necessary X11 structures.
 */
int create_acquisition(Window window_id) {
    display = XOpenDisplay(NULL);
    if (!display) {
        return -1;
    }

    screen = DefaultScreen(display);
    xid = find_client_window(display, window_id);

    XWindowAttributes attr;
    if (!XGetWindowAttributes(display, xid, &attr)) {
        return -1;
    }

    width = attr.width;
    height = attr.height;

    // Ensure the composite extension is available
    int event_base, error_base;
    if (!XCompositeQueryExtension(display, &event_base, &error_base)) {
        return -1;
    }

    // Redirect the window if not already redirected
    XCompositeRedirectWindow(display, xid, CompositeRedirectAutomatic);

    // Create a pixmap from the window
    pixmap = XCompositeNameWindowPixmap(display, xid);
    if (!pixmap) {
        return -1;
    }

    // Use the window's visual and depth
    image = XShmCreateImage(display, attr.visual, attr.depth, ZPixmap, NULL, &shminfo, width, height);
    if (!image) {
        return -1;
    }

    shminfo.shmid = shmget(IPC_PRIVATE, image->bytes_per_line * image->height, IPC_CREAT | 0777);
    if (shminfo.shmid == -1) {
        return -1;
    }

    shminfo.shmaddr = image->data = shmat(shminfo.shmid, NULL, 0);
    shminfo.readOnly = False;

    if (!XShmAttach(display, &shminfo)) {
        return -1;
    }

    rootwindow = DefaultRootWindow(display);
    return 0;
}

/**
 * Updates the internal image data by capturing the current state of the window's pixmap.
 */
int update_array() {
    if (!display || !image) {
        return -1;
    }

    pixmap = XCompositeNameWindowPixmap(display, xid);
    if (!pixmap) {
        return -1;
    }

    if (!XShmGetImage(display, pixmap, image, 0, 0, AllPlanes)) {
        return -1;
    }

    image_data = image->data;
    return 0;
}

/**
 * Returns a pointer to the current image data.
 */
void *get_image_data() {
    return image_data;
}

/**
 * Retrieves the current width and height of the window being captured.
 */
void get_window_size(int *w, int *h) {
    *w = width;
    *h = height;
}

/**
 * Retrieves the stride (byte width of each row) of the image data.
 */
int get_stride() {
    return image ? image->bytes_per_line : 0;
}

/**
 * Closes the acquisition process and frees any allocated resources.
 */
void close_acquisition() {
    if (image) {
        XShmDetach(display, &shminfo);
        shmdt(shminfo.shmaddr);
        shmctl(shminfo.shmid, IPC_RMID, 0);
        XDestroyImage(image);
    }

    if (display) {
        XCloseDisplay(display);
    }
}
