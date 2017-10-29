#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>

#include "WinMgr.h"

/// _SET would be nice to have
#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1
#define _NET_WM_STATE_TOGGLE 2

#define KEY_MODMASK(x) ((x) & (ControlMask | ShiftMask | Mod1Mask))
#define BUTTON_MASK(x) ((x) & (Button1Mask | Button2Mask | Button3Mask))
#define BUTTON_MODMASK(x) ((x) & (ControlMask | ShiftMask | Mod1Mask | Button1Mask | Button2Mask | Button3Mask))

#define COUNT(a)    (int(sizeof a / sizeof(*a)))

typedef unsigned long Pixel;

void tell(const char* msg, ...) {
    FILE* output = stdout;
    timeval now;
    gettimeofday(&now, NULL);
    struct tm *loc = localtime(&now.tv_sec);

    fprintf(output, "%02d.%03u: ",
            /* loc->tm_hour, loc->tm_min, */
            loc->tm_sec, (unsigned)(now.tv_usec / 1000));

    va_list ap;
    va_start(ap, msg);
    vfprintf(output, msg, ap);
    va_end(ap);
    fflush(output);
}

class TDisplay {
    Display *dpy;
public:
    TDisplay() : dpy(0) { }
    Display* display() { return dpy ? dpy : dpy = XOpenDisplay(0); }
    ~TDisplay() { if (dpy) { XCloseDisplay(dpy); dpy = 0; } }
    operator Display*() { return display(); }
    operator _XPrivDisplay() { return _XPrivDisplay(display()); }
};

static TDisplay display;
static Colormap colormap;
static Window root = None;
static Window window = None;
static Window unmapped = None;
///static GC gc;

static long workspaceCount = 4;
static long activeWorkspace = 0;
static long windowWorkspace = 0;
//static long state[10] = { 0, 0 };
///static bool sticky = false;

///static bool fullscreen = true;

class TAtom {
    const char* name;
    Atom atom;
public:
    explicit TAtom(const char* name) : name(name), atom(None) { }
    operator unsigned() { return atom ? atom :
        atom = XInternAtom(display, name, False); }
};

