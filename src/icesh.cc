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
#include <X11/Xlib.h>

#include <iostream>
#include <cstring>
#include <cstdlib>

#include "WinMgr.h"

Display *display(NULL);
Window root;

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
Atom _XA_WIN_TRAY;

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

static bool strpfx(char const * str, char const * pattern, char const * delim) {
    size_t const len(strlen(pattern));

    return !(strncmp(str, pattern, len) ||
    	    (str[len] && NULL == strchr(delim, str[len])));
}

static char const * basename(char const *path) {
    char const * name(strrchr(path, '/'));
    return (NULL != name ? name + 1 : name);
}

static void printUsage(char const * argv0) {
     cerr << "\
Usage: " << basename(argv0) << " [OPTIONS] ACTION\n\
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
  setState STATE            Set window state (see WinMgr.h for details)\n\
  setLayer LAYER            Moves the window to another layer (0-15)\n\
  setWorkspace WORKSPACE    Moves the window to another workspace\n\
  setTrayOption TRAYOPTION  Set tray option (see WinMgr.h for details)\n"
 	<< endl;
}

int main(int argc, char **argv) {
    char const * dpyname(NULL);
    char const * winname(NULL);
    
    char **argp(argv + 1);
    for (char const * arg; argp < argv + argc && '-' == *(arg = *argp); ++argp) {
	if (arg[1] == '-') ++arg;

	size_t sep(strcspn(arg, "=:"));
	char const * val(arg[sep] ? arg + sep + 1 : *++argp);

	if (strpfx(arg, "-display", "=:") && val != NULL)
	    dpyname = val;
	else if (strpfx(arg, "-window", "=:") && val != NULL)
	    winname = val;
	else {
	     if (strcmp(arg, "-?") && strcmp(arg, "-help"))
		cerr << "Invalid argument: " << arg << "." << endl;

	     printUsage(*argv);
	     return 1;
	}
    }

    if (argp >= argv + argc) {
	cerr << "Usage error: No actions specified." << endl;

	printUsage(*argv);
	return 1;
    }

    if (NULL == (display = XOpenDisplay(dpyname))) {
        cerr << "System error: Can not open display " << XDisplayName(dpyname) << endl;
	return 2;
    }

    root = RootWindow(display, DefaultScreen(display));

    _XA_WIN_WORKSPACE = XInternAtom(display, XA_WIN_WORKSPACE, False);
    _XA_WIN_WORKSPACE_NAMES = XInternAtom(display, XA_WIN_WORKSPACE_NAMES, False);
    _XA_WIN_STATE = XInternAtom(display, XA_WIN_STATE, False);
    _XA_WIN_LAYER = XInternAtom(display, XA_WIN_LAYER, False);
    _XA_WIN_WORKAREA = XInternAtom(display, XA_WIN_WORKAREA, False);
    _XA_WIN_TRAY = XInternAtom(display, XA_WIN_TRAY, False);
    
    if (NULL == winname) {
	cerr << "sorry, window grabbing not supported yet" << endl;
	XCloseDisplay(display);
	return 2;
    }
    
    Window window(None);

    if (!strcmp(winname, "root"))
	window = root;
    else if (!strcmp(winname, "focus")) {
	int revert;

	XGetInputFocus(display, &window, &revert);
    } else {
        char *eptr;
	
	window = strtol(winname, &eptr, 0);
	if (NULL == eptr || '\0' != *eptr) {
	    cerr << "Usage error: Invalid window identifier: " << winname << endl;
	    XCloseDisplay(display);
	    return 1;
	}
    }
    
    while (argp < argv + argc) {
	char const * action(*argp++);
	
	if (!strcmp(action, "setTitle")) {
	    if ((argv + argc - argp) < 1) {
	        cerr << "Usage error: Arguments required for action " << action << "." << endl;
		printUsage(*argv);
		XCloseDisplay(display);
	        return 1;
	    }
	    
	    char const * title(*argp++);
	    cout << "setTitle: `" << title << "´" << endl;
	    XStoreName(display, window, title);
	} else if (!strcmp(action, "setGeometry")) {
	    if ((argv + argc - argp) < 1) {
	        cerr << "Usage error: Arguments required for action " << action << "." << endl;
		printUsage(*argv);
		XCloseDisplay(display);
	        return 1;
	    }
	    
	    char const * geometry(*argp++);
	    int x, y; unsigned width, height;
	    XParseGeometry(geometry, &x, &y, &width, &height);
	    cout << "setGeometry: " << width << "x" << height
	    	 << (x > 0 ? "+" : "") << x
	    	 << (y > 0 ? "+" : "") << y << endl;
	    XMoveResizeWindow(display, window, x, y, width, height);
	} else if (!strcmp(action, "setState")) {
	    if ((argv + argc - argp) < 1) {
	        cerr << "Usage error: Arguments required for action " << action << "." << endl;
		printUsage(*argv);
		XCloseDisplay(display);
	        return 1;
	    }
	    
	    cout << "setState: 0x" << hex << window << dec << endl;
	    toggleState(window, atoi(*argp++));
	} else if (!strcmp(action, "setWorkspace")) {
	    if ((argv + argc - argp) < 1) {
	        cerr << "Usage error: Arguments required for action " << action << "." << endl;
		printUsage(*argv);
		XCloseDisplay(display);
	        return 1;
	    }
	    
	    cout << "setWorkspace: 0x" << hex << window << dec << endl;
	    changeWorkspace(window, atoi(*argp++));
	} else if (!strcmp(action, "setLayer")) {
	    if ((argv + argc - argp) < 1) {
	        cerr << "Usage error: Arguments required for action " << action << "." << endl;
		printUsage(*argv);
		XCloseDisplay(display);
	        return 1;
	    }
	    
	    cout << "setLayer: 0x" << hex << window << dec << endl;
	    setLayer(window, atoi(*argp++));
	} else if (!strcmp(action, "setTrayOption")) {
	    if ((argv + argc - argp) < 1) {
	        cerr << "Usage error: Arguments required for action " << action << "." << endl;
		printUsage(*argv);
		XCloseDisplay(display);
	        return 1;
	    }
	    
	    cout << "setTrayOption: 0x" << hex << window << dec << endl;
	    setTrayHint(window, atoi(*argp++));
	} else {
	    cerr << "Usage error: Invalid action: " << action << "." << endl;
	    printUsage(*argv);
	    XCloseDisplay(display);
	    return 1;
	}
    }
    
    XCloseDisplay(display);

    return 0;
}
