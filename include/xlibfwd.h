#ifndef INTRO_XLIBFWD_H
#define INTRO_XLIBFWD_H

typedef struct _XDisplay Display;
typedef unsigned long Window;
typedef unsigned long Atom;
typedef unsigned long Colormap;
typedef unsigned long VisualID;
typedef unsigned long KeySym;

typedef struct {
    void *visual;
    VisualID visualid;
    int screen;
    int depth;
    int c_class;
    unsigned long red_mask, green_mask, blue_mask;
    int colormap_size;
    int bits_per_rgb;
} XVisualInfo;

typedef struct {
    unsigned long background_pixmap;
    unsigned long background_pixel;
    unsigned long border_pixmap;
    unsigned long border_pixel;
    int bit_gravity;
    int win_gravity;
    int backing_store;
    unsigned long backing_planes;
    unsigned long backing_pixel;
    int save_under;
    long event_mask;
    long do_not_propagate_mask;
    int override_redirect;
    unsigned long colormap;
    unsigned long cursor;
} XSetWindowAttributes;

extern Display *XOpenDisplay(const char *name);
extern int XDefaultScreen(Display *dpy);
extern Window XDefaultRootWindow(Display *dpy);
extern Colormap XCreateColormap(Display *dpy, Window parent, void *visual, int alloc);
extern Window XCreateWindow(Display *dpy, Window parent, int x, int y,
                            unsigned int width, unsigned int height,
                            unsigned int border_width, int depth,
                            unsigned int wclass, void *visual,
                            unsigned long valuemask, XSetWindowAttributes *attrs);
extern int XStoreName(Display *dpy, Window w, const char *name);
extern Atom XInternAtom(Display *dpy, const char *name, int only_if_exists);
extern int XSetWMProtocols(Display *dpy, Window w, Atom *protocols, int count);
extern int XMapWindow(Display *dpy, Window w);
extern int XFlush(Display *dpy);
extern int XPending(Display *dpy);
extern int XNextEvent(Display *dpy, void *event);
extern KeySym XKeycodeToKeysym(Display *dpy, unsigned int code, int index);
extern int XCloseDisplay(Display *dpy);
extern int XFree(void *data);

#define KeyPressMask            (1L << 0)
#define KeyReleaseMask          (1L << 1)
#define ExposureMask            (1L << 8)
#define StructureNotifyMask     (1L << 17)
#define KeyPress                2
#define DestroyNotify           17
#define UnmapNotify             18
#define Expose                  12
#define ConfigureNotify         22
#define ClientMessage           33
#define CWBorderPixel           (1L << 3)
#define CWEventMask             (1L << 11)
#define CWColormap              (1L << 13)
#define InputOutput             1
#define XK_Escape               0xff1bUL

typedef struct {
    int type;
    unsigned long serial;
    int send_event;
    Display *display;
    unsigned long window;
} XAnyEventMin;

typedef struct {
    int type;
    unsigned long serial;
    int send_event;
    Display *display;
    unsigned long window, root, subwindow, time;
    int x_root, y_root, x, y;
    unsigned int state;
    unsigned int keycode;
    int same_screen;
} XKeyEventMin;

typedef struct {
    int type;
    unsigned long serial;
    int send_event;
    Display *display;
    unsigned long window;
    int x, y;
    unsigned int width, height;
    int border_width;
    unsigned long above;
    int override_redirect;
} XConfigureEventMin;

typedef struct {
    int type;
    unsigned long serial;
    int send_event;
    Display *display;
    unsigned long window;
    Atom message_type;
    int format;
    unsigned long data[5];
} XClientMessageEventMin;

#endif