static TAtom _XA_WIN_WORKSPACE("_WIN_WORKSPACE");
static TAtom _XA_WIN_WORKSPACE_NAMES("_WIN_WORKSPACE_NAMES");
static TAtom _XA_WIN_STATE("_WIN_STATE");
static TAtom _XA_WIN_LAYER("_WIN_LAYER");
static TAtom _XA_NET_WM_ACTION_ABOVE("_NET_WM_ACTION_ABOVE");
static TAtom _XA_NET_WM_ACTION_BELOW("_NET_WM_ACTION_BELOW");
static TAtom _XA_NET_WM_ACTION_CHANGE_DESKTOP("_NET_WM_ACTION_CHANGE_DESKTOP");
static TAtom _XA_NET_WM_ACTION_CLOSE("_NET_WM_ACTION_CLOSE");
static TAtom _XA_NET_WM_ACTION_FULLSCREEN("_NET_WM_ACTION_FULLSCREEN");
static TAtom _XA_NET_WM_ACTION_HIDE("_NET_WM_ACTION_HIDE");
static TAtom _XA_NET_WM_ACTION_MAXIMIZE_HORZ("_NET_WM_ACTION_MAXIMIZE_HORZ");
static TAtom _XA_NET_WM_ACTION_MAXIMIZE_VERT("_NET_WM_ACTION_MAXIMIZE_VERT");
static TAtom _XA_NET_WM_ACTION_MINIMIZE("_NET_WM_ACTION_MINIMIZE");
static TAtom _XA_NET_WM_ACTION_MOVE("_NET_WM_ACTION_MOVE");
static TAtom _XA_NET_WM_ACTION_RESIZE("_NET_WM_ACTION_RESIZE");
static TAtom _XA_NET_WM_ACTION_SHADE("_NET_WM_ACTION_SHADE");
static TAtom _XA_NET_WM_ACTION_STICK("_NET_WM_ACTION_STICK");
static TAtom _XA_NET_WM_ALLOWED_ACTIONS("_NET_WM_ALLOWED_ACTIONS");
static TAtom _XA_NET_WM_STATE("_NET_WM_STATE");
static TAtom _XA_NET_WM_STATE_ABOVE("_NET_WM_STATE_ABOVE");
static TAtom _XA_NET_WM_STATE_BELOW("_NET_WM_STATE_BELOW");
static TAtom _XA_NET_WM_STATE_DEMANDS_ATTENTION("_NET_WM_STATE_DEMANDS_ATTENTION");
static TAtom _XA_NET_WM_STATE_FOCUSED("_NET_WM_STATE_FOCUSED");
static TAtom _XA_NET_WM_STATE_FULLSCREEN("_NET_WM_STATE_FULLSCREEN");
static TAtom _XA_NET_WM_STATE_HIDDEN("_NET_WM_STATE_HIDDEN");
static TAtom _XA_NET_WM_STATE_MAXIMIZED_HORZ("_NET_WM_STATE_MAXIMIZED_HORZ");
static TAtom _XA_NET_WM_STATE_MAXIMIZED_VERT("_NET_WM_STATE_MAXIMIZED_VERT");
static TAtom _XA_NET_WM_STATE_MODAL("_NET_WM_STATE_MODAL");
static TAtom _XA_NET_WM_STATE_SHADED("_NET_WM_STATE_SHADED");
static TAtom _XA_NET_WM_STATE_SKIP_PAGER("_NET_WM_STATE_SKIP_PAGER");
static TAtom _XA_NET_WM_STATE_SKIP_TASKBAR("_NET_WM_STATE_SKIP_TASKBAR");
static TAtom _XA_NET_WM_STATE_STICKY("_NET_WM_STATE_STICKY");
static TAtom _XA_NET_WM_MOVERESIZE("_NET_WM_MOVERESIZE");
static TAtom _XA_NET_REQUEST_FRAME_EXTENTS("_NET_REQUEST_FRAME_EXTENTS");
static TAtom _XA_NET_FRAME_EXTENTS("_NET_FRAME_EXTENTS");
static TAtom _XA_WM_DELETE_WINDOW("WM_DELETE_WINDOW");
static TAtom _XA_WM_PROTOCOLS("WM_PROTOCOLS");
static TAtom _XA_WM_TAKE_FOCUS("WM_TAKE_FOCUS");
static TAtom _XA_NET_WM_WINDOW_TYPE("_NET_WM_WINDOW_TYPE");
static TAtom _XA_NET_WM_WINDOW_TYPE_COMBO("_NET_WM_WINDOW_TYPE_COMBO");
static TAtom _XA_NET_WM_WINDOW_TYPE_DESKTOP("_NET_WM_WINDOW_TYPE_DESKTOP");
static TAtom _XA_NET_WM_WINDOW_TYPE_DIALOG("_NET_WM_WINDOW_TYPE_DIALOG");
static TAtom _XA_NET_WM_WINDOW_TYPE_DND("_NET_WM_WINDOW_TYPE_DND");
static TAtom _XA_NET_WM_WINDOW_TYPE_DOCK("_NET_WM_WINDOW_TYPE_DOCK");
static TAtom _XA_NET_WM_WINDOW_TYPE_DROPDOWN_MENU("_NET_WM_WINDOW_TYPE_DROPDOWN_MENU");
static TAtom _XA_NET_WM_WINDOW_TYPE_MENU("_NET_WM_WINDOW_TYPE_MENU");
static TAtom _XA_NET_WM_WINDOW_TYPE_NORMAL("_NET_WM_WINDOW_TYPE_NORMAL");
static TAtom _XA_NET_WM_WINDOW_TYPE_NOTIFICATION("_NET_WM_WINDOW_TYPE_NOTIFICATION");
static TAtom _XA_NET_WM_WINDOW_TYPE_POPUP_MENU("_NET_WM_WINDOW_TYPE_POPUP_MENU");
static TAtom _XA_NET_WM_WINDOW_TYPE_SPLASH("_NET_WM_WINDOW_TYPE_SPLASH");
static TAtom _XA_NET_WM_WINDOW_TYPE_TOOLBAR("_NET_WM_WINDOW_TYPE_TOOLBAR");
static TAtom _XA_NET_WM_WINDOW_TYPE_TOOLTIP("_NET_WM_WINDOW_TYPE_TOOLTIP");
static TAtom _XA_NET_WM_WINDOW_TYPE_UTILITY("_NET_WM_WINDOW_TYPE_UTILITY");

