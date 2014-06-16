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
///static GC gc;

static long workspaceCount = 4;
static long activeWorkspace = 0;
static long windowWorkspace = 0;
//static long state[10] = { 0, 0 };
///static bool sticky = false;

///static bool fullscreen = true;

static Atom _XA_WIN_WORKSPACE;
static Atom _XA_WIN_WORKSPACE_NAMES;
static Atom _XA_WIN_STATE;
static Atom _XA_WIN_LAYER;
static Atom _XA_NET_WM_STATE;
static Atom _XA_NET_WM_STATE_FULLSCREEN;
static Atom _XA_NET_WM_STATE_ABOVE;
static Atom _XA_NET_WM_STATE_BELOW;
static Atom _XA_NET_WM_STATE_MODAL;
static Atom _XA_NET_WM_STATE_SKIP_TASKBAR;
static Atom _XA_NET_WM_MOVERESIZE;

void changeWorkspace(Window w, long workspace) {
    XClientMessageEvent xev;

    memset(&xev, 0, sizeof(xev));
    xev.type = ClientMessage;
    xev.window = w;
    xev.message_type = _XA_WIN_WORKSPACE;
    xev.format = 32;
    xev.data.l[0] = workspace;
    xev.data.l[1] = CurrentTime; //xev.data.l[1] = timeStamp;
    XSendEvent(display, root, False, SubstructureNotifyMask, (XEvent *) &xev);
}

void moveResize(Window w, int x, int y, int what) {
    XClientMessageEvent xev;

    memset(&xev, 0, sizeof(xev));
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
    XClientMessageEvent xev;

    puts("toggle state");

    memset(&xev, 0, sizeof(xev));
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
    XClientMessageEvent xev;
  
    memset(&xev, 0, sizeof(xev));
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
    XClientMessageEvent xev;
  
    memset(&xev, 0, sizeof(xev));
    xev.type = ClientMessage;
    xev.window = w;
    xev.message_type = _XA_WIN_TRAY;
    xev.format = 32;
    xev.data.l[0] = tray_opt;
    xev.data.l[1] = CurrentTime; //xev.data.l[1] = timeStamp;
    XSendEvent(display, root, False, SubstructureNotifyMask, (XEvent *) &xev);
}
#endif

int main(/*int argc, char **argv*/) {
    XSetWindowAttributes attr;
    Atom state[1];
    XClassHint *classHint = NULL;

    assert((display = XOpenDisplay(displayName)) != 0);
    root = RootWindow(display, DefaultScreen(display));
    defaultColormap = DefaultColormap(display, DefaultScreen(display));

    _XA_WIN_WORKSPACE = XInternAtom(display, XA_WIN_WORKSPACE, False);
    _XA_WIN_WORKSPACE_NAMES = XInternAtom(display, XA_WIN_WORKSPACE_NAMES, False);
///    _XA_WIN_STATE = XInternAtom(display, XA_WIN_STATE, False);
    _XA_WIN_LAYER = XInternAtom(display, XA_WIN_LAYER, False);
///    _XA_WIN_WORKAREA = XInternAtom(display, XA_WIN_WORKAREA, False);
///    _XA_WIN_TRAY = XInternAtom(display, XA_WIN_TRAY, False);
    _XA_NET_WM_STATE = XInternAtom(display, "_NET_WM_STATE", False);
    _XA_NET_WM_STATE_FULLSCREEN = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    _XA_NET_WM_STATE_ABOVE = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
    _XA_NET_WM_STATE_BELOW = XInternAtom(display, "_NET_WM_STATE_BELOW", False);
    _XA_NET_WM_STATE_MODAL = XInternAtom(display, "_NET_WM_STATE_MODAL", False);
    _XA_NET_WM_STATE_SKIP_TASKBAR = XInternAtom(display, "_NET_WM_STATE_SKIP_TASKBAR", False);
    _XA_NET_WM_MOVERESIZE = XInternAtom(display, "_NET_WM_MOVERESIZE", False);

    state[0] = _XA_NET_WM_STATE_FULLSCREEN;

    window = XCreateWindow(display, root,
                           0,
                           0,
                           64, 64,
                           0,
                           CopyFromParent, InputOutput, CopyFromParent,
                           0, &attr);

    XSetWindowBackground(display, window, BlackPixel(display, DefaultScreen(display)));

    classHint = XAllocClassHint();
    classHint->res_name = (char *)"name:1";
    classHint->res_class = (char *)"class 1";
    XSetClassHint(display, window, classHint);

    XSelectInput(display, window,
                 ExposureMask | StructureNotifyMask |
                 ButtonPressMask | ButtonReleaseMask |
                 KeyPressMask | KeyReleaseMask |
                 PropertyChangeMask);

    XSelectInput(display, root, PropertyChangeMask);
    
    XMapRaised(display, window);
    
    while (1) {
        XEvent xev;
///        XButtonEvent &button = xev.xbutton;
        XPropertyEvent &property = xev.xproperty;
        XKeyEvent &key = xev.xkey;

        XNextEvent(display, &xev);

        switch (xev.type) {
        case KeyPress:
            {
                unsigned int k = XKeycodeToKeysym(display, key.keycode, 0);
                unsigned int m = KEY_MODMASK(key.state);

                if (k == XK_Left && m == 0)
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
                else if (k == 'm')
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
                    printf("%d %d\n", key.x_root, key.y_root);
                    moveResize(window, key.x_root, key.y_root, 8); // move
                } else if (k == 'r') {
                    printf("%d %d\n", key.x_root, key.y_root);
                    moveResize(window, key.x_root, key.y_root, 4); // _|
                }
            }
            break;
        case PropertyNotify:
            {
                Atom r_type;
                int r_format;
                unsigned long count;
                unsigned long bytes_remain;
                unsigned char *prop;

                if (property.window == root) {
                    if (property.atom == _XA_WIN_WORKSPACE) {
                        if (XGetWindowProperty(display, root,
                                               _XA_WIN_WORKSPACE,
                                               0, 1, False, XA_CARDINAL,
                                               &r_type, &r_format,
                                               &count, &bytes_remain, &prop) == Success && prop)
                        {
                            if (r_type == XA_CARDINAL && r_format == 32 && count == 1) {
                                activeWorkspace = ((long *)prop)[0];
                                printf("active=%ld of %ld\n", activeWorkspace, workspaceCount);
                            }
                            XFree(prop);
                        }
                    } else if (property.atom == _XA_WIN_WORKSPACE_NAMES) {
#if 0
                    } else if (property.atom == _XA_WIN_WORKAREA) {
                        if (XGetWindowProperty(display, root,
                                               _XA_WIN_WORKAREA,
                                               0, 4, False, XA_CARDINAL,
                                               &r_type, &r_format,
                                               &count, &bytes_remain, &prop) == Success && prop)
                        {
                            if (r_type == XA_CARDINAL && r_format == 32 && count == 4) {
                                long *area = (long *)prop;
                                printf("workarea: min=%d,%d max=%d,%d\n",
                                       area[0],
                                       area[1],
                                       area[2],
                                       area[3]);
                            }
                            XFree(prop);
                        }
#endif
                    }
                } else if (property.window == window) {
                    if (property.atom == _XA_WIN_WORKSPACE) {
                        if (XGetWindowProperty(display, window,
                                               _XA_WIN_WORKSPACE,
                                               0, 1, False, XA_CARDINAL,
                                               &r_type, &r_format,
                                               &count, &bytes_remain, &prop) == Success && prop)
                        {
                            if (r_type == XA_CARDINAL && r_format == 32 && count == 1) {
                                windowWorkspace = ((long *)prop)[0];
                                printf("window=%ld of %ld\n", windowWorkspace, workspaceCount);
                            }
                            XFree(prop);
                        }
                    } else if (property.atom == _XA_WIN_STATE) {
                        if (XGetWindowProperty(display, window,
                                               _XA_WIN_STATE,
                                               0, 2, False, XA_CARDINAL,
                                               &r_type, &r_format,
                                               &count, &bytes_remain, &prop) == Success && prop)
                        {
                            if (r_type == XA_CARDINAL && r_format == 32 && count == 2) {
                                state[0] = ((long *)prop)[0];
                                state[1] = ((long *)prop)[1];
                                printf("state=%lX %lX\n", state[0], state[1]);
                            }
                            XFree(prop);
                        }
                    } else if (property.atom == _XA_WIN_LAYER) {
                        if (XGetWindowProperty(display, window,
                                               _XA_WIN_LAYER,
                                               0, 1, False, XA_CARDINAL,
                                               &r_type, &r_format,
                                               &count, &bytes_remain, &prop) == Success && prop)
                        {
                            if (r_type == XA_CARDINAL && r_format == 32 && count == 1) {
                                long layer = ((long *)prop)[0];
                                printf("layer=%ld\n", layer);
                            }
                            XFree(prop);
                        }
                    } /*else if (property.atom == _XA_WIN_TRAY) {
                        if (XGetWindowProperty(display, window,
                                               _XA_WIN_TRAY,
                                               0, 1, False, XA_CARDINAL,
                                               &r_type, &r_format,
                                               &count, &bytes_remain, &prop) == Success && prop)
                        {
                            if (r_type == XA_CARDINAL && r_format == 32 && count == 1) {
                                long tray = ((long *)prop)[0];
                                printf("tray option=%d\n", tray);
                            }
                        }
                    }*/
                }
            }
        }
    }
}
