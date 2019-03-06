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
#include <signal.h>
#include <setjmp.h>
#include <string.h>
#include <unistd.h>

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
#define GUI_EVENT_NAMES
#include "guievent.h"

/******************************************************************************/
/******************************************************************************/

char const * ApplicationName;
static Display *display;
static Window root;

/******************************************************************************/

class NAtom {
    const char* name;
    Atom atom;
public:
    NAtom(const char* name) : name(name), atom(None) { }
    operator unsigned() { return atom ? atom :
        atom = XInternAtom(display, name, False); }
};

NAtom ATOM_WM_STATE("WM_STATE");
NAtom ATOM_WIN_WORKSPACE(XA_WIN_WORKSPACE);
NAtom ATOM_WIN_WORKSPACE_NAMES(XA_WIN_WORKSPACE_NAMES);
NAtom ATOM_WIN_WORKSPACE_COUNT(XA_WIN_WORKSPACE_COUNT);
NAtom ATOM_WIN_SUPPORTING_WM_CHECK(XA_WIN_SUPPORTING_WM_CHECK);
NAtom ATOM_WIN_STATE(XA_WIN_STATE);
NAtom ATOM_WIN_HINTS(XA_WIN_HINTS);
NAtom ATOM_WIN_LAYER(XA_WIN_LAYER);
NAtom ATOM_WIN_TRAY(XA_WIN_TRAY);
NAtom ATOM_GUI_EVENT(XA_GUI_EVENT_NAME);
NAtom ATOM_ICE_ACTION("_ICEWM_ACTION");
NAtom ATOM_NET_CLIENT_LIST("_NET_CLIENT_LIST");

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
    const T* data() const { return (T *) fData; }
    template <class T>
    const T& data(unsigned index) const { return data<T>()[index]; }

    operator bool() const { return fStatus == Success && fType != 0; }

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
        fStatus(XGetTextProperty(display, window, &fProperty, property))
    {
        if (fStatus == True) {
            XTextPropertyToStringList(&fProperty, &fList, &fCount);
        }
        else {
            fProperty.value = 0;
        }
    }

    virtual ~YTextProperty() {
        if (fList)
            XFreeStringList(fList);
        if (*this && fProperty.value)
            XFree(fProperty.value);
    }

    char * item(unsigned index) const { return fList[index]; }
    int count() const { return fCount; }

    operator bool() const { return fStatus == True && count() > 0; }
    char * operator[](unsigned index) const { return item(index); }

private:
    XTextProperty fProperty;
    char ** fList;
    int fCount, fStatus;
};

class YWindowTree {
public:
    YWindowTree(Window window = None):
        fRoot(None), fParent(None),
        fChildren(NULL), fCount(0),
        fSuccess(False)
    {
        if (window)
            query(window);
    }

    void limit(unsigned limit) {
        fCount = min(fCount, limit);
    }

    void set(Window window) {
        release();
        fChildren = (Window *) malloc(sizeof(Window));
        fChildren[0] = window;
        fCount = 1;
        fSuccess = True;
    }

    Status query(Window window) {
        release();
        if (window) {
            fSuccess = XQueryTree(display, window, &fRoot, &fParent,
                                  &fChildren, &fCount);
        }
        return fSuccess;
    }

    Status getClientList() {
        release();
        YWindowProperty clients(root, ATOM_NET_CLIENT_LIST, XA_WINDOW, 10000);
        if (clients && XA_WINDOW == clients.type() && 32 == clients.format()) {
            fCount = clients.count();
            fChildren = (Window *) malloc(fCount * sizeof(Window));
            fSuccess = True;
            for (int k = 0; k < clients.count(); ++k) {
                fChildren[k] = clients.data<Window>(k);
            }
        }
        return fSuccess;
    }

    void release() {
        if (fChildren) {
            XFree(fChildren);
            fChildren = 0;
            fCount = 0;
            fSuccess = False;
        }
    }

    virtual ~YWindowTree() {
        if (fChildren)
            XFree(fChildren);
    }

    operator bool() const {
        return fSuccess == True;
    }

    operator Window*() const {
        return fChildren;
    }

    Window* end() const {
        return fChildren + count();
    }

    unsigned count() const {
        return fCount;
    }

    Window& operator[](unsigned index) const {
        return fChildren[index];
    }

private:
    Window fRoot, fParent, * fChildren;
    unsigned fCount;
    bool fSuccess;
};

class IceSh {
public:
    IceSh(int argc, char **argv);
    ~IceSh();
    operator int() const { return rc; }

private:
    int rc;
    int argc;
    char **argv;
    char **argp;

    char const *dpyname;
    char const *winname;
    char const *wmclass;
    char const *wmname;

    YWindowTree windowList;

    jmp_buf jmpbuf;
    void THROW(int val);

    void flush();
    void flags();
    void xinit();
    void details(Window window);
    void setWindow(Window window);
    void getWindows(bool interactive = true);
    void parseActions();
    bool isAction(const char* str, int argCount);
    bool icewmAction();
    bool guiEvents();
    bool listShown();
    bool listClients();
    bool listWindows();
    bool listWorkspaces();
    bool colormaps();
    bool wmcheck();
    bool check(const struct SymbolTable& symtab, long code, const char* str);
    unsigned count() const;

    static void catcher(int);
    static bool running;
};

/******************************************************************************/
/******************************************************************************/

#define FOREACH_WINDOW(W) \
    for (Window W, *P = windowList; P < windowList.end() && (W = *P, true); ++P)

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

    char *endptr(0);
    long value(strtol(id, &endptr, 0));

    return (NULL != endptr && '\0' == *endptr &&
            value >= fMin && value <= fMax ? value : fErrCode);
}

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

    if (winState && XA_CARDINAL == winState.type() &&
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
    }

    bool parseWorkspaceName(char const* name, long* workspace);

    long count();
    operator bool() const { return fCount && fNames; }

    YWindowProperty fCount;
    YTextProperty fNames;
};

long WorkspaceInfo::count() {
    return (fCount ? fCount.data<long>(0) : 0);
}

bool WorkspaceInfo::parseWorkspaceName(char const* name, long* workspace) {
    *workspace = WinWorkspaceInvalid;

    if (*this == Success) {
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

long getWorkspace(Window window) {
    YWindowProperty prop(window, ATOM_WIN_WORKSPACE, XA_CARDINAL, 1);
    return prop ? prop.data<long>(0) : 0;
}

bool IceSh::running;

void IceSh::catcher(int)
{
    running = false;
}

void IceSh::details(Window w)
{
    char c = 0, *name = &c, *wmname = &c, title[128] = "", wmtitle[200] = "";

    XFetchName(display, w, &name);
    snprintf(title, sizeof title, "\"%s\"", name ? name : "");

    YTextProperty text(w, XA_WM_CLASS);
    if (text.count()) {
        wmname = text.item(0);
        wmname[strlen(wmname)] = '.';
    }
    snprintf(wmtitle, sizeof wmtitle, "(%s)", wmname);

    XWindowAttributes a = {};
    XGetWindowAttributes(display, w, &a);

    printf(_("0x%-8lx %-14s: %-20s %dx%d%+d%+d\n"), w, title, wmtitle,
            a.width, a.height, a.x, a.y);
    if (name)
        XFree(name);
}

bool IceSh::listWindows()
{
    if (strcmp(*argp, "windows"))
        return false;
    ++argp;
    if (count() == 0)
        getWindows(false);

    FOREACH_WINDOW(w) {
        details(w);
    }
    return true;
}

bool IceSh::listClients()
{
    if (strcmp(*argp, "clients"))
        return false;
    ++argp;

    windowList.getClientList();

    FOREACH_WINDOW(w) {
        details(w);
    }
    return true;
}

bool IceSh::listShown()
{
    if (strcmp(*argp, "shown"))
        return false;
    ++argp;

    windowList.getClientList();
    long workspace = getWorkspace(root);

    FOREACH_WINDOW(w) {
        long ws = getWorkspace(w);
        if (ws == workspace || hasbits(ws, 0xFFFFFFFF))
            details(w);
    }
    return true;
}

bool IceSh::listWorkspaces()
{
    if (strcmp(*argp, "listWorkspaces"))
        return false;
    ++argp;

    YTextProperty workspaceNames(root, ATOM_WIN_WORKSPACE_NAMES);
    for (int n(0); n < workspaceNames.count(); ++n)
        if (n + 1 < workspaceNames.count() || workspaceNames.item(n)[0])
        printf(_("workspace #%d: `%s'\n"), n, workspaceNames.item(n));
    return true;
}

bool IceSh::wmcheck()
{
    if (strcmp(*argp, "check"))
        return false;
    ++argp;

    YWindowProperty check(root, ATOM_WIN_SUPPORTING_WM_CHECK, XA_CARDINAL, 4);
    if (check && check.type() == XA_CARDINAL) {
        Window win = check.data<Window>(0);
        char* name = 0;
        if (XFetchName(display, win, &name) && name) {
            printf("Name: %s\n", name);
            XFree(name);
        }
        YWindowProperty cls(win, NAtom("WM_CLASS"), XA_STRING, 1234);
        if (cls) {
            printf("Class: ");
            for (int i = 0; i + 1 < cls.count(); ++i) {
                char c = Elvis(cls.data<char>(i), '.');
                putchar(c);
            }
            putchar('\n');
        }
        YWindowProperty loc(win, NAtom("WM_LOCALE_NAME"), XA_STRING, 100);
        if (loc) {
            printf("Locale: %s\n", loc.data<char>());
        }
        YWindowProperty com(win, NAtom("WM_COMMAND"), XA_STRING, 1234);
        if (com) {
            printf("Command: ");
            for (int i = 0; i + 1 < com.count(); ++i) {
                char c = Elvis(com.data<char>(i), ' ');
                putchar(c);
            }
            putchar('\n');
        }
        YWindowProperty mac(win, NAtom("WM_CLIENT_MACHINE"), XA_STRING, 100);
        if (mac) {
            printf("Machine: %s\n", mac.data<char>());
        }
        YWindowProperty pid(win, NAtom("_NET_WM_PID"), XA_CARDINAL, 4);
        if (pid && pid.type() == XA_CARDINAL) {
            printf("PID: %lu\n", pid.data<Window>(0));
        }
    }
    return true;
}

bool IceSh::colormaps()
{
    if (strcmp(*argp, "colormaps"))
        return false;
    ++argp;

    Colormap* old = 0;
    int m = 0, k = 0;

    tlog("colormaps");
    running = true;
    sighandler_t previous = signal(SIGINT, catcher);
    while (running) {
        int n = 0;
        Colormap* map = XListInstalledColormaps(display, root, &n);
        if (n != m || old == 0 || memcmp(map, old, n * sizeof(Colormap)) || !(++k % 100)) {
            char buf[2000] = "";
            for (int i = 0; i < n; ++i) {
                snprintf(buf + strlen(buf), sizeof buf - strlen(buf),
                        " 0x%0lx", map[i]);
            }
            printf("colormaps%s\n", buf);
            fflush(stdout);
            if (old) {
                XFree(old);
            }
            old = map;
            m = n;
        }
        else {
            XFree(map);
        }
        usleep(100*1000);
    }

    if (old) {
        XFree(old);
    }

    signal(SIGINT, previous);
    return true;
}

bool IceSh::guiEvents()
{
    if (strcmp(*argp, "guievents"))
        return false;
    ++argp;

    running = true;
    sighandler_t previous = signal(SIGINT, catcher);
    XSelectInput(display, root, PropertyChangeMask);
    while (running) {
        if (XPending(display)) {
            XEvent xev = { 0 };
            XNextEvent(display, &xev);
            if (xev.type == PropertyNotify &&
                xev.xproperty.atom == ATOM_GUI_EVENT &&
                xev.xproperty.state == PropertyNewValue)
            {
                Atom type;
                int gev(-1), format;
                unsigned long nitems, lbytes;
                unsigned char *propdata(0);

                if (XGetWindowProperty(display, root, ATOM_GUI_EVENT,
                                       0, 1, False, ATOM_GUI_EVENT,
                                       &type, &format, &nitems, &lbytes,
                                       &propdata) == Success && propdata)
                {
                    gev = propdata[0];
                    XFree(propdata);

                    if (inrange(1 + gev, 1, NUM_GUI_EVENTS)) {
                        puts(gui_event_names[gev]);
                        flush();
                    }
                }
            }
        }
        else {
            int fd = ConnectionNumber(display);
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(fd, &rfds);
            select(fd + 1, SELECT_TYPE_ARG234 &rfds, NULL, NULL, NULL);
        }
    }
    signal(SIGINT, previous);
    return true;
}

bool IceSh::icewmAction()
{
    const char* str = *argp;
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
        return guiEvents()
            || listWorkspaces()
            || listWindows()
            || listClients()
            || listShown()
            || colormaps()
            || wmcheck()
            ;
    else
        ++argp;

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

unsigned IceSh::count() const
{
    return windowList.count();
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
    YWindowProperty wmstate(window, ATOM_WM_STATE);
    if (wmstate && wmstate.type() == ATOM_WM_STATE)
        return window;

    YWindowTree tree(window);
    if (tree == false) {
        warn("XQueryTree failed for window 0x%lx", window);
        return None;
    }

    Window client(None);

    for (unsigned i = 0; client == None && i < tree.count(); ++i)
        if (YWindowProperty(tree[i], ATOM_WM_STATE).type() == ATOM_WM_STATE)
            client = tree[i];

    for (unsigned i = 0; client == None && i < tree.count(); ++i)
        client = getClientWindow(tree[i]);

    return client;
}

Window pickWindow (void) {
    Cursor cursor = XCreateFontCursor(display, XC_crosshair);
    bool running(true);
    Window target(None);
    int count(0);
    KeyCode escape = XKeysymToKeycode(display, XK_Escape);

    // this is broken
    XGrabKey(display, escape, 0, root, False, GrabModeAsync, GrabModeAsync);
    XGrabPointer(display, root, False, ButtonPressMask|ButtonReleaseMask,
                 GrabModeAsync, GrabModeAsync, root, cursor, CurrentTime);

    while (running && (None == target || 0 != count)) {
        XEvent event;
        XNextEvent (display, &event);

        switch (event.type) {
        case KeyPress:
        case KeyRelease:
            if (event.xkey.keycode == escape)
                running = false;
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
    }

    XUngrabPointer(display, CurrentTime);
    // and this is broken
    XUngrabKey(display, escape, 0, root);

    return (None == target || root == target) ? target
            : getClientWindow(target);
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

/******************************************************************************/
/******************************************************************************/

static bool isOptArg(const char* arg, const char* opt, const char* val) {
    const char buf[3] = { opt[0], opt[1], '\0', };
    return (strpcmp(arg, opt) == 0 || strcmp(arg, buf) == 0) && val != 0;
}

void IceSh::flush()
{
    if (fflush(stdout) || ferror(stdout))
        THROW(1);
}

void IceSh::THROW(int val)
{
    rc = val;
    longjmp(jmpbuf, true);
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
    setvbuf(stderr, NULL, _IOLBF, BUFSIZ);

#ifdef CONFIG_I18N
    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);
#endif

    ApplicationName = my_basename(*argv);

    return IceSh(argc, argv);
}

IceSh::IceSh(int ac, char **av) :
    rc(0),
    argc(ac),
    argv(av),
    argp(av + 1),
    dpyname(0),
    winname(0),
    wmclass(0),
    wmname(0)
{
    if (setjmp(jmpbuf) == 0) {
        flags();
        xinit();

        while (argp < &argv[argc] && icewmAction());

        if (argp < &argv[argc]) {
            getWindows();
            parseActions();
        }
    }
}

bool IceSh::isAction(const char* action, int count)
{
    if (strcmp(action, *argp))
        return false;

    if (argv + argc <= argp + count) {
        msg(_("Action `%s' requires at least %d arguments."), action, count);
        THROW(1);
    }

    ++argp;
    return true;
}

bool IceSh::check(const SymbolTable& symtab, long code, const char* str)
{
    if (symtab.invalid(code)) {
        msg(_("Invalid expression: `%s'"), str);
        THROW(1);
    }
    return true;
}

void IceSh::flags()
{
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

        if (isOptArg(arg, "-root", "")) {
            winname = "root";
            continue;
        }
        if (isOptArg(arg, "-focus", "")) {
            winname = "focus";
            continue;
        }

        size_t sep(strcspn(arg, "=:"));
        char *val(arg[sep] ? arg + sep + 1 : *++argp);

        if (isOptArg(arg, "-display", val)) {
            dpyname = val;
        }
        else if (isOptArg(arg, "-window", val)) {
            winname = val;
        }
        else if (isOptArg(arg, "-class", val)) {
            wmname = val;
            char *p = val;
            char *d = val;
            while (*p) {
                if (*p == '\\') {
                    p++;
                    if (*p == '\0')
                        break;
                }
                else if (*p == '.') {
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
        }
        else if (!(strpcmp(arg, "-debug"))) {
            debug = 1;
            --argp;
#endif
        }
        else {
            msg(_("Invalid argument: `%s'."), arg);
            THROW(1);
        }
    }

    if (argp >= argv + argc) {
        msg(_("No actions specified."));
        THROW(1);
    }
}

/******************************************************************************/

void IceSh::xinit()
{
    if (NULL == (display = XOpenDisplay(dpyname))) {
        warn(_("Can't open display: %s. X must be running and $DISPLAY set."),
             XDisplayName(dpyname));
        THROW(3);
    }

    root = DefaultRootWindow(display);
}

/******************************************************************************/

void IceSh::setWindow(Window window)
{
    windowList.set(window);
}

void IceSh::getWindows(bool interactive)
{
    if (winname) {
        if (!strcmp(winname, "root")) {
            setWindow(root);

            MSG(("root window selected"));
        }
        else if (!strcmp(winname, "focus")) {
            int dummy;
            Window w = None;
            XGetInputFocus(display, &w, &dummy);
            setWindow(w);

            MSG(("focused window selected"));
        }
        else {
            char *eptr = 0;

            Window w = strtol(winname, &eptr, 0);

            if (w == None || NULL == eptr || '\0' != *eptr) {
                msg(_("Invalid window identifier: `%s'"), winname);
                THROW(1);
            }
            setWindow(w);

            MSG(("focused window selected"));
        }
    }
    else {
        if (NULL == wmname && interactive) {
            setWindow(pickWindow());

            MSG(("window picked"));
        }
        else {
            windowList.query(root);

            MSG(("window tree fetched, got %d window handles", count()));
        }
    }

    if (wmname) {
        unsigned matchingWindowCount = 0;
        FOREACH_WINDOW(window) {
            window = getClientWindow(window);
            if (window == None)
                continue;

            XClassHint classhint;
            if (XGetClassHint(display, window, &classhint)) {
                if (wmclass) {
                    if (strcmp(classhint.res_name, wmname) ||
                        strcmp(classhint.res_class, wmclass))
                        window = None;
                }
                else {
                    if (strcmp(classhint.res_name, wmname) &&
                        strcmp(classhint.res_class, wmname))
                        window = None;
                }

                if (window) {
                    MSG(("selected window 0x%lx: `%s.%s'", window,
                         classhint.res_name, classhint.res_class));

                    windowList[matchingWindowCount++] = window;
                }
            }
        }

        windowList.limit(matchingWindowCount);
    }

    MSG(("windowCount: %d", count()));
}

/******************************************************************************/

void IceSh::parseActions()
{
    while (argp < argv + argc) {
        if (isAction("setWindowTitle", 1)) {
            char const * title(*argp++);

            MSG(("setWindowTitle: `%s'", title));

            FOREACH_WINDOW(window)
                XStoreName(display, window, title);
        }
        else if (isAction("setIconTitle", 1)) {
            char const * title(*argp++);

            MSG(("setIconTitle: `%s'", title));
            FOREACH_WINDOW(window)
                XSetIconName(display, window, title);
        }
        else if (isAction("setGeometry", 1)) {
            char const * geometry(*argp++);
            int geom_x, geom_y; unsigned geom_width, geom_height;
            int status(XParseGeometry(geometry, &geom_x, &geom_y,
                                      &geom_width, &geom_height));

            FOREACH_WINDOW(window) {
                XSizeHints normal;
                long supplied;
                XGetWMNormalHints(display, window, &normal, &supplied);

                Window root; int x, y; unsigned width, height, dummy;
                XGetGeometry(display, window, &root,
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
                if (hasbit(status, XValue | YValue) &&
                    hasbit(status, WidthValue | HeightValue))
                    XMoveResizeWindow(display, window, x, y, width, height);
                else if (hasbit(status, XValue | YValue))
                    XMoveWindow(display, window, x, y);
                else if (hasbit(status, WidthValue | HeightValue))
                    XResizeWindow(display, window, width, height);
            }
        }
        else if (isAction("setState", 2)) {
            unsigned mask(states.parseExpression(*argp++));
            unsigned state(states.parseExpression(*argp++));
            check(states, mask, argp[-2]);
            check(states, state, argp[-1]);

            MSG(("setState: %d %d", mask, state));
            FOREACH_WINDOW(window)
                setState(window, mask, state);
        }
        else if (isAction("toggleState", 1)) {
            unsigned state(states.parseExpression(*argp++));
            check(states, state, argp[-1]);

            MSG(("toggleState: %d", state));
            FOREACH_WINDOW(window)
                toggleState(window, state);
        }
        else if (isAction("setHints", 1)) {
            unsigned hint(hints.parseExpression(*argp++));
            check(hints, hint, argp[-1]);

            MSG(("setHints: %d", hint));
            FOREACH_WINDOW(window)
                setHints(window, hint);
        }
        else if (isAction("setWorkspace", 1)) {
            long workspace;
            if ( ! WorkspaceInfo(root).parseWorkspaceName(*argp++, &workspace))
                THROW(1);

            MSG(("setWorkspace: %ld", workspace));
            FOREACH_WINDOW(window)
                setWorkspace(window, workspace);
        }
        else if (isAction("setLayer", 1)) {
            unsigned layer(layers.parseExpression(*argp++));
            check(layers, layer, argp[-1]);

            MSG(("setLayer: %d", layer));
            FOREACH_WINDOW(window)
                setLayer(window, layer);
        }
        else if (isAction("setTrayOption", 1)) {
            unsigned trayopt(trayOptions.parseExpression(*argp++));
            check(trayOptions, trayopt, argp[-1]);

            MSG(("setTrayOption: %d", trayopt));
            FOREACH_WINDOW(window)
                setTrayHint(window, trayopt);
        }
        else if (icewmAction()) {
        }
        else if (isAction("runonce", 1)) {
            if (windowList.count() == 0 ||
                (windowList.count() == 1 && windowList[0] == root))
            {
                execvp(argp[0], argp);
                perror(argp[0]);
                THROW(1);
            } else {
                THROW(0);
            }
        }
        else {
            msg(_("Unknown action: `%s'"), *argp);
            THROW(1);
        }

        flush();
    }
}

IceSh::~IceSh()
{
    if (display) {
        XSync(display, False);
        XCloseDisplay(display);
        display = 0;
    }
}

// vim: set sw=4 ts=4 et:
