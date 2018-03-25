/*
 *  IceSH - A command line window manager
 *  Copyright (C) 2001 Mathias Hasselmann
 *
 *  Based on Marko's testwinhints.cc.
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

#ifdef CONFIG_I18N
#include <locale.h>
#endif

#include "base.h"
#include "WinMgr.h"
#include "wmaction.h"

#if 1
#define THROW(Result) { rc = (Result); goto exceptionHandler; }
#define TRY(Command) { if ((rc = (Command))) THROW(rc); }
#define CATCH(Handler) { exceptionHandler: { Handler } return rc; }
#endif

/******************************************************************************/
/******************************************************************************/

char const * ApplicationName(NULL);
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

class YWindowProperty {
public:
    YWindowProperty(Window window, Atom property, Atom type = AnyPropertyType,
                    long length = 0, long offset = 0, Bool deleteProp = False):
    fType(None), fFormat(0), fCount(0), fAfter(0), fData(NULL),
        fStatus(XGetWindowProperty(display, window, property,
                                   offset, length, deleteProp, type,
                                   &fType, &fFormat, &fCount, &fAfter,
                                   &fData)) {
    }

    virtual ~YWindowProperty() {
        if (NULL != fData) XFree(fData);
    }

    Atom type() const { return fType; }
    int format() const { return fFormat; }
    long count() const { return fCount; }
    unsigned long after() const { return fAfter; }

    template <class T>
    T data(unsigned index) const { return ((T *) fData)[index]; }

    operator int() const { return fStatus; }

private:
    Atom fType;
    int fFormat;
    unsigned long fCount, fAfter;
    unsigned char * fData;
    int fStatus;
};

class YTextProperty {
public:
    YTextProperty(Window window, Atom property):
        fList(NULL), fCount(0),
        fStatus(XGetTextProperty(display, window, &fProperty, property)
                ? Success : BadValue) {
    }

    virtual ~YTextProperty() {
        if (NULL != fList)
            XFreeStringList(fList);
        if (fStatus == Success && fProperty.value)
            XFree(fProperty.value);
    }

    char * item(unsigned index);
    char ** list();
    int count();

    operator int() const { return fStatus; }

private:
    void allocateList();

    XTextProperty fProperty;
    char ** fList;
    int fCount, fStatus;
};

char * YTextProperty::item(unsigned index) {
    return list()[index];
}

char ** YTextProperty::list() {
    allocateList();
    return fList;
}

int YTextProperty::count() {
    allocateList();
    return fCount;
}

void YTextProperty::allocateList() {
    if (NULL == fList)
        XTextPropertyToStringList(&fProperty, &fList, &fCount);
}

class YWindowTreeNode {
public:
    YWindowTreeNode(Window window):
        fRoot(None), fParent(None), fChildren(NULL), fCount(0),
        fSuccess(XQueryTree(display, window, &fRoot, &fParent,
                            &fChildren, &fCount)) {
    }

    virtual ~YWindowTreeNode() {
        if (NULL != fChildren) XFree(fChildren);
    }

    operator bool() { return fSuccess; }

private:
    Window fRoot, fParent, * fChildren;
    unsigned fCount;
    bool fSuccess;
};

/******************************************************************************/

Atom ATOM_WM_STATE;
Atom ATOM_WIN_WORKSPACE;
Atom ATOM_WIN_WORKSPACE_NAMES;
Atom ATOM_WIN_WORKSPACE_COUNT;
Atom ATOM_WIN_STATE;
Atom ATOM_WIN_HINTS;
Atom ATOM_WIN_LAYER;
Atom ATOM_WIN_TRAY;
Atom ATOM_ICE_ACTION;

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

#define FOREACH_WINDOW(Var) \
    for (Window *Var = windowList; Var < windowList + windowCount; ++Var)

/******************************************************************************/

Symbol stateIdentifiers[] = {
    { "Sticky",                 WinStateSticky          },
    { "Minimized",              WinStateMinimized       },
    { "Maximized",              WinStateMaximizedVert   |
    WinStateMaximizedHoriz  },
    { "MaximizedVert",          WinStateMaximizedVert   },
    { "MaximizedVertical",      WinStateMaximizedVert   },
    { "MaximizedHoriz",         WinStateMaximizedHoriz  },
    { "MaximizedHorizontal",    WinStateMaximizedHoriz  },
    { "Hidden",                 WinStateHidden          },
    { "All",                    WIN_STATE_ALL           },
    { NULL,                     0                       }
};

Symbol hintIdentifiers[] = {
    { "SkipFocus",      WinHintsSkipFocus       },
    { "SkipWindowMenu", WinHintsSkipWindowMenu  },
    { "SkipTaskBar",    WinHintsSkipTaskBar     },
    { "FocusOnClick",   WinHintsFocusOnClick    },
    { "DoNotCover",     WinHintsDoNotCover      },
    { "All",            WIN_HINTS_ALL           },
    { NULL,             0                       }
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
    { "Ignore",         WinTrayIgnore           },
    { "Minimized",      WinTrayMinimized        },
    { "Exclusive",      WinTrayExclusive        },
    { NULL,             0                       }
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

#if 0
static char const * strnxt(const char * str, const char * delim) {
    str+= strcspn(str, delim);
    str+= strspn(str, delim);
    return str;
}
#endif

/*
 *      Counts the tokens separated by delim
 */
unsigned strtoken(const char * str, const char * delim) {
    unsigned count = 0;

    if (str) {
        while (*str) {
            str = strnxt(str, delim);
            ++count;
        }
    }

    return count;
}

long SymbolTable::parseExpression(char const * expression) const {
    long value(0);

    for (char const * token(expression);
         *token != '\0' && value != fErrCode; token = strnxt(token, "+|"))
    {
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
        32 == winState.format() && 1L <= winState.count()) {
        state = winState.data<long>(0);
        mask = winState.count() >= 2L
             ? winState.data<long>(1)
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

    MSG(("old mask/state: %ld/%ld", mask, state));

    XClientMessageEvent xev;
    memset(&xev, 0, sizeof(xev));

    xev.type = ClientMessage;
    xev.window = window;
    xev.message_type = ATOM_WIN_STATE;
    xev.format = 32;
    xev.data.l[0] = newState;
    xev.data.l[1] = (state & mask & newState) ^ newState;
    xev.data.l[2] = CurrentTime;

    MSG(("new mask/state: %ld/%ld", xev.data.l[0], xev.data.l[1]));

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
        fNames(root, ATOM_WIN_WORKSPACE_NAMES)
    {
        if (fCount == Success)
            fStatus = fNames;
        else
            fStatus = fCount;
    }

    bool parseWorkspaceName(char const* name, long* workspace);

    long count();
    operator int() const { return fStatus; }

    YWindowProperty fCount;
    YTextProperty fNames;
    int fStatus;
};

long WorkspaceInfo::count() {
    return (Success == fCount ? fCount.data<long>(0) : 0);
}

bool WorkspaceInfo::parseWorkspaceName(char const* name, long* workspace) {
    *workspace = WinWorkspaceInvalid;

    if (Success == fStatus) {
        for (int i = 0; i < fNames.count(); ++i)
            if (0 == strcmp(name, fNames.item(i)))
                return *workspace = i, true;

        if (0 == strcmp(name, "0xFFFFFFFF") ||
            0 == strcmp(name, "All"))
            return *workspace = 0xFFFFFFFF, true;

        char* endptr(0);
        *workspace = strtol(name, &endptr, 0);

        if (0 == endptr || '\0' != *endptr) {
            msg(_("Invalid workspace name: `%s'"), name);
            return *workspace = WinWorkspaceInvalid, false;
        }

        if (*workspace < 0 || *workspace >= count()) {
            msg(_("Workspace out of range: %ld"), *workspace);
            return *workspace = WinWorkspaceInvalid, false;
        }
    }

    return *workspace != WinWorkspaceInvalid;
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

bool icewmAction(const char* str) {
    WMAction action = WMAction(0);
    static const struct { const char *s; WMAction a; } sa[] = {
        { "logout",     ICEWM_ACTION_LOGOUT },
        { "cancel",     ICEWM_ACTION_CANCEL_LOGOUT },
        { "reboot",     ICEWM_ACTION_REBOOT },
        { "shutdown",   ICEWM_ACTION_SHUTDOWN },
        { "about",      ICEWM_ACTION_ABOUT },
        { "windowlist", ICEWM_ACTION_WINDOWLIST },
        { "restart",    ICEWM_ACTION_RESTARTWM },
        { "suspend",    ICEWM_ACTION_SUSPEND },
    };
    for (int i = 0; i < int ACOUNT(sa) && !action; ++i)
        if (0 == strcmp(str, sa[i].s))
            action = sa[i].a;
    if (!action)
        return false;

    XClientMessageEvent xev = {};
    xev.type = ClientMessage;
    xev.window = root;
    xev.message_type = ATOM_ICE_ACTION;
    xev.format = 32;
    xev.data.l[0] = CurrentTime;
    xev.data.l[1] = action;

    XSendEvent(display, root, False, SubstructureNotifyMask, (XEvent *) &xev);
    XSync(display, False);
    return true;
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
    xev.message_type = ATOM_WIN_TRAY;
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
    unsigned int i;

    if (!XQueryTree (display, window, &root, &parent, &children, &nchildren)) {
        warn("XQueryTree failed for window 0x%lx", window);
        return None;
    }

    Window client(None);

    for (i = 0; client == None && i < nchildren; ++i)
        if (None != YWindowProperty(children[i], ATOM_WM_STATE).type())
            client = children [i];

    for (i = 0; client == None && i < nchildren; ++i)
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

    // this is broken
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
    // and this is broken
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
  -d, -display DISPLAY        Connects to the X server specified by DISPLAY.\n\
                              Default: $DISPLAY or :0.0 when not set.\n\
  -w, -window WINDOW_ID       Specifies the window to manipulate. Special\n\
                              identifiers are `root' for the root window and\n\
                              `focus' for the currently focused window.\n\
  -c, -class WM_CLASS         Window management class of the window(s) to\n\
                              manipulate. If WM_CLASS contains a period, only\n\
                              windows with exactly the same WM_CLASS property\n\
                              are matched. If there is no period, windows of\n\
                              the same class and windows of the same instance\n\
                              (aka. `-name') are selected.\n\
\n\
Actions:\n\
  setIconTitle   TITLE        Set the icon title.\n\
  setWindowTitle TITLE        Set the window title.\n\
  setGeometry    geometry     Set the window geometry\n\
  setState       MASK STATE   Set the GNOME window state to STATE.\n\
                              Only the bits selected by MASK are affected.\n\
                              STATE and MASK are expressions of the domain\n\
                              `GNOME window state'.\n\
  toggleState    STATE        Toggle the GNOME window state bits specified by\n\
                              the STATE expression.\n\
  setHints       HINTS        Set the GNOME window hints to HINTS.\n\
  setLayer       LAYER        Moves the window to another GNOME window layer.\n\
  setWorkspace   WORKSPACE    Moves the window to another workspace. Select\n\
                              the root window to change the current workspace.\n\
                              Select 0xFFFFFFFF or \"All\" for all workspaces.\n\
  listWorkspaces              Lists the names of all workspaces.\n\
  setTrayOption  TRAYOPTION   Set the IceWM tray option hint.\n\
  logout                      Tell IceWM to logout.\n\
  reboot                      Tell IceWM to reboot.\n\
  shutdown                    Tell IceWM to shutdown.\n\
  cancel                      Tell IceWM to cancel the logout/reboot/shutdown.\n\
  about                       Tell IceWM to show the about window.\n\
  windowlist                  Tell IceWM to show the window list.\n\
  restart                     Tell IceWM to restart.\n\
  suspend                     Tell IceWM to suspend.\n\
\n\
Expressions:\n\
  Expressions are list of symbols of one domain concatenated by `+' or `|':\n\
\n\
  EXPRESSION ::= SYMBOL | EXPRESSION ( `+' | `|' ) SYMBOL\n\n"),
           ApplicationName);

    states.listSymbols(_("GNOME window state"));
    hints.listSymbols(_("GNOME window hint"));
    layers.listSymbols(_("GNOME window layer"));
    trayOptions.listSymbols(_("IceWM tray option"));
}

static void usageError(char const *msg, ...) {
    fprintf(stderr, "%s: ", ApplicationName);
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

static bool isOptArg(const char* arg, const char* opt, const char* val) {
    const char buf[3] = { opt[0], opt[1], '\0', };
    return (strpcmp(arg, opt) == 0 || strcmp(arg, buf) == 0) && val != 0;
}

int main(int argc, char **argv) {
#ifdef CONFIG_I18N
    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);
#endif

    ApplicationName = my_basename(*argv);
    char const *dpyname(NULL);
    char const *winname(NULL);
    char *wmclass(NULL);
    char *wmname(NULL);
    int rc(0);

    Window singleWindowList[] = { None };
    Window *windowList(NULL);
    unsigned windowCount(0);

    char **argp(argv + 1);
    for (char *arg; argp < argv + argc && '-' == *(arg = *argp); ++argp) {
        if (is_version_switch(arg)) {
            print_version_exit(VERSION);
        }
        else if (is_help_switch(arg)) {
            printUsage();
            THROW(0);
        }
        else if (is_copying_switch(arg)) {
            print_copying_exit();
        }
        else if (arg[1] == '-') {
            ++arg;
        }

        size_t sep(strcspn(arg, "=:"));
        char *val(arg[sep] ? arg + sep + 1 : *++argp);

        if (isOptArg(arg, "-display", val)) {
            dpyname = val;
        } else if (isOptArg(arg, "-window", val)) {
            winname = val;
        } else if (isOptArg(arg, "-class", val)) {
            wmname = val;
            char *p = val;
            char *d = val;
            while (*p) {
                if (*p == '\\') {
                    p++;
                    if (*p == '\0')
                        break;
                } else if (*p == '.') {
                    *d++ = 0;
                    wmclass = d;
                    p++;
                    continue;
                }
                *d++ = *p++;
            }
            *d++ = 0;

            MSG(("wmname: `%s'; wmclass: `%s'", wmname, wmclass));
#ifdef DEBUG
        } else if (!(strpcmp(arg, "-debug"))) {
            debug = 1;
            --argp;
#endif
        } else {
            usageError (_("Invalid argument: `%s'."), arg);
            THROW(1);
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

    root = DefaultRootWindow(display);

    ATOM_WM_STATE = XInternAtom(display, "WM_STATE", False);
    ATOM_WIN_WORKSPACE = XInternAtom(display, XA_WIN_WORKSPACE, False);
    ATOM_WIN_WORKSPACE_NAMES = XInternAtom(display, XA_WIN_WORKSPACE_NAMES, False);
    ATOM_WIN_WORKSPACE_COUNT = XInternAtom(display, XA_WIN_WORKSPACE_COUNT, False);
    ATOM_WIN_STATE = XInternAtom(display, XA_WIN_STATE, False);
    ATOM_WIN_HINTS = XInternAtom(display, XA_WIN_HINTS, False);
    ATOM_WIN_LAYER = XInternAtom(display, XA_WIN_LAYER, False);
    ATOM_WIN_TRAY = XInternAtom(display, XA_WIN_TRAY, False);
    ATOM_ICE_ACTION = XInternAtom(display, "_ICEWM_ACTION", False);

    /******************************************************************************/

    if (winname) {
        if (!strcmp(winname, "root")) {
            *(windowList = singleWindowList) = root;
            windowCount = 1;

            MSG(("root window selected"));
        } else if (!strcmp(winname, "focus")) {
            int dummy;

            windowList = singleWindowList;
            windowCount = 1;

            XGetInputFocus(display, windowList, &dummy);

            MSG(("focused window selected"));
        } else {
            char *eptr;

            *(windowList = singleWindowList) = strtol(winname, &eptr, 0);
            windowCount = 1;

            if (NULL == eptr || '\0' != *eptr) {
                msg(_("Invalid window identifier: `%s'"), winname);
                THROW(1);
            }

            MSG(("focused window selected"));
        }
    } else {
        if (NULL == wmname) {
            *(windowList = singleWindowList) = pickWindow();
            windowCount = 1;

            MSG(("window picked"));
        } else {
            Window dummy;
            XQueryTree(display, root, &dummy, &dummy,
                       &windowList, &windowCount);

            MSG(("window tree fetched, got %d window handles", windowCount));
        }
    }

    if (wmname) {
        unsigned matchingWindowCount = 0;

        for (unsigned i = 0; i < windowCount; ++i) {
            XClassHint classhint;

            windowList[i] = getClientWindow(windowList[i]);

            if (windowList[i] != None &&
                XGetClassHint(display, windowList[i], &classhint)) {
                if (wmclass) {
                    if (strcmp(classhint.res_name, wmname) ||
                        strcmp(classhint.res_class, wmclass))
                        windowList[i] = None;
                } else {
                    if (strcmp(classhint.res_name, wmname) &&
                        strcmp(classhint.res_class, wmname))
                        windowList[i] = None;
                }

                if (windowList[i] != None) {
                    MSG(("selected window 0x%lx: `%s.%s'", windowList[i],
                         classhint.res_name, classhint.res_class));

                    windowList[matchingWindowCount++] = windowList[i];
                }
            }
        }

        windowCount = matchingWindowCount;
    }

    MSG(("windowCount: %d", windowCount));

/******************************************************************************/

    while (argp < argv + argc) {
        char const * action(*argp++);

        if (!strcmp(action, "setWindowTitle")) {
            CHECK_ARGUMENT_COUNT (1)

                char const * title(*argp++);

            MSG(("setWindowTitle: `%s'", title));

            FOREACH_WINDOW(window) XStoreName(display, *window, title);
        } else if (!strcmp(action, "setIconTitle")) {
            CHECK_ARGUMENT_COUNT (1)

                char const * title(*argp++);

            MSG(("setIconTitle: `%s'", title));
            FOREACH_WINDOW(window) XSetIconName(display, *window, title);
        } else if (!strcmp(action, "setGeometry")) {
            CHECK_ARGUMENT_COUNT (1)

                char const * geometry(*argp++);
            int geom_x, geom_y; unsigned geom_width, geom_height;
            int status(XParseGeometry(geometry, &geom_x, &geom_y,
                                      &geom_width, &geom_height));

            FOREACH_WINDOW(window) {
                XSizeHints normal;
                long supplied;
                XGetWMNormalHints(display, *window, &normal, &supplied);

                Window root; int x, y; unsigned width, height, dummy;
                XGetGeometry(display, *window, &root,
                             &x, &y, &width, &height, &dummy, &dummy);

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
                XMoveResizeWindow(display, *window, x, y, width, height);
            }
        } else if (!strcmp(action, "setState")) {
            CHECK_ARGUMENT_COUNT (2)

                unsigned mask(states.parseExpression(*argp++));
            unsigned state(states.parseExpression(*argp++));
            CHECK_EXPRESSION(states, mask, argp[-2])
                CHECK_EXPRESSION(states, state, argp[-1])

                MSG(("setState: %d %d", mask, state));
            FOREACH_WINDOW(window) setState(*window, mask, state);
        } else if (!strcmp(action, "toggleState")) {
            CHECK_ARGUMENT_COUNT (1)

                unsigned state(states.parseExpression(*argp++));
            CHECK_EXPRESSION(states, state, argp[-1])

                MSG(("toggleState: %d", state));
            FOREACH_WINDOW(window) toggleState(*window, state);
        } else if (!strcmp(action, "setHints")) {
            CHECK_ARGUMENT_COUNT (1)

                unsigned hint(hints.parseExpression(*argp++));
            CHECK_EXPRESSION(hints, hint, argp[-1])

                MSG(("setHints: %d", hint));
            FOREACH_WINDOW(window) setHints(*window, hint);
        } else if (!strcmp(action, "setWorkspace")) {
            CHECK_ARGUMENT_COUNT (1)

            long workspace;
            if ( ! WorkspaceInfo(root).parseWorkspaceName(*argp++, &workspace))
                THROW(1);

            MSG(("setWorkspace: %ld", workspace));
            FOREACH_WINDOW(window) setWorkspace(*window, workspace);
        } else if (!strcmp(action, "listWorkspaces")) {
            YTextProperty workspaceNames(root, ATOM_WIN_WORKSPACE_NAMES);
            for (int n(0); n < workspaceNames.count(); ++n)
                if (n + 1 < workspaceNames.count() || workspaceNames.item(n)[0])
                printf(_("workspace #%d: `%s'\n"), n, workspaceNames.item(n));
        } else if (!strcmp(action, "setLayer")) {
            CHECK_ARGUMENT_COUNT (1)

                unsigned layer(layers.parseExpression(*argp++));
            CHECK_EXPRESSION(layers, layer, argp[-1])

                MSG(("setLayer: %d", layer));
            FOREACH_WINDOW(window) setLayer(*window, layer);
        } else if (!strcmp(action, "setTrayOption")) {
            CHECK_ARGUMENT_COUNT (1)

                unsigned trayopt(trayOptions.parseExpression(*argp++));
            CHECK_EXPRESSION(trayOptions, trayopt, argp[-1])

                MSG(("setTrayOption: %d", trayopt));
            FOREACH_WINDOW(window) setTrayHint(*window, trayopt);
        } else if (icewmAction(action)) {
        } else {
            msg(_("Unknown action: `%s'"), action);
            THROW(1);
        }
    }

    CATCH(
          if (windowList != singleWindowList) {
              XFree(windowList);
          }

          if (display) {
              XSync(display, False);
              XCloseDisplay(display);
          }
         )

    return 0;
}

// vim: set sw=4 ts=4 et:
