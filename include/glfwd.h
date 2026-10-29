#ifndef INTRO_GLFWD_H
#define INTRO_GLFWD_H

#include "xlibfwd.h"

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

#define GLX_DOUBLEBUFFER                 5
#define GLX_RED_SIZE                     8
#define GLX_GREEN_SIZE                   9
#define GLX_BLUE_SIZE                    10
#define GLX_ALPHA_SIZE                   11
#define GLX_DEPTH_SIZE                   12
#define GLX_STENCIL_SIZE                 13
#define GLX_DRAWABLE_TYPE                0x8010
#define GLX_RENDER_TYPE                  0x8011
#define GLX_X_RENDERABLE                 0x8012
#define GLX_WINDOW_BIT                   0x00000001
#define GLX_RGBA_BIT                     0x00000001
#define GLX_CONTEXT_MAJOR_VERSION_ARB    0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB    0x2092
#define GLX_CONTEXT_PROFILE_MASK_ARB     0x9126
#define GLX_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001

#define GL_COLOR_BUFFER_BIT              0x00004000
#define GL_VERSION                       0x1F02

extern void glClearColor(float red, float green, float blue, float alpha);
extern void glClear(unsigned mask);
extern void glViewport(int x, int y, int width, int height);
extern const unsigned char *glGetString(unsigned name);

#endif