static const char* atomName(Atom atom) {
    static const size_t count = 1024;
    static char* atoms[count];
    if (atom < count && atoms[atom] != 0)
        return atoms[atom];
    char* name = XGetAtomName(display, atom);
    if (atom < count) atoms[atom] = name;
    return name;
}

void changeWorkspace(Window w, long workspace) {
    XClientMessageEvent xev = {};

    xev.type = ClientMessage;
    xev.window = w;
    xev.message_type = _XA_WIN_WORKSPACE;
    xev.format = 32;
    xev.data.l[0] = workspace;
    xev.data.l[1] = CurrentTime; //xev.data.l[1] = timeStamp;
    XSendEvent(display, root, False, SubstructureNotifyMask, (XEvent *) &xev);
}

void moveResize(Window w, int x, int y, int what) {
    XClientMessageEvent xev = {};

    xev.type = ClientMessage;
    xev.window = w;
    xev.message_type = _XA_NET_WM_MOVERESIZE;
    xev.format = 32;
    xev.data.l[0] = x;
    xev.data.l[1] = y; //xev.data.l[1] = timeStamp;
    xev.data.l[2] = what;
    XSendEvent(display, root, False, SubstructureNotifyMask, (XEvent *) &xev);
}

void toggleState(Window w, Atom toggle_state) {
    XClientMessageEvent xev = {};

    puts("toggle state");

    xev.type = ClientMessage;
    xev.window = w;
    xev.message_type = _XA_NET_WM_STATE;
    xev.format = 32;
    xev.data.l[0] = _NET_WM_STATE_TOGGLE;
    xev.data.l[1] = (long)toggle_state;

    ///xev.data.l[4] = CurrentTime; //xev.data.l[1] = timeStamp;

    XSendEvent(display, root, False, SubstructureNotifyMask, (XEvent *) &xev);
}

void setLayer(Window w, long layer) {
    XClientMessageEvent xev = {};

    xev.type = ClientMessage;
    xev.window = w;
    xev.message_type = _XA_WIN_LAYER;
    xev.format = 32;
    xev.data.l[0] = layer;
    xev.data.l[1] = CurrentTime; //xev.data.l[1] = timeStamp;
    XSendEvent(display, root, False, SubstructureNotifyMask, (XEvent *) &xev);
}

#if 0
void setTrayHint(Window w, long tray_opt) {
    XClientMessageEvent xev = {};

    xev.type = ClientMessage;
    xev.window = w;
    xev.message_type = _XA_WIN_TRAY;
    xev.format = 32;
    xev.data.l[0] = tray_opt;
    xev.data.l[1] = CurrentTime; //xev.data.l[1] = timeStamp;
    XSendEvent(display, root, False, SubstructureNotifyMask, (XEvent *) &xev);
}
#endif

void requestExtents(Window w) {
    XClientMessageEvent xev = {};

    xev.type = ClientMessage;
    xev.window = w;
    xev.message_type = _XA_NET_REQUEST_FRAME_EXTENTS;
    xev.format = 32;
    tell("%-8s 0x%lX, 0x%lx\n", "extents", root, w);
    XSendEvent(display, root, False, SubstructureNotifyMask, (XEvent *) &xev);
}

int main(int argc, char **argv) {

    int screen = DefaultScreen(display);
    root = RootWindow(display, screen);
    colormap = DefaultColormap(display, screen);
    Pixel black = BlackPixel(display, screen);

    window = XCreateWindow(display, root,
                           0,
                           0,
                           64, 64,
                           0,
                           CopyFromParent, InputOutput, CopyFromParent,
                           None, None);

    XSetWindowBackground(display, window, black);

    Atom protocols[] = { _XA_WM_DELETE_WINDOW, _XA_WM_TAKE_FOCUS, };
    XSetWMProtocols(display, window, protocols, COUNT(protocols) );

    XClassHint classHint = { (char *)"name:1", (char *)"class 1" };
    XSetClassHint(display, window, &classHint);
    XStoreName(display, window, basename(*argv));

    long imask = ExposureMask | StructureNotifyMask |
                 ButtonPressMask | ButtonReleaseMask |
                 KeyPressMask | KeyReleaseMask |
                 PropertyChangeMask;
    XSelectInput(display, window, imask);

    XSelectInput(display, root, PropertyChangeMask);

    XMapRaised(display, window);

    unmapped = XCreateSimpleWindow(display, root, 0, 0, 100, 100, 20, 0, black);
    XClassHint unclassHint = { (char *)"unmapped:1", (char *)"class 1" };
    XSetClassHint(display, unmapped, &unclassHint);
    XStoreName(display, unmapped, "unmapped");

    Atom wmstate[] = {
        // _XA_NET_WM_STATE_ABOVE,
        // _XA_NET_WM_STATE_BELOW,
        // _XA_NET_WM_STATE_DEMANDS_ATTENTION,
        // _XA_NET_WM_STATE_FOCUSED,
        // _XA_NET_WM_STATE_FULLSCREEN,
        // _XA_NET_WM_STATE_HIDDEN,
        // _XA_NET_WM_STATE_MAXIMIZED_HORZ,
        // _XA_NET_WM_STATE_MAXIMIZED_VERT,
        // _XA_NET_WM_STATE_MODAL,
        // _XA_NET_WM_STATE_SHADED,
        // _XA_NET_WM_STATE_SKIP_PAGER,
        // _XA_NET_WM_STATE_SKIP_TASKBAR,
        // _XA_NET_WM_STATE_STICKY,
    };
    XChangeProperty(display, unmapped,
                    _XA_NET_WM_STATE, XA_ATOM,
                    32, PropModeReplace,
                    (unsigned char *) wmstate, COUNT(wmstate));
    Atom wintype[] = {
        // _XA_NET_WM_WINDOW_TYPE_COMBO,
        // _XA_NET_WM_WINDOW_TYPE_DESKTOP,
        // _XA_NET_WM_WINDOW_TYPE_DIALOG,
        // _XA_NET_WM_WINDOW_TYPE_DND,
        // _XA_NET_WM_WINDOW_TYPE_DOCK,
        // _XA_NET_WM_WINDOW_TYPE_DROPDOWN_MENU,
        // _XA_NET_WM_WINDOW_TYPE_MENU,
        // _XA_NET_WM_WINDOW_TYPE_NORMAL,
        // _XA_NET_WM_WINDOW_TYPE_NOTIFICATION,
        // _XA_NET_WM_WINDOW_TYPE_POPUP_MENU,
        // _XA_NET_WM_WINDOW_TYPE_SPLASH,
        // _XA_NET_WM_WINDOW_TYPE_TOOLBAR,
        _XA_NET_WM_WINDOW_TYPE_TOOLTIP,
        // _XA_NET_WM_WINDOW_TYPE_UTILITY,
    };
    XChangeProperty(display, unmapped,
                    _XA_NET_WM_WINDOW_TYPE, XA_ATOM,
                    32, PropModeReplace,
                    (unsigned char *) wintype, COUNT(wintype));
    Atom actions[] = {
        _XA_NET_WM_ACTION_ABOVE,
        _XA_NET_WM_ACTION_BELOW,
        _XA_NET_WM_ACTION_CHANGE_DESKTOP,
        _XA_NET_WM_ACTION_CLOSE,
        _XA_NET_WM_ACTION_FULLSCREEN,
        _XA_NET_WM_ACTION_HIDE,
        _XA_NET_WM_ACTION_MAXIMIZE_HORZ,
        _XA_NET_WM_ACTION_MAXIMIZE_VERT,
        _XA_NET_WM_ACTION_MINIMIZE,
        _XA_NET_WM_ACTION_MOVE,
        _XA_NET_WM_ACTION_RESIZE,
        _XA_NET_WM_ACTION_SHADE,
        _XA_NET_WM_ACTION_STICK,
    };
    XChangeProperty(display, unmapped,
                    _XA_NET_WM_ALLOWED_ACTIONS, XA_ATOM,
                    32, PropModeReplace,
                    (unsigned char *) actions, COUNT(actions));

    XSelectInput(display, unmapped, imask);

    XEvent xev = {};
/// XButtonEvent &button = xev.xbutton;
    XPropertyEvent& property = xev.xproperty;
    XKeyEvent& key = xev.xkey;
    XClientMessageEvent& client = xev.xclient;

    while (XNextEvent(display, &xev) == Success) switch (xev.type) {

    case ClientMessage: {
        if (client.message_type == _XA_WM_PROTOCOLS &&
            client.format == 32 &&
            client.data.l[0] == _XA_WM_DELETE_WINDOW)
        {
            return 0;
        }
        else
        if (client.message_type == _XA_WM_PROTOCOLS &&
            client.format == 32 &&
            client.data.l[0] == _XA_WM_TAKE_FOCUS)
        {
            tell("WM_TAKE_FOCUS\n");
        }
        else {
            tell("client message %lu, %d, %lu, %ld\n",
                 client.message_type, client.format,
                 client.window, client.data.l[0]);
        }

    } break;

    case KeyPress: {
        unsigned k = XkbKeycodeToKeysym(display, key.keycode, 0,
                                        (key.state & ShiftMask) != 0);
        unsigned m = KEY_MODMASK(key.state);
        if (m == ControlMask && k < '~' && isalpha(k))
            k &= 0x1F;

        if (k == 'q' || k == XK_Escape)
            return 0;
        else if (k == XK_Left && m == 0)
            changeWorkspace(root,
                            (workspaceCount +
                             activeWorkspace - 1) % workspaceCount);
        else if (k == XK_Right && m == 0)
            changeWorkspace(root,
                            (activeWorkspace + 1) % workspaceCount);
        else if (k == XK_Left && m == ShiftMask)
            changeWorkspace(window,
                            (workspaceCount +
                             windowWorkspace - 1) % workspaceCount);
        else if (k == XK_Right && m == ShiftMask)
            changeWorkspace(window,
                            (windowWorkspace + 1) % workspaceCount);
        else if (k == 'f')
            toggleState(window, _XA_NET_WM_STATE_FULLSCREEN);
        else if (k == 'a')
            toggleState(window, _XA_NET_WM_STATE_ABOVE);
        else if (k == 'b')
            toggleState(window, _XA_NET_WM_STATE_BELOW);
        else if (k == 'M')
            toggleState(window, _XA_NET_WM_STATE_MODAL);
        else if (k == 't')
            toggleState(window, _XA_NET_WM_STATE_SKIP_TASKBAR);
#if 0
            //toggleState(window, WinStateAllWorkspaces);
        /*
        else if (k == 'd')
            toggleState(window, WinStateDockHorizontal);
            */
        else if (k == '0')
            setLayer(window, WinLayerDesktop);
        else if (k == '1')
            setLayer(window, WinLayerBelow);
        else if (k == '2')
            setLayer(window, WinLayerNormal);
        else if (k == '3')
            setLayer(window, WinLayerOnTop);
        else if (k == '4')
            setLayer(window, WinLayerDock);
        else if (k == '5')
            setLayer(window, WinLayerAboveDock);
#endif
        else if (k == 'm') {
            tell("%d %d\n", key.x_root, key.y_root);
            moveResize(window, key.x_root, key.y_root, 8); // move
        } else if (k == 'r') {
            tell("%d %d\n", key.x_root, key.y_root);
            moveResize(window, key.x_root, key.y_root, 4); // _|
        }
        else if (k == 'x' || k == 'X' || k == ('X' & 0x1f)) {
            Window w = m == ShiftMask ? window :
                m == ControlMask ? root : unmapped;
            const char *s = m == ShiftMask ? "window" :
                m == ControlMask ? "root" : "unmapped";
            tell("x = '%c' for %s\n", k, s);
            requestExtents(w);
        }
        else if (k > 0 && k < 256) {
            tell("'%c' %u\n", k, k);
        }
    } break;

    case PropertyNotify: {
        Atom r_type;
        int r_format;
        unsigned long count;
        unsigned long bytes_remain;
        unsigned char *prop(0);

        if (property.window == root &&
            property.atom == _XA_WIN_WORKSPACE) {
            if (XGetWindowProperty(display, root,
                                   _XA_WIN_WORKSPACE,
                                   0, 1, False, XA_CARDINAL,
                                   &r_type, &r_format,
                                   &count, &bytes_remain, &prop) == Success && prop)
            {
                if (r_type == XA_CARDINAL && r_format == 32 && count == 1) {
                    activeWorkspace = ((long *)prop)[0];
                    tell("active=%ld of %ld\n", activeWorkspace, workspaceCount);
                }
                XFree(prop);
            }
        }
        else if (property.window == root &&
                 property.atom == _XA_WIN_WORKSPACE_NAMES) {
        }
#if 0
        else if (property.atom == _XA_WIN_WORKAREA) {
            if (XGetWindowProperty(display, root,
                                   _XA_WIN_WORKAREA,
                                   0, 4, False, XA_CARDINAL,
                                   &r_type, &r_format,
                                   &count, &bytes_remain, &prop) == Success && prop)
            {
                if (r_type == XA_CARDINAL && r_format == 32 && count == 4) {
                    long *area = (long *)prop;
                    tell("workarea: min=%d,%d max=%d,%d\n",
                           area[0],
                           area[1],
                           area[2],
                           area[3]);
                }
                XFree(prop);
            }
        }
#endif
        else if (property.window == window &&
                 property.atom == _XA_WIN_WORKSPACE) {
            if (XGetWindowProperty(display, window,
                                   _XA_WIN_WORKSPACE,
                                   0, 1, False, XA_CARDINAL,
                                   &r_type, &r_format,
                                   &count, &bytes_remain, &prop) == Success && prop)
            {
                if (r_type == XA_CARDINAL && r_format == 32 && count == 1) {
                    windowWorkspace = ((long *)prop)[0];
                    tell("window=%ld of %ld\n", windowWorkspace, workspaceCount);
                }
                XFree(prop);
            }
        }
        else if (property.atom == _XA_WIN_STATE) {
            long state[2] = {};
            if (XGetWindowProperty(display, window,
                                   _XA_WIN_STATE,
                                   0, 2, False, XA_CARDINAL,
                                   &r_type, &r_format,
                                   &count, &bytes_remain, &prop) == Success && prop)
            {
                if (r_type == XA_CARDINAL && r_format == 32 && count == 2) {
                    state[0] = ((long *)prop)[0];
                    state[1] = ((long *)prop)[1];
                    tell("state=%lX %lX\n", state[0], state[1]);
                }
                XFree(prop);
            }
        }
        else if (property.atom == _XA_WIN_LAYER) {
            if (XGetWindowProperty(display, window,
                                   _XA_WIN_LAYER,
                                   0, 1, False, XA_CARDINAL,
                                   &r_type, &r_format,
                                   &count, &bytes_remain, &prop) == Success && prop)
            {
                if (r_type == XA_CARDINAL && r_format == 32 && count == 1) {
                    long layer = ((long *)prop)[0];
                    tell("layer=%ld\n", layer);
                }
                XFree(prop);
            }
        }
        /*else if (property.atom == _XA_WIN_TRAY) {
            if (XGetWindowProperty(display, window,
                                   _XA_WIN_TRAY,
                                   0, 1, False, XA_CARDINAL,
                                   &r_type, &r_format,
                                   &count, &bytes_remain, &prop) == Success && prop)
            {
                if (r_type == XA_CARDINAL && r_format == 32 && count == 1) {
                    long tray = ((long *)prop)[0];
                    tell("tray option=%d\n", tray);
                }
            }
        }*/
        else {
            Atom atom = property.atom;
            Window w = property.window;
            const char *s = w == window ? "window" :
                            w == unmapped ? "unmapped" :
                            w == root ? "root" : "other!";
            Atom type(None);
            int format(0);
            unsigned long nitems(0);
            unsigned long after(0);
            unsigned char* prop(0);
            if (XGetWindowProperty(display, w, atom,
                                   0, 1000000, False, AnyPropertyType,
                                   &type, &format, &nitems,
                                   &after, &prop) == Success) {
                tell("%-8s %-26s %2d %s\n", s, atomName(atom), format,
                        atomName(type));
                if (atom == _XA_NET_FRAME_EXTENTS) {
                    long* data = (long *) prop;
                    int n = int(nitems);
                    tell("\t");
                    for (int i = 0; i < n; ++i) {
                        printf(" %ld%s", data[i], n-i > 1 ? "," : "\n");
                    }
                }
                XFree(prop);
            }
            else {
                tell("%-8s %s %s\n", s, atomName(atom),
                        "Cannot get window property!");
            }
        }
    } break;

    default:
        break;
    }
}

// vim: set sw=4 ts=4 et:
