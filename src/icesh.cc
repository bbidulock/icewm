/*
 *  IceSH - A command line window manager
 *  Copyright (C) 2001 Mathias Hasselmann
 *
 *  Based on Mark´s testwinhints.cc.
 *  Inspired by MJ Ray's WindowC
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
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#include "base.h"
#include "yapp.h"
#include "WinMgr.h"

/******************************************************************************/
/******************************************************************************/

char const * YApplication::Name(NULL);
Display *display(NULL);
Window root;

/******************************************************************************/
/******************************************************************************/

struct Symbol {
    char const * name;
    long code;
};

struct SymbolTable {
    long parseIdentifier(char const * identifier, size_t const len) const;
    long parseIdentifier(char const * identifier) const {
	return parseIdentifier(identifier, strlen(identifier));
    }

    long parseExpression(char const * expression) const;
    void listSymbols(char const * label) const;
    
    bool valid(long code) const { return code != fErrCode; }
    bool invalid(long code) const { return code == fErrCode; }

    Symbol const * fSymbols;
    long fMin, fMax, fErrCode;
};

/******************************************************************************/

Atom ATOM_WM_STATE;
Atom ATOM_WIN_WORKSPACE;
Atom ATOM_WIN_WORKSPACE_NAMES;
Atom ATOM_WIN_WORKSPACE_COUNT;
Atom ATOM_WIN_STATE;
Atom ATOM_WIN_HINTS;
Atom ATOM_WIN_LAYER;
Atom ATOM_WIN_TRAYOPT;

/******************************************************************************/
/******************************************************************************/

#define CHECK_ARGUMENT_COUNT(Count) { \
    if ((argv + argc - argp) < (Count)) { \
    msg(_("Action `%s' requires at least %d arguments."), action, Count); \
    THROW(1); \
    } \
}    

#define CHECK_EXPRESSION(SymTab, Code, Str) { \
    if ((SymTab).invalid(Code)) { \
	msg(_("Invalid expression: `%s'"), Str); \
    	THROW(1); \
    } \
}    

/******************************************************************************/

Symbol stateIdentifiers[] = {
    { "AllWorkspaces",  	WinStateAllWorkspaces   },
    { "Sticky",         	WinStateAllWorkspaces   },
    { "Minimized",      	WinStateMinimized       },
    { "Maximized",      	WinStateMaximizedVert   |
                        	WinStateMaximizedHoriz  },
    { "MaximizedVert",  	WinStateMaximizedVert	},
    { "MaximizedVertical", 	WinStateMaximizedVert	},
    { "MaximizedHoriz",		WinStateMaximizedHoriz  },
    { "MaximizedHorizontal",	WinStateMaximizedHoriz  },
    { "Hidden",         	WinStateHidden          },
    { "All",            	WIN_STATE_ALL           },
    { NULL,             	0                       }
};

Symbol hintIdentifiers[] = {
    { "SkipFocus",     	WinHintsSkipFocus       },
    { "SkipWindowMenu", WinHintsSkipWindowMenu  },
    { "SkipTaskBar",    WinHintsSkipTaskBar     },
    { "FocusOnClick",   WinHintsFocusOnClick    },
    { "DoNotCover",     WinHintsDoNotCover      },
    { "All",            WIN_HINTS_ALL           },
    { NULL,             0                	}
};

Symbol layerIdentifiers[] = {
    { "Desktop",    WinLayerDesktop     },
    { "Below",      WinLayerBelow       },
    { "Normal",     WinLayerNormal      },
    { "OnTop",      WinLayerOnTop       },
    { "Dock",       WinLayerDock        },
    { "AboveDock",  WinLayerAboveDock   },
    { "Menu",       WinLayerMenu        },
    { NULL,         0                   }
};

Symbol trayOptionIdentifiers[] = {
    { "Ignore",		WinTrayIgnore		},
    { "Minimized",	WinTrayMinimized	},
    { "Exclusive",	WinTrayExclusive	},
    { NULL,		0			}
};

SymbolTable layers = {
    layerIdentifiers, 0, WinLayerCount - 1, WinLayerInvalid
};

SymbolTable states = {
    stateIdentifiers, 0, WIN_STATE_ALL, -1
};

SymbolTable hints = {
    hintIdentifiers, 0, WIN_HINTS_ALL, -1
};    

SymbolTable trayOptions = {
    trayOptionIdentifiers, 0, WinTrayOptionCount - 1, WinTrayInvalid
};
    
/******************************************************************************/

long SymbolTable::parseIdentifier(char const * id, size_t const len) const {
    for (Symbol const * sym(fSymbols); NULL != sym && NULL != sym->name; ++sym)
	if (!(sym->name[len] || strncasecmp(sym->name, id, len)))
	    return sym->code;

    char *endptr; 
    long value(strtol(id, &endptr, 0));

    return (NULL != endptr && '\0' == *endptr &&
	    value >= fMin && value <= fMax ? value : fErrCode);
}

long SymbolTable::parseExpression(char const * expression) const {
    long value(0);

    for (char const * token(expression);
	 *token != '\0' && value != fErrCode; token = strnxt(token, "+|")) {
	char const * id(token + strspn(token, " \t"));
	value|= parseIdentifier(id = newstr(id, "+| \t"));
        delete[] id;
    }

    return value;
}

void SymbolTable::listSymbols(char const * label) const {
    printf(_("Named symbols of the domain `%s' (numeric range: %ld-%ld):\n"),
    	   label, fMin, fMax);

    for (Symbol const * sym(fSymbols); NULL != sym && NULL != sym->name; ++sym)
	printf("  %-20s (%ld)\n", sym->name, sym->code);

    puts("");
}

/******************************************************************************/

Status getState(Window window, long & mask, long & state) {
    YWindowProperty winState(window, ATOM_WIN_STATE, XA_CARDINAL, 2);

    if (Success == winState && XA_CARDINAL == winState.type() &&
        32 == winState.format() && 1U <= winState.count()) {
	state = winState.template data<long>(0);
	mask = winState.count() >= 2U
             ? winState.template data<long>(1)
             : WIN_STATE_ALL;
        
        return winState;
    }

    return BadValue;
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
    xev.data.l[1] = (state & mask & newState) ^ newState;
    xev.data.l[2] = CurrentTime;

    MSG(("new mask/state: %d/%d", xev.data.l[0], xev.data.l[1]));

    return XSendEvent(display, root, False, SubstructureNotifyMask, (XEvent *) &xev);
}

/******************************************************************************/

Status setHints(Window window, long hints) {
    return XChangeProperty(display, window, ATOM_WIN_HINTS, XA_CARDINAL, 32,
    		           PropModeReplace, (unsigned char *) &hints, 1);
}

/******************************************************************************/

struct WorkspaceInfo {
    WorkspaceInfo(Window root):
	fCount(root, ATOM_WIN_WORKSPACE_COUNT, XA_CARDINAL, 1),
	fNames(root, ATOM_WIN_WORKSPACE_NAMES),
	fStatus(Success == fCount ? fNames : fCount) {
    }
    
    int parseWorkspaceName(char const * name) {
	unsigned workspace(WinWorkspaceInvalid);

	if (Success == fStatus) {
	    for (int n(0); n < fNames.count() &&
			   WinWorkspaceInvalid == workspace; ++n)
		if (!strcmp(name, fNames[n])) workspace = n;

	    if (WinWorkspaceInvalid == workspace) {
		char *endptr; 
		workspace = strtol(name, &endptr, 0);

		if (NULL == endptr || '\0' != *endptr) {
		    msg(_("Invalid workspace name: `%s'"), name);
		    return WinWorkspaceInvalid;
		}
	    }
        
	    if (workspace > count()) {
		msg(_("Workspace out of range: %d"), workspace);
		return WinWorkspaceInvalid;
	    }
	}
        
        return workspace;
    }
    
    unsigned count();
    operator int() const { return fStatus; }

    YWindowProperty fCount;
    YTextProperty fNames;
    int fStatus;
};

