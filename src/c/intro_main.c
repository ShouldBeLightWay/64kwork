/*
 * Freestanding X11 (Xlib) + GLX base: window + OpenGL 4.1 core context.
 * No libc or system headers: only project-owned ABI declarations are used.
 * libX11/libGL are linked dynamically, so they cost nothing in the 64k body.
 */

#include <stddef.h>

#include "glfwd.h"
#include "minicrt/syscall.h"

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
