#include <assert.h>
#include <stdio.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

Display *display = 0;
Colormap defaultColormap;
Window root = 0;
Window win = 0;
Atom _XA_WM_DELETE_WINDOW;
Atom _XA_WM_STATE;

int main() {
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

    size.flags = USPosition | USSize;

    for (int rep = 0; rep < 100; rep++) {
        gettimeofday(&start, 0);

        win = XCreateWindow(display, root,
                            100, 100,
                            100, 100,
                            0,
                            CopyFromParent, InputOutput, CopyFromParent,
                            0, &attr);

        XSelectInput(display, win, ExposureMask | PropertyChangeMask);
        XSetWMProtocols(display, win, proto, sizeof(proto)/sizeof(proto[0]));
        XSetWMNormalHints(display, win, &size);
        XMapRaised(display, win);

        bool got_expose = false;
        bool got_state = false;
        while (true) {
            XNextEvent(display, &xev);
            if (xev.type == PropertyNotify && xev.xproperty.atom == _XA_WM_STATE)
                got_state = true;
            if (xev.type == Expose)
                got_expose = true;
            if (got_expose && got_state)
                break;

        }
        XUnmapWindow(display, win);

        got_state = false;
        while (!got_state) {
            XNextEvent(display, &xev);
            if (xev.type == PropertyNotify &&
                xev.xproperty.atom == _XA_WM_STATE)
                got_state = true;
        }
#if 1
        XDestroyWindow(display, win);
        while (XPending(display)) {
            XNextEvent(display, &xev);
        }
#endif

        gettimeofday(&end, 0);
        timersub(&end, &start, &total);
#define TF "%ld.%06ld"
#define TA(a) a.tv_sec, a.tv_usec
        printf("%02d map+unmap: " TF " sec\n", rep, TA(total));
    }
}
