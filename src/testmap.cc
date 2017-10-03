#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/time.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/keysym.h>

#include "WinMgr.h"

/// _SET would be nice to have
#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1
#define _NET_WM_STATE_TOGGLE 2

#define KEY_MODMASK(x) ((x) & (ControlMask | ShiftMask | Mod1Mask))
#define BUTTON_MASK(x) ((x) & (Button1Mask | Button2Mask | Button3Mask))
#define BUTTON_MODMASK(x) ((x) & (ControlMask | ShiftMask | Mod1Mask | Button1Mask | Button2Mask | Button3Mask))

static char *displayName = 0;
static Display *display = 0;
static Colormap defaultColormap;
static Window root = None;
static Window window = None;


int rx = -1, ry = -1;
int wx = -1, wy = -1;
unsigned int ww = 0, wh = 0;

int main(int argc, char ** /*argv*/) {
    XSetWindowAttributes attr;

    assert((display = XOpenDisplay(displayName)) != 0);
    root = RootWindow(display, DefaultScreen(display));
    defaultColormap = DefaultColormap(display, DefaultScreen(display));

    window = XCreateWindow(display, root,
                           100,
                           100,
                           64, 64,
                           0,
                           CopyFromParent, InputOutput, CopyFromParent,
                           0, &attr);

    XSetWindowBackground(display, window, BlackPixel(display, DefaultScreen(display)));

    XSelectInput(display, window,
                 StructureNotifyMask |
                 ButtonPressMask | ButtonReleaseMask |
                 KeyPressMask | KeyReleaseMask |
                 PropertyChangeMask);

    XSelectInput(display, root, PropertyChangeMask);

    XMapRaised(display, window);

    while (1) {
        if (argc > 1) {
            int nwx, nwy;
            unsigned int nww, nwh;
            int nrx, nry;
            unsigned int bw;
            unsigned int depth;
            Window xroot, child;

            XGetGeometry(display, window, &xroot, &nwx, &nwy, &nww, &nwh, &bw, &depth);
            XTranslateCoordinates(display, window, root, 0, 0, &nrx, &nry, &child);

            if (nwx != wx || nwy != wy || nww != ww || nwh != wh || nrx != rx || nry != ry) {
                fprintf(stderr, "%d %d %d %d %d %d\n", nwx, nwy, nww, nwh, nrx, nry);
                wx = nwx;
                wy = nwy;
                ww = nww;
                wh = nwh;
                rx = nrx;
                ry = nry;
            }

            XFlush(display);
        } else {
            XEvent xev;
            XConfigureEvent &configure = xev.xconfigure;
//            XMapEvent &map = xev.xmap;

            XNextEvent(display, &xev);

            switch (xev.type) {
            case ConfigureNotify:
                fprintf(stderr, "ConfigureNotify %d %d %d %d %d\n",
                        configure.x, configure.y,
                        configure.width, configure.height,
                        configure.send_event);
                break;
            case MapNotify:
                fprintf(stderr, "MapNotify\n");
                break;
            case PropertyNotify:
                break;
            default:
                fprintf(stderr, "event=%d\n", xev.type);
                break;
            }
        }
    }
}

// vim: set sw=4 ts=4 et:
