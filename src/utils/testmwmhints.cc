#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/time.h>
//#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
//#include <X11/Xresource.h>
#include <X11/keysym.h>

#include "WinMgr.h"
#include "MwmUtil.h"

#define KEY_MODMASK(x) ((x) & (ControlMask | ShiftMask | Mod1Mask))

char *displayName = 0;
Display *display = 0;
Colormap defaultColormap;
Window root = None;
Window window = None;
GC gc;

Atom _XATOM_MWM_HINTS = None;

int main() {
    XSetWindowAttributes attr;

    assert((display = XOpenDisplay(displayName)) != 0);
    root = RootWindow(display, DefaultScreen(display));
    defaultColormap = DefaultColormap(display, DefaultScreen(display));

    window = XCreateWindow(display, root,
                           0,
                           0,
                           300, 240,
                           0,
                           CopyFromParent, InputOutput, CopyFromParent,
                           0, &attr);

    _XATOM_MWM_HINTS = XInternAtom(display, _XA_MOTIF_WM_HINTS, False);

    XSetWindowBackground(display, window, BlackPixel(display, DefaultScreen(display)));

    XSelectInput(display, window,
                 ExposureMask | StructureNotifyMask |
                 ButtonPressMask | ButtonReleaseMask |
                 KeyPressMask | KeyReleaseMask |
                 PropertyChangeMask);

    XSelectInput(display, root, PropertyChangeMask);
    
    XMapRaised(display, window);

    MwmHints mwm = { 0, 0, 0, 0, 0 };

    mwm.flags = MWM_HINTS_DECORATIONS | MWM_HINTS_FUNCTIONS;
    mwm.decorations = MWM_DECOR_ALL;
    mwm.functions = MWM_FUNC_ALL;

    XChangeProperty(display, window,
                    _XATOM_MWM_HINTS, _XATOM_MWM_HINTS,
                    32, PropModeReplace,
                    (const unsigned char *)&mwm, sizeof(mwm) / sizeof(long));

    puts("decor: i=min, m=max, t=title, s=menu, r=resize, b=border, a=all");
    puts("func: C=close, A=all");
    
    while (1) {
        XEvent xev;
        //XButtonEvent &button = xev.xbutton;
        //XPropertyEvent &property = xev.xproperty;
        XKeyEvent &key = xev.xkey;

        XNextEvent(display, &xev);

        switch (xev.type) {
        case KeyPress:
            {
                unsigned int k = XKeycodeToKeysym(display, key.keycode, 0);
                unsigned int m = KEY_MODMASK(key.state);

                if (k == XK_Delete) {
                    XDeleteProperty(display, window, _XATOM_MWM_HINTS);
                } else {
                    if (m & ShiftMask) {
                    if (k == 'c')
                        mwm.functions ^= MWM_FUNC_CLOSE;
                    if (k == 'a')
                        mwm.functions ^= MWM_FUNC_ALL;

                    } else {
                        if (k == 'i')
                            mwm.decorations ^= MWM_DECOR_MINIMIZE;
                        if (k == 'm')
                            mwm.decorations ^= MWM_DECOR_MAXIMIZE;
                        if (k == 't')
                            mwm.decorations ^= MWM_DECOR_TITLE;
                        if (k == 's')
                            mwm.decorations ^= MWM_DECOR_MENU;
                        if (k == 'r')
                            mwm.decorations ^= MWM_DECOR_RESIZEH;
                        if (k == 'b')
                            mwm.decorations ^= MWM_DECOR_BORDER;
                        if (k == 'a')
                            mwm.decorations ^= MWM_DECOR_ALL;
                        if (k == 'q')
                            exit(1);
                    }
                    XChangeProperty(display, window,
                                    _XATOM_MWM_HINTS, _XATOM_MWM_HINTS,
                                    32, PropModeReplace,
                                    (const unsigned char *)&mwm, sizeof(mwm) / sizeof(long));
                }

            }
            break;
        }
    }
}
