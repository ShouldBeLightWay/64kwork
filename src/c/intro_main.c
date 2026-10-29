/*
 * Freestanding X11 (Xlib) + GLX base: window + OpenGL 4.1 core context.
 * No libc, no headers: only opaque types and manual prototypes.
 * libX11/libGL are linked dynamically, so they cost nothing in the 64k body.
 */

#include <stddef.h>

/* ---------- minimal syscall helpers (stderr out, sleep) ---------- */

static long sys_write(int fd, const void *buf, unsigned long len)
{
    long ret;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(1L), "D"((long)fd), "S"(buf), "d"(len)
                     : "rcx", "r11", "memory");
    return ret;
}

struct timespec { long tv_sec; long tv_nsec; };

static void sys_sleep_ms(long ms)
{
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
    long ret;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(35L), "D"(&ts), "S"((void *)0)
                     : "rcx", "r11", "memory");
}

static unsigned long kstrlen(const char *s)
{
    const char *p = s;
    while (*p)
        p++;
    return (unsigned long)(p - s);
}

static void kputs(const char *s)
{
    sys_write(2, s, kstrlen(s));
}

/* ---------- X11 types and prototypes ---------- */

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

/* X event constants */
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
/* XSetWindowAttributes bits */
#define CWBorderPixel           (1L << 3)
#define CWEventMask             (1L << 11)
#define CWColormap              (1L << 13)
#define InputOutput             1
#define XK_Escape               0xff1bUL

/* Events are passed as a 24-long padded buffer (XEvent union layout). */
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

/* ---------- GLX types and prototypes ---------- */

typedef struct __GLXFBConfigRec *GLXFBConfig;
typedef struct __GLXcontextRec *GLXContext;
typedef unsigned long GLXDrawable;
typedef void (*GLXProcPtr)(void);

extern GLXFBConfig *glXChooseFBConfig(Display *dpy, int screen,
                                      const int *attrib_list, int *nitems);
extern XVisualInfo *glXGetVisualFromFBConfig(Display *dpy, GLXFBConfig fbconfig);
extern GLXProcPtr glXGetProcAddressARB(const unsigned char *name);
extern int glXMakeCurrent(Display *dpy, GLXDrawable drawable, GLXContext ctx);
extern void glXSwapBuffers(Display *dpy, GLXDrawable drawable);
extern void glXDestroyContext(Display *dpy, GLXContext ctx);

typedef GLXContext (*PFNGLXCREATECONTEXTATTRIBSARB)(
    Display *dpy, GLXFBConfig fbconfig, GLXContext share,
    int direct, const int *attrib_list);

/* GLX FBConfig attribute names */
#define GLX_DOUBLEBUFFER        5
#define GLX_RED_SIZE            8
#define GLX_GREEN_SIZE          9
#define GLX_BLUE_SIZE           10
#define GLX_ALPHA_SIZE          11
#define GLX_DEPTH_SIZE          12
#define GLX_STENCIL_SIZE        13
#define GLX_DRAWABLE_TYPE       0x8010
#define GLX_RENDER_TYPE         0x8011
#define GLX_X_RENDERABLE        0x8012
#define GLX_WINDOW_BIT          0x00000001
#define GLX_RGBA_BIT            0x00000001
/* GLX_ARB_create_context */
#define GLX_CONTEXT_MAJOR_VERSION_ARB   0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB   0x2092
#define GLX_CONTEXT_PROFILE_MASK_ARB    0x9126
#define GLX_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001

/* ---------- minimal GL prototypes ---------- */

#define GL_COLOR_BUFFER_BIT     0x00004000
#define GL_VERSION              0x1F02

extern void glClearColor(float red, float green, float blue, float alpha);
extern void glClear(unsigned mask);
extern void glViewport(int x, int y, int width, int height);
extern const unsigned char *glGetString(unsigned name);

/* ---------- intro ---------- */

#define WIN_W 1024
#define WIN_H 640