unsigned WorkspaceInfo::count() { 
    return (Success == fCount ? fCount.template data<long>(0) : 0);
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

/******************************************************************************/

Status setLayer(Window window, long layer) {
    XClientMessageEvent xev;
    memset(&xev, 0, sizeof(xev));

    xev.type = ClientMessage;
    xev.window = window;
    xev.message_type = ATOM_WIN_LAYER;
    xev.format = 32;
    xev.data.l[0] = layer;
    xev.data.l[1] = CurrentTime;

    return XSendEvent(display, root, False, SubstructureNotifyMask, (XEvent *) &xev);
}

Status setTrayHint(Window window, long trayopt) {
    XClientMessageEvent xev;
    memset(&xev, 0, sizeof(xev));

    xev.type = ClientMessage;
    xev.window = window;
    xev.message_type = ATOM_WIN_TRAYOPT;
    xev.format = 32;
    xev.data.l[0] = trayopt;
    xev.data.l[1] = CurrentTime;

    return XSendEvent(display, root, False, SubstructureNotifyMask, (XEvent *) &xev);
}

/******************************************************************************/

Window getClientWindow(Window window)
{
    if (None != YWindowProperty(window, ATOM_WM_STATE).type())
	return window;
	
    Window root, parent;
    unsigned nchildren;
    Window *children;

    if (!XQueryTree (display, window, &root, &parent, &children, &nchildren)) {
	warn(_("XQueryTree failed for window 0x%x"), window);
	return None;
    }

    Window client(None);

    for (unsigned i = 0; client == None && i < nchildren; ++i)
	if (None != YWindowProperty(children[i], ATOM_WM_STATE).type())
	    client = children [i];

    for (unsigned i = 0; client == None && i < nchildren; ++i)
	client = getClientWindow(children [i]);

    XFree(children);
    return client;
}

Window pickWindow (void) {
    Cursor cursor;
    bool running(true);
    Window target(None);
    int count(0);
    unsigned escape;

    cursor = XCreateFontCursor(display, XC_crosshair);
    escape = XKeysymToKeycode(display, XK_Escape);

    XGrabKey(display, escape, 0, root, False, GrabModeAsync, GrabModeAsync);
    XGrabPointer(display, root, False, ButtonPressMask|ButtonReleaseMask, 
		 GrabModeAsync, GrabModeAsync, root, cursor, CurrentTime);

    do {
	XEvent event;
	XNextEvent (display, &event);
	
	switch (event.type) {
	    case KeyPress:
	    case KeyRelease:
                if (event.xkey.keycode == escape) running = false;
		break;

	    case ButtonPress:
		++count;

		if (target == None)
		    target = event.xbutton.subwindow == None
			   ? event.xbutton.window
			   : event.xbutton.subwindow;
		break;
                
	    case ButtonRelease:
		--count;
		break;
	}
    } while (running && (None == target || 0 != count));

    XUngrabPointer(display, CurrentTime);
    XUngrabKey(display, escape, 0, root);
    
    return (None == target || root == target ? target
    					     : getClientWindow(target));
}

/******************************************************************************/
/******************************************************************************/

static void printUsage() {
    printf(_("\
Usage: %s [OPTIONS] ACTIONS\n\
\n\
Options:\n\
  -display DISPLAY            Connects to the X server specified by DISPLAY.\n\
                              Default: $DISPLAY or :0.0 when not set.\n\
  -window WINDOW_ID           Specifies the window to manipulate. Special\n\
                              identifiers are `root' for the root window and\n\
			      `focus' for the currently focused window.\n\
\n\
Actions:\n\
  setIconTitle TITLE          Set the icon title.\n\
  setWindowTitle TITLE        Set the window title.\n\
  setState MASK STATE         Set the GNOME window state to STATE.\n\
  			      Only the bits selected by MASK are affected.\n\
                              STATE and MASK are expressions of the domain\n\
                              `GNOME window state'.\n\
  toggleState STATE           Toggle the GNOME window state bits specified by\n\
                              the STATE expression.\n\
  setHints HINTS              Set th GNOME window hints to HINTS.\n\
  setLayer LAYER              Moves the window to another GNOME window layer.\n\
  setWorkspace WORKSPACE      Moves the window to another workspace. Select\n\
  			      the root window to change the current workspace.\n\
  listWorkspaces   	      Lists the names of all workspaces.\n\
  setTrayOption TRAYOPTION    Set the IceWM tray option hint.\n\
\n\
Expressions:\n\
  Expressions are list of symbols of one domain concatenated by `+' or `|':\n\
\n\
  EXPRESSION ::= SYMBOL | EXPRESSION ( `+' | `|' ) SYMBOL\n\n"),
	    YApplication::Name);
            
    states.listSymbols(_("GNOME window state"));
    hints.listSymbols(_("GNOME window hint"));
    layers.listSymbols(_("GNOME window layer"));
    trayOptions.listSymbols(_("IceWM tray option"));
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

    char **argp(argv + 1);
    for (char const * arg; argp < argv + argc && '-' == *(arg = *argp); ++argp) {
	if (arg[1] == '-') ++arg;

	size_t sep(strcspn(arg, "=:"));
	char const * val(arg[sep] ? arg + sep + 1 : *++argp);

	if (!(strpcmp(arg, "-display") || val == NULL)) {
	    dpyname = val;
	} else if (!(strpcmp(arg, "-window") || val == NULL)) {
	    winname = val;
#ifdef DEBUG	    
	} else if (!(strpcmp(arg, "-debug"))) {
	    debug = 1;
            --argp;
#endif	    
	} else if (strcmp(arg, "-?") && strcmp(arg, "-help")) {
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
    ATOM_WIN_WORKSPACE_COUNT = XInternAtom(display, XA_WIN_WORKSPACE_COUNT, False);
    ATOM_WIN_STATE = XInternAtom(display, XA_WIN_STATE, False);
    ATOM_WIN_HINTS = XInternAtom(display, XA_WIN_HINTS, False);
    ATOM_WIN_LAYER = XInternAtom(display, XA_WIN_LAYER, False);
    ATOM_WIN_TRAYOPT = XInternAtom(display, XA_ICEWM_TRAYOPT, False);
    
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
	    msg(_("Invalid window identifier: `%s'"), winname);
	    THROW(1);
	}
    }
    
    MSG(("selected window: 0x%x", window));
    
/******************************************************************************/
    while (argp < argv + argc) {
	char const * action(*argp++);
	
	if (!strcmp(action, "setWindowTitle")) {
	    CHECK_ARGUMENT_COUNT (1)

	    char const * title(*argp++);

	    MSG(("setWindowTitle: `%s'", title));
	    XStoreName(display, window, title);
	} else if (!strcmp(action, "setIconTitle")) {
	    CHECK_ARGUMENT_COUNT (1)

	    char const * title(*argp++);

	    MSG(("setIconTitle: `%s'", title));
	    XSetIconName(display, window, title);
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

	    unsigned mask(states.parseExpression(*argp++));
	    unsigned state(states.parseExpression(*argp++));
	    CHECK_EXPRESSION(states, mask, argp[-2])
	    CHECK_EXPRESSION(states, state, argp[-1])

	    MSG(("setState: %d %d", mask, state));
	    setState(window, mask, state);
	} else if (!strcmp(action, "toggleState")) {
	    CHECK_ARGUMENT_COUNT (1)

	    unsigned state(states.parseExpression(*argp++));
	    CHECK_EXPRESSION(states, state, argp[-1])

	    MSG(("toggleState: %d", state));
	    toggleState(window, state);
	} else if (!strcmp(action, "setHints")) {
	    CHECK_ARGUMENT_COUNT (1)

	    unsigned hint(hints.parseExpression(*argp++));
	    CHECK_EXPRESSION(hints, hint, argp[-1])

	    MSG(("setHints: %d", hint));
	    setHints(window, hint);
	} else if (!strcmp(action, "setWorkspace")) {
	    CHECK_ARGUMENT_COUNT (1)

            unsigned const workspace(WorkspaceInfo(root).
            			     parseWorkspaceName(*argp++));
	    if (WinWorkspaceInvalid == workspace) THROW(1);

	    MSG(("setWorkspace: %d", workspace));
	    setWorkspace(window, workspace);
	} else if (!strcmp(action, "listWorkspaces")) {
	    YTextProperty workspaceNames(root, ATOM_WIN_WORKSPACE_NAMES);
	    for (int n(0); n < workspaceNames.count(); ++n)
		printf(_("workspace #%d: `%s'\n"), n, workspaceNames[n]);
	} else if (!strcmp(action, "setLayer")) {
	    CHECK_ARGUMENT_COUNT (1)
	    
	    unsigned layer(layers.parseExpression(*argp++));
	    CHECK_EXPRESSION(layers, layer, argp[-1])

	    MSG(("setLayer: %d", layer));
	    setLayer(window, layer);
	} else if (!strcmp(action, "setTrayOption")) {
	    CHECK_ARGUMENT_COUNT (1)
	    
	    unsigned trayopt(trayOptions.parseExpression(*argp++));
	    CHECK_EXPRESSION(trayOptions, trayopt, argp[-1])

	    MSG(("setTrayOption: %d", trayopt));
	    setTrayHint(window, trayopt);
	} else {
	    msg(_("Unknown action: `%s'"), action);
	    THROW(1);
	}
    }
    
    CATCH(
	if (display) {
            XSync(display, False);
            XCloseDisplay(display);
	}
    )

    return 0;
}
