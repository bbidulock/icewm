/*
 *  IceSH - A command line window manager
 *  Copyright (C) 2001 Mathias Hasselmann
 *
 *  Based on Mark´s testwinhints.cc.
 *  Inspired by slef´s windowC
 *
 *  Release under terms of the GNU Library General Public License
 *
 *  2001/07/18: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *  - initial version
 */
#include "config.h"
#include "intl.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>

#include "base.h"
#include "yapp.h"
#include "WinMgr.h"

/******************************************************************************/

long workspaceCount = 4;
long activeWorkspace = 0;
long state[2] = { 0, 0 };

/******************************************************************************/

Display *display(NULL);
Window root;

char const * YApplication::Name(NULL);

/******************************************************************************/

Atom ATOM_WM_STATE;
Atom ATOM_WIN_WORKSPACE;
Atom ATOM_WIN_WORKSPACE_NAMES;
Atom ATOM_WIN_STATE;
Atom ATOM_WIN_LAYER;
Atom ATOM_WIN_WORKAREA;
Atom ATOM_WIN_TRAY;

/******************************************************************************/

#define CHECK_ARGUMENT_COUNT(Count) { \
    if ((argv + argc - argp) < (Count)) { \
	usageError(_("Action `%s' requires at least %d arguments."), \
		     action, Count); \
	THROW(1); \
    } \
}    

/******************************************************************************/
/******************************************************************************/

Status getState(Window window, long & mask, long & state) {
    Atom type; int rc, format;
    unsigned long nitems, lbytes;
    unsigned char * data;
     
    rc = XGetWindowProperty(display, window, ATOM_WIN_STATE, 0, 2, False,
     			    XA_CARDINAL, &type, &format, &nitems, &lbytes, &data);

    if (Success == rc) {
	if (XA_CARDINAL == type && format == 32 && nitems >= 1U) {
	    if (nitems >= 2U) {
		state = ((long *) data)[0];
		mask = ((long *) data)[1];
	    } else {
		state = ((long *) data)[0];
		mask = WIN_STATE_ALL;
	    }
	} else
	    rc = BadValue;

	XFree(data);
    }
     
    return rc;
}

Status setState(Window window, long mask, long state) {
    XClientMessageEvent xev;
    memset(&xev, 0, sizeof(xev));

    xev.type = ClientMessage;
    xev.window = window;
    xev.message_type = ATOM_WIN_STATE;
    xev.format = 32;
    xev.data.l[0] = mask;
    xev.data.l[1] = state;
    xev.data.l[2] = CurrentTime;

    return XSendEvent(display, root, False, SubstructureNotifyMask, (XEvent *) &xev);
}

Status toggleState(Window window, long newState) {
    long mask, state;
    
    if (Success != getState(window, mask, state))
	state = mask = 0;

    MSG(("old mask/state: %d/%d", mask, state));
    
    XClientMessageEvent xev;
    memset(&xev, 0, sizeof(xev));

    xev.type = ClientMessage;
    xev.window = window;
    xev.message_type = ATOM_WIN_STATE;
    xev.format = 32;
    xev.data.l[0] = newState;
    xev.data.l[1] = newState;
    xev.data.l[2] = CurrentTime;

    MSG(("new mask/state: %d/%d", xev.data.l[0], xev.data.l[1]));

    return XSendEvent(display, root, False, SubstructureNotifyMask, (XEvent *) &xev);
}

Status setWorkspace(Window window, long workspace) {
    XClientMessageEvent xev;
    memset(&xev, 0, sizeof(xev));

    xev.type = ClientMessage;
    xev.window = window;
    xev.message_type = ATOM_WIN_WORKSPACE;
    xev.format = 32;
    xev.data.l[0] = workspace;
    xev.data.l[1] = CurrentTime;

    return XSendEvent(display, root, False, SubstructureNotifyMask, (XEvent *) &xev);
}

Status setLayer(Window w, long layer) {
    XClientMessageEvent xev;
    memset(&xev, 0, sizeof(xev));

    xev.type = ClientMessage;
    xev.window = w;
    xev.message_type = ATOM_WIN_LAYER;
    xev.format = 32;
    xev.data.l[0] = layer;
    xev.data.l[1] = CurrentTime;

    return XSendEvent(display, root, False, SubstructureNotifyMask, (XEvent *) &xev);
}

Status setTrayHint(Window w, long tray_opt) {
    XClientMessageEvent xev;
    memset(&xev, 0, sizeof(xev));

    xev.type = ClientMessage;
    xev.window = w;
    xev.message_type = ATOM_WIN_TRAY;
    xev.format = 32;
    xev.data.l[0] = tray_opt;
    xev.data.l[1] = CurrentTime;

    return XSendEvent(display, root, False, SubstructureNotifyMask, (XEvent *) &xev);
}

/******************************************************************************/

Window getClientWindow(Window window)
{
    Atom type(None); int format;
    unsigned long nitems, lbytes;
    unsigned char * data;
     
    XGetWindowProperty(display, window, ATOM_WM_STATE, 0, 0, False, 
		       AnyPropertyType, &type, &format,
		       &nitems, &lbytes, &data);

    if (type != None) return window;
	
    Window root, parent;
    unsigned nchildren;
    Window *children;

    if (!XQueryTree (display, window, &root, &parent, &children, &nchildren)) {
	warn("XQueryTree failed for window 0x%x", window);
	return None;
    }

    Window client(None);

    for (unsigned i = 0; client == None && i < nchildren; ++i) {
	XGetWindowProperty (display, children[i], ATOM_WM_STATE, 0, 0, False,
			    AnyPropertyType, &type, &format,
			    &nitems, &lbytes, &data);

        if (None != type) client = children [i];
    }

    for (unsigned i = 0; client == None && i < nchildren; ++i)
	client = getClientWindow(children [i]);

    XFree(children);
    return client;
}

Window pickWindow (void) {
    Cursor cursor;
    Window target(None);
    int count(0);

    cursor = XCreateFontCursor(display, XC_crosshair);
    XGrabPointer(display, root, False, ButtonPressMask|ButtonReleaseMask, 
		 GrabModeAsync, GrabModeSync, root, cursor, CurrentTime);

    do {
	XEvent event;
	XNextEvent (display, &event);
	
	if (event.type == ButtonPress) {
	    ++count;

	    if (target == None)
		target = event.xbutton.subwindow == None
		       ? event.xbutton.window
		       : event.xbutton.subwindow;
	} else if (event.type == ButtonRelease) {
	    --count;
	}
    } while (None == target || 0 != count);

    XUngrabPointer(display, CurrentTime);
    
    return getClientWindow(target);
}

/******************************************************************************/
/******************************************************************************/

static void printUsage() {
    fprintf(stderr, _("\
Usage: %s [OPTIONS] ACTIONS\n\
\n\
Options:\n\
  -display DISPLAY          Connects to the X server specified by DISPLAY.\n\
                            Default: $DISPLAY or :0.0 when not set.\n\
  -window WINDOW_ID         Specifies the window to manipulate. Special\n\
                            identifiers are `root' for the root window and\n\
			    `focus' for the currently focused window.\n\
\n\
Actions:\n\
  setTitle TITLE            Set window title\n\
  setState MASK STATE       Set window state (see WinMgr.h for details)\n\
  toggleState STATE         Set window state (see WinMgr.h for details)\n\
  setLayer LAYER            Moves the window to another layer (0-15)\n\
  setWorkspace WORKSPACE    Moves the window to another workspace\n\
  setTrayOption TRAYOPTION  Set tray option (see WinMgr.h for details)\n\n"),
	    YApplication::Name);
}

static void usageError(char const *msg, ...) {
    fprintf(stderr, "%s: ", YApplication::Name);
    fputs(_("Usage error: "), stderr);

    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    
    fputs("\n\n", stderr);
    printUsage();
}

/******************************************************************************/
/******************************************************************************/

int main(int argc, char **argv) {
    YApplication::Name = basename(*argv);
    char const * dpyname(NULL);
    char const * winname(NULL);
    Window window(None);
    int rc(0);
    
    debug = true;

    char **argp(argv + 1);
    for (char const * arg; argp < argv + argc && '-' == *(arg = *argp); ++argp) {
	if (arg[1] == '-') ++arg;

	size_t sep(strcspn(arg, "=:"));
	char const * val(arg[sep] ? arg + sep + 1 : *++argp);

	if (!(strpcmp(arg, "-display") || val == NULL))
	    dpyname = val;
	else if (!(strpcmp(arg, "-window") || val == NULL))
	    winname = val;
	else if (!(strpcmp(arg, "-debug") || val != NULL))
	    debug = 1;
	else if (strcmp(arg, "-?") && strcmp(arg, "-help")) {
	    usageError (_("Invalid argument: `%s'."), arg);
	    THROW(1);
	} else {
	    printUsage();
	    THROW(0);
	}
    }

    if (argp >= argv + argc) {
	usageError(_("No actions specified."));
	THROW(1);
    }

/******************************************************************************/

    if (NULL == (display = XOpenDisplay(dpyname))) {
	warn(_("Can't open display: %s. X must be running and $DISPLAY set."),
	       XDisplayName(dpyname));
	THROW(3);
    }

    root = RootWindow(display, DefaultScreen(display));

    ATOM_WM_STATE = XInternAtom(display, "WM_STATE", False);
    ATOM_WIN_WORKSPACE = XInternAtom(display, XA_WIN_WORKSPACE, False);
    ATOM_WIN_WORKSPACE_NAMES = XInternAtom(display, XA_WIN_WORKSPACE_NAMES, False);
    ATOM_WIN_STATE = XInternAtom(display, XA_WIN_STATE, False);
    ATOM_WIN_LAYER = XInternAtom(display, XA_WIN_LAYER, False);
    ATOM_WIN_WORKAREA = XInternAtom(display, XA_WIN_WORKAREA, False);
    ATOM_WIN_TRAY = XInternAtom(display, XA_WIN_TRAY, False);
    
/******************************************************************************/

    if (NULL == winname)
	window = pickWindow();
    else if (!strcmp(winname, "root"))
	window = root;
    else if (!strcmp(winname, "focus")) {
	int revert;

	XGetInputFocus(display, &window, &revert);
    } else {
        char *eptr;
	
	window = strtol(winname, &eptr, 0);
	if (NULL == eptr || '\0' != *eptr) {
	    usageError(_("Invalid window identifier: `%s'"), winname);
	    THROW(1);
	}
    }
    
    MSG(("selected window: 0x%x", window));
    
/******************************************************************************/

    while (argp < argv + argc) {
	char const * action(*argp++);
	
	if (!strcmp(action, "setTitle")) {
	    CHECK_ARGUMENT_COUNT (1)

	    char const * title(*argp++);

	    MSG(("setTitle: `%s'", title));
	    XStoreName(display, window, title);
	} else if (!strcmp(action, "setGeometry")) {
	    CHECK_ARGUMENT_COUNT (1)

	    XSizeHints normal;
	    long supplied;
	    XGetWMNormalHints(display, window, &normal, &supplied);

	    Window root; int x, y; unsigned width, height, dummy;
	    XGetGeometry(display, window, &root,
	    		 &x, &y, &width, &height, &dummy, &dummy);

	    char const * geometry(*argp++);
	    int geom_x, geom_y; unsigned geom_width, geom_height;
	    int status(XParseGeometry(geometry, &geom_x, &geom_y,
	    					&geom_width, &geom_height));
	    
	    if (status & XValue) x = geom_x;
	    if (status & YValue) y = geom_y;
	    if (status & WidthValue) width = geom_width;
	    if (status & HeightValue) height = geom_height;

	    if (normal.flags & PResizeInc) {
		width*= max(1, normal.width_inc);
		height*= max(1, normal.height_inc);
	    }

	    if (normal.flags & PBaseSize) {
		width+= normal.base_width;
		height+= normal.base_height;
	    }
	    
	    if (status & XNegative)
		x+= DisplayWidth(display, DefaultScreen(display)) - width;
	    if (status & YNegative)
		y+= DisplayHeight(display, DefaultScreen(display)) - height;

	    MSG(("setGeometry: %dx%d%+i%+i", width, height, x, y));
	    XMoveResizeWindow(display, window, x, y, width, height);
	} else if (!strcmp(action, "setState")) {
	    CHECK_ARGUMENT_COUNT (2)

	    unsigned mask(atoi(*argp++));
	    unsigned state(atoi(*argp++));

	    MSG(("setState: %d %d", mask, state));
	    setState(window, mask, state);
	} else if (!strcmp(action, "toggleState")) {
	    CHECK_ARGUMENT_COUNT (1)

	    unsigned state(atoi(*argp++));

	    MSG(("toggleState: %d", state));
	    toggleState(window, state);
	} else if (!strcmp(action, "setWorkspace")) {
	    CHECK_ARGUMENT_COUNT (1)
	    
	    unsigned workspace(atoi(*argp++));

	    MSG(("setWorkspace: %d", workspace));
	    setWorkspace(window, workspace);
	} else if (!strcmp(action, "setLayer")) {
	    CHECK_ARGUMENT_COUNT (1)
	    
	    unsigned layer(atoi(*argp++));

	    MSG(("setLayer: %d0x", layer));
	    setLayer(window, layer);
	} else if (!strcmp(action, "setTrayOption")) {
	    CHECK_ARGUMENT_COUNT (1)
	    
	    unsigned trayopt(atoi(*argp++));

	    MSG(("setTrayOption: %d", trayopt));
	    setTrayHint(window, trayopt);
	} else {
	    usageError(_("Unknown action: `%s'"), action);
	    THROW(1);
	}
    }
    
    CATCH(
	if (display) XCloseDisplay(display);
    )

    return 0;
}