int intro_main(int argv, char *argvc)
{
    (void)argv;
    (void)argvc;

    Display *dpy = XOpenDisplay((void *)0);
    if (!dpy) {
        kputs("x11: cannot open display\n");
        return 1;
    }

    int screen = XDefaultScreen(dpy);
    Window root = XDefaultRootWindow(dpy);
    (void)screen;

    int fb_attribs[] = {
        GLX_X_RENDERABLE, 1,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DOUBLEBUFFER, 1,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        GLX_STENCIL_SIZE, 8,
        0
    };

    int nfb = 0;
    GLXFBConfig *fbconfigs = glXChooseFBConfig(dpy, screen, fb_attribs, &nfb);
    if (!fbconfigs || nfb < 1) {
        kputs("glx: no matching FBConfig\n");
        return 2;
    }
    GLXFBConfig fb = fbconfigs[0];
    XFree(fbconfigs);

    XVisualInfo *vi = glXGetVisualFromFBConfig(dpy, fb);
    if (!vi) {
        kputs("glx: no visual for FBConfig\n");
        return 3;
    }
    void *visual = vi->visual;
    int depth = vi->depth;
    XFree(vi);

    Colormap cmap = XCreateColormap(dpy, root, visual, 0);

    XSetWindowAttributes attrs;
    attrs.background_pixmap = 0;
    attrs.background_pixel = 0;
    attrs.border_pixmap = 0;
    attrs.border_pixel = 0;
    attrs.bit_gravity = 0;
    attrs.win_gravity = 0;
    attrs.backing_store = 0;
    attrs.backing_planes = 0;
    attrs.backing_pixel = 0;
    attrs.save_under = 0;
    attrs.event_mask = KeyPressMask | ExposureMask | StructureNotifyMask;
    attrs.do_not_propagate_mask = 0;
    attrs.override_redirect = 0;
    attrs.colormap = cmap;
    attrs.cursor = 0;

    Window win = XCreateWindow(dpy, root, 0, 0, WIN_W, WIN_H, 0, depth,
                               InputOutput, visual,
                               CWColormap | CWBorderPixel | CWEventMask, &attrs);
    if (!win) {
        kputs("x11: window creation failed\n");
        return 4;
    }

    XStoreName(dpy, win, "64kwork intro");

    Atom wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", 0);
    Atom wm_protocols_atom = XInternAtom(dpy, "WM_PROTOCOLS", 0);
    XSetWMProtocols(dpy, win, &wm_delete, 1);

    XMapWindow(dpy, win);
    XFlush(dpy);

    PFNGLXCREATECONTEXTATTRIBSARB createContext =
        (PFNGLXCREATECONTEXTATTRIBSARB)glXGetProcAddressARB(
            (const unsigned char *)"glXCreateContextAttribsARB");
    if (!createContext) {
        kputs("glx: glXCreateContextAttribsARB missing\n");
        return 5;
    }

    int ctx_attribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
        GLX_CONTEXT_MINOR_VERSION_ARB, 1,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };

    GLXContext ctx = createContext(dpy, fb, (void *)0, 1, ctx_attribs);
    if (!ctx) {
        kputs("glx: cannot create GL 4.1 core context\n");
        return 6;
    }

    if (!glXMakeCurrent(dpy, win, ctx)) {
        kputs("glx: glXMakeCurrent failed\n");
        return 7;
    }

    const unsigned char *ver = glGetString(GL_VERSION);
    kputs("GL context: ");
    sys_write(2, ver, kstrlen((const char *)ver));
    kputs(" (4.1 core base ready)\n");

    int running = 1;
    int width = WIN_W, height = WIN_H;
    unsigned long frame = 0;
    long ev[24];

    while (running) {
        while (XPending(dpy)) {
            XNextEvent(dpy, ev);
            XAnyEventMin *any = (XAnyEventMin *)ev;
            if (any->window != win && any->type != ClientMessage)
                continue;
            switch (any->type) {
            case KeyPress: {
                XKeyEventMin *key = (XKeyEventMin *)ev;
                KeySym ks = XKeycodeToKeysym(dpy, key->keycode, 0);
                if (ks == XK_Escape || ks == 'q' || ks == 'Q')
                    running = 0;
                break;
            }
            case ClientMessage: {
                XClientMessageEventMin *msg = (XClientMessageEventMin *)ev;
                if (msg->message_type == wm_protocols_atom &&
                    msg->data[0] == wm_delete)
                    running = 0;
                break;
            }
            case Expose:
                break;
            case UnmapNotify:
            case DestroyNotify:
                running = 0;
                break;
            case ConfigureNotify: {
                XConfigureEventMin *cfg = (XConfigureEventMin *)ev;
                width = (int)cfg->width;
                height = (int)cfg->height;
                break;
            }
            }
        }

        glViewport(0, 0, width, height);

        float t = (float)(frame & 0x3f) / 63.0f;
        glClearColor(t, 1.0f - t, 0.25f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glXSwapBuffers(dpy, win);

        frame++;
        sys_sleep_ms(16);
    }

    glXMakeCurrent(dpy, 0, (void *)0);
    glXDestroyContext(dpy, ctx);
    XCloseDisplay(dpy);
    return 0;
}
