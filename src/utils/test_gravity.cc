#include <assert.h>
#include <stdio.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>

Display *display = 0;
Colormap defaultColormap;
Window root = 0;
Window win = 0;
Atom _XA_WM_DELETE_WINDOW;
Atom _XA_WM_STATE;

int main(int argc, char **argv) {
    assert((display = XOpenDisplay(0)) != 0);
    root = RootWindow(display, DefaultScreen(display));
    defaultColormap = DefaultColormap(display, DefaultScreen(display));
    _XA_WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", False);
    _XA_WM_STATE = XInternAtom(display, "WM_STATE", False);
    struct timeval start, end, total;
    XSetWindowAttributes attr;
    XSizeHints size;
    XEvent xev;
    Atom proto[1];
    proto[0] = _XA_WM_DELETE_WINDOW;
    int ww = 151;
    int hh = 251;

    int grav = strtol(argv[1], NULL, 0);
    size.win_gravity = grav;
    size.flags = USSize | PWinGravity;
    //attr.win_gravity = grav;

    win = XCreateWindow(display, root,
                        100, 100,
                        ww, hh,
                        0,
                        CopyFromParent, InputOutput, CopyFromParent,
                        0, &attr);

    XSetWindowBackground(display, win,
                         BlackPixel(display, DefaultScreen(display)));
    XSelectInput(display, win, ExposureMask | PropertyChangeMask | ButtonPressMask);
    XSetWMProtocols(display, win, proto, sizeof(proto)/sizeof(proto[0]));
    XSetWMNormalHints(display, win, &size);
    XMapRaised(display, win);

    while (true) {
        XNextEvent(display, &xev);
        if (xev.type == Expose) {

        } else if (xev.type == ClientMessage &&
                   xev.xclient.data.l[0] == _XA_WM_DELETE_WINDOW)
        {
            break;
        } else if (xev.type == ButtonPress) {
            if (xev.xbutton.button == 1) {
                ww += 10;
                hh += 10;
                XResizeWindow(display, win, ww, hh);
            } else if (xev.xbutton.button == 3) {
                ww -= 10;
                hh -= 10;
                XResizeWindow(display, win, ww, hh);
            }
        }
    }
}
