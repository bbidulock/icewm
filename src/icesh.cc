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
 *  2019/03/04: Bert Gijsbers
 *  - total rewrite
 */

#include "config.h"
#include "intl.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <vector>
#include <algorithm>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#ifdef CONFIG_XRANDR
#include <X11/extensions/Xrandr.h>
#endif
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#ifdef CONFIG_I18N
#include <locale.h>
#endif

#include "ascii.h"
#include "base.h"
#include "WinMgr.h"
#include "MwmUtil.h"
#include "wmaction.h"
#include "ypointer.h"
#include "ytime.h"
#include "yrect.h"
#define GUI_EVENT_NAMES
#include "guievent.h"
#include "logevent.h"

#ifndef __GLIBC__
typedef void (*sighandler_t)(int);
#endif

using std::vector;
using std::find;

/******************************************************************************/

using namespace ASCII;
const long SourceIndication = 1L;
const long Sticky = long(0xFFFFFFFF);
char const* ApplicationName = "icesh";
static Display *display;
static Window root;

static const char* get_help_text() {
    return _("For help please consult the man page icesh(1).\n");
}

static bool tolong(const char* str, long& num, int base = 10) {
    char* end = nullptr;
    if (str) {
        num = strtol(str, &end, base);
    }
    return end && str < end && 0 == *end;
}

static bool tofloat(const char* str, float& num) {
    char* end = nullptr;
    if (str) {
        num = strtof(str, &end);
    }
    return end && str < end && 0 == *end;
}

template<class T>
bool contains(vector<T>& v, T t) {
    return find(v.begin(), v.end(), t) != v.end();
}

/******************************************************************************/

class NAtom {
    const char* fName;
    Atom fAtom;
    bool fExists;
    bool fDynamic;
    static vector<NAtom*> fAtoms;
    static bool lookup(Atom atom, int* pos) {
        int lo = 0, hi = int(fAtoms.size());
        while (lo < hi) {
            const int pv = (lo + hi) / 2;
            NAtom* pivot = fAtoms[pv];
            if (atom > pivot->fAtom)
                lo = pv + 1;
            else if (atom < pivot->fAtom)
                hi = pv;
            else {
                *pos = pv;
                return true;
            }
        }
        *pos = lo;
        return false;
    }
    void insert() {
        if (fAtom) {
            int pos;
            if (lookup(fAtom, &pos)) {
                NAtom* old = this;
                std::swap(old, fAtoms[pos]);
                if (old->fDynamic) {
                    XFree(const_cast<char *>(old->fName));
                    delete old;
                }
            } else {
                fAtoms.insert(fAtoms.begin() + pos, this);
            }
        }
    }
public:
    explicit NAtom(const char* name, bool exists = false) :
        fName(name), fAtom(None), fExists(exists), fDynamic(false)
    { }
    const char* name() const { return fName; }
    operator Atom() {
        if (fAtom == None && fName) {
            fAtom = XInternAtom(display, fName, fExists);
            if (fExists == false) {
                insert();
            }
        }
        return fAtom;
    }
    static const char* lookup(Atom atom) {
        int pos;
        if (lookup(atom, &pos)) {
            return fAtoms[pos]->fName;
        } else {
            char* name = XGetAtomName(display, atom);
            NAtom* ptr = new NAtom(name);
            ptr->fAtom = atom;
            ptr->fDynamic = true;
            fAtoms.insert(fAtoms.begin() + pos, ptr);
            return name;
        }
    }
    static void free() {
        for (auto ptr : fAtoms) {
            if (ptr && ptr->fDynamic) {
                XFree(const_cast<char *>(ptr->fName));
                delete ptr;
            }
        }
        fAtoms.clear();
    }
};

vector<NAtom*> NAtom::fAtoms;

static NAtom ATOM_WM_STATE("WM_STATE");
static NAtom ATOM_WM_CHANGE_STATE("WM_CHANGE_STATE");
static NAtom ATOM_WM_CLIENT_LEADER("WM_CLIENT_LEADER");
static NAtom ATOM_WM_LOCALE_NAME("WM_LOCALE_NAME");
static NAtom ATOM_WM_TAKE_FOCUS("WM_TAKE_FOCUS");
static NAtom ATOM_WM_WINDOW_ROLE("WM_WINDOW_ROLE");
static NAtom ATOM_NET_WM_PID("_NET_WM_PID");
static NAtom ATOM_NET_WM_NAME("_NET_WM_NAME");
static NAtom ATOM_NET_WM_ICON_NAME("_NET_WM_ICON_NAME");
static NAtom ATOM_NET_WM_DESKTOP("_NET_WM_DESKTOP");
static NAtom ATOM_NET_DESKTOP_NAMES("_NET_DESKTOP_NAMES");
static NAtom ATOM_NET_CURRENT_DESKTOP("_NET_CURRENT_DESKTOP");
static NAtom ATOM_NET_SHOWING_DESKTOP("_NET_SHOWING_DESKTOP");
static NAtom ATOM_NET_NUMBER_OF_DESKTOPS("_NET_NUMBER_OF_DESKTOPS");
static NAtom ATOM_NET_SUPPORTING_WM_CHECK("_NET_SUPPORTING_WM_CHECK");
static NAtom ATOM_WIN_LAYER(XA_WIN_LAYER);
static NAtom ATOM_WIN_TRAY(XA_WIN_TRAY);
static NAtom ATOM_WIN_PROTOCOLS(XA_WIN_PROTOCOLS);
static NAtom ATOM_GUI_EVENT(XA_GUI_EVENT_NAME);
static NAtom ATOM_ICE_ACTION("_ICEWM_ACTION");
static NAtom ATOM_ICE_DOCKAPPS("_ICEWM_DOCKAPPS");
static NAtom ATOM_ICE_WINOPT("_ICEWM_WINOPTHINT");
static NAtom ATOM_MOTIF_HINTS(_XA_MOTIF_WM_HINTS);
static NAtom ATOM_NET_CLIENT_LIST("_NET_CLIENT_LIST");
static NAtom ATOM_NET_CLIENT_LIST_STACKING("_NET_CLIENT_LIST_STACKING");
static NAtom ATOM_NET_CLOSE_WINDOW("_NET_CLOSE_WINDOW");
static NAtom ATOM_NET_ACTIVE_WINDOW("_NET_ACTIVE_WINDOW");
static NAtom ATOM_NET_FRAME_EXTENTS("_NET_FRAME_EXTENTS");
static NAtom ATOM_NET_RESTACK_WINDOW("_NET_RESTACK_WINDOW");
static NAtom ATOM_NET_MOVERESIZE_WINDOW("_NET_MOVERESIZE_WINDOW");
static NAtom ATOM_NET_WM_WINDOW_OPACITY("_NET_WM_WINDOW_OPACITY");
static NAtom ATOM_NET_WM_WINDOW_TYPE("_NET_WM_WINDOW_TYPE");
static NAtom ATOM_NET_SYSTEM_TRAY_WINDOWS("_KDE_NET_SYSTEM_TRAY_WINDOWS");
static NAtom ATOM_COMPOUND_TEXT("COMPOUND_TEXT");
static NAtom ATOM_UTF8_STRING("UTF8_STRING");
static NAtom ATOM_XEMBED_INFO("_XEMBED_INFO");
static NAtom ATOM_NET_WORKAREA("_NET_WORKAREA");
static NAtom ATOM_NET_WM_STATE("_NET_WM_STATE");
static NAtom ATOM_NET_WM_STATE_ABOVE("_NET_WM_STATE_ABOVE");
static NAtom ATOM_NET_WM_STATE_BELOW("_NET_WM_STATE_BELOW");
static NAtom ATOM_NET_WM_STATE_DEMANDS_ATTENTION("_NET_WM_STATE_DEMANDS_ATTENTION");
static NAtom ATOM_NET_WM_STATE_FOCUSED("_NET_WM_STATE_FOCUSED");
static NAtom ATOM_NET_WM_STATE_FULLSCREEN("_NET_WM_STATE_FULLSCREEN");
static NAtom ATOM_NET_WM_STATE_HIDDEN("_NET_WM_STATE_HIDDEN");
static NAtom ATOM_NET_WM_STATE_MAXIMIZED_HORZ("_NET_WM_STATE_MAXIMIZED_HORZ");
static NAtom ATOM_NET_WM_STATE_MAXIMIZED_VERT("_NET_WM_STATE_MAXIMIZED_VERT");
static NAtom ATOM_NET_WM_STATE_MODAL("_NET_WM_STATE_MODAL");
static NAtom ATOM_NET_WM_STATE_SHADED("_NET_WM_STATE_SHADED");
static NAtom ATOM_NET_WM_STATE_SKIP_PAGER("_NET_WM_STATE_SKIP_PAGER");
static NAtom ATOM_NET_WM_STATE_SKIP_TASKBAR("_NET_WM_STATE_SKIP_TASKBAR");
static NAtom ATOM_NET_WM_STATE_STICKY("_NET_WM_STATE_STICKY");
static NAtom ATOM_NET_WM_USER_TIME("_NET_WM_USER_TIME");
static NAtom ATOM_NET_WM_USER_TIME_WINDOW("_NET_WM_USER_TIME_WINDOW");
static NAtom ATOM_NET_WM_FULLSCREEN_MONITORS("_NET_WM_FULLSCREEN_MONITORS");

enum NetStateBits {
    NetAbove       = 1,
    NetBelow       = 2,
    NetDemands     = 4,
    NetFocused     = 8,
    NetFullscreen  = 16,
    NetHidden      = 32,
    NetHorizontal  = 64,
    NetVertical    = 128,
    NetModal       = 256,
    NetShaded      = 512,
    NetSkipPager   = 1024,
    NetSkipTaskbar = 2048,
    NetSticky      = 4096,
};

static const struct NetStateAtoms {
    NetStateBits flag;
    NAtom& atom;
} netStateAtoms[] = {
    { NetAbove,       ATOM_NET_WM_STATE_ABOVE },
    { NetBelow,       ATOM_NET_WM_STATE_BELOW },
    { NetDemands,     ATOM_NET_WM_STATE_DEMANDS_ATTENTION },
    { NetFocused,     ATOM_NET_WM_STATE_FOCUSED },
    { NetFullscreen,  ATOM_NET_WM_STATE_FULLSCREEN },
    { NetHidden,      ATOM_NET_WM_STATE_HIDDEN },
    { NetHorizontal,  ATOM_NET_WM_STATE_MAXIMIZED_HORZ },
    { NetVertical,    ATOM_NET_WM_STATE_MAXIMIZED_VERT },
    { NetModal,       ATOM_NET_WM_STATE_MODAL },
    { NetShaded,      ATOM_NET_WM_STATE_SHADED },
    { NetSkipPager,   ATOM_NET_WM_STATE_SKIP_PAGER },
    { NetSkipTaskbar, ATOM_NET_WM_STATE_SKIP_TASKBAR },
    { NetSticky,      ATOM_NET_WM_STATE_STICKY },
};
const int netStateAtomCount = int ACOUNT(netStateAtoms);

static inline void newline() {
    putchar('\n');
}

static Time serverTime()
{
    Window window = XCreateSimpleWindow(display, root, -1, -1, 1, 1, 0, 0, 0);
    XSelectInput(display, window, PropertyChangeMask);
    XID pid = getpid();
    XChangeProperty(display, window, ATOM_NET_WM_PID, XA_CARDINAL,
                    32, PropModeReplace, (unsigned char *)&pid, 1);
    XEvent event;
    XWindowEvent(display, window, PropertyChangeMask, &event);
    Time now = event.xproperty.time;
    XDestroyWindow(display, window);
    return now;
}

static void sendEvent(void* event) {
    XSendEvent(display, root, False, SubstructureNotifyMask,
               reinterpret_cast<XEvent *>(event));
}

static void send(NAtom& typ, Window win, long l0, long l1,
                 long l2 = 0L, long l3 = 0L, long l4 = 0L)
{
    XClientMessageEvent xev;
    memset(&xev, 0, sizeof(xev));

    xev.type = ClientMessage;
    xev.window = win;
    xev.message_type = typ;
    xev.format = 32;
    xev.data.l[0] = l0;
    xev.data.l[1] = l1;
    xev.data.l[2] = l2;
    xev.data.l[3] = l3;
    xev.data.l[4] = l4;

    sendEvent(&xev);
}

/******************************************************************************/

static bool getGeometry(Window window, int& x, int& y, int& width, int& height);

static int displayWidth() {
    return DisplayWidth(display, DefaultScreen(display));
}

static int displayHeight() {
    return DisplayHeight(display, DefaultScreen(display));
}

class YScreen : public YRect {
public:
    int index;

    YScreen(int i = 0, int x = 0, int y = 0, unsigned w = 0, unsigned h = 0):
        YRect(x, y, w, h), index(i)
    {
    }
};

class Confine {
public:
    Confine(int screen = -1) : fScreen(screen) { }

    void confineTo(int screen) { fScreen = screen; }
    bool confining() { return 0 <= fScreen && fScreen < count(); }
    int count() { load(); return fInfo.size(); }
    int screen() const { return fScreen; }
    const YScreen& operator[](int i) const { return fInfo[i]; }

    bool load() {
        return fInfo.size() || load_rand() || load_xine() || load_deft();
    }

    bool load_rand() {
#ifdef CONFIG_XRANDR
        XRRScreenResources *rr = XRRGetScreenResources(display, root);
        if (rr && 0 < rr->ncrtc) {
            const int screens = rr->ncrtc;
            fInfo.clear();
            for (int k = 0; k < screens; ++k) {
                RRCrtc crt = rr->crtcs[k];
                XRRCrtcInfo *ci = XRRGetCrtcInfo(display, rr, crt);
                fInfo.push_back(YScreen(k, ci->x, ci->y, ci->width, ci->height));
                XRRFreeCrtcInfo(ci);
            }
            XRRFreeScreenResources(rr);
        }
#endif
        return 0 < fInfo.size();
    }

    bool load_xine() {
#ifdef XINERAMA
        int i;
        if (XineramaQueryExtension(display, &i, &i) &&
            XineramaIsActive(display))
        {
            int screens = 0;
            xsmart<XineramaScreenInfo> xine(
                   XineramaQueryScreens(display, &screens));
            if (xine) {
                fInfo.clear();
                for (int k = 0; k < screens; ++k) {
                    fInfo.push_back(YScreen(xine[k].screen_number,
                                    xine[k].x_org,
                                    xine[k].y_org,
                                    xine[k].width,
                                    xine[k].height));
                }
            }
        }
#endif
        return 0 < fInfo.size();
    }

    bool load_deft() {
        fInfo.clear();
        fInfo.push_back(YScreen(0, 0, 0, displayWidth(), displayHeight()));
        return 0 < fInfo.size();
    }

private:
    int fScreen;
    vector<YScreen> fInfo;
};

/******************************************************************************/

struct Symbol {
    char const * name;
    long const code;
};

struct SymbolTable {
    long parseIdentifier(char const * identifier, size_t const len) const;
    long parseIdentifier(char const * identifier) const {
        return parseIdentifier(identifier, strlen(identifier));
    }

    long parseExpression(char const * expression) const;
    void listSymbols(char const * label) const;
    bool lookup(long code, char const ** name) const;

    bool valid(long code) const { return code != fErrCode; }
    bool invalid(long code) const { return code == fErrCode; }

    Symbol const * fSymbols;
    long const fMin, fMax, fErrCode;
};

static void setAtom(Window window, Atom property, Atom newValue)
{
    XChangeProperty(display, window, property, XA_ATOM, 32, PropModeReplace,
                    reinterpret_cast<unsigned char *>(&newValue), 1);
}

class YProperty {
public:
    YProperty(Window window, Atom property, Atom type, long length = 1):
        fWindow(window), fProp(property), fType(type),
        fLength(length), fCount(0), fAfter(0),
        fData(nullptr), fFormat(0), fStatus(BadValue)
    {
        update();
    }
    YProperty(const YProperty& copy):
        fWindow(copy.fWindow), fProp(copy.fProp), fType(copy.fType),
        fLength(copy.fLength), fCount(0), fAfter(0),
        fData(nullptr), fFormat(0), fStatus(BadValue)
    {
        update();
    }

    virtual ~YProperty() {
        release();
    }

    void release() {
        if (fData) {
            XFree(fData);
            fData = nullptr;
            fCount = 0;
            fFormat = 0;
        }
    }

    void update(Window wind = 0, Atom prop = 0, Atom type = 0, long leng = 0)
    {
        release();
        if (wind) fWindow = wind;
        if (prop) fProp = prop;
        if (type) fType = type;
        if (leng) fLength = leng;
        if (fWindow && fProp && fLength) {
            fStatus = XGetWindowProperty(display, fWindow, fProp, 0L,
                                         fLength, False, fType, &type,
                                         &fFormat, &fCount, &fAfter, &fData);
            if (type && !fStatus) {
                fType = type;
            }
        }
    }

    template <class T>
    void replace(const T* data, int nelem, int format,
                 int mode = PropModeReplace)
    {
        if (format) fFormat = format;
        if (fFormat == 0) {
            fFormat = ((sizeof(T) == 1) ? 8 : 32);
        }
        if (fWindow && fProp && fType) {
            XChangeProperty(display, fWindow, fProp,
                            fType, fFormat, mode,
                            reinterpret_cast<const unsigned char *>(data),
                            nelem);
        }
    }

    template <class T>
    void append(const T* data, int nelem, int format) {
        replace(data, nelem, format, PropModeAppend);
    }

