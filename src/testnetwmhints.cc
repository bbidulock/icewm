#include "config.h"
#include <assert.h>
#include <ctype.h>
#include <libgen.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
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
#define GUI_EVENT_NAMES
#include "guievent.h"
#include "logevent.h"

/// _SET would be nice to have
#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1
#define _NET_WM_STATE_TOGGLE 2

#define _NET_WM_ORIENTATION_HORZ    0
#define _NET_WM_ORIENTATION_VERT    1
#define _NET_WM_TOPLEFT     0
#define _NET_WM_TOPRIGHT    1
#define _NET_WM_BOTTOMRIGHT 2
#define _NET_WM_BOTTOMLEFT  3

#define KEY_MODMASK(x) ((x) & (ControlMask | ShiftMask | Mod1Mask))

#define COUNT(a)    (int(sizeof a / sizeof(*a)))
#define CTRL(k)     ((k) & 0x1F)
#define TEST(p)     if (p); else fail( #p, __func__, __LINE__ )

static struct Layout {
    long orient;
    long columns;
    long rows;
    long corner;
} layout = { 0L, 4L, 3L, _NET_WM_TOPLEFT, };

typedef unsigned long Pixel;
const char* ApplicationName = "testnetwmhints";

static void tell(const char* msg, ...) {
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

static void fail(const char* expr, const char* func, const int line) {
    tell("%s: %3d: ( %s ) FAILED!\n", func, line, expr);
}

class TDisplay {
    Display *dpy;
public:
    TDisplay() : dpy(0) { }
    Display* display() { return dpy ? dpy : dpy = XOpenDisplay(0); }
    ~TDisplay() { if (dpy) { XCloseDisplay(dpy); dpy = 0; } }
    operator Display*() { return display(); }
    // operator _XPrivDisplay() { return _XPrivDisplay(display()); }
};

static TDisplay display;
static Colormap colormap;
static Window root = None;
static Window window = None;
static Window unmapped = None;
static Window unviewed = None;
///static GC gc;

static long workspaceCount = 4;
static long activeWorkspace = 0;
static long windowWorkspace = 0;
//static long state[10] = { 0, 0 };
///static bool sticky = false;

///static bool fullscreen = true;

class TAtom {
public:
    const char* const name;
private:
    Atom atom;
public:
    explicit TAtom(const char* name) : name(name), atom(None) { }
    operator unsigned() { return atom ? atom :
        atom = XInternAtom(display, name, False); }
};

static TAtom _XA_WM_CLIENT_LEADER("WM_CLIENT_LEADER");
static TAtom _XA_WM_DELETE_WINDOW("WM_DELETE_WINDOW");
static TAtom _XA_WM_PROTOCOLS("WM_PROTOCOLS");
static TAtom _XA_WM_TAKE_FOCUS("WM_TAKE_FOCUS");
static TAtom _XA_WM_STATE("WM_STATE");
static TAtom _XA_WIN_TRAY("WIN_TRAY");
static TAtom _XA_WIN_LAYER("_WIN_LAYER");
static TAtom _XA_ICEWM_GUI_EVENT("ICEWM_GUI_EVENT");
static TAtom _XA_NET_WM_DESKTOP("_NET_WM_DESKTOP");
static TAtom _XA_NET_CURRENT_DESKTOP("_NET_CURRENT_DESKTOP");
static TAtom _XA_NET_NUMBER_OF_DESKTOPS("_NET_NUMBER_OF_DESKTOPS");
static TAtom _XA_NET_DESKTOP_NAMES("_NET_DESKTOP_NAMES");
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
static TAtom _XA_NET_WM_PID("_NET_WM_PID");
static TAtom _XA_NET_WM_PING("_NET_WM_PING");
static TAtom _XA_NET_ACTIVE_WINDOW("_NET_ACTIVE_WINDOW");
static TAtom _XA_NET_DESKTOP_LAYOUT("_NET_DESKTOP_LAYOUT");
static TAtom _XA_NET_REQUEST_FRAME_EXTENTS("_NET_REQUEST_FRAME_EXTENTS");
static TAtom _XA_NET_FRAME_EXTENTS("_NET_FRAME_EXTENTS");
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
    if (atom <= 0) return "invalid_atom_zero";
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
    xev.message_type = _XA_NET_WM_DESKTOP;
    xev.format = 32;
    xev.data.l[0] = workspace;
    xev.data.l[1] = CurrentTime; //xev.data.l[1] = timeStamp;
    XSendEvent(display, root, False, SubstructureNotifyMask, (XEvent *) &xev);
    tell("changeWorkspace %ld\n", workspace);
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

void toggleState(Window w, TAtom& toggle_state) {
    XClientMessageEvent xev = {};

    tell("toggle state %s\n", toggle_state.name);

    xev.type = ClientMessage;
    xev.window = w;
    xev.message_type = _XA_NET_WM_STATE;
    xev.format = 32;
    xev.data.l[0] = _NET_WM_STATE_TOGGLE;
    xev.data.l[1] = (long)toggle_state;

    ///xev.data.l[4] = CurrentTime; //xev.data.l[1] = timeStamp;

    XSendEvent(display, root, False, SubstructureNotifyMask, (XEvent *) &xev);
}

void toggleUrgency(Display* d, Window w) {
    XWMHints* h = XGetWMHints(d, w);
    if (!h)
        h = XAllocWMHints();
    h->flags = XUrgencyHint;
    XSetWMHints(d, w, h);
    XFree(h);
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

void setProperty(Window w, Atom prop, Atom type, Atom* data, size_t count) {
    XChangeProperty(display, w, prop, type, 32, PropModeReplace,
                    (unsigned char *) data, int(count));
}

void setLayout(Window w) {
    tell("setLayout %ld, %ld, %ld, %ld\n",
            layout.orient, layout.columns, layout.rows, layout.corner);
    setProperty(w, _XA_NET_DESKTOP_LAYOUT, XA_CARDINAL, (Atom*)&layout, 4);
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

static bool getProperty(Window w, Atom p, Atom t, long* c, long limit = 1L) {
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char* prop = nullptr;

    if (XGetWindowProperty(display, w, p,
                           0L, limit, False, t,
                           &r_type, &r_format,
                           &count, &bytes_remain,
                           &prop) == Success && prop)
    {
        TEST(r_type == t && r_format == 32 && count == 1);
        *c = ((long *)prop)[0];
        XFree(prop);
        return true;
    } else {
        return false;
    }
}

static void updateWorkspaceCount() {
    getProperty(root, _XA_NET_NUMBER_OF_DESKTOPS, XA_CARDINAL, &workspaceCount);
}

static void updateActiveWorkspace() {
    getProperty(root, _XA_NET_CURRENT_DESKTOP, XA_CARDINAL, &activeWorkspace);
}

static void updateWindowWorkspace() {
    getProperty(window, _XA_NET_WM_DESKTOP, XA_CARDINAL, &windowWorkspace);
}

static int xfail(Display* display, XErrorEvent* xev) {

    char message[80], req[80], number[80];

    snprintf(number, sizeof number, "%d", xev->request_code);
    XGetErrorDatabaseText(display, "XRequest", number, "",
                          req, sizeof(req));
    if (!req[0])
        snprintf(req, sizeof req, "[request_code=%d]", xev->request_code);

    if (XGetErrorText(display,
                      xev->error_code,
                      message, sizeof(message)) !=
                      Success)
        *message = '\0';

    tell("X error %s(0x%lX): %s\n", req, xev->resourceid, message);
    return 0;
}

static void sigcatch(int signo) {
    tell("Received signal %d: \"%s\".\n", signo, strsignal(signo));
    if (signo == SIGALRM) {
        Display* d = XOpenDisplay(nullptr);
        toggleUrgency(d, window);
        XCloseDisplay(d);
    }
}

static void help(char* name) {
    printf("Usage: %s [options]\n"
            "\t-d display\n"
            "\t-e : handle errors\n"
            "\t-p : enable pinging\n"
            "\t-s : synchronize\n"
            , name);
    exit(1);
}

static Pixel getColor(const char* name) {
    int screen = XDefaultScreen(display);
    colormap = XDefaultColormap(display, screen);
    XColor color = {}, exact;
    Pixel pixel;
    if (XLookupColor(display, colormap, name, &exact, &color) &&
        XAllocColor(display, colormap, &color))
        pixel = color.pixel;
    else if (XParseColor(display, colormap, name, &color) &&
        XAllocColor(display, colormap, &color))
        pixel = color.pixel;
    else
        pixel = XWhitePixel(display, screen);
    tell("color %s = 0x%lX\n", name, pixel);
    return pixel;
}

static void test_run(char* progname, bool pinging) {
    int screen = XDefaultScreen(display);
    root = XRootWindow(display, screen);
    colormap = XDefaultColormap(display, screen);
    // Pixel black = XBlackPixel(display, screen);
    Pixel white = XWhitePixel(display, screen);
    Pixel blue = getColor("blue");
    Pixel green = getColor("green");
    Pixel yellow = getColor("yellow");

    window = XCreateWindow(display, root,
                           0,
                           0,
                           64, 64,
                           0,
                           CopyFromParent, InputOutput, CopyFromParent,
                           None, None);

    XSetWindowBackground(display, window, blue);

    Atom protocols[] = {
        _XA_WM_DELETE_WINDOW, _XA_WM_TAKE_FOCUS,
        _XA_NET_WM_PING,
    };
    XSetWMProtocols(display, window, protocols, COUNT(protocols) );

    XClassHint classHint = { (char *)"window", (char *)"testnet" };
    XSetClassHint(display, window, &classHint);
    XStoreName(display, window, basename(progname));

    char hostname[HOST_NAME_MAX + 1] = {};
    gethostname(hostname, HOST_NAME_MAX);
    XTextProperty hname = {
        (unsigned char *) hostname,
        XA_STRING,
        8,
        strnlen(hostname, HOST_NAME_MAX),
    };
    XSetWMClientMachine(display, window, &hname);

    XID pid = getpid();
    setProperty(window, _XA_NET_WM_PID, XA_CARDINAL, &pid, 1);

    long imask = ExposureMask | StructureNotifyMask |
                 ButtonPressMask | ButtonReleaseMask |
                 KeyPressMask | KeyReleaseMask |
                 PropertyChangeMask | SubstructureNotifyMask;
    XSelectInput(display, window, imask);

    XSelectInput(display, root, PropertyChangeMask);

    Window leader = XCreateWindow(display, root, 0, 0, 10, 10, 0, CopyFromParent,
                                  InputOnly, CopyFromParent, None, None);
    setProperty(window, _XA_WM_CLIENT_LEADER, XA_WINDOW, &leader, 1);

    XMapRaised(display, window);

    unmapped = XCreateSimpleWindow(display, root, 0, 0, 100, 100, 20,
                                   white, yellow);
    XClassHint unclassHint = { (char *)"unmapped", (char *)"testnet" };
    XSetClassHint(display, unmapped, &unclassHint);
    XStoreName(display, unmapped, "unmapped");
    setProperty(unmapped, _XA_NET_WM_PID, XA_CARDINAL, &pid, 1);

    setProperty(unmapped, _XA_WM_CLIENT_LEADER, XA_WINDOW, &leader, 1);

    unviewed = XCreateSimpleWindow(display, root, 0, 0, 100, 100, 20,
                                   white, green);
    XClassHint vwclassHint = { (char *)"unviewed", (char *)"testnet" };
    XSetClassHint(display, unviewed, &vwclassHint);
    XStoreName(display, unviewed, "unviewed");
    setProperty(unviewed, _XA_NET_WM_PID, XA_CARDINAL, &pid, 1);

    setProperty(unviewed, _XA_WM_CLIENT_LEADER, XA_WINDOW, &leader, 1);

    Atom wmstate[] = {
        // _XA_NET_WM_STATE_ABOVE,
        // _XA_NET_WM_STATE_BELOW,
        _XA_NET_WM_STATE_DEMANDS_ATTENTION,
        // _XA_NET_WM_STATE_FOCUSED,
        // _XA_NET_WM_STATE_FULLSCREEN,
        // _XA_NET_WM_STATE_HIDDEN,
        // _XA_NET_WM_STATE_MAXIMIZED_HORZ,
        // _XA_NET_WM_STATE_MAXIMIZED_VERT,
        _XA_NET_WM_STATE_MODAL,
        // _XA_NET_WM_STATE_SHADED,
        // _XA_NET_WM_STATE_SKIP_PAGER,
        // _XA_NET_WM_STATE_SKIP_TASKBAR,
        _XA_NET_WM_STATE_STICKY,
    };
    setProperty(unmapped, _XA_NET_WM_STATE, XA_ATOM, wmstate, COUNT(wmstate));
    setProperty(unviewed, _XA_NET_WM_STATE, XA_ATOM, wmstate, COUNT(wmstate));

    Atom wintype[] = {
        // _XA_NET_WM_WINDOW_TYPE_COMBO,
        // _XA_NET_WM_WINDOW_TYPE_DESKTOP,
        _XA_NET_WM_WINDOW_TYPE_DIALOG,
        // _XA_NET_WM_WINDOW_TYPE_DND,
        // _XA_NET_WM_WINDOW_TYPE_DOCK,
        // _XA_NET_WM_WINDOW_TYPE_DROPDOWN_MENU,
        // _XA_NET_WM_WINDOW_TYPE_MENU,
        // _XA_NET_WM_WINDOW_TYPE_NORMAL,
        // _XA_NET_WM_WINDOW_TYPE_NOTIFICATION,
        // _XA_NET_WM_WINDOW_TYPE_POPUP_MENU,
        // _XA_NET_WM_WINDOW_TYPE_SPLASH,
        // _XA_NET_WM_WINDOW_TYPE_TOOLBAR,
        // _XA_NET_WM_WINDOW_TYPE_TOOLTIP,
        // _XA_NET_WM_WINDOW_TYPE_UTILITY,
    };
    setProperty(unmapped, _XA_NET_WM_WINDOW_TYPE, XA_ATOM,
                wintype, COUNT(wintype));
    setProperty(unviewed, _XA_NET_WM_WINDOW_TYPE, XA_ATOM,
                wintype, COUNT(wintype));

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
    setProperty(unmapped, _XA_NET_WM_ALLOWED_ACTIONS, XA_ATOM,
                actions, COUNT(actions));
    setProperty(unviewed, _XA_NET_WM_ALLOWED_ACTIONS, XA_ATOM,
                actions, COUNT(actions));

    XSelectInput(display, unmapped, imask);
    XSelectInput(display, unviewed, imask);

    updateWorkspaceCount();
    updateActiveWorkspace();

    XEvent xev = {};
/// XButtonEvent &button = xev.xbutton;
    XPropertyEvent& property = xev.xproperty;
    XKeyEvent& key = xev.xkey;
    XClientMessageEvent& client = xev.xclient;

    while (XNextEvent(display, &xev) == Success) switch (xev.type) {

    case ClientMessage: {
        if (client.message_type == _XA_WM_PROTOCOLS && client.window == window) {
            TEST(client.format == 32);
            if (client.data.l[0] == _XA_WM_DELETE_WINDOW) {
                tell("WM_DELETE_WINDOW\n");
                break;
            }
            else
            if (client.data.l[0] == _XA_WM_TAKE_FOCUS) {
                tell("WM_TAKE_FOCUS\n");
                XSetInputFocus(display, window, None, CurrentTime);
                break;
            }
            else
            if (client.data.l[0] == _XA_NET_WM_PING) {
                long* l = client.data.l;
                tell("_NET_WM_PING %ld, 0x%lX, 0x%lX, 0x%lX\n",
                        l[1], l[2], l[3], l[4]);
                if (pinging) {
                    client.window = root;
                    XSendEvent(display, root, False,
                               SubstructureRedirectMask|SubstructureNotifyMask,
                               &xev);
                    XFlush(display);
                    tell("\tSent pong.\n");
                }
                else {
                    tell("\tNo pong sent.\n");
                }
                break;
            }
        }
        tell("client message %lu, %d, %lu, %ld\n",
             client.message_type, client.format,
             client.window, client.data.l[0]);
    } break;

    case KeyRelease: {
        unsigned k = XkbKeycodeToKeysym(display, key.keycode, 0,
                                        (key.state & ShiftMask) != 0);
        unsigned m = KEY_MODMASK(key.state);
        if (m == ControlMask && k < '~' && isalpha(k))
            k = CTRL(k);
        if (k == 'q' || (k == XK_Escape && m == 0))
            return;
    } break;

    case KeyPress: {
        unsigned k = XkbKeycodeToKeysym(display, key.keycode, 0,
                                        (key.state & ShiftMask) != 0);
        unsigned m = KEY_MODMASK(key.state);
        if (m == ControlMask && k < '~' && isalpha(k))
            k = CTRL(k);

        if (k == '?' || k == 'h') {
            printf("0 : layer desktop\n"
                   "1 : layer below\n"
                   "2 : layer normal\n"
                   "3 : layer on top\n"
                   "4 : layer dock\n"
                   "5 : layer above dock\n"
                   "? : help\n"
                   "A : urgent alarm 5 seconds\n"
                   "M : toggle state modal\n"
                   "a : toggle state above\n"
                   "b : toggle state below\n"
                   "d : toggle state urgent\n"
                   "f : toggle fullscreen\n"
                   "s : toggle sticky\n"
                   "t : toggle skip taskbar\n"
                   "m : move resize 8\n"
                   "r : move resize 4\n"
                   "u : map the unmapped\n"
                   "v : map the unviewed\n"
                   "x : extents unmapped\n"
                   "X : extents window\n"
                   "^X : extents root\n"
                   "Left : goto previous workspace\n"
                   "Right : goto next workspace\n"
                   "Shift+Left : move to previous workspace\n"
                   "Shift+Right : move to next workspace\n"
                   "^B : layout bottom left\n"
                   "^C : control layout dimensions\n"
                   "^H : horizontal layout orientation\n"
                   "^L : layout top left\n"
                   "^R : layout bottom right\n"
                   "^T : layout top right\n"
                   "^V : vertical layout orientation\n"
                   "\n");
        }
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
        else if (k == 'd')
            toggleState(window, _XA_NET_WM_STATE_DEMANDS_ATTENTION);
        else if (k == 'M')
            toggleState(window, _XA_NET_WM_STATE_MODAL);
        else if (k == 's')
            toggleState(window, _XA_NET_WM_STATE_STICKY);
        else if (k == 't')
            toggleState(window, _XA_NET_WM_STATE_SKIP_TASKBAR);
        else if (k == 'A') {
            int sec = 5; tell("alarm %d seconds\n", sec); alarm(sec);
        }
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
        else if (k == 'm') {
            tell("%d %d\n", key.x_root, key.y_root);
            moveResize(window, key.x_root, key.y_root, 8); // move
        } else if (k == 'r') {
            tell("%d %d\n", key.x_root, key.y_root);
            moveResize(window, key.x_root, key.y_root, 4); // _|
        }
        else if (k == 'u') {
            XMapRaised(display, unmapped);
        }
        else if (k == 'v') {
            XMapRaised(display, unviewed);
        }
        else if (k == 'x' || k == 'X' || k == CTRL('X')) {
            Window w = m == ShiftMask ? window :
                m == ControlMask ? root : unmapped;
            const char *s = m == ShiftMask ? "window" :
                m == ControlMask ? "root" : "unmapped";
            tell("x = '%c' for %s\n", k, s);
            requestExtents(w);
        }
        else if (k == CTRL('V')) {
            layout.orient = _NET_WM_ORIENTATION_VERT;
            setLayout(root);
        }
        else if (k == CTRL('H')) {
            layout.orient = _NET_WM_ORIENTATION_HORZ;
            setLayout(root);
        }
        else if (k == CTRL('L')) {
            layout.corner = _NET_WM_TOPLEFT;
            setLayout(root);
        }
        else if (k == CTRL('T')) {
            layout.corner = _NET_WM_TOPRIGHT;
            setLayout(root);
        }
        else if (k == CTRL('B')) {
            layout.corner = _NET_WM_BOTTOMLEFT;
            setLayout(root);
        }
        else if (k == CTRL('R')) {
            layout.corner = _NET_WM_BOTTOMRIGHT;
            setLayout(root);
        }
        else if (k == CTRL('C')) {
            char buf[100];
            printf("Desktop Layout Columns (%ld): ", layout.columns);
            fflush(stdout);
            if (fgets(buf, sizeof buf, stdin))
                sscanf(buf, " %ld", &layout.columns);
            printf("Desktop Layout Rows (%ld): ", layout.rows);
            fflush(stdout);
            if (fgets(buf, sizeof buf, stdin))
                sscanf(buf, " %ld", &layout.rows);
            setLayout(root);
        }
        else if (k != XK_Shift_L && k != XK_Shift_R &&
                 k != XK_Control_L && k != XK_Control_R)
        {
            tell("Unrecognized key %d\n", k);
        }
    } break;

    case PropertyNotify: {
        if (property.state == PropertyDelete)
            break;

        Atom r_type;
        int r_format;
        unsigned long count;
        unsigned long bytes_remain;
        unsigned char *prop(0);

        if (property.window == root &&
            property.atom == _XA_NET_CURRENT_DESKTOP) {
            updateWorkspaceCount();
            updateActiveWorkspace();
            tell("active=%ld of %ld\n", activeWorkspace, workspaceCount);
        }
        else if (property.window == root &&
                 property.atom == _XA_NET_DESKTOP_NAMES) {
        }
        else if (property.window == window &&
                 property.atom == _XA_NET_WM_DESKTOP) {
            updateWindowWorkspace();
        }
        else if (property.window == root &&
                 property.atom == _XA_NET_ACTIVE_WINDOW) {
            long win = None;
            getProperty(root, _XA_NET_ACTIVE_WINDOW, XA_WINDOW, &win);
            tell("active window = 0x%lX\n", win);
        }
        else if (property.atom == _XA_NET_WM_STATE) {
            if (XGetWindowProperty(display, property.window,
                                   _XA_NET_WM_STATE,
                                   0, 20, False, AnyPropertyType,
                                   &r_type, &r_format,
                                   &count, &bytes_remain,
                                   &prop) == Success && prop)
            {
                TEST(r_type == XA_ATOM && r_format == 32);
                tell("net wm state %s: ",
                        property.window == window ? "window" :
                        property.window == unmapped ? "unmapped" :
                        property.window == unviewed ? "unviewed" :
                        "unknown");
                for (unsigned long i = 0; i < count; ++i) {
                    printf("%s%s", i ? ", " : "",
                            atomName(((long *)prop)[i]));
                }
                printf("%s\n", count == 0 ? "_" : "");
                XFree(prop);
            }
        }
        else if (property.atom == _XA_WM_STATE) {
            if (XGetWindowProperty(display, property.window,
                                   _XA_WM_STATE,
                                   0, 2, False, AnyPropertyType,
                                   &r_type, &r_format,
                                   &count, &bytes_remain,
                                   &prop) == Success && prop)
            {
                TEST(r_type == _XA_WM_STATE && r_format == 32 && count == 2);
                tell("wm state %s:",
                        property.window == window ? "window" :
                        property.window == unmapped ? "unmapped" :
                        property.window == unviewed ? "unviewed" :
                        "unknown");
                for (unsigned i = 0; i < count; ++i) {
                    printf("%s%ld", i ? ", " : "  ", ((long *)prop)[i]);
                }
                printf("%s\n", count == 0 ? "_" : "");
                XFree(prop);
            }
        }
        else if (property.atom == _XA_WIN_LAYER) {
            if (XGetWindowProperty(display, property.window,
                                   _XA_WIN_LAYER,
                                   0, 1, False, AnyPropertyType,
                                   &r_type, &r_format,
                                   &count, &bytes_remain,
                                   &prop) == Success && prop)
            {
                TEST(r_type == XA_CARDINAL && r_format == 32 && count == 1);
                long layer = ((long *)prop)[0];
                tell("win layer %s: %ld\n",
                        property.window == window ? "window" :
                        property.window == unmapped ? "unmapped" :
                        property.window == unviewed ? "unviewed" :
                        "unknown", layer);
                XFree(prop);
            }
        }
        else if (property.atom == _XA_ICEWM_GUI_EVENT) {
            if (XGetWindowProperty(display, property.window,
                                   _XA_ICEWM_GUI_EVENT,
                                   0, 1, False, AnyPropertyType,
                                   &r_type, &r_format,
                                   &count, &bytes_remain,
                                   &prop) == Success && prop)
            {
                TEST(r_type == _XA_ICEWM_GUI_EVENT && r_format == 8 && count == 1);
                if (*prop < COUNT(gui_event_names))
                    tell("IceWM GUI event %s\n", gui_event_names[*prop]);
                else
                    tell("IceWM GUI event %d\n", *prop);
            }
        }
        else if (property.atom == _XA_WIN_TRAY) {
            if (XGetWindowProperty(display, property.window,
                                   _XA_WIN_TRAY,
                                   0, 1, False, AnyPropertyType,
                                   &r_type, &r_format,
                                   &count, &bytes_remain,
                                   &prop) == Success && prop)
            {
                TEST(r_type == XA_CARDINAL && r_format == 32 && count == 1);
                long tray = ((long *)prop)[0];
                tell("tray option=%d\n", tray);
            }
        }
        else {
            Atom atom = property.atom;
            Window w = property.window;
            const char *s = w == window ? "window" :
                            w == unmapped ? "unmapped" :
                            w == unviewed ? "unviewed" :
                            w == root ? "root" : "other!";
            Atom type(None);
            int format(0);
            unsigned long nitems(0);
            unsigned long after(0);
            unsigned char* prop(0);
            if (XGetWindowProperty(display, w, atom,
                                   0, 4, False, AnyPropertyType,
                                   &type, &format, &nitems,
                                   &after, &prop) == Success && prop) {
                tell("%-8s %-26s %2d %s\n", s, atomName(atom), format,
                        atomName(type));
                if (atom == _XA_NET_FRAME_EXTENTS) {
                    TEST(type == XA_CARDINAL && format == 32 && nitems == 4);
                    long* data = (long *) prop;
                    int n = int(nitems);
                    for (int i = 0; i < n; ++i) {
                        printf("%s%ld", i ? ", " : "\t\t", data[i]);
                    }
                    if (n)
                        printf("\n");
                }
                else if (atom == XA_WINDOW) {
                    TEST(type == XA_WINDOW && format == 32 && nitems == 1);
                    long* data = (long *) prop;
                    int n = int(nitems);
                    for (int i = 0; i < n; ++i) {
                        printf("%s%ld", i ? ", " : "\t\t", data[i]);
                    }
                    if (n)
                        printf("\n");
                }
                else if (atom == XA_CARDINAL) {
                    TEST(type == XA_CARDINAL && format == 32 && nitems == 1);
                    long* data = (long *) prop;
                    int n = int(nitems);
                    for (int i = 0; i < n; ++i) {
                        printf("%s%ld", i ? ", " : "\t\t", data[i]);
                    }
                    if (n)
                        printf("\n");
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
        tell("%s\n", eventName(xev.type));
        break;
    }
}

int main(int argc, char **argv) {

    signal(SIGTERM, sigcatch);
    signal(SIGALRM, sigcatch);

    bool pinging = false;

    for (int opt; (opt = getopt(argc, argv, "d:epsh?")) > 0; ) {
        switch (opt) {
        case 'd':
            setenv("DISPLAY", optarg, True);
            break;

        case 'e':
            XSetErrorHandler(xfail);
            break;

        case 'p':
            pinging ^= true;
            break;

        case 's':
            XSynchronize(display, True);
            break;

        default:
            help(*argv);
        }
    }
    if (optind < argc) {
        printf("%s: bad arg '%s'.\n", *argv, argv[optind]);
        help(*argv);
    } else {
        test_run(*argv, pinging);
    }
    return 0;
}

// vim: set sw=4 ts=4 et:
