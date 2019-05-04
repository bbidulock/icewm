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

#include "ascii.h"
#include "base.h"
#include "WinMgr.h"
#include "wmaction.h"
#define GUI_EVENT_NAMES
#include "guievent.h"

/******************************************************************************/
/******************************************************************************/

using namespace ASCII;
const long SourceIndication = 1L;
char const* ApplicationName = "icesh";
static Display *display;
static Window root;

static void printUsage() {
    printf(_("Usage: %s [OPTIONS] ACTIONS\n"
             "For help please consult the man page %s(1).\n"),
            ApplicationName, ApplicationName);
}

static bool tolong(const char* str, long& num) {
    char* end = 0;
    num = strtol(str, &end, 10);
    return end && str < end && 0 == *end;
}

/******************************************************************************/

class NAtom {
    const char* name;
    Atom atom;
public:
    explicit NAtom(const char* aName) : name(aName), atom(None) { }
    operator Atom() { return atom ? atom : get(); }
private:
    Atom get() { atom = XInternAtom(display, name, False); return atom; }
};

static NAtom ATOM_WM_STATE("WM_STATE");
static NAtom ATOM_WM_LOCALE_NAME("WM_LOCALE_NAME");
static NAtom ATOM_NET_WM_PID("_NET_WM_PID");
static NAtom ATOM_NET_WM_DESKTOP("_NET_WM_DESKTOP");
static NAtom ATOM_NET_DESKTOP_NAMES("_NET_DESKTOP_NAMES");
static NAtom ATOM_NET_CURRENT_DESKTOP("_NET_CURRENT_DESKTOP");
static NAtom ATOM_NET_NUMBER_OF_DESKTOPS("_NET_NUMBER_OF_DESKTOPS");
static NAtom ATOM_NET_SUPPORTING_WM_CHECK("_NET_SUPPORTING_WM_CHECK");
static NAtom ATOM_WIN_STATE(XA_WIN_STATE);
static NAtom ATOM_WIN_HINTS(XA_WIN_HINTS);
static NAtom ATOM_WIN_LAYER(XA_WIN_LAYER);
static NAtom ATOM_WIN_TRAY(XA_WIN_TRAY);
static NAtom ATOM_GUI_EVENT(XA_GUI_EVENT_NAME);
static NAtom ATOM_ICE_ACTION("_ICEWM_ACTION");
static NAtom ATOM_NET_CLIENT_LIST("_NET_CLIENT_LIST");
static NAtom ATOM_NET_CLOSE_WINDOW("_NET_CLOSE_WINDOW");
static NAtom ATOM_NET_ACTIVE_WINDOW("_NET_ACTIVE_WINDOW");
static NAtom ATOM_NET_RESTACK_WINDOW("_NET_RESTACK_WINDOW");
static NAtom ATOM_NET_WM_WINDOW_OPACITY("_NET_WM_WINDOW_OPACITY");

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
        fType(0), fCount(0), fAfter(0), fData(nullptr), fFormat(0), fStatus(0)
    {
        update(window, property, type, length, offset, deleteProp);
    }

    virtual ~YWindowProperty() {
        release();
    }

    void release() {
        if (fData) {
            XFree(fData);
            fData = nullptr;
            fCount = 0;
        }
    }

    void update(Window window, Atom property, Atom type = AnyPropertyType,
                long length = 0, long offset = 0, bool deleteProp = False)
    {
        release();
        fStatus = XGetWindowProperty(display, window, property, offset,
                                     length, deleteProp, type, &fType,
                                     &fFormat, &fCount, &fAfter, &fData);
    }

    Atom type() const { return fType; }
    int format() const { return fFormat; }
    long count() const { return fCount; }
    unsigned long after() const { return fAfter; }

    template <class T>
    const T* data() const { return reinterpret_cast<T *>(fData); }
    template <class T>
    T data(unsigned index) const {
        return index < fCount ? data<T>()[index] : T(0);
    }

    operator bool() const { return fStatus == Success && fType && fCount; }

private:
    Atom fType;
    unsigned long fCount, fAfter;
    unsigned char* fData;
    int fFormat, fStatus;
};

enum YTextKind {
    YText,
    YEmby,
};

class YTextProperty {
public:
    YTextProperty(Window window, Atom property, YTextKind kind = YText):
        fList(nullptr), fProp(property), fCount(0), fStatus(False)
    {
        XTextProperty text;
        if (XGetTextProperty(display, window, &text, fProp)) {
            if (kind == YEmby) {
                int xmb = XmbTextPropertyToTextList(display, &text,
                                                    &fList, &fCount);
                fStatus = (xmb == Success);
                if (*this && isEmpty(fList[fCount - 1])) {
                    fCount--;
                }
            }
            else {
                fStatus = XTextPropertyToStringList(&text, &fList, &fCount);
            }
            XFree(text.value);
        }
    }

    virtual ~YTextProperty() {
        if (fList) {
            XFreeStringList(fList);
        }
    }

    int count() const { return fCount; }

    operator bool() const { return fStatus == True && count() > 0; }
    char* operator[](int index) const { return fList[index]; }

    bool set(int index, const char* name) {
        if (*this) {
            int num = max(1 + index, fCount);
            char** list = (char **) malloc((1 + num) * sizeof(char *));
            if (list) {
                size_t size = 1;
                for (int i = 0; i < num; ++i) {
                    size += 1 + strlen(i == index ? name : fList[i]);
                }
                char* data = (char *) malloc(size);
                char* copy = data;
                if (copy) {
                    copy[size] = '\0';
                    for (int i = 0; i < num; ++i) {
                        const char* str = (i == index ? name :
                                           i < fCount ? fList[i] : nullptr);
                        size_t len = 1 + (str ? strlen(str) : 0);
                        list[i] = copy;
                        memcpy(copy, str, len);
                        copy += len;
                    }
                    XFreeStringList(fList);
                    fList = list;
                    fCount = num;
                    fList[fCount] = data + size;
                    return true;
                }
                XFree(list);
            }
        }
        return false;
    }

    void commit() {
        if (*this) {
            XTextProperty text;
            if (XmbTextListToTextProperty(display, fList, 1 + fCount,
                                          XUTF8StringStyle, &text) == Success)
            {
                XSetTextProperty(display, root, &text, fProp);
                XFree(text.value);
            }
        }
    }

private:
    char** fList;
    Atom fProp;
    int fCount, fStatus;
};

static long getWorkspace(Window window) {
    YWindowProperty prop(window, ATOM_NET_WM_DESKTOP, XA_CARDINAL, 1);
    return prop.data<long>(0);
}

static Window getParent(Window window) {
    Window parent = None, rootwin = None, *subwins = nullptr;
    unsigned count = 0;

    if (XQueryTree(display, window, &rootwin, &parent, &subwins, &count)) {
        XFree(subwins);
    }

    return parent;
}

static Window getFrameWindow(Window window)
{
    while (window && window != root) {
        Window parent = getParent(window);
        if (parent == root)
            return window;
        window = parent;
    }
    return None;
}

class YWindowTree {
public:
    YWindowTree():
        fRoot(None), fParent(None),
        fChildren(NULL), fCount(0),
        fSuccess(False)
    {
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

    bool getClientList() {
        release();
        YWindowProperty clients(root, ATOM_NET_CLIENT_LIST, XA_WINDOW, 10000);
        if (clients && XA_WINDOW == clients.type() && 32 == clients.format()) {
            fCount = clients.count();
            fChildren = (Window *) malloc(fCount * sizeof(Window));
            fSuccess = True;
            for (int k = 0; k < clients.count(); ++k) {
                fChildren[k] = clients.data<long>(k);
            }
        }
        return *this;
    }

    bool filterByWorkspace(long workspace) {
        unsigned keep = 0;
        for (unsigned k = 0; k < fCount; ++k) {
            Window client = fChildren[k];
            long ws = getWorkspace(client);
            if (ws == workspace || hasbits(ws, 0xFFFFFFFF)) {
                fChildren[keep++] = client;
            }
        }
        fCount = keep;
        return *this;
    }

    bool filterByPid(long pid) {
        unsigned keep = 0;
        for (unsigned k = 0; k < fCount; ++k) {
            Window client = fChildren[k];
            YWindowProperty prop(client, ATOM_NET_WM_PID, XA_CARDINAL, 1);
            if (prop && prop.data<long>(0) == pid) {
                fChildren[keep++] = client;
            }
        }
        fCount = keep;
        return *this;
    }

    bool filterByClass(const char* wmname, const char* wmclass) {
        unsigned keep = 0;
        for (unsigned k = 0; k < fCount; ++k) {
            Window window = fChildren[k];
            XClassHint classhint;
            if (XGetClassHint(display, window, &classhint)) {
                if (wmclass) {
                    if (strcmp(classhint.res_name, wmname) ||
                        strcmp(classhint.res_class, wmclass))
                        window = None;
                }
                else if (wmname) {
                    if (strcmp(classhint.res_name, wmname) &&
                        strcmp(classhint.res_class, wmname))
                        window = None;
                }

                if (window) {
                    MSG(("selected window 0x%lx: `%s.%s'", window,
                         classhint.res_name, classhint.res_class));
                    fChildren[keep++] = window;
                }
            }
        }
        fCount = keep;
        return *this;
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
        return fSuccess == True && fCount > 0;
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
    char const *winprop;
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
    char* getArg();
    bool haveArg();
    bool isAction(const char* str, int argCount);
    bool icewmAction();
    bool guiEvents();
    bool listShown();
    bool listClients();
    bool listWindows();
    bool listWorkspaces();
    bool setWorkspaceName();
    bool setWorkspaceNames();
    bool colormaps();
    bool desktops();
    bool wmcheck();
    bool change();
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

#define WinStateMaximized   (WinStateMaximizedVert | WinStateMaximizedHoriz)
#define WinStateSkip        (WinStateSkipPager     | WinStateSkipTaskBar)

static Symbol stateIdentifiers[] = {
    { "Sticky",                 WinStateSticky          },
    { "Minimized",              WinStateMinimized       },
    { "Maximized",              WinStateMaximized       },
    { "MaximizedVert",          WinStateMaximizedVert   },
    { "MaximizedVertical",      WinStateMaximizedVert   },
    { "MaximizedHoriz",         WinStateMaximizedHoriz  },
    { "MaximizedHorizontal",    WinStateMaximizedHoriz  },
    { "Hidden",                 WinStateHidden          },
    { "Rollup",                 WinStateRollup          },
    { "Skip",                   WinStateSkip            },
    { "SkipPager",              WinStateSkipPager       },
    { "SkipTaskBar",            WinStateSkipTaskBar     },
    { "Fullscreen",             WinStateFullscreen      },
    { "All",                    WIN_STATE_ALL           },
    { NULL,                     0                       }
};

static Symbol hintIdentifiers[] = {
    { "SkipFocus",      WinHintsSkipFocus       },
    { "SkipWindowMenu", WinHintsSkipWindowMenu  },
    { "SkipTaskBar",    WinHintsSkipTaskBar     },
    { "FocusOnClick",   WinHintsFocusOnClick    },
    { "DoNotCover",     WinHintsDoNotCover      },
    { "All",            WIN_HINTS_ALL           },
    { NULL,             0                       }
};

static Symbol layerIdentifiers[] = {
    { "Desktop",    WinLayerDesktop     },
    { "Below",      WinLayerBelow       },
    { "Normal",     WinLayerNormal      },
    { "OnTop",      WinLayerOnTop       },
    { "Dock",       WinLayerDock        },
    { "AboveDock",  WinLayerAboveDock   },
    { "Menu",       WinLayerMenu        },
    { NULL,         0                   }
};

static Symbol trayOptionIdentifiers[] = {
    { "Ignore",         WinTrayIgnore           },
    { "Minimized",      WinTrayMinimized        },
    { "Exclusive",      WinTrayExclusive        },
    { NULL,             0                       }
};

static SymbolTable layers = {
    layerIdentifiers, 0, WinLayerCount - 1, WinLayerInvalid
};

static SymbolTable states = {
    stateIdentifiers, 0, WIN_STATE_ALL | WinStateFullscreen, -1
};

static SymbolTable hints = {
    hintIdentifiers, 0, WIN_HINTS_ALL, -1
};

static SymbolTable trayOptions = {
    trayOptionIdentifiers, 0, WinTrayOptionCount - 1, WinTrayInvalid
};

/******************************************************************************/

long SymbolTable::parseIdentifier(char const * id, size_t const len) const {
    for (Symbol const * sym(fSymbols); sym && sym->name; ++sym)
        if (!strncasecmp(sym->name, id, len) && !sym->name[len])
            return sym->code;

    char *endptr(nullptr);
    long value(strtol(id, &endptr, 0));

    return (endptr && '\0' == *endptr &&
            value >= fMin && value <= fMax ? value : fErrCode);
}

long SymbolTable::parseExpression(char const * expression) const {
    long value(0);

    for (char const * token(expression);
         *token != '\0' && value != fErrCode; token = strnxt(token, "+|"))
    {
        char const * id(token + strspn(token, " \t"));
        value |= parseIdentifier(id = newstr(id, "+| \t"));
        delete[] id;
    }

    return value;
}

void SymbolTable::listSymbols(char const * label) const {
    printf(_("Named symbols of the domain `%s' (numeric range: %ld-%ld):\n"),
           label, fMin, fMax);

    for (Symbol const * sym(fSymbols); sym && sym->name; ++sym)
        printf("  %-20s (%ld)\n", sym->name, sym->code);

    puts("");
}

/******************************************************************************/

static Status send(NAtom& typ, Window win, long l0, long l1, long l2 = 0L) {
    XClientMessageEvent xev;
    memset(&xev, 0, sizeof(xev));

    xev.type = ClientMessage;
    xev.window = win;
    xev.message_type = typ;
    xev.format = 32;
    xev.data.l[0] = l0;
    xev.data.l[1] = l1;
    xev.data.l[2] = l2;

    return XSendEvent(display, root, False, SubstructureNotifyMask,
                      reinterpret_cast<XEvent *>(&xev));
}

static Status getState(Window window, long & mask, long & state) {
    YWindowProperty winState(window, ATOM_WIN_STATE, XA_CARDINAL, 2);

    if (winState && XA_CARDINAL == winState.type() &&
        32 == winState.format() && 1L <= winState.count()) {
        state = winState.data<long>(0);
        mask = winState.count() >= 2L
             ? winState.data<long>(1)
             : WIN_STATE_ALL;

        return Success;
    }

    return BadValue;
}

static Status setState(Window window, long mask, long state) {
    return send(ATOM_WIN_STATE, window, mask, state, CurrentTime);
}

static Status toggleState(Window window, long newState) {
    long mask, state;

    if (Success != getState(window, mask, state))
        state = mask = 0;

    MSG(("old mask/state: %ld/%ld", mask, state));

    long newMask = (state & mask & newState) ^ newState;
    MSG(("new mask/state: %ld/%ld", newMask, newState));

    return send(ATOM_WIN_STATE, window, newMask, newState, CurrentTime);
}

static void getState(Window window) {
    long mask = 0, state = 0;

    if (getState(window, mask, state) == Success) {
        printf("0x%lx", window);
        for (int k = 0; k < 2; ++k) {
            if (((k ? state : mask) & WIN_STATE_ALL) == WIN_STATE_ALL ||
                (k == 0 && mask == 0)) {
                printf(" All");
            }
            else if (k ? state : mask) {
                int more = 0;
                long old = 0;
                for (int i = 0; stateIdentifiers[i + 1].code; ++i) {
                    long c = stateIdentifiers[i].code;
                    if (((k ? state : mask) & c) == c) {
                        if (i == 0 || (old & c) != c) {
                            printf("%c%s",
                                   more++ ? '|' : ' ',
                                   stateIdentifiers[i].name);
                            old = c;
                        }
                    }
                }
            }
            else {
                printf(" 0");
            }
        }
        printf("\n");
    }
}

/******************************************************************************/

static Status setHints(Window window, long hints) {
    return XChangeProperty(display, window, ATOM_WIN_HINTS, XA_CARDINAL, 32,
                           PropModeReplace, (unsigned char *) &hints, 1);
}

static void getHints(Window window) {
    YWindowProperty prop(window, ATOM_WIN_HINTS, XA_CARDINAL, 1L);
    if (prop) {
        long hint = prop.data<long>(0);
        printf("0x%lx", window);
        if ((hint & WIN_HINTS_ALL) == WIN_HINTS_ALL) {
            printf(" All");
        }
        else if (hint == 0) {
            printf(" 0");
        }
        else {
            int more = 0;
            for (int i = 0; hintIdentifiers[i + 1].code; ++i) {
                long c = hintIdentifiers[i].code;
                if ((hint & c) == c) {
                    printf("%c%s",
                           more++ ? '|' : ' ',
                           hintIdentifiers[i].name);
                }
            }
        }
        printf("\n");
    }
}

/******************************************************************************/

class WorkspaceInfo {
public:
    WorkspaceInfo():
        fCount(root, ATOM_NET_NUMBER_OF_DESKTOPS, XA_CARDINAL, 1),
        fNames(root, ATOM_NET_DESKTOP_NAMES, YEmby)
    {
    }

    bool parseWorkspace(char const* name, long* workspace);

    long count() const { return fCount.data<long>(0); }
    operator bool() const { return fCount && fNames; }
    char* operator[](int i) const { return fNames[i]; }

private:
    YWindowProperty fCount;
    YTextProperty fNames;
};

bool WorkspaceInfo::parseWorkspace(char const* name, long* workspace) {
    if (*this) {
        for (int i = 0; i < fNames.count(); ++i)
            if (0 == strcmp(name, fNames[i]))
                return *workspace = i, true;

        if (0 == strcmp(name, "All") || 0 == strcmp(name, "0xFFFFFFFF"))
            return *workspace = 0xFFFFFFFF, true;

        if (tolong(name, *workspace) == false) {
            msg(_("Invalid workspace name: `%s'"), name);
        }
        else if (inrange(*workspace, 0L, count() - 1L) == false) {
            msg(_("Workspace out of range: %ld"), *workspace);
        }
        else return true;
    }

    return false;
}

static Status setWorkspace(Window window, long workspace) {
    return send(ATOM_NET_WM_DESKTOP, window, workspace, SourceIndication);
}

static bool getGeometry(Window window, int& x, int& y, int& width, int& height) {
    XWindowAttributes a = {};
    bool got = XGetWindowAttributes(display, window, &a);
    if (got) {
        x = a.x, y = a.y, width = a.width, height = a.height;
        while (window != root
            && (window = getParent(window)) != None
            && XGetWindowAttributes(display, window, &a))
        {
            x += a.x;
            y += a.y;
        }
    }
    return got;
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
        wmname = text[0];
        wmname[strlen(wmname)] = '.';
    }
    snprintf(wmtitle, sizeof wmtitle, "(%s)", wmname);

    int x = 0, y = 0, width = 0, height = 0;
    getGeometry(w, x, y, width, height);

    printf(_("0x%-8lx %-20s: %-20s %dx%d%+d%+d\n"), w, title, wmtitle,
            width, height, x, y);
    if (name)
        XFree(name);
}

bool IceSh::listWindows()
{
    if ( !isAction("windows", 0))
        return false;

    if (count() == 0)
        windowList.query(root);

    FOREACH_WINDOW(w) {
        details(w);
    }
    return true;
}

bool IceSh::listClients()
{
    if ( !isAction("clients", 0))
        return false;

    windowList.getClientList();

    FOREACH_WINDOW(w) {
        details(w);
    }
    return true;
}

bool IceSh::listShown()
{
    if ( !isAction("shown", 0))
        return false;

    windowList.getClientList();
    long workspace = getWorkspace(root);
    windowList.filterByWorkspace(workspace);

    FOREACH_WINDOW(w) {
        details(w);
    }
    return true;
}

bool IceSh::listWorkspaces()
{
    if ( !isAction("listWorkspaces", 0))
        return false;

    WorkspaceInfo info;
    for (int n(0); n < info.count(); ++n)
        printf(_("workspace #%d: `%s'\n"), n, info[n]);
    return true;
}

bool IceSh::setWorkspaceName()
{
    if ( !isAction("setWorkspaceName", 0))
        return false;

    char* id = getArg();
    char* nm = getArg();
    long ws;
    if (id && nm && tolong(id, ws)) {
        YTextProperty net(root, ATOM_NET_DESKTOP_NAMES, YEmby);
        net.set(int(ws), nm);
        net.commit();
    }
    return true;
}

bool IceSh::setWorkspaceNames()
{
    if ( !isAction("setWorkspaceNames", 1))
        return false;

    YTextProperty names(root, ATOM_NET_DESKTOP_NAMES, YEmby);
    for (int k = 0; haveArg(); ++k) {
        names.set(k, getArg());
    }
    names.commit();

    return true;
}

bool IceSh::wmcheck()
{
    if ( !isAction("check", 0))
        return false;

    YWindowProperty check(root, ATOM_NET_SUPPORTING_WM_CHECK, XA_WINDOW, 4);
    if (check && check.type() == XA_WINDOW) {
        Window win = check.data<long>(0);
        char* name = 0;
        if (XFetchName(display, win, &name) && name) {
            printf("Name: %s\n", name);
            XFree(name);
        }
        YWindowProperty cls(win, XA_WM_CLASS, XA_STRING, 1234);
        if (cls) {
            printf("Class: ");
            for (int i = 0; i + 1 < cls.count(); ++i) {
                char c = Elvis(cls.data<char>(i), '.');
                putchar(c);
            }
            putchar('\n');
        }
        YWindowProperty loc(win, ATOM_WM_LOCALE_NAME, XA_STRING, 100);
        if (loc) {
            printf("Locale: %s\n", loc.data<char>());
        }
        YWindowProperty com(win, XA_WM_COMMAND, XA_STRING, 1234);
        if (com) {
            printf("Command: ");
            for (int i = 0; i + 1 < com.count(); ++i) {
                char c = Elvis(com.data<char>(i), ' ');
                putchar(c);
            }
            putchar('\n');
        }
        YWindowProperty mac(win, XA_WM_CLIENT_MACHINE, XA_STRING, 100);
        if (mac) {
            printf("Machine: %s\n", mac.data<char>());
        }
        YWindowProperty pid(win, ATOM_NET_WM_PID, XA_CARDINAL, 1);
        if (pid) {
            printf("PID: %lu\n", pid.data<long>(0));
        }
    }
    return true;
}

bool IceSh::desktops()
{
    if ( !isAction("workspaces", 0))
        return false;

    long n;
    if (haveArg() && tolong(*argp, n)) {
        ++argp;
        if (inrange(n, 1L, 12345L)) {
            send(ATOM_NET_NUMBER_OF_DESKTOPS, root, n, 0L);
        }
    }
    else {
        YWindowProperty prop(root, ATOM_NET_NUMBER_OF_DESKTOPS, XA_CARDINAL, 1L);
        if (prop) {
            printf("%ld workspaces\n", prop.data<long>(0));
        }
    }
    return true;
}

bool IceSh::change()
{
    if ( !isAction("goto", 0))
        return false;

    char* name = getArg();
    if (name == nullptr)
        return false;

    long workspace;
    if ( ! WorkspaceInfo().parseWorkspace(name, &workspace))
        THROW(1);

    send(ATOM_NET_CURRENT_DESKTOP, root, workspace, CurrentTime);

    return true;
}

bool IceSh::colormaps()
{
    if ( !isAction("colormaps", 0))
        return false;

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
    if ( !isAction("guievents", 0))
        return false;

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
            || setWorkspaceNames()
            || setWorkspaceName()
            || listWorkspaces()
            || listWindows()
            || listClients()
            || listShown()
            || colormaps()
            || desktops()
            || wmcheck()
            || change()
            ;
    else
        ++argp;

    send(ATOM_ICE_ACTION, root, CurrentTime, action);

    return true;
}

unsigned IceSh::count() const
{
    return windowList.count();
}

/******************************************************************************/

Status setLayer(Window window, long layer) {
    return send(ATOM_WIN_LAYER, window, layer, CurrentTime);
}

static void getLayer(Window window) {
    YWindowProperty prop(window, ATOM_WIN_LAYER, XA_CARDINAL, 1);
    if (prop) {
        bool found = false;
        long layer = prop.data<long>(0);
        for (int i = 0; layerIdentifiers[i].name; ++i) {
            if (layer == layerIdentifiers[i].code) {
                printf("0x%lx %s\n", window, layerIdentifiers[i].name);
                found = true;
            }
        }
        if (found == false) {
            printf("0x%lx %ld\n", window, layer);
        }
    }
}

Status setTrayHint(Window window, long trayopt) {
    return send(ATOM_WIN_TRAY, window, trayopt, CurrentTime);
}

static void getTrayOption(Window window) {
    YWindowProperty prop(window, ATOM_WIN_TRAY, XA_CARDINAL, 1);
    if (prop) {
        bool found = false;
        long tropt = prop.data<long>(0);
        for (int i = 0; trayOptionIdentifiers[i].name; ++i) {
            if (tropt == trayOptionIdentifiers[i].code) {
                printf("0x%lx %s\n", window, trayOptionIdentifiers[i].name);
                found = true;
            }
        }
        if (found == false) {
            printf("0x%lx %ld\n", window, tropt);
        }
    }
}

/******************************************************************************/

static void setGeometry(Window window, const char* geometry) {
    int geom_x, geom_y;
    unsigned geom_width, geom_height;
    int status(XParseGeometry(geometry, &geom_x, &geom_y,
                              &geom_width, &geom_height));
    if (status == NoValue)
        return;

    XSizeHints normal;
    long supplied;
    if (XGetWMNormalHints(display, window, &normal, &supplied) != True)
        return;

    Window root; int x, y; unsigned width, height, dummy;
    if (XGetGeometry(display, window, &root, &x, &y, &width, &height,
                     &dummy, &dummy) != True)
        return;

    if (status & XValue) x = geom_x;
    if (status & YValue) y = geom_y;
    if (status & WidthValue) width = geom_width;
    if (status & HeightValue) height = geom_height;

    if (hasbits(status, XValue | XNegative))
        x += DisplayWidth(display, DefaultScreen(display)) - width;
    if (hasbits(status, YValue | YNegative))
        y += DisplayHeight(display, DefaultScreen(display)) - height;

    if (normal.flags & PResizeInc) {
        width *= max(1, normal.width_inc);
        height *= max(1, normal.height_inc);
    }

    if (normal.flags & PBaseSize) {
        width += normal.base_width;
        height += normal.base_height;
    }

    MSG(("setGeometry: %dx%d%+i%+i", width, height, x, y));
    if (hasbit(status, XValue | YValue) &&
        hasbit(status, WidthValue | HeightValue))
        XMoveResizeWindow(display, window, x, y, width, height);
    else if (hasbit(status, XValue | YValue))
        XMoveWindow(display, window, x, y);
    else if (hasbit(status, WidthValue | HeightValue))
        XResizeWindow(display, window, width, height);
}

static void getGeometry(Window window) {
    int x = 0, y = 0, width = 0, height = 0;
    if (getGeometry(window, x, y, width, height)) {
        printf("%dx%d+%d+%d\n", width, height, x, y);
    }
}

/******************************************************************************/

static void setWindowTitle(Window window, const char* title) {
    XStoreName(display, window, title);
}

static void getWindowTitle(Window window) {
    char* title(0);
    if (XFetchName(display, window, &title)) {
        puts(title);
        XFree(title);
    }
}

static void getIconTitle(Window window) {
    char* title(0);
    if (XGetIconName(display, window, &title)) {
        puts(title);
        XFree(title);
    }
}

static void setIconTitle(Window window, const char* title) {
    XSetIconName(display, window, title);
}

/******************************************************************************/

static Window getActive() {
    Window active;

    YWindowProperty prop(root, ATOM_NET_ACTIVE_WINDOW, XA_WINDOW, 1);
    if (prop) {
        active = prop.data<long>(0);
    }
    else {
        int revertTo;
        XGetInputFocus(display, &active, &revertTo);
    }
    return active;
}

static Status closeWindow(Window window) {
    return send(ATOM_NET_CLOSE_WINDOW, window, CurrentTime, SourceIndication);
}

static Status activateWindow(Window window) {
    return send(ATOM_NET_ACTIVE_WINDOW, window, SourceIndication, CurrentTime);
}

static Status raiseWindow(Window window) {
    return send(ATOM_NET_RESTACK_WINDOW, window, SourceIndication, None, Above);
}

static Status lowerWindow(Window window) {
    return send(ATOM_NET_RESTACK_WINDOW, window, SourceIndication, None, Below);
}

static Window getClientWindow(Window window)
{
    YWindowProperty wmstate(window, ATOM_WM_STATE);
    if (wmstate && wmstate.type() == ATOM_WM_STATE)
        return window;

    YWindowTree tree;
    if (tree.query(window) == false) {
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

static Window pickWindow (void) {
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

static bool isOptArg(const char* arg, const char* opt, const char* val) {
    const char buf[3] = { opt[0], opt[1], '\0', };
    return (strpcmp(arg, opt) == 0 || strcmp(arg, buf) == 0) && val != 0;
}

static void removeOpacity(Window window) {
    Window framew = getFrameWindow(window);
    XDeleteProperty(display, framew, ATOM_NET_WM_WINDOW_OPACITY);
    XDeleteProperty(display, window, ATOM_NET_WM_WINDOW_OPACITY);
}

static void setOpacity(Window window, unsigned opaqueness) {
    Window framew = getFrameWindow(window);
    XChangeProperty(display, framew, ATOM_NET_WM_WINDOW_OPACITY,
                    XA_CARDINAL, 32, PropModeReplace,
                    reinterpret_cast<unsigned char *>(&opaqueness), 1);
}

static unsigned getOpacity(Window window) {
    Window framew = getFrameWindow(window);

    YWindowProperty prop(framew, ATOM_NET_WM_WINDOW_OPACITY, XA_CARDINAL, 1);
    if (prop == false)
        prop.update(window, ATOM_NET_WM_WINDOW_OPACITY, XA_CARDINAL, 1);

    unsigned opaq = prop.data<long>(0);
    unsigned omax = 0xFFFFFFFF;
    unsigned perc = opaq ? unsigned(100.0 * opaq / omax) : 0;
    return perc;
}

static void opacity(Window window, char* opaq) {
    const unsigned omax = 0xFFFFFFFF;
    if (opaq) {
        unsigned oset = 0;
        if ((*opaq == '.' && isDigit(opaq[1])) ||
            (inrange(*opaq, '0', '1') && opaq[1] == '.'))
        {
            double val = atof(opaq);
            if (inrange(val, 0.0, 1.0)) {
                oset = unsigned(omax * val);
                return setOpacity(window, oset);
            }
        }
        else if (strcmp(opaq, "0") == 0) {
            removeOpacity(window);
        }
        else if (strspn(opaq, "0123456789")) {
            unsigned val = unsigned(atol(opaq));
            if (val <= 100) {
                oset = unsigned(val * double(omax) * 0.01);
                return setOpacity(window, oset);
            }
            else {
                setOpacity(window, oset);
            }
        }
    }
    else {
        unsigned perc = getOpacity(window);
        printf("0x%lx %u\n", window, perc);
    }
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

    return IceSh(argc, argv);
}

IceSh::IceSh(int ac, char **av) :
    rc(0),
    argc(ac),
    argv(av),
    argp(av + 1),
    dpyname(nullptr),
    winname(nullptr),
    winprop(nullptr),
    wmclass(nullptr),
    wmname(nullptr)
{
    if (setjmp(jmpbuf) == 0) {
        flags();
        xinit();

        while (haveArg() && icewmAction());

        if (haveArg()) {
            getWindows();
            parseActions();
        }
    }
}

bool IceSh::haveArg()
{
    return argp < argv + argc;
}

char* IceSh::getArg()
{
    return haveArg() ? *argp++ : nullptr;
}

bool IceSh::isAction(const char* action, int count)
{
    if (argp == &argv[argc] || strcmp(action, *argp))
        return false;

    if (++argp + count > &argv[argc]) {
        msg(_("Action `%s' requires at least %d arguments."), action, count);
        THROW(1);
    }

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
    for (char *arg; haveArg() && '-' == *(arg = *argp); ++argp) {
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
        if (isOptArg(arg, "-shown", "")) {
            winname = "shown";
            continue;
        }
        if (isOptArg(arg, "-all", "")) {
            winname = "all";
            continue;
        }

        size_t sep(strcspn(arg, "=:"));
        char *val(arg[sep] ? arg + sep + 1 : *++argp);

        if (isOptArg(arg, "-display", val)) {
            dpyname = val;
        }
        else if (isOptArg(arg, "-pid", val)) {
            winname = "pid";
            winprop = val;
        }
        else if (isOptArg(arg, "-Workspace", val)) {
            winname = "Workspace";
            winprop = val;
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

    if (haveArg() == false) {
        msg(_("No actions specified."));
        THROW(1);
    }
}

/******************************************************************************/

void IceSh::xinit()
{
    if (nullptr == (display = XOpenDisplay(dpyname))) {
        warn(_("Can't open display: %s. X must be running and $DISPLAY set."),
             XDisplayName(dpyname));
        THROW(3);
    }

    root = DefaultRootWindow(display);
    XSynchronize(display, True);
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
            setWindow(getActive());

            MSG(("focused window selected"));
        }
        else if (!strcmp(winname, "all")) {
            windowList.getClientList();

            MSG(("all windows selected"));
        }
        else if (!strcmp(winname, "shown")) {
            windowList.getClientList();
            windowList.filterByWorkspace(getWorkspace(root));

            MSG(("shown windows selected"));
        }
        else if (!strcmp(winname, "pid")) {
            long pid;
            if (tolong(winprop, pid)) {
                windowList.getClientList();
                windowList.filterByPid(pid);

                MSG(("pid windows selected"));
            }
            else {
                msg("Invalid PID");
                THROW(1);
            }
        }
        else if (!strcmp(winname, "Workspace")) {
            long ws;
            if (WorkspaceInfo().parseWorkspace(winprop, &ws)) {
                windowList.getClientList();
                windowList.filterByWorkspace(ws);

                MSG(("pid windows selected"));
            }
            else {
                THROW(1);
            }
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
        if (wmname == nullptr && interactive) {
            setWindow(pickWindow());

            MSG(("window picked"));
        }
        else {
            windowList.getClientList();

            MSG(("window tree fetched, got %d window handles", count()));
        }
    }

    if (wmname && windowList) {
        windowList.filterByClass(wmname, wmclass);
    }

    MSG(("windowCount: %d", count()));
}

/******************************************************************************/

void IceSh::parseActions()
{
    while (haveArg()) {
        if (isAction("setWindowTitle", 1)) {
            char const * title(*argp++);

            MSG(("setWindowTitle: `%s'", title));
            FOREACH_WINDOW(window)
                setWindowTitle(window, title);
        }
        else if (isAction("getWindowTitle", 0)) {
            FOREACH_WINDOW(window)
                getWindowTitle(window);
        }
        else if (isAction("getIconTitle", 0)) {
            FOREACH_WINDOW(window)
                getIconTitle(window);
        }
        else if (isAction("setIconTitle", 1)) {
            char const * title(*argp++);

            MSG(("setIconTitle: `%s'", title));
            FOREACH_WINDOW(window)
                setIconTitle(window, title);
        }
        else if (isAction("setGeometry", 1)) {
            char const * geometry(*argp++);
            FOREACH_WINDOW(window)
                setGeometry(window, geometry);
        }
        else if (isAction("getGeometry", 0)) {
            FOREACH_WINDOW(window)
                getGeometry(window);
        }
        else if (isAction("resize", 2)) {
            const char* ws = *argp++;
            const char* hs = *argp++;
            long lw, lh;
            if (tolong(ws, lw) && tolong(hs, lh)) {
                if (lw >= 1 && lh >= 1) {
                    char buf[64];
                    snprintf(buf, sizeof buf, "%ldx%ld", lw, lh);
                    FOREACH_WINDOW(window)
                        setGeometry(window, buf);
                }
            }
        }
        else if (isAction("move", 2)) {
            const char* xs = *argp++;
            const char* ys = *argp++;
            long lx, ly;
            if (tolong(xs, lx) && tolong(ys, ly)) {
                char buf[64];
                snprintf(buf, sizeof buf, "%s%s%s%s",
                         isDigit(*xs) ? "+" : "", xs,
                         isDigit(*ys) ? "+" : "", ys);
                FOREACH_WINDOW(window) {
                    Window frame = getFrameWindow(window);
                    if (frame) {
                        setGeometry(frame, buf);
                    }
                }
            }
        }
        else if (isAction("moveby", 2)) {
            const char* xs = *argp++;
            const char* ys = *argp++;
            long lx, ly;
            if (tolong(xs, lx) && tolong(ys, ly)) {
                FOREACH_WINDOW(window) {
                    Window frame = getFrameWindow(window);
                    if (frame) {
                        int x, y, w, h;
                        if (getGeometry(frame, x, y, w, h)) {
                            int tx = int(x + lx), ty = int(y + ly);
                            XMoveWindow(display, frame, tx, ty);
                        }
                    }
                }
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
        else if (isAction("getState", 0)) {
            FOREACH_WINDOW(window)
                getState(window);
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
        else if (isAction("getHints", 0)) {
            FOREACH_WINDOW(window)
                getHints(window);
        }
        else if (isAction("setWorkspace", 1)) {
            long workspace;
            if ( ! WorkspaceInfo().parseWorkspace(*argp++, &workspace))
                THROW(1);

            MSG(("setWorkspace: %ld", workspace));
            FOREACH_WINDOW(window)
                setWorkspace(window, workspace);
        }
        else if (isAction("getWorkspace", 0)) {
            FOREACH_WINDOW(window)
                printf("0x%lx %ld\n", window, getWorkspace(window));
        }
        else if (isAction("setLayer", 1)) {
            unsigned layer(layers.parseExpression(*argp++));
            check(layers, layer, argp[-1]);

            MSG(("setLayer: %d", layer));
            FOREACH_WINDOW(window)
                setLayer(window, layer);
        }
        else if (isAction("getLayer", 0)) {
            FOREACH_WINDOW(window)
                getLayer(window);
        }
        else if (isAction("setTrayOption", 1)) {
            unsigned trayopt(trayOptions.parseExpression(*argp++));
            check(trayOptions, trayopt, argp[-1]);

            MSG(("setTrayOption: %d", trayopt));
            FOREACH_WINDOW(window)
                setTrayHint(window, trayopt);
        }
        else if (isAction("getTrayOption", 0)) {
            FOREACH_WINDOW(window)
                getTrayOption(window);
        }
        else if (isAction("id", 0)) {
            FOREACH_WINDOW(window)
                printf("0x%06lx\n", window);
        }
        else if (isAction("list", 0)) {
            FOREACH_WINDOW(window)
                details(window);
        }
        else if (isAction("close", 0)) {
            FOREACH_WINDOW(window)
                closeWindow(window);
        }
        else if (isAction("kill", 0)) {
            FOREACH_WINDOW(window)
                XKillClient(display, window);
        }
        else if (isAction("activate", 0)) {
            FOREACH_WINDOW(window)
                activateWindow(window);
        }
        else if (isAction("raise", 0)) {
            FOREACH_WINDOW(window)
                raiseWindow(window);
        }
        else if (isAction("lower", 0)) {
            FOREACH_WINDOW(window)
                lowerWindow(window);
        }
        else if (isAction("fullscreen", 0)) {
            FOREACH_WINDOW(window)
                setState(window,
                         WinStateFullscreen|WinStateMaximized|
                         WinStateMinimized|WinStateRollup,
                         WinStateFullscreen);
        }
        else if (isAction("maximize", 0)) {
            FOREACH_WINDOW(window)
                setState(window,
                         WinStateFullscreen|WinStateMaximized|
                         WinStateMinimized|WinStateRollup,
                         WinStateMaximized);
        }
        else if (isAction("minimize", 0)) {
            FOREACH_WINDOW(window)
                setState(window,
                         WinStateFullscreen|WinStateMaximized|
                         WinStateMinimized|WinStateRollup,
                         WinStateMinimized);
        }
        else if (isAction("rollup", 0)) {
            FOREACH_WINDOW(window)
                setState(window,
                         WinStateFullscreen|WinStateMaximized|
                         WinStateMinimized|WinStateRollup,
                         WinStateRollup);
        }
        else if (isAction("above", 0)) {
            FOREACH_WINDOW(window)
                setState(window,
                         WinStateAbove|WinStateBelow,
                         WinStateAbove);
        }
        else if (isAction("below", 0)) {
            FOREACH_WINDOW(window)
                setState(window,
                         WinStateAbove|WinStateBelow,
                         WinStateBelow);
        }
        else if (isAction("hide", 0)) {
            FOREACH_WINDOW(window)
                setState(window, WinStateHidden, WinStateHidden);
        }
        else if (isAction("unhide", 0)) {
            FOREACH_WINDOW(window)
                setState(window, WinStateHidden, 0L);
        }
        else if (isAction("skip", 0)) {
            FOREACH_WINDOW(window)
                setState(window, WinStateSkip, WinStateSkip);
        }
        else if (isAction("unskip", 0)) {
            FOREACH_WINDOW(window)
                setState(window, WinStateSkip, 0L);
        }
        else if (isAction("restore", 0)) {
            FOREACH_WINDOW(window) {
                long mask = 0L;
                mask |= WinStateFullscreen;
                mask |= WinStateMaximized;
                mask |= WinStateMinimized;
                mask |= WinStateHidden;
                mask |= WinStateRollup;
                mask |= WinStateAbove;
                mask |= WinStateBelow;
                setState(window, mask, 0L);
            }
        }
        else if (isAction("opacity", 0)) {
            char* opaq = nullptr;
            if (haveArg()) {
                if (isDigit(**argp)) {
                    opaq = *argp++;
                }
                else if (**argp == '.' && isDigit(argp[0][1])) {
                    opaq = *argp++;
                }
            }
            FOREACH_WINDOW(window)
                opacity(window, opaq);
        }
        else if (icewmAction()) {
        }
        else if (isAction("runonce", 1)) {
            if (count() == 0 || (count() == 1 && *windowList == root)) {
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