    void remove() {
        XDeleteProperty(display, fWindow, fProp);
        release();
    }

    void substitute(char* data, Atom type) {
        if (fData) XFree(fData);
        fData = (unsigned char *) data;
        fType = type;
        fCount = data ? long(strlen(data)) : 0;
    }

    Atom type() const { return fType; }
    int format() const { return fFormat; }
    int status() const { return fStatus; }
    long count() const { return fCount; }
    Window window() const { return fWindow; }
    Atom property() const { return fProp; }
    unsigned long after() const { return fAfter; }

    template <class T>
    T* data() const { return reinterpret_cast<T *>(fData); }
    template <class T>
    T data(unsigned index) const {
        return index < fCount ? data<T>()[index] : T(0);
    }
    template <class T>
    T* extract() { T* t(data<T>()); fData = nullptr; fCount = 0; return t; }

    operator bool() const { return !fStatus && fData && fType && fCount; }

    long operator*() const { return data<long>(0); }

    long operator[](int index) const { return data<long>(index); }

private:
    Window fWindow;
    Atom fProp, fType;
    long fLength;
    unsigned long fCount, fAfter;
    unsigned char* fData;
    int fFormat, fStatus;

    YProperty& operator=(const YProperty& copy);
};

class YCardinal : public YProperty {
public:
    YCardinal(Window window, Atom property, int count = 1) :
        YProperty(window, property, XA_CARDINAL, count)
    {
    }

    void update(Window window = None) {
        YProperty::update(window);
    }

    static void set(Window window, Atom property, long newValue)
    {
        XChangeProperty(display, window, property,
                        XA_CARDINAL, 32, PropModeReplace,
                        reinterpret_cast<unsigned char *>(&newValue), 1);
    }

    void replace(long newValue) {
        set(window(), property(), newValue);
        update();
    }
};

class YClient : public YProperty {
public:
    YClient(Window window, Atom property, int count = 1) :
        YProperty(window, property, XA_WINDOW, count)
    {
    }
    Window* data() { return YProperty::data<Window>(); }
    Window* extract() { return YProperty::extract<Window>(); }
};

class YWmState : public YProperty {
public:
    YWmState(Window window) :
        YProperty(window, ATOM_WM_STATE, ATOM_WM_STATE, 2)
    {
    }
};

class YNetState : public YProperty {
    long fState;
public:
    YNetState(Window window) :
        YProperty(window, ATOM_NET_WM_STATE, XA_ATOM, 32),
        fState(0)
    {
        for (int i = 0; i < count(); ++i) {
            Atom atom = data<long>(i);
            for (int k = 0; k < netStateAtomCount; ++k) {
                if (atom == netStateAtoms[k].atom) {
                    fState |= netStateAtoms[k].flag;
                    break;
                }
            }
        }
    }
    long state() const { return fState; }
    long operator*() const { return state(); }
    operator bool() const { return !status() && format() == 32; }

    enum {
        NetStateRemove, NetStateAdd, NetStateToggle
    };

    void operator +=(long state) {
        const int size = 16;
        Atom atoms[size];
        int n = 0;
        for (int i = 0; i < netStateAtomCount; ++i) {
            bool want = hasbit(state, netStateAtoms[i].flag);
            bool have = hasbit(fState, netStateAtoms[i].flag);
            if (have < want && n < size) {
                atoms[n++] = netStateAtoms[i].atom;
            }
        }
        for (int i = 0; i < n; i += 2) {
            send(ATOM_NET_WM_STATE, window(), NetStateAdd,
                 atoms[i], (i + 1 < n) ? atoms[i + 1] : None,
                 SourceIndication, None);
        }
    }

    void operator -=(long state) {
        const int size = 16;
        Atom atoms[size];
        int n = 0;
        for (int i = 0; i < netStateAtomCount; ++i) {
            bool want = hasbit(state, netStateAtoms[i].flag);
            bool have = hasbit(fState, netStateAtoms[i].flag);
            if (want && have && n < size) {
                atoms[n++] = netStateAtoms[i].atom;
            }
        }
        for (int i = 0; i < n; i += 2) {
            send(ATOM_NET_WM_STATE, window(), NetStateRemove,
                 atoms[i], (i + 1 < n) ? atoms[i + 1] : None,
                 SourceIndication, None);
        }
    }

    void operator ^=(long state) {
        const int size = 16;
        Atom atoms[size];
        int n = 0;
        for (int i = 0; i < netStateAtomCount && n < size; ++i) {
            bool flag = hasbit(state, netStateAtoms[i].flag);
            if (flag && n < size) {
                atoms[n++] = netStateAtoms[i].atom;
            }
        }
        for (int i = 0; i < n; i += 2) {
            send(ATOM_NET_WM_STATE, window(), NetStateToggle,
                 atoms[i], (i + 1 < n) ? atoms[i + 1] : None,
                 SourceIndication, None);
        }
    }

    void operator =(long state) {
        const int size = 16;
        Atom atoms[size];
        int n = 0;
        for (int i = 0; i < netStateAtomCount; ++i) {
            bool want = hasbit(state, netStateAtoms[i].flag);
            bool have = hasbit(fState, netStateAtoms[i].flag);
            if (want != have && n < size) {
                atoms[n++] = netStateAtoms[i].atom;
            }
        }
        for (int i = 0; i < n; i += 2) {
            send(ATOM_NET_WM_STATE, window(), NetStateToggle,
                 atoms[i], (i + 1 < n) ? atoms[i + 1] : None,
                 SourceIndication, None);
        }
    }
};

class YMotifHints : public YProperty {
public:
    YMotifHints(Window window) :
        YProperty(window, ATOM_MOTIF_HINTS, ATOM_MOTIF_HINTS, 5)
    {
    }

    const MwmHints* operator->() const { return data<MwmHints>(); }
    const MwmHints& operator*() const { return *data<MwmHints>(); }

    void replace(const MwmHints& replacement) {
        YProperty::replace<MwmHints>(&replacement, 5, 32);
    }
};

class YStringProperty : public YProperty {
public:
    YStringProperty(Window window, Atom property, Atom kind = AnyPropertyType) :
        YProperty(window, property, kind, BUFSIZ)
    {
        if (status() == Success && kind == AnyPropertyType) {
            if (type() == ATOM_COMPOUND_TEXT) {
                XTextProperty text = { data<unsigned char>(), type(),
                                       format(), (unsigned long) count() };
                char** list = nullptr;
                int count = 0;
                if (XmbTextPropertyToTextList(display, &text, &list, &count)
                    == Success && 0 < count) {
                    char* copy = strdup(*list);
                    XFreeStringList(list);
                    substitute(copy, XA_STRING);
                }
            }
            if (type() != XA_STRING && type() != kind) {
                substitute(nullptr, XA_STRING);
            }
        }
    }

    const char* operator&() const { return data<char>(); }
    bool operator==(const char* str) { return *this && !strcmp(&*this, str); }

    char operator[](int index) const { return data<char>()[index]; }

    void replace(const char* text) {
        YProperty::replace(text, int(strlen(text)), 8);
    }
};

class YUtf8Property : public YStringProperty {
public:
    YUtf8Property(Window window, Atom property) :
        YStringProperty(window, property, ATOM_UTF8_STRING)
    {
    }
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

    operator bool() const { return fStatus == True && count() > 0 && fList; }
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

static void moveResize(Window window, long gravity,
        long x, long y, long width, long height, long flags)
{
    send(ATOM_NET_MOVERESIZE_WINDOW, window,
        gravity | (flags << 8) | (SourceIndication << 12),
        x, y, width, height);
}

static void changeWorkspace(long workspace) {
    send(ATOM_NET_CURRENT_DESKTOP, root, workspace, CurrentTime);
}

static long currentWorkspace() {
    return max(*YCardinal(root, ATOM_NET_CURRENT_DESKTOP), 0L);
}

static void setWorkspace(Window window, long workspace) {
    send(ATOM_NET_WM_DESKTOP, window, workspace, SourceIndication);
}

static long getWorkspace(Window window) {
    return *YCardinal(window, ATOM_NET_WM_DESKTOP);
}

static void setWindowGravity(Window window, long gravity) {
    unsigned long mask = CWWinGravity;
    XSetWindowAttributes attr = {};
    attr.win_gravity = int(gravity);
    XChangeWindowAttributes(display, window, mask, &attr);
}

static int getWindowGravity(Window window) {
    XWindowAttributes attr = {};
    XGetWindowAttributes(display, window, &attr);
    return attr.win_gravity;
}

static void setBitGravity(Window window, long gravity) {
    unsigned long mask = CWBitGravity;
    XSetWindowAttributes attr = {};
    attr.bit_gravity = int(gravity);
    XChangeWindowAttributes(display, window, mask, &attr);
}

static int getBitGravity(Window window) {
    XWindowAttributes attr = {};
    XGetWindowAttributes(display, window, &attr);
    return attr.bit_gravity;
}

static void setNormalGravity(Window window, long gravity) {
    XSizeHints normal;
    long supplied;
    if (XGetWMNormalHints(display, window, &normal, &supplied)) {
        if (inrange(gravity, 1L, 10L)) {
            normal.win_gravity = int(gravity);
            normal.flags |= PWinGravity;
        } else {
            normal.flags &= ~PWinGravity;
        }
    }
    else {
        normal.win_gravity = int(gravity);
        normal.flags = PWinGravity;
    }
    XSetWMNormalHints(display, window, &normal);
}

static int getNormalGravity(Window window) {
    int gravity = NorthWestGravity;
    XSizeHints normal;
    long supplied;
    if (XGetWMNormalHints(display, window, &normal, &supplied)) {
        if (hasbit(normal.flags, PWinGravity)) {
            gravity = normal.win_gravity;
        }
    }
    return gravity;
}

class YWindowTree;

class YTreeLeaf {
private:
    Window leaf;
public:
    YTreeLeaf(Window win = None) : leaf(win) { }
    operator Window() const { return leaf; }
    YTreeLeaf& operator=(Window win) { leaf = win; return *this; }

    YStringProperty wmName() { return YStringProperty(leaf, XA_WM_NAME); }
    YUtf8Property netName() { return YUtf8Property(leaf, ATOM_NET_WM_NAME); }
    YStringProperty wmRole() { return YStringProperty(leaf, ATOM_WM_WINDOW_ROLE); }
};

class YTreeIter {
public:
    YTreeIter(YWindowTree& tree) : fTree(tree), fIndex(0) { }

    operator Window() const;
    Window operator*() const { return Window(*this); }
    void operator++() { ++fIndex; }
    YTreeLeaf* operator->();

private:
    YWindowTree& fTree;
    unsigned fIndex;
};

class YWindowTree {
public:
    YWindowTree(Window window = None):
        fParent(None)
    {
        if (window)
            query(window);
    }

    void single(Window window) {
        release();
        append(window);
    }

    bool append(Window window) {
        return have(window) ? false
            : (fChildren.push_back(window), true);
    }

    bool query(Window window) {
        release();
        if (window) {
            Window rootw;
            Window* data;
            unsigned num;
            if (XQueryTree(display, window, &rootw, &fParent, &data, &num)) {
                copy(data, data + num, back_inserter(fChildren));
                XFree(data);
            }
        }
        return bool(fParent);
    }

    void getWindowList(NAtom& property) {
        release();
        YClient clients(root, property, 100000);
        if (clients) {
            unsigned num = clients.count();
            Window* data = clients.data();
            copy(data, data + num, back_inserter(fChildren));
            fParent = None;
        }
    }

    void getClientList() {
        getWindowList(ATOM_NET_CLIENT_LIST);
    }

    void getSystrayList() {
        getWindowList(ATOM_NET_SYSTEM_TRAY_WINDOWS);
    }

    void filterLast() {
        if (count()) {
            Window w = fChildren.back();
            fChildren.pop_back();
            fChildren.clear();
            fChildren.push_back(w);
        }
    }

    void filterByMapState(int state) {
        vector<Window> keep;
        for (YTreeIter client(*this); client; ++client) {
            XWindowAttributes attr = {};
            if (XGetWindowAttributes(display, client, &attr)
                && attr.map_state == state) {
                keep.push_back(client);
            }
        }
        fChildren = keep;
    }

    void findTaskbar() {
        vector<Window> keep;
        for (YTreeIter client(*this); client; ++client) {
            if (client->wmName() == "Frame") {
                YWindowTree frame(client);
                if (frame && frame->wmName() == "TaskBarFrame") {
                    frame.query(*frame);
                    if (frame && frame->wmName() == "TaskBar") {
                        keep.push_back(*frame);
                    }
                }
            }
        }
        fChildren = keep;
    }

    void filterByWorkspace(long workspace, bool inverse = false) {
        vector<Window> keep;
        for (YTreeIter client(*this); client; ++client) {
            long ws = getWorkspace(client);
            if ((ws == workspace || hasbits(ws, Sticky)) != inverse) {
                keep.push_back(client);
            }
        }
        fChildren = keep;
    }

    void filterByScreen() {
        if (fConfine.confining()) {
            vector<Window> keep;
            for (YTreeIter client(*this); client; ++client) {
                int x, y, w, h;
                if (getGeometry(client, x, y, w, h)) {
                    YRect r(x, y, w, h);
                    YRect s(fConfine[fConfine.screen()]);
                    if (s.overlap(r)) {
                        keep.push_back(client);
                    }
                }
            }
            fChildren = keep;
        }
    }

    void filterByLayer(long layer, bool inverse) {
        vector<Window> keep;
        for (YTreeIter client(*this); client; ++client) {
            YCardinal prop(client, ATOM_WIN_LAYER);
            if (prop && ((*prop == layer) != inverse)) {
                keep.push_back(client);
            }
        }
        fChildren = keep;
    }

    void filterByProperty(char* propertyString, bool inverse) {
        char* valueString = strchr(propertyString, '=');
        if (valueString) {
            *valueString++ = '\0';
        }
        NAtom propertyAtom(propertyString, true);
        if (propertyAtom == None) {
            release();
            return;
        }
        NAtom propertyValue(valueString, true);

        vector<Window> keep;
        for (YTreeIter client(*this); client; ++client) {
            bool match = false;
            YProperty prop(client, propertyAtom, AnyPropertyType, 123);
            if (prop && valueString == nullptr) {
                match = true;
            }
            else if (prop.format() == 8 && valueString) {
                for (int i = 0;; ++i) {
                    char letter = prop.data<char>(i);
                    if (valueString[i] == '\0') {
                        match = (i >= prop.count() || letter == '\0');
                        break;
                    }
                    else if (i >= prop.count()) {
                        match = (0 == strcmp(valueString + i, "."));
                        break;
                    }
                    else if (letter == '\0') {
                        if (valueString[i] != '.') {
                            break;
                        }
                        if (valueString[i + 1] == '\0') {
                            match = true;
                            break;
                        }
                    }
                    else if (letter != valueString[i]) {
                        break;
                    }
                }
            }
            else if (prop.format() == 32 && valueString) {
                if (prop.type() == XA_WINDOW ||
                    prop.type() == XA_CARDINAL)
                {
                    long value = None;
                    if (tolong(valueString, value, 0)) {
                        if (value == *prop) {
                            match = true;
                        }
                    }
                }
                else if (prop.type() == XA_ATOM) {
                    if (propertyValue) {
                        for (long i = 0; i < prop.count(); ++i) {
                            if (propertyValue == Atom(prop[i])) {
                                match = true;
                                break;
                            }
                        }
                    }
                }
            }
            if (match != inverse) {
                keep.push_back(client);
            }
        }
        fChildren = keep;
    }

    void filterByRole(char* role, bool inverse) {
        vector<Window> keep;
        for (YTreeIter client(*this); client; ++client) {
            if ((client->wmRole() == role) != inverse) {
                keep.push_back(client);
            }
        }
        fChildren = keep;
    }

    void filterByNetState(long state, bool inverse, bool anybit) {
        vector<Window> keep;
        for (YTreeIter client(*this); client; ++client) {
            YNetState prop(client);
            bool test(anybit ? hasbit(*prop, state) : hasbits(*prop, state));
            if (test != inverse) {
                keep.push_back(client);
            }
        }
        fChildren = keep;
    }

    void filterByGravity(long gravity, bool inverse) {
        vector<Window> keep;
        for (YTreeIter client(*this); client; ++client) {
            long winGrav = getNormalGravity(client);
            if ((winGrav == gravity) != inverse) {
                keep.push_back(client);
            }
        }
        fChildren = keep;
    }

    void filterByPid(long pid) {
        vector<Window> keep;
        for (YTreeIter client(*this); client; ++client) {
            YCardinal prop(client, ATOM_NET_WM_PID);
            if (prop && *prop == pid) {
                keep.push_back(client);
            }
        }
        fChildren = keep;
    }

    void filterByMachine(const char* machine) {
        size_t len = strlen(machine);
        vector<Window> keep;
        for (YTreeIter client(*this); client; ++client) {
            YStringProperty mac(client, XA_WM_CLIENT_MACHINE);
            if (mac && 0 == strncmp(&mac, machine, len) &&
                        (mac[len] == 0 || mac[len] == '.'))
            {
                keep.push_back(client);
            }
        }
        fChildren = keep;
    }

    void filterByName(char* name) {
        bool head = (*name == '^');
        name += head;
        size_t len = strlen(name);
        bool tail = (len && name[len - 1] == '$');
        len -= tail;
        name[len] = '\0';

        vector<Window> keep;
        for (YTreeIter client(*this); client; ++client) {
            asmart<char> title(newstr(&client->netName()));
            if (isEmpty(title))
                title = newstr(&client->wmName());
            if (nonempty(title)) {
                const char* find = strstr(title, name);
                if (find) {
                    if (head && find > title)
                        continue;
                    if (tail && len != strlen(find))
                        continue;
                    keep.push_back(client);
                }
            }
        }
        fChildren = keep;
    }

    void filterByClass(const char* wmname, const char* wmclass) {
        vector<Window> keep;
        for (YTreeIter client(*this); client; ++client) {
            Window window = client;
            XClassHint hint;
            if (XGetClassHint(display, window, &hint)) {
                if (wmclass) {
                    if (strcmp(hint.res_name, wmname) ||
                        strcmp(hint.res_class, wmclass))
                        window = None;
                }
                else if (wmname) {
                    if (strcmp(hint.res_name, wmname) &&
                        strcmp(hint.res_class, wmname))
                        window = None;
                }

                if (window) {
                    MSG(("selected window 0x%lx: `%s.%s'", window,
                         hint.res_name, hint.res_class));
                    keep.push_back(window);
                }

                XFree(hint.res_name);
                XFree(hint.res_class);
            }
        }
        fChildren = keep;
    }

    void release() {
        fChildren.clear();
        fParent = None;
    }

    operator bool() const {
        return count();
    }

    Window operator[](unsigned index) const {
        return index < count() ? fChildren[index] : None;
    }

    Window operator*() const {
        return operator[](0);
    }

    YTreeLeaf* leaf(unsigned index) {
        fLeaf = operator[](index);
        return &fLeaf;
    }

    YTreeLeaf* operator->() {
        return leaf(0);
    }

    unsigned count() const {
        return fChildren.size();
    }

    bool isRoot() const {
        return count() == 1 && **this == root;
    }

    Window parent() const {
        return fParent;
    }

    Confine& xine() { return fConfine; }

    bool have(Window window) {
        return contains(fChildren, window);
    }

    void remove(Window window) {
        vector<Window>::iterator it;
        it = find(fChildren.begin(), fChildren.end(), window);
        if (it != fChildren.end())
            fChildren.erase(it);
    }

    void operator+=(const YWindowTree& other) {
        for (Window w : other.fChildren) {
            if (have(w) == false) {
                fChildren.push_back(w);
            }
        }
    }

    void stacking() {
        YWindowTree stack;
        stack.getWindowList(ATOM_NET_CLIENT_LIST_STACKING);
        if (stack) {
            std::sort(fChildren.begin(), fChildren.end(),
                      [&stack](Window &a, Window &b){
                return
                    std::find(stack.fChildren.begin(),
                              stack.fChildren.end(), a) >
                    std::find(stack.fChildren.begin(),
                              stack.fChildren.end(), b);
            });
        }
    }

    void reverse() {
        std::reverse(fChildren.begin(), fChildren.end());
    }

private:
    Confine fConfine;
    Window fParent;
    vector<Window> fChildren;
    YTreeLeaf fLeaf;
};

YTreeIter::operator Window() const { return fTree[fIndex]; }
YTreeLeaf* YTreeIter::operator->() { return fTree.leaf(fIndex); }

/******************************************************************************/

#define FOREACH_WINDOW(W) \
    for (YTreeIter W(windowList); W; ++W)
#define CHANGES_WINDOW(W) \
    for (YTreeIter W(windowList); use(W); modified(W), ++W)

/******************************************************************************/

class IceSh {
public:
    IceSh(int argc, char **argv);
    ~IceSh();
    operator int() const { return rc ? rc : windowList ? 0 : 1; }

private:
    int rc;
    int argc;
    char **argv;
    char **argp;
    char *dpyname;
    bool quietude;
    bool selecting;
    bool filtering;

    YWindowTree windowList;

    enum IfTE { NoS, If, IfT, IfF, ElF, ElT, EIF, };
    vector<IfTE> ifs;
    IfTE ifte();
    vector<YWindowTree> trees;
    vector<Window> modifications;

    bool use(Window w);
    void modified(Window w);
    void spy();
    void spyEvent(const XEvent& event);
    void spyClient(const XClientMessageEvent& event, const char* head);
    void flush();
    void flags();
    void flag(char* arg);
    bool plus(char* arg);
    void xinit();
    void motif(Window window, char** args, int count);
    void setBorderTitle(int border, int title);
    void sizeto();
    void sizeby();
    void detail();
    void extents();
    void details(Window window);
    void setWindow(Window window);
    void addWindow(Window window);
    void showProperty(Window window, Atom atom, const char* prefix);
    void parseAction();
    void confine(const char* str);
    void invalidArgument(const char* str);
    char* getArg();
    long getLong();
    bool haveArg();
    bool isArg(const char* str);
    bool isAction(const char* str, int argCount);
    bool icewmAction();
    bool conditional();
    bool evaluating();
    void unexpected();
    bool guiEvents();
    bool listShown();
    bool listXembed();
    void listXembed(Window w);
    void queryXembed(Window w);
    void queryDockapps();
    void extendClass();
    void extendGroup();
    void extendPid();
    bool listClients();
    bool listWindows();
    bool listScreens();
    bool listSymbols();
    bool listSystray();
    void listWindowType();
    void setWindowType(const char* arg);
    bool addWorkspace();
    bool listWorkspaces();
    bool setWorkspaceName();
    bool setWorkspaceNames();
    void changeState(const char* arg);
    void changeState(int state);
    void changeState(int state, Window window);
    bool colormaps();
    bool workarea();
    bool current();
    bool runonce();
    void click();
    bool delay();
    void tabTo(char* arg);
    void monitors();
    bool desktops();
    bool desktop();
    bool wmcheck();
    bool change();
    bool sync();
    void doSync();
    bool check(const struct SymbolTable& symtab, long code, const char* str);
    unsigned count() const;
    void xerror(XErrorEvent* evt);
    void getGeometry(Window window);
    void setGeometry(Window window, const char* geometry);

    const char* atomName(Atom atom) {
        return NAtom::lookup(atom);
    }
    static int xerrors(Display* dpy, XErrorEvent* evt);
    static void catcher(int);
    static bool running;
    static IceSh* singleton;
};

/******************************************************************************/

static const Symbol layerIdentifiers[] = {
    { "Desktop",    WinLayerDesktop     },
    { "Below",      WinLayerBelow       },
    { "Normal",     WinLayerNormal      },
    { "OnTop",      WinLayerOnTop       },
    { "Dock",       WinLayerDock        },
    { "AboveDock",  WinLayerAboveDock   },
    { "Menu",       WinLayerMenu        },
    { nullptr,      0                   }
};

static const Symbol trayOptionIdentifiers[] = {
    { "Ignore",         WinTrayIgnore    },
    { "Minimized",      WinTrayMinimized },
    { "Exclusive",      WinTrayExclusive },
    { nullptr,          0                }
};

static const Symbol gravityIdentifiers[] = {
    { "ForgetGravity",    ForgetGravity    },
    { "NorthWestGravity", NorthWestGravity },
    { "NorthGravity",     NorthGravity     },
    { "NorthEastGravity", NorthEastGravity },
    { "WestGravity",      WestGravity      },
    { "CenterGravity",    CenterGravity    },
    { "EastGravity",      EastGravity      },
    { "SouthWestGravity", SouthWestGravity },
    { "SouthGravity",     SouthGravity     },
    { "SouthEastGravity", SouthEastGravity },
    { "StaticGravity",    StaticGravity    },
    { "UnmapGravity",     UnmapGravity     },
    { nullptr,            0                }
};

static const Symbol motifFunctions[] = {
    { "All",              MWM_FUNC_ALL        },
    { "Resize",           MWM_FUNC_RESIZE     },
    { "Move",             MWM_FUNC_MOVE       },
    { "Minimize",         MWM_FUNC_MINIMIZE   },
    { "Maximize",         MWM_FUNC_MAXIMIZE   },
    { "Close",            MWM_FUNC_CLOSE      },
    { nullptr,            0                   }
};

static const Symbol motifDecorations[] = {
    { "All",              MWM_DECOR_ALL       },
    { "Border",           MWM_DECOR_BORDER    },
    { "Resize",           MWM_DECOR_RESIZEH   },
    { "Title",            MWM_DECOR_TITLE     },
    { "Menu",             MWM_DECOR_MENU      },
    { "Minimize",         MWM_DECOR_MINIMIZE  },
    { "Maximize",         MWM_DECOR_MAXIMIZE  },
    { nullptr,            0                   }
};

static const Symbol netStateIdentifiers[] = {
    { "ABOVE",             NetAbove },
    { "BELOW",             NetBelow },
    { "DEMANDS_ATTENTION", NetDemands },
    { "FOCUSED",           NetFocused },
    { "FULLSCREEN",        NetFullscreen },
    { "HIDDEN",            NetHidden },
    { "MAXIMIZED_HORZ",    NetHorizontal },
    { "MAXIMIZED_VERT",    NetVertical },
    { "MODAL",             NetModal },
    { "SHADED",            NetShaded },
    { "SKIP_PAGER",        NetSkipPager },
    { "SKIP_TASKBAR",      NetSkipTaskbar },
    { "STICKY",            NetSticky },
    { nullptr,             None }
};

static const SymbolTable netstates = {
    netStateIdentifiers, 0, 8191, -1
};

static const SymbolTable layers = {
    layerIdentifiers, 0, WinLayerCount - 1, WinLayerInvalid
};

static const SymbolTable trayOptions = {
    trayOptionIdentifiers, 0, WinTrayOptionCount - 1, WinTrayInvalid
};

static const SymbolTable gravities = {
    gravityIdentifiers, 0, 10, -1
};

static const SymbolTable motifFunctionsTable = {
    motifFunctions, 0, MWM_FUNC_MASK, -1
};

static const SymbolTable motifDecorationsTable = {
    motifDecorations, 0, MWM_DECOR_MASK, -1
};

/******************************************************************************/

long SymbolTable::parseIdentifier(char const * id, size_t const len) const {
    for (Symbol const * sym(fSymbols); sym && sym->name; ++sym)
        if (!strncasecmp(sym->name, id, len) && !sym->name[len])
            return sym->code;

    long value;
    if ( !tolong(id, value, 0) || !inrange(value, fMin, fMax))
        value = fErrCode;
    return value;
}

long SymbolTable::parseExpression(char const * expression) const {
    long value(0);
    const char* token(expression);
    for (int count = 0;; ++count) {
        token = pastSpacesAndTabs(token);
        if (isEmpty(token)) {
            break;
        }
        bool add = *token == '+' || *token == '|';
        bool sub = *token == '-';
        if (count && !(add | sub)) {
            value = fErrCode;
            break;
        }
        if (add | sub) {
            token = pastSpacesAndTabs(token + 1);
        }
        if (isEmpty(token)) {
            value = fErrCode;
            break;
        }
        size_t len = strcspn(token, "-+| \t");
        if (len == 0) {
            value = fErrCode;
            break;
        }
        csmart copy(newstr(token, int(len)));
        if (copy == nullptr) {
            value = fErrCode;
            break;
        }
        long ident = parseIdentifier(copy, len);
        if (ident == fErrCode) {
            value = fErrCode;
            break;
        }
        if (add) {
            value |= ident;
        }
        else if (sub) {
            value &= ~ident;
        }
        else if (count == 0) {
            value = ident;
        }
        token += len;
    }

    return value;
}

void SymbolTable::listSymbols(char const * label) const {
    const long limit = fMax == 8191 ? fMax : 2047L;
    printf(_("Named symbols of the domain `%s' (numeric range: %ld-%ld):\n"),
           label, fMin, min(fMax, limit));

    for (Symbol const * sym(fSymbols); sym && sym->name; ++sym)
        if (sym->code <= limit && (sym->code || sym == fSymbols))
            printf("  %-20s (%ld)\n", sym->name, sym->code);

    puts("");
}

bool SymbolTable::lookup(long code, char const ** name) const {
    for (Symbol const * sym(fSymbols); sym && sym->name; ++sym)
        if (sym->code == code)
            return *name = sym->name, true;
    return false;
}

/******************************************************************************/

class WorkspaceInfo {
public:
    WorkspaceInfo():
        fCount(root, ATOM_NET_NUMBER_OF_DESKTOPS),
        fNames(root, ATOM_NET_DESKTOP_NAMES, YEmby)
    {
    }

    bool parseWorkspace(char const* name, long* workspace);

    long count() const { return max(*fCount, 1L); }
    operator bool() const { return fCount && fNames; }
    bool valid(long i) const { return inrange(i, 0L, count() - 1L); }
    const char* operator[](int i) const {
        return valid(i) ? fNames[i] : (i == -1) ? "All" : "";
    }

private:
    YCardinal fCount;
    YTextProperty fNames;
};

bool WorkspaceInfo::parseWorkspace(char const* name, long* workspace) {
    if (fNames) {
        for (int i = 0; i < fNames.count(); ++i)
            if (0 == strcmp(name, fNames[i]))
                return *workspace = i, true;
    }

    if (0 == strcmp(name, "All") || 0 == strcmp(name, "0xFFFFFFFF"))
        return *workspace = Sticky, true;

    if (0 == strcmp(name, "this"))
        return *workspace = currentWorkspace(), true;

    if (0 == strcmp(name, "next")) {
        *workspace = (currentWorkspace() + 1L) % count();
        return true;
    }

    if (0 == strcmp(name, "prev")) {
        long w = currentWorkspace();
        *workspace = (w > 0L) ? (w - 1L) : (count() - 1L);
        return true;
    }

    if (tolong(name, *workspace)) {
        if (valid(*workspace)) {
            return true;
        } else {
            msg(_("Workspace out of range: %ld"), *workspace);
            return false;
        }
    }

    if (fNames) {
        for (int i = 0; i < fNames.count(); ++i) {
            const char* str = strstr(fNames[i], name);
            if (str) {
                const char* end = str + strlen(name);
                while (*end && isSpaceOrTab(*end))
                    ++end;
                while (str > fNames[i] && isSpaceOrTab(str[-1]))
                    --str;
                if (str == fNames[i] && end[0] == '\0') {
                    return *workspace = i, true;
                }
            }
        }
    }

    msg(_("Invalid workspace name: `%s'"), name);
    return false;
}

static Window getParent(Window window) {
    return YWindowTree(window).parent();
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

static int countTabs(Window frame) {
    YWindowTree windowList(frame);
    int count = 0;
    FOREACH_WINDOW(window)
        if (window->wmName() == "Container")
            ++count;
    return count;
}

static bool isTabbed(Window client) {
    return countTabs(getFrameWindow(client)) > 1;
}

static Window getGroupLeader(Window window) {
    Window lead = None;
    YClient prop(window, ATOM_WM_CLIENT_LEADER);
    if (prop) {
        lead = prop.data()[0];
    } else {
        XWMHints* h = XGetWMHints(display, window);
        if (h) {
            if (h->flags & WindowGroupHint) {
                lead = h->window_group;
            }
            XFree(h);
        }
    }
    return lead;
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

static void getArea(Window window, int& x, int& y, int& w, int& h) {
    long wmin = 0;
    long hmin = 0;
    long wmax = displayWidth();
    long hmax = displayHeight();
    long ws = Sticky;
    if (window && window != root) {
        ws = getWorkspace(window);
    }
    if (ws < 0 || ws == Sticky) {
        ws = currentWorkspace();
    }
    YCardinal net(root, ATOM_NET_WORKAREA, 5000);
    if (net && ws < net.count() / 4) {
        wmin = net[ws * 4 + 0];
        hmin = net[ws * 4 + 1];
        wmax = net[ws * 4 + 2];
        hmax = net[ws * 4 + 3];
    }
    x = int(wmin);
    y = int(hmin);
    w = int(wmax);
    h = int(hmax);
}

static bool getExtents(Window window, int& l, int& r, int& t, int& b) {
    YCardinal exts(window, ATOM_NET_FRAME_EXTENTS, 4);
    if (exts && exts.count() == 4) {
        l = int(exts[0]);
        r = int(exts[1]);
        t = int(exts[2]);
        b = int(exts[3]);
        return true;
    } else {
        l = r = t = b = 0;
        return false;
    }
}

static void extArea(Window window, int& x, int& y, int& w, int& h) {
    int l, r, t, b;
    if (getExtents(window, l, r, t, b)) {
        x += l;
        y += t;
        w = max(1, w - (l + r));
        h = max(1, h - (t + b));
    }
}

bool IceSh::running;

void IceSh::catcher(int)
{
    running = false;
}

bool IceSh::use(Window window)
{
    if (window && contains(modifications, window)) {
        sync();
        fsleep(0.1);
        modifications.clear();
    }
    return window;
}

void IceSh::modified(Window window)
{
    if (contains(modifications, window) == false) {
        modifications.push_back(window);
    }
}

void IceSh::getGeometry(Window window)
{
    use(window);
    int x = 0, y = 0, width = 0, height = 0;
    if (::getGeometry(window, x, y, width, height)) {
        printf("%dx%d+%d+%d\n", width, height, x, y);
    }
}

void IceSh::details(Window w)
{
    YTreeLeaf leaf(w);
    asmart<char> name(newstr(&leaf.netName()));
    if (isEmpty(name))
        name = newstr(Elvis(&leaf.wmName(), ""));

    char title[54] = "";
    snprintf(title, sizeof title, "\"%.*s\"", 50, (char *) name);

    char c = 0, *wmname = &c;
    YTextProperty text(w, XA_WM_CLASS);
    if (text) {
        wmname = text[0];
        wmname[strlen(wmname)] = '.';
    }

    char wmtitle[54] = "";
    snprintf(wmtitle, sizeof wmtitle, "(%.*s)", 50, wmname);

    int x = 0, y = 0, width = 0, height = 0;
    ::getGeometry(w, x, y, width, height);

    long pid = *YCardinal(w, ATOM_NET_WM_PID);
    long work = getWorkspace(w);

    printf("0x%-8lx %2ld %5ld %-20s: %-20s %dx%d%+d%+d\n",
            w, work, pid, title, wmtitle,
            width, height, x, y);
}

void IceSh::detail()
{
    FOREACH_WINDOW(window) {
        use(window);
        details(window);
    }
}

void IceSh::extents()
{
    FOREACH_WINDOW(window) {
        use(window);
        int l, t, r, b;
        if (getExtents(*window, l, r, t, b)) {
            printf("0x%-8lx %d %d %d %d\n", *window, l, r, t, b);
        }
    }
}

void IceSh::sizeto()
{
    char* wstr = getArg();
    char* hstr = getArg();
    bool wper = *wstr && wstr[strlen(wstr)-1] == '%';
    bool hper = *hstr && hstr[strlen(hstr)-1] == '%';
    if (wper) wstr[strlen(wstr)-1] = '\0';
    if (hper) hstr[strlen(hstr)-1] = '\0';
    long wlen = 0, hlen = 0, supplied;
    float wflt = 0, hflt = 0;
    if ((wper ? tofloat(wstr, wflt) && 0 < wflt
              : tolong(wstr, wlen) && 0 < wlen) &&
        (hper ? tofloat(hstr, hflt) && 0 < hflt
              : tolong(hstr, hlen) && 0 < hlen))
    {
        FOREACH_WINDOW(window) {
            use(window);

            long w = wlen;
            long h = hlen;
            if (wper | hper) {
                int ax, ay, aw, ah;
                int l, r, t, b;
                getArea(window, ax, ay, aw, ah);
                getExtents(window, l, r, t, b);
                if (wper) {
                    w = min(long(aw * wflt / 100 - l - r), 32732L);
                }
                if (hper) {
                    h = min(long(ah * hflt / 100 - t - b), 32732L);
                }
                if (w <= 0 || h <= 0) {
                    continue;
                }
            }

            xsmart<XSizeHints> sh(XAllocSizeHints());
            if (XGetWMNormalHints(display, window, sh, &supplied)) {
                if (sh->flags & PMaxSize) {
                    w = min<long>(w, sh->max_width);
                    h = min<long>(h, sh->max_height);
                }
                if (sh->flags & PBaseSize) {
                    w -= sh->base_width;
                    h -= sh->base_height;
                }
                if (sh->flags & PResizeInc) {
                    w -= w % max(1, sh->width_inc);
                    h -= h % max(1, sh->height_inc);
                }
                if (w <= 0 || h <= 0) {
                    continue;
                }
                if (sh->flags & PBaseSize) {
                    w += sh->base_width;
                    h += sh->base_height;
                }
                if (sh->flags & PMinSize) {
                    w = max<long>(w, sh->min_width);
                    h = max<long>(h, sh->min_height);
                }
            }

            if (0 < w && 0 < h) {
                moveResize(window, NorthWestGravity,
                           None, None, w, h, CWWidth | CWHeight);
                modified(window);
            }
        }
    }
    else {
        invalidArgument("sizeto parameters");
    }
}

void IceSh::sizeby()
{
    char* wstr = getArg();
    char* hstr = getArg();
    bool wper = *wstr && wstr[strlen(wstr)-1] == '%';
    bool hper = *hstr && hstr[strlen(hstr)-1] == '%';
    if (wper) wstr[strlen(wstr)-1] = '\0';
    if (hper) hstr[strlen(hstr)-1] = '\0';
    long wlen = 0, hlen = 0, supplied;
    float wflt = 0, hflt = 0;
    if ((wper ? tofloat(wstr, wflt)
              : tolong(wstr, wlen)) &&
        (hper ? tofloat(hstr, hflt)
              : tolong(hstr, hlen)))
    {
        FOREACH_WINDOW(window) {
            use(window);

            int gx, gy, gw, gh;
            if (::getGeometry(window, gx, gy, gw, gh) == false) {
                continue;
            }

            long w = gw, h = gh;
            if (wper) {
                long px = long(wflt * w / 100);
                if (px < 0) w = max(1L, w + px);
                else if (w < 32732L) w = min(32732L, w + px);
            }
            else if (wlen < 0) {
                w = max(1L, w + wlen);
            } else if (w < 32732L) {
                w = min(32732L, w + wlen);
            }
            if (hper) {
                long py = long(hflt * h / 100);
                if (py < 0) h = max(1L, h + py);
                else if (h < 32732L) h = min(32732L, h + py);
            }
            else if (hlen < 0) {
                h = max(1L, h + hlen);
            } else if (h < 32732L) {
                h = min(32732L, h + hlen);
            }

            xsmart<XSizeHints> sh(XAllocSizeHints());
            if (XGetWMNormalHints(display, window, sh, &supplied)) {
                if (sh->flags & PMaxSize) {
                    w = min<long>(w, sh->max_width);
                    h = min<long>(h, sh->max_height);
                }
                if (sh->flags & PBaseSize) {
                    w -= sh->base_width;
                    h -= sh->base_height;
                }
                if (sh->flags & PResizeInc) {
                    w -= w % max(1, sh->width_inc);
                    h -= h % max(1, sh->height_inc);
                }
                if (w <= 0 || h <= 0) {
                    continue;
                }
                if (sh->flags & PBaseSize) {
                    w += sh->base_width;
                    h += sh->base_height;
                }
                if (sh->flags & PMinSize) {
                    w = max<long>(w, sh->min_width);
                    h = max<long>(h, sh->min_height);
                }
            }

            if (0 < w && 0 < h) {
                XResizeWindow(display, window,
                              unsigned(w), unsigned(h));
                modified(window);
            }
        }
    }
    else {
        invalidArgument("sizeby parameters");
    }
}

void IceSh::queryXembed(Window parent)
{
    YWindowTree windowList(parent);
    FOREACH_WINDOW(window) {
        YProperty info(window, ATOM_XEMBED_INFO, AnyPropertyType, 2);
        if (info) {
            this->windowList.append(window);
        }
        queryXembed(window);
    }
}

void IceSh::queryDockapps()
{
    YProperty info(root, ATOM_ICE_DOCKAPPS, XA_WINDOW, 123);
    for (int i = 0; i < info.count(); ++i) {
        windowList.append(info[i]);
    }
}

void IceSh::listXembed(Window parent)
{
    YWindowTree windowList(parent);
    FOREACH_WINDOW(window) {
        YProperty info(window, ATOM_XEMBED_INFO, AnyPropertyType, 2);
        if (info) {
            details(window);
        }
        listXembed(window);
    }
}

bool IceSh::listXembed()
{
    if ( !isAction("xembed", 0))
        return false;

    listXembed(root);
    return true;
}

bool IceSh::listSystray()
{
    if ( !isAction("systray", 0))
        return false;

    windowList.getSystrayList();

    detail();
    return true;
}

bool IceSh::listSymbols()
{
    if ( !isAction("symbols", 0))
        return false;

    layers.listSymbols(_("GNOME window layer"));
    trayOptions.listSymbols(_("IceWM tray option"));
    gravities.listSymbols(_("Gravity symbols"));
    motifFunctionsTable.listSymbols(_("Motif functions"));
    motifDecorationsTable.listSymbols(_("Motif decorations"));
    netstates.listSymbols(_("EWMH window state"));

    return true;
}

bool IceSh::listWindows()
{
    if ( !isAction("windows", 0))
        return false;

    if (count() == 0)
        windowList.query(root);

    detail();
    return true;
}

bool IceSh::listClients()
{
    if ( !isAction("clients", 0))
        return false;

    windowList.getClientList();

    detail();
    return true;
}

bool IceSh::listShown()
{
    if ( !isAction("shown", 0))
        return false;

    windowList.getClientList();
    long workspace = currentWorkspace();
    windowList.filterByWorkspace(workspace);

    detail();
    return true;
}

bool IceSh::listScreens()
{
    if (isAction("randr", 0)) {
        Confine c;
        if (c.load_rand()) {
            for (int i = 0; i < c.count(); ++i) {
                printf("%d: %ux%u+%d+%d\n",
                        i, c[i].width(), c[i].height(), c[i].x(), c[i].y());
            }
        } else {
            puts("No RandR");
        }
        return true;
    }
    if (isAction("xinerama", 0)) {
        Confine c;
        if (c.load_xine()) {
            for (int i = 0; i < c.count(); ++i) {
                printf("%d: %ux%u+%d+%d\n",
                        i, c[i].width(), c[i].height(), c[i].x(), c[i].y());
            }
        } else {
            puts("No Xinerama");
        }
        return true;
    }

    return false;
}

bool IceSh::current()
{
    if ( !isAction("current", 0))
        return false;

    long current = currentWorkspace();
    const char* name = "";
    WorkspaceInfo info;
    if (info && inrange(current, 0L, info.count())) {
        name = info[current];
    }
    printf(_("workspace #%d: `%s'\n"), int(current), name);

    return true;
}

bool IceSh::runonce()
{
    if ( !isAction("runonce", 1))
        return false;

    if (count() == 0 || windowList.isRoot()) {
        execvp(argp[0], argp);
        perror(argp[0]);
        throw 1;
    } else {
        throw 0;
    }

    return true;
}

void IceSh::listWindowType()
{
    static const char prefix[] = "_NET_WM_WINDOW_TYPE_";
    FOREACH_WINDOW(window) {
        YProperty prop(window, ATOM_NET_WM_WINDOW_TYPE, XA_ATOM);
        if (prop) {
            const char* name = NAtom::lookup(*prop);
            if (name) {
                if (strncasecmp(name, prefix, strlen(prefix)) == 0) {
                    name += strlen(prefix);
                }
                printf("0x%-7lx %s\n", *window, name);
            }
        }
    }
}

void IceSh::setWindowType(const char* arg)
{
    static const char* const types[] = {
        "COMBO", "DESKTOP", "DIALOG", "DND", "DOCK", "DROPDOWN_MENU",
        "MENU", "NORMAL", "NOTIFICATION", "POPUP_MENU", "SPLASH",
        "TOOLBAR", "TOOLTIP", "UTILITY", nullptr
    };
    static const char prefix[] = "_NET_WM_WINDOW_TYPE_";
    if (strncasecmp(arg, prefix, strlen(prefix)) == 0) {
        arg += strlen(prefix);
    }
    size_t len = strlen(arg);
    for (int i = 0; types[i]; ++i) {
        if (strncasecmp(arg, types[i], len) == 0 && len) {
            if (types[i][len] == '\0' || types[i][len] == '_') {
                char buf[99] = "_NET_WM_WINDOW_TYPE_";
                strlcat(buf, types[i], sizeof buf);
                NAtom atom(buf);
                CHANGES_WINDOW(window) {
                    setAtom(window, ATOM_NET_WM_WINDOW_TYPE, atom);
                }
                break;
            }
        }
    }
}

bool IceSh::listWorkspaces()
{
    if ( !isAction("listWorkspaces", 0))
        return false;

    WorkspaceInfo info;
    if (info) {
        for (int n(0); n < info.count(); ++n)
            printf(_("workspace #%d: `%s'\n"), n, info[n]);
    }
    return true;
}

bool IceSh::addWorkspace()
{
    if ( !isAction("addWorkspace", 1))
        return false;

    char* name = getArg();
    YCardinal prop(root, ATOM_NET_NUMBER_OF_DESKTOPS);
    if (prop) {
        long workspace = *prop;
        if (inrange(workspace, 0L, 1233L)) {
            send(ATOM_NET_NUMBER_OF_DESKTOPS, root, workspace + 1L, 0L);
            for (int i = 0; i < 3; ++i) {
                doSync();
                prop.update();
                if (prop && workspace < *prop) {
                    YTextProperty names(root, ATOM_NET_DESKTOP_NAMES, YEmby);
                    names.set(int(workspace), name);
                    names.commit();
                    doSync();
                    break;
                }
            }
        }
    }
    return true;
}

bool IceSh::setWorkspaceName()
{
    if ( !isAction("setWorkspaceName", 2))
        return false;

    char* id = getArg();
    char* nm = getArg();
    long ws;
    if (id && nm && tolong(id, ws)) {
        YTextProperty names(root, ATOM_NET_DESKTOP_NAMES, YEmby);
        names.set(int(ws), nm);
        names.commit();
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

    YClient check(root, ATOM_NET_SUPPORTING_WM_CHECK);
    if (check) {
        YUtf8Property netName(*check, ATOM_NET_WM_NAME);
        YStringProperty name(*check, XA_WM_NAME);
        if (netName || name) {
            printf("Name: %s\n", netName ? &netName : &name);
        }
        YStringProperty cls(*check, XA_WM_CLASS);
        if (cls) {
            printf("Class: ");
            for (int i = 0; i + 1 < cls.count(); ++i) {
                char c = Elvis(cls[i], '.');
                putchar(c);
            }
            newline();
        }
        YStringProperty loc(*check, ATOM_WM_LOCALE_NAME);
        if (loc) {
            printf("Locale: %s\n", &loc);
        }
        YStringProperty com(*check, XA_WM_COMMAND);
        if (com) {
            printf("Command: ");
            for (int i = 0; i + 1 < com.count(); ++i) {
                char c = Elvis(com[i], ' ');
                putchar(c);
            }
            newline();
        }
        YStringProperty mac(*check, XA_WM_CLIENT_MACHINE);
        if (mac) {
            printf("Machine: %s\n", &mac);
        }
        YCardinal pid(*check, ATOM_NET_WM_PID);
        if (pid) {
            printf("PID: %lu\n", *pid);
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
        YCardinal prop(root, ATOM_NET_NUMBER_OF_DESKTOPS);
        if (prop) {
            printf("%ld workspaces\n", *prop);
        }
    }
    return true;
}

bool IceSh::desktop()
{
    if ( !isAction("desktop", 0))
        return false;

    long n;
    if (haveArg() && tolong(*argp, n)) {
        ++argp;
        if (inrange(n, 0L, 1L)) {
            send(ATOM_NET_SHOWING_DESKTOP, root, n, 0L);
        }
    }
    else {
        YCardinal prop(root, ATOM_NET_SHOWING_DESKTOP);
        if (prop) {
            printf("desktop %ld\n", *prop);
        }
    }
    return true;
}

bool IceSh::change()
{
    if ( !isAction("goto", 1))
        return false;

    char* name = getArg();
    long workspace;
    if ( ! WorkspaceInfo().parseWorkspace(name, &workspace))
        throw 1;

    changeWorkspace(workspace);

    return true;
}

bool IceSh::sync()
{
    if ( !isAction("sync", 0))
        return false;

    doSync();

    if (haveArg() && 0 == strcmp(*argp, "delay"))
        modifications.clear();

    return true;
}

void IceSh::doSync()
{
    YProperty winp(root, ATOM_WIN_PROTOCOLS, XA_ATOM, 20);
    if (winp) {
        YProperty wopt(root, ATOM_ICE_WINOPT, ATOM_ICE_WINOPT, 20);
        if (wopt == false) {
            unsigned char data[3] = { 0, 0, 0, };
            wopt.append(data, 3, 8);
        }
        for (int i = 1; i <= 5 && winp && wopt; ++i) {
            usleep(i*1000);
            winp.update();
            wopt.update();
        }
    }
}

bool IceSh::colormaps()
{
    if ( !isAction("colormaps", 0))
        return false;

    xsmart<Colormap> old;
    int m = 0, k = 0;

    tlog("colormaps");
    running = true;
    sighandler_t previous = signal(SIGINT, catcher);
    while (running) {
        int n = 0;
        Colormap* map = XListInstalledColormaps(display, root, &n);
        if (n != m || old == nullptr || memcmp(map, old, n * sizeof(Colormap)) || !(++k % 100)) {
            char buf[2000] = "";
            for (int i = 0; i < n; ++i) {
                snprintf(buf + strlen(buf), sizeof buf - strlen(buf),
                        " 0x%0lx", map[i]);
            }
            printf("colormaps%s\n", buf);
            flush();
            old = map;
            m = n;
        }
        else {
            XFree(map);
        }
        usleep(100*1000);
    }

    signal(SIGINT, previous);
    return true;
}

bool IceSh::workarea()
{
    if ( !isAction("workarea", 0))
        return false;

    int ax, ay, aw, ah;
    getArea(root, ax, ay, aw, ah);
    printf("%d %d %d %d\n", ax, ay, aw, ah);

    return true;
}

void IceSh::changeState(const char* arg)
{
    if (!strncmp(arg, "NormalState", strlen(arg)))
        changeState(NormalState);
    else if (!strncmp(arg, "IconicState", strlen(arg)))
        changeState(IconicState);
    else if (!strncmp(arg, "WithdrawnState", strlen(arg)))
        changeState(WithdrawnState);
    else {
        msg(_("Invalid state: `%s'."), arg);
        throw 1;
    }
}

void IceSh::changeState(int state, Window window) {
    use(window);
    send(ATOM_WM_CHANGE_STATE, window, state, None);
    modified(window);
}

void IceSh::changeState(int state) {
    FOREACH_WINDOW(window) {
        changeState(state, window);
    }
}

void IceSh::click()
{
    const char* xs = getArg();
    const char* ys = getArg();
    const char* bs = getArg();
    long lx, ly, lb;
    if (tolong(xs, lx) &&
        tolong(ys, ly) &&
        tolong(bs, lb) &&
        inrange<long>(lb, Button1, Button5 + 4))
    {
        FOREACH_WINDOW(window) {
            use(window);

            int gx, gy, gw, gh;
            if (::getGeometry(window, gx, gy, gw, gh)) {
                if (lx < 0)
                    lx += gw;
                if (ly < 0)
                    ly += gh;
            } else {
                continue;
            }
            int dx = 0, dy = 0;
            Window child = None;
            if (XTranslateCoordinates(display, window, window,
                        int(lx), int(ly), &dx, &dy, &child))
            {
                XWarpPointer(display, None, window,
                             0, 0, 0, 0, int(lx), int(ly));

                XButtonEvent be = {
                    ButtonPress, None, True, display,
                    child ? child : window,
                    root, child, serverTime(),
                    dx, dy, int(gx + lx), int(gy + ly),
                    None, unsigned(lb), True
                };

                XEvent ev;
                ev.xbutton = be;
                XSendEvent(display, child, True, None, &ev);

                be.type = ButtonRelease;
                be.state = Button1Mask << (lb - Button1);
                be.time = serverTime();
                ev.xbutton = be;
                XSendEvent(display, child, True, None, &ev);
            }
        }
    }
}

bool IceSh::delay()
{
    if ( !isAction("delay", 0))
        return false;

    double delay = 0.1;
    if (haveArg()) {
        char* arg = *argp;
        char* end = nullptr;
        double val = strtod(arg, &end);
        if (end && arg < end && *end == '\0') {
            delay = val;
            getArg();
        }
    }
    fsleep(delay);

    if (haveArg() && 0 == strcmp(*argp, "sync"))
        modifications.clear();

    return true;
}

void IceSh::monitors()
{
    long ty = getLong();
    long by = getLong();
    long lx = getLong();
    long rx = getLong();
    FOREACH_WINDOW(window) {
        send(ATOM_NET_WM_FULLSCREEN_MONITORS, window,
             ty, by, lx, rx, None);
    }
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
                YProperty prop(root, ATOM_GUI_EVENT, ATOM_GUI_EVENT);
                if (prop) {
                    int gev = prop.data<unsigned char>(0);
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
            select(fd + 1, SELECT_TYPE_ARG234 &rfds, nullptr, nullptr, nullptr);
        }
    }
    signal(SIGINT, previous);
    return true;
}

bool IceSh::icewmAction()
{
    static const Symbol sa[] = {
        { "logout",     ICEWM_ACTION_LOGOUT },
        { "cancel",     ICEWM_ACTION_CANCEL_LOGOUT },
        { "reboot",     ICEWM_ACTION_REBOOT },
        { "shutdown",   ICEWM_ACTION_SHUTDOWN },
        { "about",      ICEWM_ACTION_ABOUT },
        { "windowlist", ICEWM_ACTION_WINDOWLIST },
        { "restart",    ICEWM_ACTION_RESTARTWM },
        { "suspend",    ICEWM_ACTION_SUSPEND },
        { "winoptions", ICEWM_ACTION_WINOPTIONS },
        { "keys",       ICEWM_ACTION_RELOADKEYS },
    };
    for (Symbol sym : sa) {
        if (0 == strcmp(*argp, sym.name)) {
            ++argp;
            send(ATOM_ICE_ACTION, root, CurrentTime, sym.code);
            return true;
        }
    }

    return guiEvents()
        || setWorkspaceNames()
        || setWorkspaceName()
        || listWorkspaces()
        || addWorkspace()
        || listScreens()
        || listWindows()
        || listClients()
        || listSymbols()
        || listSystray()
        || listXembed()
        || listShown()
        || colormaps()
        || workarea()
        || delay()
        || desktops()
        || desktop()
        || current()
        || runonce()
        || wmcheck()
        || change()
        || sync()
        ;
}

unsigned IceSh::count() const
{
    return windowList.count();
}

/******************************************************************************/

static void setLayer(Window window, long layer) {
    send(ATOM_WIN_LAYER, window, layer, CurrentTime);
}

static void getLayer(Window window) {
    YCardinal prop(window, ATOM_WIN_LAYER);
    if (prop) {
        long layer = *prop;
        const char* name;
        if (layers.lookup(layer, &name))
            printf("0x%-7lx %s\n", window, name);
        else
            printf("0x%-7lx %ld\n", window, layer);
    }
}

static void setTrayHint(Window window, long trayopt) {
    send(ATOM_WIN_TRAY, window, trayopt, CurrentTime);
}

static void getTrayOption(Window window) {
    YCardinal prop(window, ATOM_WIN_TRAY);
    if (prop) {
        long tropt = *prop;
        const char* name;
        if (trayOptions.lookup(tropt, &name))
            printf("0x%-7lx %s\n", window, name);
        else
            printf("0x%-7lx %ld\n", window, tropt);
    }
}

static void printWindowGravity(Window window) {
    long grav = getWindowGravity(window);
    const char* name = nullptr;
    if (gravities.lookup(grav, &name))
        printf("0x%-7lx %s\n", window, name);
    else
        printf("0x%-7lx %ld\n", window, grav);
}

static void printBitGravity(Window window) {
    long grav = getBitGravity(window);
    const char* name = nullptr;
    if (gravities.lookup(grav, &name))
        printf("0x%-7lx %s\n", window, name);
    else
        printf("0x%-7lx %ld\n", window, grav);
}

static void printNormalGravity(Window window) {
    long grav = getNormalGravity(window);
    const char* name = nullptr;
    if (gravities.lookup(grav, &name))
        printf("0x%-7lx %s\n", window, name);
    else
        printf("0x%-7lx %ld\n", window, grav);
}

/******************************************************************************/

void IceSh::setGeometry(Window window, const char* geometry) {
    int geom_x, geom_y;
    unsigned geom_width, geom_height;
    int status(XParseGeometry(geometry, &geom_x, &geom_y,
                              &geom_width, &geom_height));
    if (status == NoValue)
        return;

    use(window);

    XSizeHints normal;
    long supplied;
    if (XGetWMNormalHints(display, window, &normal, &supplied) != True)
        return;

    Window root;
    int x, y;
    unsigned width, height, dummy;
    if (XGetGeometry(display, window, &root, &x, &y, &width, &height,
                     &dummy, &dummy) != True)
        return;

    if (status & XValue) x = geom_x;
    if (status & YValue) y = geom_y;
    if (status & WidthValue) width = geom_width;
    if (status & HeightValue) height = geom_height;

    if (normal.flags & PResizeInc) {
        width *= max(1, normal.width_inc);
        height *= max(1, normal.height_inc);
    }

    if (normal.flags & PBaseSize) {
        width += normal.base_width;
        height += normal.base_height;
    }

    if (hasbit(status, XNegative | YNegative)) {
        int maxWidth = displayWidth();
        int maxHeight = displayHeight();
        YCardinal exts(window, ATOM_NET_FRAME_EXTENTS, 4);
        if (exts && exts.count() == 4) {
            maxWidth -= exts[0] + exts[1];
            maxHeight -= exts[2] + exts[3];
        }
        if (hasbits(status, XValue | XNegative))
            x += maxWidth - width;
        if (hasbits(status, YValue | YNegative))
            y += maxHeight - height;
    }

    moveResize(window, StaticGravity, x, y, width, height,
               (status & AllValues));
    modified(window);
}

/******************************************************************************/

static void setWindowTitle(Window window, const char* title) {
    YUtf8Property net(window, ATOM_NET_WM_NAME);
    YStringProperty old(window, XA_WM_NAME);
    if (old)
        old.replace(title);
    if (net || !old)
        net.replace(title);
}

static void getWindowTitle(Window window) {
    YUtf8Property net(window, ATOM_NET_WM_NAME);
    if (net)
        puts(&net);
    else {
        YStringProperty name(window, XA_WM_NAME);
        if (name) {
            puts(&name);
        }
    }
}

static void getIconTitle(Window window) {
    YUtf8Property net(window, ATOM_NET_WM_ICON_NAME);
    if (net)
        puts(&net);
    else {
        YStringProperty title(window, XA_WM_ICON_NAME);
        if (title) {
            puts(&title);
        }
    }
}

static void setIconTitle(Window window, const char* title) {
    YUtf8Property net(window, ATOM_NET_WM_ICON_NAME);
    YStringProperty old(window, XA_WM_ICON_NAME);
    if (old)
        old.replace(title);
    if (net || !old)
        net.replace(title);
}

/******************************************************************************/

static Window getActive() {
    Window active = None;

    YClient client(root, ATOM_NET_ACTIVE_WINDOW);
    if (client) {
        active = *client;
    }
    else {
        int revertTo;
        XGetInputFocus(display, &active, &revertTo);
    }
    return active;
}

static void closeWindow(Window window) {
    send(ATOM_NET_CLOSE_WINDOW, window, CurrentTime, SourceIndication);
}

static void activateWindow(Window window) {
    send(ATOM_NET_ACTIVE_WINDOW, window, SourceIndication, CurrentTime);
}

static void raiseWindow(Window window) {
    send(ATOM_NET_RESTACK_WINDOW, window, SourceIndication, None, Above);
}

static void lowerWindow(Window window) {
    send(ATOM_NET_RESTACK_WINDOW, window, SourceIndication, None, Below);
}

static Window getClientWindow(Window window)
{
    if (YWmState(window))
        return window;

    YWindowTree windowList(window);
    if (windowList == false)
        return None;

    FOREACH_WINDOW(client)
        if (YWmState(client))
            return client;

    FOREACH_WINDOW(client) {
        YWindowTree windowList(client);
        if (windowList) {
            FOREACH_WINDOW(subwin)
                if (YWmState(subwin))
                    return subwin;
        }
    }

    return None;
}

static Window pickWindow() {
    Cursor cursor = XCreateFontCursor(display, XC_crosshair);
    bool running(true);
    Window target(None);
    int count(0);
    KeyCode escape = XKeysymToKeycode(display, XK_Escape);

    // this is broken
    XGrabKey(display, escape, 0, root, False, GrabModeAsync, GrabModeAsync);
    XGrabPointer(display, root, False, ButtonPressMask|ButtonReleaseMask,
                 GrabModeAsync, GrabModeAsync, root, cursor, CurrentTime);

    while (running && (None == target || 0 < count)) {
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

    if (target && target != root)
        target = getClientWindow(target);

    return target;
}

static bool isOptArg(const char* arg, const char* opt, const char* val) {
    const char buf[3] = { opt[0], opt[1], '\0', };
    return (strpcmp(arg, opt) == 0 || strcmp(arg, buf) == 0) && val != nullptr;
}

static void removeOpacity(Window window) {
    Window framew = getFrameWindow(window);
    XDeleteProperty(display, framew, ATOM_NET_WM_WINDOW_OPACITY);
    XDeleteProperty(display, window, ATOM_NET_WM_WINDOW_OPACITY);
}

static void setOpacity(Window window, unsigned opaqueness) {
    Window framew = getFrameWindow(window);
    YCardinal::set(framew, ATOM_NET_WM_WINDOW_OPACITY, opaqueness);
}

static unsigned getOpacity(Window window) {
    Window framew = getFrameWindow(window);

    YCardinal prop(framew, ATOM_NET_WM_WINDOW_OPACITY);
    if (prop == false)
        prop.update(window);
    unsigned opaq = (unsigned(*prop) & 0xFFFFFFFF);

    return unsigned(100.0 * opaq / 0xFFFFFFFF);
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
        else {
            long val;
            if (tolong(opaq, val) && inrange<long>(val, 0, 100)) {
                oset = unsigned(val * double(omax) * 0.01);
                return setOpacity(window, oset);
            }
        }
    }
    else {
        unsigned perc = getOpacity(window);
        printf("0x%-7lx %u\n", window, perc);
    }
}

void IceSh::motif(Window window, char** args, int count) {
    YMotifHints hints(window);

    if (hints && !count) { // && hasbit(hints->flags, 017)) {
        const char spaces[] = "          ";
        printf("0x%-7lx motif:%s%s%s%s\n", window,
                hints->hasFuncs() ? " funcs" : "",
                hints->hasDecor() ? " decor" : "",
                hints->hasInput() ? " input" : "",
                hints->hasStatus() ? " status" : "");
        if (hints->hasFuncs()) {
            unsigned long funcs = hints->functions;
            char sign = '+';
            printf("%sfuncs:", spaces);
            for (int i = 0, n = 0; motifFunctions[i].name; ++i) {
                if (funcs & motifFunctions[i].code) {
                    printf("%c%s",
                            ++n == 1 ? ' ' : sign,
                            motifFunctions[i].name);
                    sign = (funcs & MWM_FUNC_ALL) ? '-' : '+';
                }
            }
            newline();
        }
        if (hints->hasDecor()) {
            unsigned long decor = hints->decorations;
            char sign = '+';
            printf("%sdecor:", spaces);
            for (int i = 0, n = 0; motifDecorations[i].name; ++i) {
                if (decor & motifDecorations[i].code) {
                    printf("%c%s",
                            ++n == 1 ? ' ' : sign,
                            motifDecorations[i].name);
                    sign = (decor & MWM_DECOR_ALL) ? '-' : '+';
                }
            }
            newline();
        }
        if (hints->hasInput()) {
            long input = hints->input_mode;
            printf("%sinput: %s\n", spaces,
                    input == MWM_INPUT_MODELESS ?
                            "modeless" :
                    input == MWM_INPUT_APPLICATION_MODAL ?
                            "application_modal" :
                    input == MWM_INPUT_SYSTEM_MODAL ?
                            "system_modal" :
                    input == MWM_INPUT_FULL_APPLICATION_MODAL ?
                            "full_application_modal" : "");
        }
        if (hints->hasStatus()) {
            printf("%sstatus:%s\n", spaces,
                    (hints->status & MWM_TEAROFF_WINDOW) ?
                            " tearoff" : "");
        }
    }

    if (0 == count) {
        return;
    }

    MwmHints mwm;
    if (hints) {
        mwm = *hints;
    }
    bool removing = false;

    for (int k = 0; k < count; ++k) {
        char* arg = args[k];
        if (0 == strcmp(arg, "remove")) {
            mwm = MwmHints();
            removing = true;
        }
        else if (0 == strcmp(arg, "funcs") && k + 1 < count) {
            arg = args[++k];
            csmart tmp;
            if (strchr("-+|", *arg) && mwm.hasFuncs()) {
                size_t len = 32 + strlen(arg);
                tmp = new char[len];
                snprintf(tmp, len, "%ld%s", mwm.functions, arg);
                arg = tmp;
            }
            long val = motifFunctionsTable.parseExpression(arg);
            if (val < None) {
                return;
            }
            mwm.functions = val;
            mwm.setFuncs();
        }
        else if (0 == strcmp(arg, "decor") && k + 1 < count) {
            arg = args[++k];
            csmart tmp;
            if (strchr("-+|", *arg) && mwm.hasDecor()) {
                size_t len = 32 + strlen(arg);
                tmp = new char[len];
                snprintf(tmp, len, "%ld%s", mwm.decorations, arg);
                arg = tmp;
            }
            long val = motifDecorationsTable.parseExpression(arg);
            if (val < None) {
                return;
            }
            mwm.decorations = val;
            mwm.setDecor();
        }
    }

    if (mwm.hasFlags()) {
        hints.replace(mwm);
        modified(window);
    }
    else if (removing && hints) {
        hints.remove();
        modified(window);
    }
}

void IceSh::setBorderTitle(int border, int title) {
    CHANGES_WINDOW(window) {
        YMotifHints hints(window);
        MwmHints mwm;
        if (hints) {
            mwm = *hints;
        }
        if (mwm.hasDecor() == false) {
            mwm.decorations = (MWM_DECOR_MASK & ~MWM_DECOR_ALL);
            if (border == false)
                mwm.decorations &= ~MWM_DECOR_BORDER;
            if (border == true)
                mwm.decorations |= MWM_DECOR_BORDER;
            if (title == false)
                mwm.decorations &= ~MWM_DECOR_TITLE;
            if (title == true)
                mwm.decorations |= MWM_DECOR_TITLE;
        }
        else if (mwm.decorAll()) {
            if (border == true)
                mwm.decorations &= ~MWM_DECOR_BORDER;
            if (border == false)
                mwm.decorations |= MWM_DECOR_BORDER;
            if (title == true)
                mwm.decorations &= ~MWM_DECOR_TITLE;
            if (title == false)
                mwm.decorations |= MWM_DECOR_TITLE;
        } else {
            if (border == false)
                mwm.decorations &= ~MWM_DECOR_BORDER;
            if (border == true)
                mwm.decorations |= MWM_DECOR_BORDER;
            if (title == false)
                mwm.decorations &= ~MWM_DECOR_TITLE;
            if (title == true)
                mwm.decorations |= MWM_DECOR_TITLE;
        }
        if (mwm.decorations == (MWM_DECOR_MASK & ~MWM_DECOR_ALL))
            mwm.notDecor();
        else if (mwm.decorations == MWM_DECOR_ALL)
            mwm.notDecor();
        else
            mwm.setDecor();
        if (mwm.hasFlags())
            hints.replace(mwm);
        else if (hints)
            hints.remove();
    }
}

void IceSh::showProperty(Window window, Atom atom, const char* prefix) {
    if (atom == XA_WM_NORMAL_HINTS || atom == XA_WM_SIZE_HINTS) {
        XSizeHints h;
        long supplied;
        if (XGetWMSizeHints(display, window, &h, &supplied, atom) == True) {
            const char* name(atomName(atom));
            printf("%s%s", prefix, (char *) name);
            if (h.flags & USPosition) {
                printf(" UPos(%d,%d)", h.x, h.y);
            }
            else if (h.flags & PPosition) {
                printf(" PPos(%d,%d)", h.x, h.y);
            }
            if (h.flags & USSize) {
                printf(" USize(%d,%d)", h.width, h.height);
            }
            else if (h.flags & PSize) {
                printf(" PSize(%d,%d)", h.width, h.height);
            }
            if (h.flags & PMinSize) {
                printf(" MinSize(%d,%d)", h.min_width, h.min_height);
            }
            if (h.flags & PMaxSize) {
                printf(" MaxSize(%d,%d)", h.max_width, h.max_height);
            }
            if (h.flags & PResizeInc) {
                printf(" Inc(%d,%d)", h.width_inc, h.height_inc);
            }
            if (h.flags & PBaseSize) {
                printf(" Base(%d,%d)", h.base_width, h.base_height);
            }
            newline();
        }
        return;
    }

    if (atom == ATOM_WM_STATE) {
        YWmState s(window);
        if (s) {
            const char* name(atomName(atom));
            printf("%s%s = ", prefix, (char *) name);
            if (s[0] == IconicState)
                printf("Iconic");
            else if (s[0] == NormalState)
                printf("Normal");
            else if (s[0] == WithdrawnState)
                printf("Withdrawn");
            else
                printf("%ld", s[0]);
            if (2 == s.count())
                printf(", 0x%lx", s[1]);
            newline();
        }
        return;
    }

    if (atom == XA_WM_HINTS) {
        xsmart<XWMHints> h(XGetWMHints(display, window));
        if (h) {
            long f = h->flags;
            const char* name(atomName(atom));
            printf("%s%s", prefix, (char *) name);
            if (f & InputHint) {
                printf(" Input(%d)", h->input & True);
            }
            if (f & StateHint) {
                printf(" %s",
                        h->initial_state == WithdrawnState
                            ? "Withdrawn" :
                        h->initial_state == NormalState
                            ? "Normal" :
                        h->initial_state == IconicState
                            ? "Iconic" : ""
                        );
            }
            if (f & WindowGroupHint) {
                printf(" Group(%lu)", h->window_group);
            }
            if (f & IconPixmapHint) {
                printf(" Pixmap(0x%lx)", h->icon_pixmap);
            }
            if (f & IconWindowHint) {
                printf(" Window(0x%lx)", h->icon_window);
            }
            newline();
        }
        return;
    }

    YProperty prop(window, atom, AnyPropertyType, 64);
    if (prop.status() == Success && prop.data<void>()) {
        if (prop.format() == 8) {
            const char* name(atomName(atom));
            printf("%s%s = ", prefix, (char*) name);
            if (prop.type() == ATOM_GUI_EVENT) {
                int gev = prop.data<unsigned char>(0);
                if (inrange(1 + gev, 1, NUM_GUI_EVENTS)) {
                    puts(gui_event_names[gev]);
                }
            }
            else {
                for (int i = 0; i < prop.count(); ++i) {
                    unsigned char ch = prop.data<unsigned char>(i);
                    if (ch == '\0') {
                        if (i + 1 == prop.count()) {
                            break;
                        }
                    }
                    putchar(isPrint(ch) ? ch : '.');
                }
                newline();
            }
        }
        else if (prop.format() == 32) {
            if (prop.type() == XA_WINDOW) {
                const char* name(atomName(atom));
                printf("%s%s = ", prefix, (char *) name);
                for (int i = 0; i < prop.count(); ++i)
                    printf("%s0x%lx", i ? ", " : "", prop[i]);
                newline();
            }
            else if (prop.type() == XA_ATOM) {
                const char* name(atomName(atom));
                printf("%s%s = ", prefix, (char*) name);
                for (int i = 0; i < prop.count(); ++i) {
                    name = atomName(prop[i]);
                    printf("%s%s", i ? ", " : "", (char*) name);
                }
                newline();
            }
            else if (prop.type() == XA_CARDINAL) {
                const char* name(atomName(atom));
                printf("%s%s = ", prefix, (char *) name);
                for (int i = 0; i < prop.count(); ++i)
                    printf("%s%u", i ? ", " : "", unsigned(prop[i]));
                newline();
            }
            else {
                const char* name(atomName(atom));
                const char* type(atomName(prop.type()));
                printf("%s%s(%s) = ", prefix, (char *) name, (char *) type);
                for (int i = 0; i < prop.count(); ++i)
                    printf("%s%ld", i ? ", " : "", prop[i]);
                newline();
            }
        }
        else {
            const char* name(atomName(atom));
            printf("%s%s(%d)\n", prefix, (char *) name, prop.format());
        }
    }
    else if (atom == ATOM_NET_WM_USER_TIME) {
        YClient user(window, ATOM_NET_WM_USER_TIME_WINDOW);
        if (user) {
            showProperty(*user.data(), atom, prefix);
        }
    }
}

void IceSh::confine(const char* val) {
    long screen = -1L;
    if (strcmp(val, "All") == 0) {
        windowList.xine().confineTo(screen);
    }
    else if (strcmp(val, "this") == 0) {
        Window active = getActive();
        if (active) {
            int x, y, w, h, most = 0;
            if (::getGeometry(active, x, y, w, h)) {
                YRect area(x, y, w, h);
                Confine& c = windowList.xine();
                for (int i = 0; i < c.count(); ++i) {
                    int overlap(int(area.overlap(c[i])));
                    if (most < overlap) {
                        most = overlap;
                        screen = i;
                    }
                }
                c.confineTo(screen);
            }
            else {
                msg(_("Cannot get geometry of window 0x%lx"), active);
                throw 1;
            }
        }
        else {
            Window r, s;
            int x, y, a, b;
            unsigned m;
            if (XQueryPointer(display, root, &r, &s, &x, &y, &a, &b, &m)) {
                YRect mouse(x, y, 1, 1);
                Confine& c = windowList.xine();
                for (int i = 0; i < c.count(); ++i) {
                    if (c[i].contains(mouse)) {
                        screen = i;
                        break;
                    }
                }
                c.confineTo(screen);
            }
        }
    }
    else if (tolong(val, screen)) {
        windowList.xine().confineTo(screen);
    }
    else {
        msg(_("Invalid Xinerama: `%s'."), val);
        throw 1;
    }
}

void IceSh::flush()
{
    if (fflush(stdout) || ferror(stdout))
        throw 1;
}

static void randomSetup() {
    static unsigned seed;
    if (seed == 0) {
        timeval now = walltime();
        seed = unsigned((getpid() * now.tv_usec) ^ now.tv_sec);
        srand(seed);
    }
}

static char* randomLabel() {
    static const char data[] =
        "abcdefghijklmnopqrstuvwxyz-12345"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ_67890";
    const int length = 7;
    static char label[length + 1];
    randomSetup();
    for (int i = 0, r = 0; i < length; ++i) {
        if (r < 32)
            r = rand();
        label[i] = data[r & 63];
        r >>= 6;
    }
    label[length] = '\0';
    return label;
}

void IceSh::tabTo(char* defaultLabel)
{
    if ( ! windowList && ! selecting) {
        Window pick = pickWindow();
        if (pick <= root)
            throw 1;
        setWindow(pick);
        selecting = true;
    }
    CHANGES_WINDOW(window) {
        if (defaultLabel == nullptr && isTabbed(window) == false)
            continue;

        XClassHint h = { nullptr, nullptr };
        bool xgot = (XGetClassHint(display, window, &h) == True);
        YStringProperty role(window, ATOM_WM_WINDOW_ROLE);
        const char* str1 = nullptr, *str2 = nullptr;
        if (xgot && h.res_class)
            str1 = h.res_class, str2 = h.res_name;
        else if (xgot && h.res_name)
            str1 = h.res_name, str2 = role ? &role : nullptr;
        else if (role)
            str1 = &role;
        size_t length = (str1 ? strlen(str1) : 0)
                      + (str2 ? strlen(str2) : 0);
        XWindowAttributes attr = {};
        if (length && XGetWindowAttributes(display, window, &attr)) {
            char* label = defaultLabel ? defaultLabel : randomLabel();
            size_t size = length + 10 + strlen(label);
            char* buf = new char[size];
            if (buf) {
                int len;
                if (str2) {
                    len = snprintf(buf, size, "%s.%s%cframe%c%s",
                                   str1, str2, '\0', '\0', label);
                } else {
                    len = snprintf(buf, size, "%s%cframe%c%s",
                                   str1, '\0', '\0', label);
                }
                XUnmapWindow(display, window);
                XUnmapEvent unmap = {
                    UnmapNotify, None, True, display, root, window, False
                };
                sendEvent(&unmap);
                for (;;) {
                    YWindowTree tree;
                    tree.getClientList();
                    if (tree.have(window) == false)
                        break;
                    if ( !tree.query(window) || tree.parent() == root)
                        break;
                }
                YProperty wopt(root, ATOM_ICE_WINOPT, ATOM_ICE_WINOPT, 0);
                wopt.append((unsigned char *) buf, len + 1, 8);
                doSync();
                XMapWindow(display, window);
                for (;;) {
                    YWindowTree tree;
                    tree.getClientList();
                    if (tree.have(window))
                        break;
                    if ( !tree.query(window) || tree.parent() != root)
                        break;
                }
                delete[] buf;
            }
        }
        XFree(h.res_name);
        XFree(h.res_class);
    }
}

void IceSh::extendClass()
{
    if ( ! windowList && ! selecting) {
        Window pick = pickWindow();
        if (pick <= root)
            throw 1;
        setWindow(pick);
        selecting = true;
    }
    if (windowList) {
        vector<XClassHint> classes;
        FOREACH_WINDOW(window) {
            XClassHint h = { nullptr, nullptr };
            if (XGetClassHint(display, window, &h) == True) {
                classes.push_back(h);
            }
        }
        YWindowTree clients;
        clients.getClientList();
        for (YTreeIter window(clients); window; ++window) {
            if (windowList.have(window) == false) {
                XClassHint h = { nullptr, nullptr };
                if (XGetClassHint(display, window, &h) == True) {
                    if (nonempty(h.res_class)) {
                        for (XClassHint c : classes) {
                            if (nonempty(c.res_class)
                                && !strcmp(c.res_class, h.res_class)) {
                                addWindow(window);
                                break;
                            }
                        }
                    }
                    else if (nonempty(h.res_name)) {
                        for (XClassHint c : classes) {
                            if (isEmpty(c.res_class)
                                && nonempty(c.res_name)
                                && !strcmp(c.res_name, h.res_name)) {
                                addWindow(window);
                                break;
                            }
                        }
                    }
                }
                XFree(h.res_name);
                XFree(h.res_class);
            }
        }
        for (XClassHint h : classes) {
            XFree(h.res_name);
            XFree(h.res_class);
        }
        classes.clear();
    }
}

void IceSh::extendGroup()
{
    if ( ! windowList && ! selecting) {
        Window pick = pickWindow();
        if (pick <= root)
            throw 1;
        setWindow(pick);
        selecting = true;
    }
    if (windowList) {
        YWindowTree leaders;
        FOREACH_WINDOW(window) {
            Window lead = getGroupLeader(window);
            if (lead) {
                leaders.append(lead);
            }
        }
        if (leaders) {
            YWindowTree clients;
            clients.getClientList();
            for (YTreeIter window(clients); window; ++window) {
                if (windowList.have(window) == false) {
                    Window lead = getGroupLeader(window);
                    if (lead && leaders.have(lead)) {
                        addWindow(window);
                    }
                }
            }
        }
    }
}

void IceSh::extendPid()
{
    if ( ! windowList && ! selecting) {
        Window pick = pickWindow();
        if (pick <= root)
            throw 1;
        setWindow(pick);
        selecting = true;
    }
    if (windowList) {
        vector<long> pidset;
        FOREACH_WINDOW(window) {
            long pid = *YCardinal(window, ATOM_NET_WM_PID);
            if (1 < pid && !contains(pidset, pid))
                pidset.push_back(pid);
        }
        if (pidset.size()) {
            YWindowTree clients;
            clients.getClientList();
            for (YTreeIter window(clients); window; ++window) {
                if (windowList.have(window) == false) {
                    long pid = *YCardinal(window, ATOM_NET_WM_PID);
                    if (1 < pid && contains(pidset, pid)) {
                        windowList.append(window);
                    }
                }
            }
        }
    }
}

IceSh::IceSh(int ac, char **av) :
    rc(0),
    argc(ac),
    argv(av),
    argp(av + 1),
    dpyname(nullptr),
    quietude(false),
    selecting(false),
    filtering(false)
{
    singleton = this;
    setAtomName(NAtom::lookup);
    try {
        xinit();
        flags();
    }
    catch (int code) {
        rc = code;
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

long IceSh::getLong()
{
    char* str = getArg();
    long num;
    if ( !tolong(str, num)) {
        msg(_("Invalid argument: `%s'"), str);
        throw 1;
    }
    return num;
}

bool IceSh::isArg(const char* str)
{
    return haveArg() && 0 == strcmp(str, *argp)
        ? ++argp, true : false;
}

bool IceSh::isAction(const char* action, int count)
{
    bool is = isArg(action);
    if (is && argp + count > argv + argc) {
        msg(_("Action `%s' requires at least %d arguments."), action, count);
        throw 1;
    }
    return is;
}

bool IceSh::check(const SymbolTable& symtab, long code, const char* str)
{
    if (symtab.invalid(code)) {
        msg(_("Invalid expression: `%s'"), str);
        throw 1;
    }
    return true;
}

/******************************************************************************/

void IceSh::xinit()
{
    if (nullptr == (display = XOpenDisplay(dpyname))) {
        warn(_("Can't open display: %s. X must be running and $DISPLAY set."),
             XDisplayName(dpyname));
        throw 3;
    }

    root = DefaultRootWindow(display);
    XSynchronize(display, True);
    XSetErrorHandler(xerrors);
}

/******************************************************************************/

void IceSh::invalidArgument(const char* str)
{
    msg(_("Invalid argument: `%s'"), str);
    throw 1;
}

void IceSh::setWindow(Window window)
{
    windowList.single(window);
}

void IceSh::addWindow(Window window)
{
    windowList.append(window);
}

IceSh::IfTE IceSh::ifte()
{
    IfTE state = NoS;
    if (ifs.size()) {
        state = ifs.back();
        ifs.pop_back();
    }
    return state;
}

bool IceSh::conditional()
{
    if (isArg("if")) {
        ifs.push_back(If);
        trees.push_back(windowList);
    }
    else if (isArg("then")) {
        IfTE state = ifte();
        if (state == If && windowList)
            ifs.push_back(IfT);
        else if (state == If)
            ifs.push_back(IfF);
        else if (state == EIF)
            ifs.push_back(IfF);
        else
            unexpected();
    }
    else if (isArg("elif")) {
        IfTE state = ifte();
        if (state == If && windowList)
            ifs.push_back(EIF);
        else if (state == If)
            ifs.push_back(If);
        else if (state == IfT)
            ifs.push_back(EIF);
        else if (state == IfF)
            ifs.push_back(If);
        else if (state == EIF)
            ifs.push_back(EIF);
        else
            unexpected();
        if (ifs.back() == If && evaluating())
            windowList = trees.back();
    }
    else if (isArg("else")) {
        IfTE state = ifte();
        if (state == If && windowList)
            ifs.push_back(ElF);
        else if (state == If)
            ifs.push_back(ElT);
        else if (state == IfT)
            ifs.push_back(ElF);
        else if (state == IfF)
            ifs.push_back(ElT);
        else if (state == EIF)
            ifs.push_back(ElF);
        else
            unexpected();
        if (ifs.back() == ElT && evaluating())
            windowList = trees.back();
    }
    else if (isArg("end")) {
        if (ifte() == NoS)
            unexpected();
        trees.pop_back();
        if (trees.size())
            windowList = trees.back();
    }
    else if (evaluating()) {
        return false;
    }
    else {
        ++argp;
    }
    return true;
}

bool IceSh::evaluating()
{
    for (IfTE i : ifs) {
        if (i == IfF || i == EIF || i == ElF)
            return false;
    }
    return true;
}

void IceSh::unexpected()
{
    msg(_("Unexpected: `%s'."), argp[-1]);
    throw 2;
}

void IceSh::flags()
{
    bool act = false;

    while (haveArg()) {
        if (conditional()) {
            /*ignore*/;
        }
        else if (argp[0][0] == '-') {
            char* arg = getArg();
            if (arg[1] == '-')
                arg++;
            flag(arg);
        }
        else if (plus(argp[0])) {
            flag(getArg());
        }
        else {
            act = true;
            if (icewmAction())
                ;
            else if (windowList)
                parseAction();
            else if (selecting | filtering) {
                if (!quietude)
                    msg(_("No windows found."));
                throw 1;
            }
            else {
                Window w = pickWindow();
                if (w <= root)
                    throw 1;
                setWindow(w);
                selecting = true;
                parseAction();
            }
            flush();
        }
    }

    if (act == false) {
        msg(_("No actions specified."));
        throw 1;
    }
}

bool IceSh::plus(char* arg)
{
    return '+' == *arg && strchr("cfgrwCPT", arg[1]);
}

void IceSh::flag(char* arg)
{
    if (isOptArg(arg, "-root", "") || isOptArg(arg, "+root", "")) {
        if (*arg == '+') {
            addWindow(root);
        } else {
            setWindow(root);
        }
        MSG(("root window selected"));
        selecting = true;
        return;
    }
    if (isOptArg(arg, "-focus", "") || isOptArg(arg, "+focus", "")) {
        Window active = getActive();
        if (active && *arg == '+') {
            addWindow(active);
        }
        else if (active && *arg == '-') {
            setWindow(active);
        }
        else if (*arg == '-') {
            windowList.release();
        }
        MSG(("focus window selected"));
        selecting = true;
        return;
    }
    if (isOptArg(arg, "-shown", "")) {
        windowList.getClientList();
        windowList.filterByWorkspace(currentWorkspace());
        MSG(("shown windows selected"));
        selecting = true;
        return;
    }
    if (isOptArg(arg, "-all", "")) {
        windowList.getClientList();
        MSG(("all windows selected"));
        selecting = true;
        return;
    }
    if (isOptArg(arg, "-top", "")) {
        windowList.query(root);
        MSG(("top windows selected"));
        selecting = true;
        return;
    }
    if (isOptArg(arg, "-xembed", "")) {
        windowList.release();
        queryXembed(root);
        MSG(("xembed windows selected"));
        selecting = true;
        return;
    }
    if (isOptArg(arg, "-Dockapps", "")) {
        windowList.release();
        queryDockapps();
        MSG(("dockapps windows selected"));
        selecting = true;
        return;
    }
    if (isOptArg(arg, "+group", "")) {
        extendGroup();
        return;
    }
    if (isOptArg(arg, "+Pid", "")) {
        extendPid();
        return;
    }
    if (isOptArg(arg, "+Class", "")) {
        extendClass();
        return;
    }
    if (isOptArg(arg, "-last", "")) {
        if ( ! windowList && ! selecting)
            windowList.getClientList();
        windowList.filterLast();
        MSG(("last window selected"));
        selecting = true;
        return;
    }
    if (isOptArg(arg, "-unmapped", "")) {
        if ( ! windowList && ! selecting)
            windowList.getClientList();
        windowList.filterByMapState(IsUnmapped);
        selecting = true;
        return;
    }
    if (isOptArg(arg, "-viewable", "")) {
        if ( ! windowList && ! selecting)
            windowList.getClientList();
        windowList.filterByMapState(IsViewable);
        selecting = true;
        return;
    }
    if (isOptArg(arg, "-T", "")) {
        windowList.query(root);
        windowList.findTaskbar();
        MSG(("taskbar selected"));
        selecting = true;
        return;
    }
    if (isOptArg(arg, "+T", "")) {
        YWindowTree taskbar(root);
        taskbar.findTaskbar();
        windowList += taskbar;
        MSG(("taskbar appended"));
        selecting = true;
        return;
    }
    if (isOptArg(arg, "-quiet", "")) {
        quietude = true;
        return;
    }

    size_t sep(strcspn(arg, "=:"));
    char *val(arg[sep] ? &arg[sep + 1] : getArg());

    if (isOptArg(arg, "-window", val) || isOptArg(arg, "+window", val)) {
        long window;
        if (0 == strcmp(val, "root")) {
            char str[] = { *arg, 'r', '\0' };
            return flag(str);
        }
        else if (0 == strcmp(val, "focus")) {
            char str[] = { *arg, 'f', '\0' };
            return flag(str);
        }
        else if (tolong(val, window, 0) && root <= Window(window)) {
            if (*arg == '+') {
                addWindow(window);
            } else {
                setWindow(window);
            }
            MSG(("window %s selected", val));
            selecting = true;
        }
        else {
            msg(_("Invalid window identifier: `%s'"), val);
            throw 1;
        }
    }
    else if (isOptArg(arg, "-display", val)) {
        dpyname = val;
    }
    else if (isOptArg(arg, "-pid", val)) {
        long pid;
        if (tolong(val, pid)) {
            if ( ! windowList)
                windowList.getClientList();
            windowList.filterByPid(pid);
            MSG(("pid window selected"));
            filtering = true;
        }
        else {
            msg(_("Invalid PID: `%s'"), val);
            throw 1;
        }
    }
    else if (isOptArg(arg, "-machine", val)) {
        if ( ! windowList)
            windowList.getClientList();
        windowList.filterByMachine(val);

        MSG(("machine windows selected"));
        filtering = true;
    }
    else if (isOptArg(arg, "-name", val)) {
        if ( ! windowList)
            windowList.getClientList();
        windowList.filterByName(val);

        MSG(("name windows selected"));
        filtering = true;
    }
    else if (isOptArg(arg, "-Workspace", val)) {
        bool inverse(*val == '!');
        long ws;
        if ( ! WorkspaceInfo().parseWorkspace(val + inverse, &ws))
            throw 1;

        if ( ! windowList)
            windowList.getClientList();
        windowList.filterByWorkspace(ws, inverse);
        MSG(("workspace windows selected"));
        filtering = true;
    }
    else if (isOptArg(arg, "-Layer", val)) {
        bool inverse(*val == '!');
        long layer = layers.parseIdentifier(val + inverse);
        if (layer == WinLayerInvalid) {
            msg(_("Invalid layer: `%s'."), val);
            throw 1;
        }
        if ( ! windowList)
            windowList.getClientList();
        windowList.filterByLayer(layer, inverse);
        MSG(("layer windows selected"));
        filtering = true;
    }
    else if (isOptArg(arg, "-Property", val)) {
        bool inverse(*val == '!');
        char* prop = val + inverse;
        if ( ! windowList)
            windowList.getClientList();
        windowList.filterByProperty(prop, inverse);
        filtering = true;
    }
    else if (isOptArg(arg, "-Role", val)) {
        bool inverse(*val == '!');
        char* role = val + inverse;
        if ( ! windowList)
            windowList.getClientList();
        windowList.filterByRole(role, inverse);
        filtering = true;
    }
    else if (isOptArg(arg, "-Netstate", val)) {
        bool inverse(*val == '!');
        bool question(val[inverse] == '?');
        long flags(netstates.parseExpression(val + inverse + question));
        if (flags == -1L) {
            msg(_("Invalid state: `%s'."), val + inverse + question);
            throw 1;
        }
        if ( ! windowList)
            windowList.getClientList();
        windowList.filterByNetState(flags, inverse, question);
        MSG(("netstate windows selected"));
        filtering = true;
    }
    else if (isOptArg(arg, "-Gravity", val)) {
        bool inverse(*val == '!');
        long gravity(gravities.parseExpression(val + inverse));
        check(gravities, gravity, val + inverse);
        if ( ! windowList)
            windowList.getClientList();
        windowList.filterByGravity(gravity, inverse);
        MSG(("gravity windows selected"));
        filtering = true;
    }
    else if (isOptArg(arg, "-Xinerama", val)) {
        if ( ! windowList)
            windowList.getClientList();
        confine(val);
        windowList.filterByScreen();
        MSG(("xinerama %s selected", val));
        filtering = true;
    }
    else if (isOptArg(arg, "-class", val) || isOptArg(arg, "+class", val)) {
        char *wmname = val;
        char *wmclass = nullptr;
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
                if (wmclass == nullptr) {
                    wmclass = d;
                }
                p++;
                continue;
            }
            *d++ = *p++;
        }
        *d++ = 0;

        MSG(("wmname: `%s'; wmclass: `%s'", wmname, wmclass));

        if (*arg == '-') {
            if ( ! windowList)
                windowList.getClientList();
            windowList.filterByClass(wmname, wmclass);
            filtering = true;
        }
        else {
            YWindowTree tree;
            tree.getClientList();
            tree.filterByClass(wmname, wmclass);
            windowList += tree;
            selecting = true;
        }
    }
#ifdef DEBUG
    else if (strpcmp(arg, "-debug") == 0) {
        debug = 1;
    }
#endif
    else {
        invalidArgument(arg);
    }
    MSG(("windowCount: %d", count()));
}

void IceSh::spy()
{
    const long selectMask =
        FocusChangeMask | VisibilityChangeMask |
        EnterWindowMask | LeaveWindowMask |
        StructureNotifyMask | PropertyChangeMask;

    FOREACH_WINDOW(window) {
        XSelectInput(display, window, selectMask +
                SubstructureNotifyMask * (window == root));
    }
    while (windowList) {
        XEvent event;
        XNextEvent(display, &event);
        Window window = event.xany.window;
        if (windowList.have(window)) {
            spyEvent(event);
        }
    }
}

void IceSh::spyEvent(const XEvent& event)
{
    Window window = event.xany.window;
    timeval now(walltime());
    struct tm* local = localtime(&now.tv_sec);
    int secs = local->tm_sec;
    int mins = local->tm_min;
    int mils = int(now.tv_usec / 1000L);
    char head[80], xarg[80];
    snprintf(head, sizeof head,
            "%02d:%02d.%03d: 0x%07x: %s",
            mins, secs, mils, int(window),
            (event.xany.send_event && event.type != ConfigureNotify)
                ? "Send " : "");
    switch (event.type) {
    case EnterNotify:
    case LeaveNotify:
        printf("%s%s%s%s%s\n", head,
            event.type == EnterNotify ? "Enter" : "Leave",
            event.xcrossing.mode == NotifyNormal
                ? " Normal" :
            event.xcrossing.mode == NotifyGrab
                ? " Grab" :
            event.xcrossing.mode == NotifyUngrab
                ? " Ungrab" :
            event.xcrossing.mode == NotifyWhileGrabbed
                ? " Grabbed" : " Unknown",
            event.xcrossing.detail == NotifyAncestor
                ? " Ancestor" :
            event.xcrossing.detail == NotifyVirtual
                ? " Virtual" :
            event.xcrossing.detail == NotifyInferior
                ? " Inferior" :
            event.xcrossing.detail == NotifyNonlinear
                ? " Nonlinear" :
            event.xcrossing.detail == NotifyNonlinearVirtual
                ? " NonlinearVirtual" :
            event.xcrossing.detail == NotifyPointer
                ? " Pointer" :
            event.xcrossing.detail == NotifyPointerRoot
                ? " PointerRoot" :
            event.xcrossing.detail == NotifyDetailNone
                ? "" : " ???",
            event.xcrossing.focus
                ? " Focus" : " Nofocus");
        break;
    case FocusIn:
    case FocusOut:
        printf("%s%s%s%s\n",
            head,
            event.type == FocusIn
                ? "Focus" : "Defocus",
            event.xfocus.mode == NotifyNormal
                ? " Normal" :
            event.xfocus.mode == NotifyWhileGrabbed
                ? " WhileGrabbed" :
            event.xfocus.mode == NotifyGrab
                ? " Grab" :
            event.xfocus.mode == NotifyUngrab
                ? " Ungrab" : " ???",
            event.xfocus.detail == NotifyAncestor
                ? " Ancestor" :
            event.xfocus.detail == NotifyVirtual
                ? " Virtual" :
            event.xfocus.detail == NotifyInferior
                ? " Inferior" :
            event.xfocus.detail == NotifyNonlinear
                ? " Nonlinear" :
            event.xfocus.detail == NotifyNonlinearVirtual
                ? " NonlinearVirtual" :
            event.xfocus.detail == NotifyPointer
                ? " Pointer" :
            event.xfocus.detail == NotifyPointerRoot
                ? " PointerRoot" :
            event.xfocus.detail == NotifyDetailNone
                ? "" : " ???");
        break;
    case UnmapNotify:
        printf("%sUnmap 0x%lx%s%s\n", head,
                event.xunmap.window,
                event.xunmap.from_configure ? " Configure" : "",
                event.xunmap.send_event ? " Send" : "");
        break;
    case VisibilityNotify:
        printf("%sVisibility %s\n", head,
            event.xvisibility.state == VisibilityPartiallyObscured
                ? "PartiallyObscured " :
            event.xvisibility.state == VisibilityFullyObscured
                ? "FullyObscured " :
            event.xvisibility.state == VisibilityUnobscured
                ? "Unobscured " : "Bogus "
            );
        break;
    case CreateNotify:
        printf("%sCreate 0x%lx %dx%d%+d%+d%s\n", head,
                event.xcreatewindow.window,
                event.xcreatewindow.width, event.xcreatewindow.height,
                event.xcreatewindow.x, event.xcreatewindow.y,
                event.xcreatewindow.override_redirect ? " Override" : "");
        break;
    case DestroyNotify:
        printf("%sDestroyed 0x%lx\n", head, event.xdestroywindow.window);
        windowList.remove(event.xdestroywindow.window);
        break;
    case MapNotify:
        printf("%sMapped 0x%lx%s\n", head, event.xmap.window,
                event.xmap.override_redirect ? " Override" : "");
        break;
    case ReparentNotify:
        break;
    case ConfigureNotify:
        if (event.xconfigure.above)
            snprintf(xarg, sizeof xarg, " Above=0x%lx", event.xconfigure.above);
        else
            xarg[0] = '\0';
        printf("%sConfigure 0x%lx %dx%d+%d+%d%s%s%s\n", head,
                event.xconfigure.window,
                event.xconfigure.width, event.xconfigure.height,
                event.xconfigure.x, event.xconfigure.y,
                event.xconfigure.send_event ? " Send" : "",
                event.xconfigure.override_redirect ? " Override" : "",
                xarg);
        break;
    case GravityNotify:
        break;
    case CirculateNotify:
        printf("%sCirculate 0x%lx %s\n", head,
                event.xcirculate.window,
                event.xcirculate.place == PlaceOnTop
                    ? "PlaceOnTop" :
                event.xcirculate.place == PlaceOnBottom
                    ? "PlaceOnBottom" : "Bogus");
        break;
    case PropertyNotify:
        if (event.xproperty.state == PropertyNewValue) {
            showProperty(window, event.xproperty.atom, head);
        } else {
            const char* name(atomName(event.xproperty.atom));
            printf("%sDelete %s\n", head, (char *) name);
        }
        break;
    case ClientMessage:
        spyClient(event.xclient, head);
        break;
    default:
        printf("%sUnknown event type %d\n", head, event.type);
        break;
    }
}

void IceSh::spyClient(const XClientMessageEvent& event, const char* head) {
    const char* name = atomName(event.message_type);
    const long* data = event.data.l;
    if (event.message_type == ATOM_NET_WM_STATE) {
        const char* op =
            data[0] == 0 ? "REMOVE" :
            data[0] == 1 ? "ADD" :
            data[0] == 2 ? "TOGGLE" : "?";
        const char* p1 = data[1] ? atomName(data[1]) : "";
        const char* p2 = data[2] ? atomName(data[2]) : "";
        printf("%sClientMessage %s %d data=%s,%s,%s\n",
                head, name, event.format, op, p1, p2);
    }
    else if (event.message_type == ATOM_WM_CHANGE_STATE) {
        const char* op =
            data[0] == 0 ? "WithdrawnState" :
            data[0] == 1 ? "NormalState" :
            data[0] == 3 ? "IconicState" : "?";
        printf("%sClientMessage %s %s\n", head, name, op);
    }
    else {
        printf("%sClientMessage %s %d data=%ld,0x%lx,0x%lx\n",
                head, name, event.format, data[0], data[1], data[2]);
    }
}

IceSh* IceSh::singleton;

int IceSh::xerrors(Display* dpy, XErrorEvent* evt) {
    singleton->xerror(evt);
    return Success;
}

void IceSh::xerror(XErrorEvent* evt) {
    char message[80], req[80], number[80];

    snprintf(number, 80, "%d", evt->request_code);
    XGetErrorDatabaseText(display, "XRequest",
                          number, "",
                          req, sizeof(req));
    if (req[0] == 0)
        snprintf(req, 80, "[request_code=%d]", evt->request_code);

    if (XGetErrorText(display, evt->error_code, message, 80) != Success)
        *message = '\0';

    tlog("%s(0x%lx): %s", req, evt->resourceid, message);
    argp = argv + argc;
    rc = 4;
}

/******************************************************************************/

void IceSh::parseAction()
{
    if (true) {
        if (isAction("setWindowTitle", 1)) {
            char const * title(getArg());

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
            char const * title(getArg());

            MSG(("setIconTitle: `%s'", title));
            FOREACH_WINDOW(window)
                setIconTitle(window, title);
        }
        else if (isAction("setGeometry", 1)) {
            char const * geometry(getArg());
            FOREACH_WINDOW(window)
                setGeometry(window, geometry);
        }
        else if (isAction("getGeometry", 0)) {
            FOREACH_WINDOW(window) {
                use(window);
                getGeometry(window);
            }
        }
        else if (isAction("resize", 2)) {
            const char* ws = getArg();
            const char* hs = getArg();
            long lw, lh;
            if (tolong(ws, lw) && tolong(hs, lh) && 0 < lw && 0 < lh) {
                char buf[64];
                snprintf(buf, sizeof buf, "%ldx%ld", lw, lh);
                FOREACH_WINDOW(window)
                    setGeometry(window, buf);
            }
            else {
                invalidArgument("resize parameters");
            }
        }
        else if (isAction("sizeto", 2)) {
            sizeto();
        }
        else if (isAction("sizeby", 2)) {
            sizeby();
        }
        else if (isAction("move", 2)) {
            const char* xa = getArg();
            const char* ya = getArg();
            const char* xs = &xa[isSign(xa[0]) && isSign(xa[1])];
            const char* ys = &ya[isSign(ya[0]) && isSign(ya[1])];
            long lx, ly;
            if (tolong(xs, lx) && tolong(ys, ly)) {
                FOREACH_WINDOW(window) {
                    use(window);
                    Window frame = getFrameWindow(window);
                    if (frame) {
                        int x, y, w, h;
                        if (::getGeometry(frame, x, y, w, h)) {
                            int tx, ty;
                            if (xa < xs && *xa == '-') {
                                tx = displayWidth() - w + lx;
                            } else {
                                tx = lx;
                            }
                            if (ya < ys && *ya == '-') {
                                ty = displayHeight() - h + ly;
                            } else {
                                ty = ly;
                            }
                            moveResize(window, NorthWestGravity,
                                       tx, ty, None, None, CWX | CWY);
                            modified(window);
                        }
                    }
                }
            }
            else {
                invalidArgument("move parameters");
            }
        }
        else if (isAction("moveby", 2)) {
            const char* xs = getArg();
            const char* ys = getArg();
            long lx, ly;
            if (tolong(xs, lx) && tolong(ys, ly)) {
                FOREACH_WINDOW(window) {
                    use(window);
                    Window frame = getFrameWindow(window);
                    if (frame) {
                        int x, y, w, h;
                        if (::getGeometry(frame, x, y, w, h)) {
                            int tx = int(x + lx), ty = int(y + ly);
                            moveResize(window, NorthWestGravity,
                                       tx, ty, None, None, CWX | CWY);
                            modified(window);
                        }
                    }
                }
            }
            else {
                invalidArgument("moveby parameters");
            }
        }
        else if (isAction("centre", 0) || isAction("center", 0)) {
            FOREACH_WINDOW(window) {
                use(window);
                int x, y, w, h;
                if (::getGeometry(window, x, y, w, h)) {
                    XWindowChanges c;
                    int ax, ay, aw, ah;
                    getArea(window, ax, ay, aw, ah);
                    extArea(window, ax, ay, aw, ah);
                    c.x = (aw - w) / 2;
                    c.y = (ah - h) / 2;
                    XConfigureWindow(display, window, CWX | CWY, &c);
                    modified(window);
                }
            }
        }
        else if (isAction("left", 0)) {
            FOREACH_WINDOW(window) {
                use(window);
                int x, y, w, h;
                if (::getGeometry(window, x, y, w, h)) {
                    XWindowChanges c;
                    int ax, ay, aw, ah;
                    getArea(window, ax, ay, aw, ah);
                    c.x = max(0, ax);
                    XConfigureWindow(display, window, CWX, &c);
                    modified(window);
                }
            }
        }
        else if (isAction("right", 0)) {
            FOREACH_WINDOW(window) {
                use(window);
                YCardinal exts(window, ATOM_NET_FRAME_EXTENTS, 4);
                if (exts) {
                    int x, y, w, h;
                    if (::getGeometry(window, x, y, w, h)) {
                        int ax, ay, aw, ah;
                        getArea(window, ax, ay, aw, ah);
                        XWindowChanges c;
                        c.x = ax + aw - w - exts[0] - exts[1];
                        XConfigureWindow(display, window, CWX, &c);
                        modified(window);
                    }
                }
            }
        }
        else if (isAction("top", 0)) {
            FOREACH_WINDOW(window) {
                use(window);
                int x, y, w, h;
                if (::getGeometry(window, x, y, w, h)) {
                    XWindowChanges c;
                    int ax, ay, aw, ah;
                    getArea(window, ax, ay, aw, ah);
                    c.y = max(0, ay);
                    XConfigureWindow(display, window, CWY, &c);
                    modified(window);
                }
            }
        }
        else if (isAction("bottom", 0)) {
            FOREACH_WINDOW(window) {
                use(window);
                YCardinal exts(window, ATOM_NET_FRAME_EXTENTS, 4);
                if (exts) {
                    int x, y, w, h;
                    if (::getGeometry(window, x, y, w, h)) {
                        int ax, ay, aw, ah;
                        getArea(window, ax, ay, aw, ah);
                        XWindowChanges c;
                        c.y = ay + ah - h - exts[2] - exts[3];
                        XConfigureWindow(display, window, CWY, &c);
                        modified(window);
                    }
                }
            }
        }
        else if (isAction("netState", 0)) {
            if (haveArg() && strchr("+-=^", **argp)) {
                char* arg = getArg();
                long state(netstates.parseExpression(arg + 1));
                if (state == -1) {
                    msg(_("Invalid state: `%s'."), arg);
                }
                else {
                    FOREACH_WINDOW(window) {
                        use(window);
                        YNetState prop(window);
                        if (*arg == '+')
                            prop += state;
                        else if (*arg == '-')
                            prop -= state;
                        else if (*arg == '=')
                            prop = state;
                        else if (*arg == '^')
                            prop ^= state;
                    }
                }
            }
            else {
                FOREACH_WINDOW(window) {
                    use(window);
                    printf("0x%07lx = ", *window);
                    long state = YNetState(window).state();
                    int n = 0;
                    for (int k = 0; k < netStateAtomCount; ++k) {
                        if (hasbit(state, netStateAtoms[k].flag)) {
                            printf("%s%s", n++ ? ", " : "", 14 +
                                    netStateAtoms[k].atom.name());
                        }
                    }
                    newline();
                }
            }
        }
        else if (isAction("setWorkspace", 1)) {
            long workspace;
            if ( ! WorkspaceInfo().parseWorkspace(getArg(), &workspace))
                throw 1;

            MSG(("setWorkspace: %ld", workspace));
            if (windowList.isRoot())
                changeWorkspace(workspace);
            else
                FOREACH_WINDOW(window) {
                    use(window);
                    setWorkspace(window, workspace);
                    modified(window);
                }
        }
        else if (isAction("getWorkspace", 0)) {
            WorkspaceInfo info;
            FOREACH_WINDOW(window) {
                use(window);
                int ws = int(getWorkspace(window));
                const char* name = info ? info[ws] : "";
                printf("0x%-7lx %d \"%s\"\n", Window(window), ws, name);
            }
        }
        else if (isAction("sticky", 0)) {
            FOREACH_WINDOW(window) {
                use(window);
                setWorkspace(window, Sticky);
                modified(window);
            }
        }
        else if (isAction("unsticky", 0)) {
            long current = currentWorkspace();
            FOREACH_WINDOW(window) {
                use(window);
                long ws = getWorkspace(window);
                if (hasbits(ws, Sticky)) {
                    setWorkspace(window, current);
                    modified(window);
                }
            }
        }
        else if (isAction("borderless", 0)) {
            setBorderTitle(false, false);
        }
        else if (isAction("bordered", 0)) {
            setBorderTitle(true, true);
        }
        else if (isAction("setLayer", 1)) {
            unsigned layer(layers.parseExpression(getArg()));
            check(layers, layer, argp[-1]);

            MSG(("setLayer: %d", layer));
            CHANGES_WINDOW(window)
                setLayer(window, layer);
        }
        else if (isAction("getLayer", 0)) {
            FOREACH_WINDOW(window)
                getLayer(window);
        }
        else if (isAction("setTrayOption", 1)) {
            unsigned trayopt(trayOptions.parseExpression(getArg()));
            check(trayOptions, trayopt, argp[-1]);

            MSG(("setTrayOption: %d", trayopt));
            FOREACH_WINDOW(window)
                setTrayHint(window, trayopt);
        }
        else if (isAction("getTrayOption", 0)) {
            FOREACH_WINDOW(window)
                getTrayOption(window);
        }
        else if (isAction("setType", 1)) {
            setWindowType(getArg());
        }
        else if (isAction("getType", 0)) {
            listWindowType();
        }
        else if (isAction("setWindowGravity", 1)) {
            long grav(gravities.parseExpression(getArg()));
            check(gravities, grav, argp[-1]);
            FOREACH_WINDOW(window)
                setWindowGravity(window, grav);
        }
        else if (isAction("getWindowGravity", 0)) {
            FOREACH_WINDOW(window)
                printWindowGravity(window);
        }
        else if (isAction("setBitGravity", 1)) {
            long grav(gravities.parseExpression(getArg()));
            check(gravities, grav, argp[-1]);
            FOREACH_WINDOW(window)
                setBitGravity(window, grav);
        }
        else if (isAction("getBitGravity", 0)) {
            FOREACH_WINDOW(window)
                printBitGravity(window);
        }
        else if (isAction("setNormalGravity", 1)) {
            long grav(gravities.parseExpression(getArg()));
            check(gravities, grav, argp[-1]);
            FOREACH_WINDOW(window)
                setNormalGravity(window, grav);
        }
        else if (isAction("getNormalGravity", 0)) {
            FOREACH_WINDOW(window)
                printNormalGravity(window);
        }
        else if (isAction("id", 0)) {
            FOREACH_WINDOW(window)
                printf("0x%07lx\n", Window(window));
        }
        else if (isAction("frame", 0)) {
            FOREACH_WINDOW(window) {
                use(window);
                printf("0x%07lx\n", getFrameWindow(window));
            }
        }
        else if (isAction("pid", 0)) {
            FOREACH_WINDOW(window) {
                long pid = *YCardinal(window, ATOM_NET_WM_PID);
                if (1 < pid) {
                    printf("%ld\n", pid);
                }
            }
        }
        else if (isAction("list", 0)) {
            detail();
        }
        else if (isAction("extents", 0)) {
            extents();
        }
        else if (isAction("spy", 0)) {
            spy();
        }
        else if (isAction("close", 0)) {
            CHANGES_WINDOW(window)
                closeWindow(window);
        }
        else if (isAction("kill", 0)) {
            CHANGES_WINDOW(window)
                XKillClient(display, window);
        }
        else if (isAction("activate", 0)) {
            CHANGES_WINDOW(window)
                activateWindow(window);
        }
        else if (isAction("raise", 0)) {
            CHANGES_WINDOW(window)
                raiseWindow(window);
        }
        else if (isAction("lower", 0)) {
            CHANGES_WINDOW(window)
                lowerWindow(window);
        }
        else if (isAction("fullscreen", 0)) {
            FOREACH_WINDOW(window) {
                use(window);
                YNetState(window) += NetFullscreen;
                modified(window);
            }
        }
        else if (isAction("maximize", 0)) {
            FOREACH_WINDOW(window) {
                use(window);
                YNetState(window) += NetHorizontal | NetVertical;
                modified(window);
            }
        }
        else if (isAction("minimize", 0)) {
            CHANGES_WINDOW(window)
                YNetState(window) += NetHidden;
        }
        else if (isAction("vertical", 0)) {
            CHANGES_WINDOW(window)
                YNetState(window) += NetVertical;
        }
        else if (isAction("horizontal", 0)) {
            CHANGES_WINDOW(window)
                YNetState(window) += NetHorizontal;
        }
        else if (isAction("rollup", 0)) {
            CHANGES_WINDOW(window)
                YNetState(window) += NetShaded;
        }
        else if (isAction("above", 0)) {
            CHANGES_WINDOW(window)
                YNetState(window) += NetAbove;
        }
        else if (isAction("below", 0)) {
            CHANGES_WINDOW(window)
                YNetState(window) += NetBelow;
        }
        else if (isAction("urgent", 0)) {
            FOREACH_WINDOW(window)
                YNetState(window) += NetDemands;
        }
        else if (isAction("hide", 0)) {
            changeState(WithdrawnState);
        }
        else if (isAction("unhide", 0)) {
            changeState(NormalState);
        }
        else if (isAction("xmap", 0)) {
            CHANGES_WINDOW(window)
                XMapWindow(display, window);
        }
        else if (isAction("xunmap", 0)) {
            CHANGES_WINDOW(window)
                XUnmapWindow(display, window);
        }
        else if (isAction("xmove", 2)) {
            const char* xs = getArg();
            const char* ys = getArg();
            long lx, ly;
            if (tolong(xs, lx) && tolong(ys, ly)) {
                FOREACH_WINDOW(window) {
                    use(window);
                    XMoveWindow(display, window, int(lx), int(ly));
                    modified(window);
                }
            } else {
                invalidArgument("xmove parameters");
            }
        }
        else if (isAction("xsize", 2)) {
            const char* xs = getArg();
            const char* ys = getArg();
            long lx, ly;
            if (tolong(xs, lx) && tolong(ys, ly)) {
                FOREACH_WINDOW(window) {
                    use(window);
                    XResizeWindow(display, window, unsigned(lx), unsigned(ly));
                    modified(window);
                }
            } else {
                invalidArgument("xsize parameters");
            }
        }
        else if (isAction("skip", 0)) {
            FOREACH_WINDOW(window)
                YNetState(window) += NetSkipPager | NetSkipTaskbar;
        }
        else if (isAction("unskip", 0)) {
            FOREACH_WINDOW(window)
                YNetState(window) -= NetSkipPager | NetSkipTaskbar;
        }
        else if (isAction("restore", 0)) {
            FOREACH_WINDOW(window)
                YNetState(window) -= NetFullscreen | NetShaded | NetDemands |
                                     NetHorizontal | NetVertical;
            changeState(NormalState);
        }
        else if (isAction("denormal", 0)) {
            FOREACH_WINDOW(window) {
                use(window);
                XDeleteProperty(display, window, XA_WM_NORMAL_HINTS);
                modified(window);
            }
        }
        else if (isAction("opacity", 0)) {
            char* opaq = nullptr;
            if (haveArg()) {
                if (isDigit(**argp)) {
                    opaq = getArg();
                }
                else if (**argp == '.' && isDigit(argp[0][1])) {
                    opaq = getArg();
                }
            }
            FOREACH_WINDOW(window)
                opacity(window, opaq);
        }
        else if (isAction("override", 0)) {
            char* flag = nullptr;
            bool enable = false;
            if (haveArg() && isDigit(**argp)) {
                flag = getArg();
                if (strcmp(flag, "1") == 0) {
                    enable = true;
                } else if (strcmp(flag, "0")) {
                    msg(_("Invalid argument: `%s'"), flag);
                    throw 1;
                }
            }
            FOREACH_WINDOW(window) {
                if (flag) {
                    unsigned long mask = CWOverrideRedirect;
                    XSetWindowAttributes attr;
                    attr.override_redirect = enable;
                    XChangeWindowAttributes(display, window, mask, &attr);
                } else {
                    XWindowAttributes attr;
                    if (XGetWindowAttributes(display, window, &attr)) {
                        printf("0x%-8lx override %d\n",
                                *window, attr.override_redirect);
                    }
                }
            }
        }
        else if (isAction("focusmodel", 0)) {
            FOREACH_WINDOW(window) {
                xsmart<XWMHints> h(XGetWMHints(display, window));
                if (h) {
                    bool input = (h->flags & InputHint) && (h->input & True);
                    Atom* prot = nullptr;
                    int count = 0;
                    bool take = false;
                    if (XGetWMProtocols(display, window, &prot, &count)) {
                        for (int i = 0; i < count; ++i) {
                            if (prot[i] == ATOM_WM_TAKE_FOCUS) {
                                take = true;
                            }
                        }
                    }
                    printf("0x%-8lx focusmodel %s\n", *window,
                            input ? take ? "Locally" : "Passive"
                                : take ? "Globally" : "NoInput");
                }
            }
        }
        else if (isAction("motif", 0)) {
            char** args = argp;
            int count = 0;
            while (args + count < argv + argc) {
                int more = (argv + argc) - (args + count);
                if (0 == strcmp(args[count], "remove")) {
                    ++count;
                }
                else if (0 == strcmp(args[count], "funcs") && 2 <= more) {
                    count += 2;
                }
                else if (0 == strcmp(args[count], "decor") && 2 <= more) {
                    count += 2;
                }
                else {
                    break;
                }
            }
            FOREACH_WINDOW(window) {
                use(window);
                motif(window, args, count);
            }
            argp = args + count;
        }
        else if (isAction("properties", 0)) {
            FOREACH_WINDOW(window) {
                use(window);
                int count = 0;
                Atom* atoms = XListProperties(display, window, &count);
                if (count && atoms) {
                    char buf[32];
                    snprintf(buf, sizeof buf, "0x%07x ", unsigned(window));
                    for (int i = 0; i < count; ++i) {
                        showProperty(window, atoms[i], buf);
                    }
                    XFree(atoms);
                }
            }
        }
        else if (isAction("prop", 1)) {
            NAtom prop(getArg(), true);
            if (prop) {
                FOREACH_WINDOW(window) {
                    use(window);
                    char buf[32];
                    snprintf(buf, sizeof buf, "0x%07lx ", Window(window));
                    showProperty(window, prop, buf);
                }
            }
        }
        else if (isAction("click", 3)) {
            click();
        }
        else if (isAction("changeState", 1)) {
            changeState(getArg());
        }
        else if (isAction("iconic", 0)) {
            changeState(IconicState);
        }
        else if (isAction("normal", 0)) {
            changeState(NormalState);
        }
        else if (isAction("monitors", 4)) {
            monitors();
        }
        else if (isAction("tabto", 1)) {
            tabTo(getArg());
        }
        else if (isAction("untab", 0)) {
            tabTo(nullptr);
        }
        else if (isAction("stacking", 0)) {
            windowList.stacking();
        }
        else if (isAction("reverse", 0)) {
            windowList.reverse();
        }
        else {
            msg(_("Unknown action: `%s'"), *argp);
            throw 1;
        }
    }
}

IceSh::~IceSh()
{
    if (display) {
        XSync(display, False);
        XCloseDisplay(display);
        display = nullptr;
        root = None;
        NAtom::free();
    }
}

int main(int argc, char **argv) {
    setvbuf(stdout, nullptr, _IOLBF, BUFSIZ);
    setvbuf(stderr, nullptr, _IOLBF, BUFSIZ);
    signal(SIGHUP, SIG_IGN);

#ifdef CONFIG_I18N
    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);
#endif

    check_argv(argc, argv, get_help_text, VERSION);

    return IceSh(argc, argv);
}

// vim: set sw=4 ts=4 et:
