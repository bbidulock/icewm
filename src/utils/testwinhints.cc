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

#define KEY_MODMASK(x) ((x) & (ControlMask | ShiftMask | Mod1Mask))

char *displayName = 0;
Display *display = 0;
Colormap defaultColormap;
Window root = None;
Window window = None;
GC gc;

long workspaceCount = 4;
long activeWorkspace = 0;
long windowWorkspace = 0;
long state[2] = { 0, 0 };
bool sticky = false;

Atom _XA_WIN_WORKSPACE;
Atom _XA_WIN_WORKSPACE_NAMES;
Atom _XA_WIN_STATE;
Atom _XA_WIN_LAYER;
Atom _XA_WIN_WORKAREA;

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

void toggleState(Window w, long new_state) {
    XClientMessageEvent xev;
  
    memset(&xev, 0, sizeof(xev));
    xev.type = ClientMessage;
    xev.window = w;
    xev.message_type = _XA_WIN_STATE;
    xev.data.l[0] = (state[0] & state[1] & new_state) ^ new_state;
    xev.data.l[1] = new_state;
    xev.data.l[2] = CurrentTime; //xev.data.l[1] = timeStamp;
    
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

int main(int argc, char **argv) {
    XSetWindowAttributes attr;

    assert((display = XOpenDisplay(displayName)) != 0);
    root = RootWindow(display, DefaultScreen(display));
    defaultColormap = DefaultColormap(display, DefaultScreen(display));

    _XA_WIN_WORKSPACE = XInternAtom(display, XA_WIN_WORKSPACE, False);
    _XA_WIN_WORKSPACE_NAMES = XInternAtom(display, XA_WIN_WORKSPACE_NAMES, False);
    _XA_WIN_STATE = XInternAtom(display, XA_WIN_STATE, False);
    _XA_WIN_LAYER = XInternAtom(display, XA_WIN_LAYER, False);
    _XA_WIN_WORKAREA = XInternAtom(display, XA_WIN_WORKAREA, False);

    window = XCreateWindow(display, root,
                           0,
                           0,
                           64, 64,
                           0,
                           CopyFromParent, InputOutput, CopyFromParent,
                           0, &attr);

    XSetWindowBackground(display, window, BlackPixel(display, DefaultScreen(display)));

    XSelectInput(display, window,
                 ExposureMask | StructureNotifyMask |
                 ButtonPressMask | ButtonReleaseMask |
                 KeyPressMask | KeyReleaseMask |
                 PropertyChangeMask);

    XSelectInput(display, root, PropertyChangeMask);
    
    XMapRaised(display, window);
    
    while (1) {
        XEvent xev;
        XButtonEvent &button = xev.xbutton;
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
                else if (k == 's')
                    toggleState(window, WinStateAllWorkspaces);
                else if (k == 'd')
                    toggleState(window, WinStateDockHorizontal);
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
                                printf("active=%d of %d\n", activeWorkspace, workspaceCount);
                            }
                            XFree(prop);
                        }
                    } else if (property.atom == _XA_WIN_WORKSPACE_NAMES) {
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
                                printf("window=%d of %d\n", windowWorkspace, workspaceCount);
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
                                printf("layer=%d\n", layer);
                            }
                            XFree(prop);
                        }
                    }
                }
            }
        }
    }
}
