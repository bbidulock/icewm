#include "config.h"

#include "yxapp.h"

#include "yfull.h"

#include "wmprog.h"
#include "wmmgr.h"
#include "MwmUtil.h"
#include "prefs.h"
#include "yprefs.h"
#include "ypixbuf.h"
#include "yconfig.h"

#include <sys/resource.h>
#include <stdlib.h>

#include "intl.h"

YXApplication *xapp = 0;

YDesktop *desktop = 0;
XContext windowContext;

YCursor YXApplication::leftPointer;
YCursor YXApplication::rightPointer;
YCursor YXApplication::movePointer;

Atom _XA_WM_PROTOCOLS;
Atom _XA_WM_TAKE_FOCUS;
Atom _XA_WM_DELETE_WINDOW;
Atom _XA_WM_STATE;
Atom _XA_WM_CHANGE_STATE;
Atom _XATOM_MWM_HINTS;
//Atom _XA_MOTIF_WM_INFO;!!!
Atom _XA_WM_COLORMAP_WINDOWS;
Atom _XA_WM_CLIENT_LEADER;
Atom _XA_WM_WINDOW_ROLE;
Atom _XA_WINDOW_ROLE;
Atom _XA_SM_CLIENT_ID;
Atom _XA_ICEWM_ACTION;
Atom _XA_CLIPBOARD;
Atom _XA_TARGETS;

Atom _XA_WIN_PROTOCOLS;
Atom _XA_WIN_WORKSPACE;
Atom _XA_WIN_WORKSPACE_COUNT;
Atom _XA_WIN_WORKSPACE_NAMES;
Atom _XA_WIN_WORKAREA;
#ifdef CONFIG_TRAY
Atom _XA_WIN_TRAY;
#endif
Atom _XA_WIN_ICONS;
Atom _XA_WIN_STATE;
Atom _XA_WIN_LAYER;
Atom _XA_WIN_HINTS;
Atom _XA_WIN_SUPPORTING_WM_CHECK;
Atom _XA_WIN_CLIENT_LIST;
Atom _XA_WIN_DESKTOP_BUTTON_PROXY;
Atom _XA_WIN_AREA;
Atom _XA_WIN_AREA_COUNT;

Atom _XA_NET_SUPPORTED;
Atom _XA_NET_SUPPORTING_WM_CHECK;
Atom _XA_NET_CLIENT_LIST;
Atom _XA_NET_CLIENT_LIST_STACKING;
Atom _XA_NET_NUMBER_OF_DESKTOPS;
Atom _XA_NET_CURRENT_DESKTOP;
Atom _XA_NET_WORKAREA;
Atom _XA_NET_WM_MOVERESIZE;

Atom _XA_NET_WM_STRUT;
Atom _XA_NET_WM_DESKTOP;
Atom _XA_NET_CLOSE_WINDOW;
Atom _XA_NET_ACTIVE_WINDOW;
Atom _XA_NET_WM_STATE;

Atom _XA_NET_WM_STATE_SHADED;
Atom _XA_NET_WM_STATE_MAXIMIZED_VERT;
Atom _XA_NET_WM_STATE_MAXIMIZED_HORZ;
Atom _XA_NET_WM_STATE_SKIP_TASKBAR;
Atom _XA_NET_WM_STATE_HIDDEN;
Atom _XA_NET_WM_STATE_FULLSCREEN;
Atom _XA_NET_WM_STATE_ABOVE;
Atom _XA_NET_WM_STATE_BELOW;
Atom _XA_NET_WM_STATE_MODAL;
Atom _XA_NET_WM_WINDOW_TYPE;
Atom _XA_NET_WM_WINDOW_TYPE_DOCK;
Atom _XA_NET_WM_WINDOW_TYPE_DESKTOP;
Atom _XA_NET_WM_WINDOW_TYPE_SPLASH;

Atom _XA_NET_WM_NAME;
Atom _XA_NET_WM_PID;

Atom _XA_KWM_WIN_ICON;
Atom _XA_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR = 0;

Atom XA_XdndAware;
Atom XA_XdndEnter;
Atom XA_XdndLeave;
Atom XA_XdndPosition;
Atom XA_XdndStatus;
Atom XA_XdndDrop;
Atom XA_XdndFinished;

YColor *YColor::black(NULL);
YColor *YColor::white(NULL);

ref<YPixmap> buttonIPixmap;
ref<YPixmap> buttonAPixmap;

ref<YPixmap> logoutPixmap;
ref<YPixmap> switchbackPixmap;
ref<YPixmap> listbackPixmap;
ref<YPixmap> dialogbackPixmap;

ref<YPixmap> menubackPixmap;
ref<YPixmap> menusepPixmap;
ref<YPixmap> menuselPixmap;

#ifdef CONFIG_GRADIENTS
ref<YPixbuf> buttonIPixbuf;
ref<YPixbuf> buttonAPixbuf;

ref<YPixbuf> logoutPixbuf;
ref<YPixbuf> switchbackPixbuf;
ref<YPixbuf> listbackPixbuf;
ref<YPixbuf> dialogbackPixbuf;

ref<YPixbuf> menubackPixbuf;
ref<YPixbuf> menuselPixbuf;
ref<YPixbuf> menusepPixbuf;
#endif

//changed robc
ref<YPixmap> closePixmap[3];
ref<YPixmap> minimizePixmap[3];
ref<YPixmap> maximizePixmap[3];
ref<YPixmap> restorePixmap[3];
ref<YPixmap> hidePixmap[3];
ref<YPixmap> rollupPixmap[3];
ref<YPixmap> rolldownPixmap[3];
ref<YPixmap> depthPixmap[3];

#ifdef CONFIG_SHAPE
int shapesSupported;
int shapeEventBase, shapeErrorBase;
#endif

#ifdef CONFIG_XRANDR
int xrandrSupported;
int xrandrEventBase, xrandrErrorBase;
#endif

#ifdef DEBUG
int xeventcount = 0;
#endif



class YClipboard: public YWindow {
public:
    YClipboard(): YWindow() {
        fLen = 0;
        fData = 0;
    }
    ~YClipboard() {
        if (fData)
            delete [] fData;
        fData = 0;
        fLen = 0;
    }

    void setData(char *data, int len) {
        if (fData)
            delete [] fData;
        fLen = len;
        fData = new char[len];
        if (fData)
            memcpy(fData, data, len);
        if (fLen == 0)
            clearSelection(false);
        else
            acquireSelection(false);
    }
    void handleSelectionClear(const XSelectionClearEvent &clear) {
        if (clear.selection == _XA_CLIPBOARD) {
            if (fData)
                delete [] fData;
            fLen = 0;
            fData = 0;
        }
    }
    void handleSelectionRequest(const XSelectionRequestEvent &request) {
        if (request.selection == _XA_CLIPBOARD) {
            XSelectionEvent notify;

            notify.type = SelectionNotify;
            notify.requestor = request.requestor;
            notify.selection = request.selection;
            notify.target = request.target;
            notify.time = request.time;
            notify.property = request.property;

            if (request.selection == _XA_CLIPBOARD &&
                request.target == XA_STRING &&
                fLen > 0)
            {
                XChangeProperty(xapp->display(),
                                request.requestor,
                                request.property,
                                request.target,
                                8, PropModeReplace,
                                (unsigned char *)(fData ? fData : ""),
                                fLen);
            } else if (request.selection == _XA_CLIPBOARD &&
                       request.target == _XA_TARGETS &&
                       fLen > 0)
            {
                Atom type = XA_STRING;

                XChangeProperty(xapp->display(),
                                request.requestor,
                                request.property,
                                request.target,
                                32, PropModeReplace,
                                (unsigned char *)&type, 1);
            } else {
                notify.property = None;
            }

            XSendEvent(xapp->display(), notify.requestor, False, 0L, (XEvent *)&notify);
        }
    }

private:
    int fLen;
    char *fData;
};


static void initAtoms() {
    struct {
        Atom *atom;
        const char *name;
    } atom_info[] = {
        { &_XA_WM_PROTOCOLS, "WM_PROTOCOLS" },
        { &_XA_WM_TAKE_FOCUS, "WM_TAKE_FOCUS" },
        { &_XA_WM_DELETE_WINDOW, "WM_DELETE_WINDOW" },
        { &_XA_WM_STATE, "WM_STATE" },
        { &_XA_WM_CHANGE_STATE, "WM_CHANGE_STATE" },
        { &_XA_WM_COLORMAP_WINDOWS, "WM_COLORMAP_WINDOWS" },
        { &_XA_WM_CLIENT_LEADER, "WM_CLIENT_LEADER" },
        { &_XA_WINDOW_ROLE, "WINDOW_ROLE" },
        { &_XA_WM_WINDOW_ROLE, "WM_WINDOW_ROLE" },
        { &_XA_SM_CLIENT_ID, "SM_CLIENT_ID" },
        { &_XA_ICEWM_ACTION, "_ICEWM_ACTION" },
        { &_XATOM_MWM_HINTS, _XA_MOTIF_WM_HINTS },
        { &_XA_KWM_WIN_ICON, "KWM_WIN_ICON" },
        { &_XA_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR, "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR" },
        { &_XA_WIN_WORKSPACE, XA_WIN_WORKSPACE },
        { &_XA_WIN_WORKSPACE_COUNT, XA_WIN_WORKSPACE_COUNT },
        { &_XA_WIN_WORKSPACE_NAMES, XA_WIN_WORKSPACE_NAMES },
        { &_XA_WIN_WORKAREA, XA_WIN_WORKAREA },
        { &_XA_WIN_ICONS, XA_WIN_ICONS },
        { &_XA_WIN_LAYER, XA_WIN_LAYER },
#ifdef CONFIG_TRAY
        { &_XA_WIN_TRAY, XA_WIN_TRAY },
#endif
        { &_XA_WIN_STATE, XA_WIN_STATE },
        { &_XA_WIN_HINTS, XA_WIN_HINTS },
        { &_XA_WIN_PROTOCOLS, XA_WIN_PROTOCOLS },
        { &_XA_WIN_SUPPORTING_WM_CHECK, XA_WIN_SUPPORTING_WM_CHECK },
        { &_XA_WIN_CLIENT_LIST, XA_WIN_CLIENT_LIST },
        { &_XA_WIN_DESKTOP_BUTTON_PROXY, XA_WIN_DESKTOP_BUTTON_PROXY },
        { &_XA_WIN_AREA, XA_WIN_AREA },
        { &_XA_WIN_AREA_COUNT, XA_WIN_AREA_COUNT },

        { &_XA_NET_SUPPORTED, "_NET_SUPPORTED" },
        { &_XA_NET_SUPPORTING_WM_CHECK, "_NET_SUPPORTING_WM_CHECK" },
        { &_XA_NET_CLIENT_LIST, "_NET_CLIENT_LIST" },
        { &_XA_NET_CLIENT_LIST_STACKING, "_NET_CLIENT_LIST_STACKING" },
        { &_XA_NET_NUMBER_OF_DESKTOPS, "_NET_NUMBER_OF_DESKTOPS" },
        { &_XA_NET_CURRENT_DESKTOP, "_NET_CURRENT_DESKTOP" },
        { &_XA_NET_WORKAREA, "_NET_WORKAREA" },
        { &_XA_NET_WM_MOVERESIZE, "_NET_WM_MOVERESIZE" },

        { &_XA_NET_WM_STRUT, "_NET_WM_STRUT" },
        { &_XA_NET_WM_DESKTOP, "_NET_WM_DESKTOP" },
        { &_XA_NET_CLOSE_WINDOW, "_NET_CLOSE_WINDOW" },
        { &_XA_NET_ACTIVE_WINDOW, "_NET_ACTIVE_WINDOW" },
        { &_XA_NET_WM_STATE, "_NET_WM_STATE" },

        { &_XA_NET_WM_STATE_SHADED, "_NET_WM_STATE_SHADED" },
        { &_XA_NET_WM_STATE_MAXIMIZED_VERT, "_NET_WM_STATE_MAXIMIZED_VERT" },
        { &_XA_NET_WM_STATE_MAXIMIZED_HORZ, "_NET_WM_STATE_MAXIMIZED_HORZ" },
        { &_XA_NET_WM_STATE_SKIP_TASKBAR, "_NET_WM_STATE_SKIP_TASKBAR" },
        { &_XA_NET_WM_STATE_HIDDEN, "_NET_WM_STATE_HIDDEN" },
        { &_XA_NET_WM_STATE_FULLSCREEN, "_NET_WM_STATE_FULLSCREEN" },
        { &_XA_NET_WM_STATE_ABOVE, "_NET_WM_STATE_ABOVE" },
        { &_XA_NET_WM_STATE_BELOW, "_NET_WM_STATE_BELOW" },
        { &_XA_NET_WM_STATE_MODAL, "_NET_WM_STATE_MODAL" },
        { &_XA_NET_WM_WINDOW_TYPE, "_NET_WM_WINDOW_TYPE" },
        { &_XA_NET_WM_WINDOW_TYPE_DOCK, "_NET_WM_WINDOW_TYPE_DOCK" },
        { &_XA_NET_WM_WINDOW_TYPE_DESKTOP, "_NET_WM_WINDOW_TYPE_DESKTOP" },
        { &_XA_NET_WM_WINDOW_TYPE_SPLASH, "_NET_WM_WINDOW_TYPE_SPLASH" },

        { &_XA_NET_WM_NAME, "_NET_WM_NAME" },
        { &_XA_NET_WM_PID, "_NET_WM_PID" },

        { &_XA_CLIPBOARD, "CLIPBOARD" },
        { &_XA_TARGETS, "TARGETS" },
        { &XA_XdndAware, "XdndAware" },
        { &XA_XdndEnter, "XdndEnter" },
        { &XA_XdndLeave, "XdndLeave" },
        { &XA_XdndPosition, "XdndPosition" },
        { &XA_XdndStatus, "XdndStatus" },
        { &XA_XdndDrop, "XdndDrop" },
        { &XA_XdndFinished, "XdndFinished" }
    };
    unsigned int i;

#ifdef HAVE_XINTERNATOMS
    const char *names[ACOUNT(atom_info)];
    Atom atoms[ACOUNT(atom_info)];

    for (i = 0; i < ACOUNT(atom_info); i++)
        names[i] = atom_info[i].name;

    XInternAtoms(xapp->display(), (char **)names, ACOUNT(atom_info), False, atoms);

    for (i = 0; i < ACOUNT(atom_info); i++)
        *(atom_info[i].atom) = atoms[i];
#else
    for (i = 0; i < ACOUNT(atom_info); i++)
        *(atom_info[i].atom) = XInternAtom(xapp->display(),
                                           atom_info[i].name, False);
#endif
}

static void initPointers() {
    YXApplication::leftPointer.load("left.xpm",  XC_left_ptr);
    YXApplication::rightPointer.load("right.xpm", XC_right_ptr);
    YXApplication::movePointer.load("move.xpm",  XC_fleur);
}

static void initColors() {
    YColor::black = new YColor("rgb:00/00/00");
    YColor::white = new YColor("rgb:FF/FF/FF");
}

void YXApplication::initModifiers() {
    XModifierKeymap *xmk = XGetModifierMapping(xapp->display());
    AltMask = MetaMask = WinMask = SuperMask = HyperMask =
        NumLockMask = ScrollLockMask = ModeSwitchMask = 0;

    if (xmk) {
        KeyCode *c = xmk->modifiermap;

        for (int m = 0; m < 8; m++)
            for (int k = 0; k < xmk->max_keypermod; k++, c++) {
                if (*c == NoSymbol)
                    continue;
                KeySym kc = XKeycodeToKeysym(xapp->display(), *c, 0);
                if (kc == NoSymbol)
                    kc = XKeycodeToKeysym(xapp->display(), *c, 1);
                if (kc == XK_Num_Lock && NumLockMask == 0)
                    NumLockMask = (1 << m);
                if (kc == XK_Scroll_Lock && ScrollLockMask == 0)
                    ScrollLockMask = (1 << m);
                if ((kc == XK_Alt_L || kc == XK_Alt_R) && AltMask == 0)
                    AltMask = (1 << m);
                if ((kc == XK_Meta_L || kc == XK_Meta_R) && MetaMask == 0)
                    MetaMask = (1 << m);
                if ((kc == XK_Super_L || kc == XK_Super_R) && SuperMask == 0)
                    SuperMask = (1 << m);
                if ((kc == XK_Hyper_L || kc == XK_Hyper_R) && HyperMask == 0)
                    HyperMask = (1 << m);
                if (kc == XK_Mode_switch && ModeSwitchMask == 0)
                    ModeSwitchMask = (1 << m);
            }

        XFreeModifiermap(xmk);
    }
    if (MetaMask == AltMask)
        MetaMask = 0;

    MSG(("alt:%d meta:%d super:%d hyper:%d mode:%d num:%d scroll:%d",
         AltMask, MetaMask, SuperMask, HyperMask, ModeSwitchMask,
         NumLockMask, ScrollLockMask));

    // some hacks for "broken" modifier configurations
    if (HyperMask == SuperMask)
        HyperMask = 0;

    // this basically does what <0.9.13 versions did
    if (AltMask != 0 && MetaMask == Mod1Mask) {
        MetaMask = AltMask;
        AltMask = Mod1Mask;
    }

    if (AltMask == 0 && MetaMask != 0) {
        if (MetaMask != Mod1Mask) {
            AltMask = Mod1Mask;
        }
        else {
            AltMask = MetaMask;
            MetaMask = 0;
        }
    }

    if (AltMask == 0)
        AltMask = Mod1Mask;

    PRECONDITION(xapp->AltMask != 0);
    PRECONDITION(xapp->AltMask != ShiftMask);
    PRECONDITION(xapp->AltMask != ControlMask);
    PRECONDITION(xapp->AltMask != xapp->MetaMask);

    KeyMask =
        ControlMask |
        ShiftMask |
        AltMask |
        MetaMask |
        SuperMask |
        HyperMask |
        ModeSwitchMask;

    ButtonMask =
        Button1Mask |
        Button2Mask |
        Button3Mask |
        Button4Mask |
        Button5Mask;

    ButtonKeyMask = KeyMask | ButtonMask;

#if 0
    KeySym wl = XKeycodeToKeysym(app->display(), 115, 0);
    KeySym wr = XKeycodeToKeysym(app->display(), 116, 0);

    if (wl == XK_Super_L) {
    } else if (wl == XK_Meta_L) {
    }
#endif
    // this will do for now, but we should actualy check the keycodes
    Win_L = Win_R = 0;

    if (SuperMask != 0) {
        WinMask = SuperMask;

        Win_L = XK_Super_L;
        Win_R = XK_Super_R;
    }
    MSG(("alt:%d meta:%d super:%d hyper:%d win:%d mode:%d num:%d scroll:%d",
         AltMask, MetaMask, SuperMask, HyperMask, WinMask, ModeSwitchMask,
         NumLockMask, ScrollLockMask));

}

void YXApplication::dispatchEvent(YWindow *win, XEvent &xev) {
    if (xev.type == KeyPress || xev.type == KeyRelease) {
        YWindow *w = win;
        while (w && (w->handleKey(xev.xkey) == false)) {
            if (fGrabTree && w == fXGrabWindow)
                break;
            w = w->parent();
        }
    } else {
        Window child;

        if (xev.type == MotionNotify) {
            if (xev.xmotion.window != win->handle())
                if (XTranslateCoordinates(xapp->display(),
                                          xev.xany.window, win->handle(),
                                          xev.xmotion.x, xev.xmotion.y,
                                          &xev.xmotion.x, &xev.xmotion.y, &child) == True)
                    xev.xmotion.window = win->handle();
                else
                    return ;
        } else if (xev.type == ButtonPress || xev.type == ButtonRelease ||
                   xev.type == EnterNotify || xev.type == LeaveNotify)
        {
            if (xev.xbutton.window != win->handle())
                if (XTranslateCoordinates(xapp->display(),
                                          xev.xany.window, win->handle(),
                                          xev.xbutton.x, xev.xbutton.y,
                                          &xev.xbutton.x, &xev.xbutton.y, &child) == True)
                    xev.xbutton.window = win->handle();
                else
                    return ;
        } else if (xev.type == KeyPress || xev.type == KeyRelease) {
            if (xev.xkey.window != win->handle())
                if (XTranslateCoordinates(xapp->display(),
                                          xev.xany.window, win->handle(),
                                          xev.xkey.x, xev.xkey.y,
                                          &xev.xkey.x, &xev.xkey.y, &child) == True)
                    xev.xkey.window = win->handle();
                else
                    return ;
        }
        win->handleEvent(xev);
    }
}

void YXApplication::handleGrabEvent(YWindow *winx, XEvent &xev) {
    YWindow *win = winx;

    PRECONDITION(win != 0);
    if (fGrabTree) {
        if (xev.xbutton.subwindow != None) {
            if (XFindContext(display(),
                         xev.xbutton.subwindow,
                         windowContext,
                         (XPointer *)&win) != 0);
                if (xev.type == EnterNotify || xev.type == LeaveNotify)
                    win = 0;
                else
                    win = fGrabWindow;
        } else {
            if (XFindContext(display(),
                         xev.xbutton.window,
                         windowContext,
                         (XPointer *)&win) != 0)
                if (xev.type == EnterNotify || xev.type == LeaveNotify)
                    win = 0;
                else
                    win = fGrabWindow;
        }
        if (win == 0)
            return ;
        {
            YWindow *p = win;
            while (p) {
                if (p == fXGrabWindow)
                    break;
                p = p->parent();
            }
            if (p == 0) {
                if (xev.type == EnterNotify || xev.type == LeaveNotify)
                    return ;
                else
                    win = fGrabWindow;
            }
        }
        if (xev.type == EnterNotify || xev.type == LeaveNotify)
            if (win != fGrabWindow)
                return ;
        if (fGrabWindow != fXGrabWindow)
            win = fGrabWindow;
    }
    dispatchEvent(win, xev);
}

void YXApplication::replayEvent() {
    if (!fReplayEvent) {
        fReplayEvent = true;
        XAllowEvents(xapp->display(), ReplayPointer, CurrentTime);
    }
}

void YXApplication::captureGrabEvents(YWindow *win) {
    if (fGrabWindow == fXGrabWindow && fGrabTree) {
        fGrabWindow = win;
    }
}

void YXApplication::releaseGrabEvents(YWindow *win) {
    if (win == fGrabWindow && fGrabTree) {
        fGrabWindow = fXGrabWindow;
    }
}

int YXApplication::grabEvents(YWindow *win, Cursor ptr, unsigned int eventMask, int grabMouse, int grabKeyboard, int grabTree) {
    int rc;

    if (fGrabWindow != 0)
        return 0;
    if (win == 0)
        return 0;

    XSync(display(), 0);
    fGrabTree = grabTree;
    if (grabMouse) {
        fGrabMouse = 1;
        rc = XGrabPointer(display(), win->handle(),
                          grabTree ? True : False,
                          eventMask,
                          GrabModeSync, GrabModeAsync,
                          None, ptr, CurrentTime);

        if (rc != Success) {
            MSG(("grab status = %d\x7", rc));
            return 0;
        }
    } else {
        fGrabMouse = 0;

        XChangeActivePointerGrab(display(),
                                 eventMask,
                                 ptr, CurrentTime);
    }

    if (grabKeyboard) {
        rc = XGrabKeyboard(display(), win->handle(),
                           ///False,
                           grabTree ? True : False,
                           GrabModeSync, GrabModeAsync, CurrentTime);
        if (rc != Success && grabMouse) {
            MSG(("grab status = %d\x7", rc));
            XUngrabPointer(display(), CurrentTime);
            return 0;
        }
    }
    XAllowEvents(xapp->display(), SyncPointer, CurrentTime);

    desktop->resetColormapFocus(false);

    fXGrabWindow = win;
    fGrabWindow = win;
    return 1;
}

int YXApplication::releaseEvents() {
    if (fGrabWindow == 0)
        return 0;
    fGrabWindow = 0;
    fXGrabWindow = 0;
    fGrabTree = 0;
    if (fGrabMouse) {
        XUngrabPointer(display(), CurrentTime);
        fGrabMouse = 0;
    }
    XUngrabKeyboard(display(), CurrentTime);
    desktop->resetColormapFocus(true);
    return 1;
}

void YXApplication::afterWindowEvent(XEvent & /*xev*/) {
}

bool YXApplication::filterEvent(const XEvent &xev) {
    if (xev.type == KeymapNotify) {
        XMappingEvent xmapping = xev.xmapping; /// X headers const missing?
        XRefreshKeyboardMapping(&xmapping);

        // !!! we should probably regrab everything ?
        initModifiers();
        return true;
    }
    return false;
}

void YXApplication::saveEventTime(const XEvent &xev) {
    switch (xev.type) {
    case ButtonPress:
    case ButtonRelease:
        lastEventTime = xev.xbutton.time;
        break;

    case MotionNotify:
        lastEventTime = xev.xmotion.time;
        break;

    case KeyPress:
    case KeyRelease:
        lastEventTime = xev.xkey.time;
        break;

    case EnterNotify:
    case LeaveNotify:
        lastEventTime = xev.xcrossing.time;
        break;

    case PropertyNotify:
        lastEventTime = xev.xproperty.time;
        break;

    case SelectionClear:
        lastEventTime = xev.xselectionclear.time;
        break;

    case SelectionRequest:
        lastEventTime = xev.xselectionrequest.time;
        break;

    case SelectionNotify:
        lastEventTime = xev.xselection.time;
        break;
    }
}

Time YXApplication::getEventTime(const char */*debug*/) const {
    return lastEventTime;
}


extern void logEvent(const XEvent &xev);


bool YXApplication::hasColormap() {
    XVisualInfo pattern;
    pattern.screen = DefaultScreen(display());

    int nVisuals;
    bool rc = false;

    XVisualInfo *first_visual(XGetVisualInfo(display(), VisualScreenMask,
                                              &pattern, &nVisuals));
    XVisualInfo *visual = first_visual;

    while(visual && nVisuals--) {
        if (visual->c_class & 1)
            rc = true;
        visual++;
    }

    if (first_visual)
        XFree(first_visual);

    return rc;
}


void YXApplication::alert() {
    XBell(display(), 100);
}

void YXApplication::setClipboardText(char *data, int len) {
    if (fClip == 0)
        fClip = new YClipboard();
    if (!fClip)
        return ;
    fClip->setData(data, len);
}

YXApplication::YXApplication(int *argc, char ***argv, const char *displayName):
    YApplication(argc, argv)
{
    xapp = this;
    lastEventTime = CurrentTime;
    fGrabWindow = 0;
    fGrabTree = 0;
    fXGrabWindow = 0;
    fGrabMouse = 0;
    fPopup = 0;
    fClip = 0;
    fReplayEvent = false;

    bool runSynchronized(false);

    for (char ** arg = *argv + 1; arg < *argv + *argc; ++arg) {
        if (**arg == '-') {
            char *value;

            if ((value = GET_LONG_ARGUMENT("display")) != NULL)
                displayName = value;
            else if (IS_LONG_SWITCH("sync"))
                runSynchronized = true;
        }
    }

    if (displayName == 0)
        displayName = getenv("DISPLAY");
    else {
        static char disp[256] = "DISPLAY=";
        strcat(disp, displayName);
        putenv(disp);
    }

    if (!(fDisplay = XOpenDisplay(displayName)))
        die(1, _("Can't open display: %s. X must be running and $DISPLAY set."),
            displayName ? displayName : _("<none>"));

    if (runSynchronized)
        XSynchronize(display(), True);

    windowContext = XUniqueContext();

    new YDesktop(0, RootWindow(display(), DefaultScreen(display())));
    YPixbuf::init();

    initAtoms();
    initModifiers();
    initPointers();
    initColors();

#ifdef CONFIG_SHAPE
    shapesSupported = XShapeQueryExtension(display(),
                                           &shapeEventBase, &shapeErrorBase);
#endif
#ifdef CONFIG_XRANDR
    xrandrSupported = XRRQueryExtension(display(),
                                           &xrandrEventBase, &xrandrErrorBase);
#endif
}

YXApplication::~YXApplication() {
    XCloseDisplay(display());
    fDisplay = 0;
    xapp = 0;
}

/// TODO #warning "fixme"
extern struct timeval idletime;

bool YXApplication::handleXEvents() {
    static struct timeval prevtime, curtime, difftime, maxtime = { 0, 0 };

    if (XPending(display()) > 0) {
        XEvent xev;

        XNextEvent(display(), &xev);
#ifdef DEBUG
        xeventcount++;
#endif
        //msg("%d", xev.type);

        gettimeofday(&prevtime, 0);
        saveEventTime(xev);

#ifdef DEBUG
        DBG logEvent(xev);
#endif

        if (filterEvent(xev)) {
            ;
        } else {
            int ge = (xev.type == ButtonPress ||
                      xev.type == ButtonRelease ||
                      xev.type == MotionNotify ||
                      xev.type == KeyPress ||
                      xev.type == KeyRelease /*||
                      xev.type == EnterNotify ||
                      xev.type == LeaveNotify*/) ? 1 : 0;

            fReplayEvent = false;

            if (fPopup && ge) {
                handleGrabEvent(fPopup, xev);
            } else if (fGrabWindow && ge) {
                handleGrabEvent(fGrabWindow, xev);
            } else {
                handleWindowEvent(xev.xany.window, xev);
            }
            if (fGrabWindow) {
                if (xev.type == ButtonPress || xev.type == ButtonRelease || xev.type == MotionNotify)
                {
                    if (!fReplayEvent) {
                        XAllowEvents(xapp->display(), SyncPointer, CurrentTime);
                    }
                }
            }
        }
        gettimeofday(&curtime, 0);
        difftime.tv_sec = curtime.tv_sec - idletime.tv_sec;
        difftime.tv_usec = curtime.tv_usec - idletime.tv_usec;
        if (idletime.tv_usec < 0) {
            idletime.tv_sec--;
            idletime.tv_usec += 1000000;
        }
        if (idletime.tv_sec != 0 || idletime.tv_usec > 100000) {
            handleIdle();
            gettimeofday(&curtime, 0);
            memcpy(&idletime, &curtime, sizeof(idletime));
        }

        difftime.tv_sec = curtime.tv_sec - prevtime.tv_sec;
        difftime.tv_usec = curtime.tv_usec - prevtime.tv_usec;
        if (difftime.tv_usec < 0) {
            difftime.tv_sec--;
            difftime.tv_usec += 1000000;
        }
        if (difftime.tv_sec > maxtime.tv_sec ||
            (difftime.tv_sec == maxtime.tv_sec && difftime.tv_usec > maxtime.tv_usec))
        {
            MSG(("max_latency: %d.%06d", difftime.tv_sec, difftime.tv_usec));
            maxtime = difftime;
        }
        return true;
    }
    return false;
}

void YXApplication::handleWindowEvent(Window xwindow, XEvent &xev) {
    int rc = 0;
    YWindow *window = 0;

    if ((rc = XFindContext(display(),
                           xwindow,
                           windowContext,
                           (XPointer *)&window)) == 0)
    {
         window->handleEvent(xev);
    } else {
        if (xev.type == MapRequest) {
	// !!! java seems to do this ugliness
		//YFrameWindow *f = getFrame(xev.xany.window);
		msg("APP BUG? mapRequest for window %lX sent to destroyed frame %lX!",
		    xev.xmaprequest.parent,
		    xev.xmaprequest.window);
		desktop->handleEvent(xev);
	    } else if (xev.type == ConfigureRequest) {
		msg("APP BUG? configureRequest for window %lX sent to destroyed frame %lX!",
		    xev.xmaprequest.parent,
		    xev.xmaprequest.window);
		desktop->handleEvent(xev);
	    } else if (xev.type != DestroyNotify) {
		MSG(("unknown window 0x%lX event=%d", xev.xany.window, xev.type));
	    }
    }
    if (xev.type == KeyPress || xev.type == KeyRelease) ///!!!
        afterWindowEvent(xev);
}

int YXApplication::readFDCheckX() {
    return ConnectionNumber(display());
}

void YXApplication::flushXEvents() {
    XSync(xapp->display(), False);
}
