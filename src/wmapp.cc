/*
 * IceWM
 *
 * Copyright (C) 1997-2003 Marko Macek
 */
#include "config.h"
#define WMAPP
#include "appnames.h"
#include "yfull.h"
#include "wmprog.h"
#include "wmwinmenu.h"
#include "workspaces.h"
#include "wmapp.h"
#include "wmframe.h"
#include "wmswitch.h"
#include "wmstatus.h"
#include "wmabout.h"
#include "wmdialog.h"
#include "wmconfig.h"
#include "wmwinlist.h"
#include "wmtaskbar.h"
#include "wmsession.h"
#include "wpixres.h"
#include "sysdep.h"
#include "ylocale.h"
#include "yprefs.h"
#include "prefs.h"
#include "udir.h"
#include "ascii.h"
#include "ypaths.h"
#include "yxcontext.h"
#ifdef CONFIG_XFREETYPE
#include <ft2build.h>
#include <X11/Xft/Xft.h>
#endif
#undef override
#include <X11/Xproto.h>
#include "ywordexp.h"
#include "intl.h"

char const *ApplicationName("IceWM");
RebootShutdown rebootOrShutdown = Logout;
static bool initializing(true);

YWMApp *wmapp;
YWindowManager *manager;

YCursor YWMApp::sizeRightPointer;
YCursor YWMApp::sizeTopRightPointer;
YCursor YWMApp::sizeTopPointer;
YCursor YWMApp::sizeTopLeftPointer;
YCursor YWMApp::sizeLeftPointer;
YCursor YWMApp::sizeBottomLeftPointer;
YCursor YWMApp::sizeBottomPointer;
YCursor YWMApp::sizeBottomRightPointer;
YCursor YWMApp::scrollLeftPointer;
YCursor YWMApp::scrollRightPointer;
YCursor YWMApp::scrollUpPointer;
YCursor YWMApp::scrollDownPointer;

lazy<MoveMenu> moveMenu;
lazy<LayerMenu> layerMenu;
lazily<SharedWindowList> windowListMenu;
lazy<LogoutMenu> logoutMenu;
lazily<RootMenu> rootMenu;

static bool replace_wm;
static bool post_preferences;
static bool show_extensions;

static Window registerProtocols1(char **argv, int argc) {
    long timestamp = CurrentTime;
    YAtom wmSx("WM_S", true);

    Window current_wm = XGetSelectionOwner(xapp->display(), wmSx);

    if (current_wm != None) {
        if (!replace_wm)
            die(1, _("A window manager is already running, use --replace to replace it"));
      XSetWindowAttributes attrs;
      attrs.event_mask = StructureNotifyMask;
      XChangeWindowAttributes (
          xapp->display(), current_wm,
          CWEventMask, &attrs);
    }

    Window xroot = xapp->root();
    Window xid =
        XCreateSimpleWindow(xapp->display(), xroot,
            0, 0, 1, 1, 0,
            xapp->black(),
            xapp->black());

    XSetSelectionOwner(xapp->display(), wmSx, xid, timestamp);

    if (XGetSelectionOwner(xapp->display(), wmSx) != xid)
        die(1, _("Failed to become the owner of the %s selection"), wmSx.str());

    if (current_wm != None) {
        XEvent event;
        msg(_("Waiting to replace the old window manager"));
        do {
            XWindowEvent(xapp->display(), current_wm,
                         StructureNotifyMask, &event);
        } while (event.type != DestroyNotify);
        msg(_("done."));
    }

    static char wm_class[] = "IceWM";
    static char wm_instance[] = "icewm";

    XClassHint class_hint = {
        argv ? nullptr : wm_instance,
        wm_class
    };

    static char wm_name[] = "IceWM " VERSION " (" HOSTOS "/" HOSTCPU ")";

    Xutf8SetWMProperties(xapp->display(), xid, wm_name, nullptr,
            argv, argc, nullptr, nullptr, &class_hint);

    XClientMessageEvent ev;

    memset(&ev, 0, sizeof(ev));
    ev.type = ClientMessage;
    ev.window = xroot;
    ev.message_type = _XA_MANAGER;
    ev.format = 32;
    ev.data.l[0] = timestamp;
    ev.data.l[1] = wmSx;
    ev.data.l[2] = xid;

    XSendEvent (xapp->display(), xroot, False, StructureNotifyMask, (XEvent*)&ev);
    return xid;
}

static void registerWinProtocols(Window xid) {
    Atom win_proto[] = {
//      _XA_WIN_APP_STATE,
        _XA_WIN_AREA,
        _XA_WIN_AREA_COUNT,
        _XA_WIN_CLIENT_LIST,
        _XA_WIN_DESKTOP_BUTTON_PROXY,
//      _XA_WIN_EXPANDED_SIZE,
        _XA_WIN_HINTS,
        _XA_WIN_ICONS,
        _XA_WIN_LAYER,
        _XA_WIN_PROTOCOLS,
        _XA_WIN_STATE,
        _XA_WIN_SUPPORTING_WM_CHECK,
        _XA_WIN_TRAY,
        _XA_WIN_WORKAREA,
        _XA_WIN_WORKSPACE,
        _XA_WIN_WORKSPACE_COUNT,
        _XA_WIN_WORKSPACE_NAMES
    };
    int win_count = int ACOUNT(win_proto);

    XChangeProperty(xapp->display(), desktop->handle(),
                    _XA_WIN_PROTOCOLS, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)win_proto, win_count);
}

static void registerWinProperties(Window xid) {
    XChangeProperty(xapp->display(), xid,
                    _XA_WIN_SUPPORTING_WM_CHECK, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&xid, 1);

    XChangeProperty(xapp->display(), desktop->handle(),
                    _XA_WIN_SUPPORTING_WM_CHECK, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&xid, 1);

    unsigned long ac[2] = { 1, 1 };
    unsigned long ca[2] = { 0, 0 };

    XChangeProperty(xapp->display(), desktop->handle(),
                    _XA_WIN_AREA_COUNT, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&ac, 2);
    XChangeProperty(xapp->display(), desktop->handle(),
                    _XA_WIN_AREA, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&ca, 2);
}

static void registerNetProtocols(Window xid) {
    Atom net_proto[] = {
        _XA_NET_ACTIVE_WINDOW,
        _XA_NET_CLIENT_LIST,
        _XA_NET_CLIENT_LIST_STACKING,
        _XA_NET_CLOSE_WINDOW,
        _XA_NET_CURRENT_DESKTOP,
        _XA_NET_DESKTOP_GEOMETRY,
        _XA_NET_DESKTOP_LAYOUT,
        _XA_NET_DESKTOP_NAMES,
        _XA_NET_DESKTOP_VIEWPORT,
        _XA_NET_FRAME_EXTENTS,
        _XA_NET_MOVERESIZE_WINDOW,
        _XA_NET_NUMBER_OF_DESKTOPS,
//      _XA_NET_PROPERTIES,
        _XA_NET_REQUEST_FRAME_EXTENTS,
        _XA_NET_RESTACK_WINDOW,
        _XA_NET_SHOWING_DESKTOP,
        _XA_NET_STARTUP_ID,
//      _XA_NET_STARTUP_INFO,
//      _XA_NET_STARTUP_INFO_BEGIN,
        _XA_NET_SUPPORTED,
        _XA_NET_SUPPORTING_WM_CHECK,
        _XA_NET_SYSTEM_TRAY_MESSAGE_DATA,
        _XA_NET_SYSTEM_TRAY_OPCODE,
        _XA_NET_SYSTEM_TRAY_ORIENTATION,
        _XA_NET_SYSTEM_TRAY_VISUAL,
//      _XA_NET_VIRTUAL_ROOTS,
        _XA_NET_WM_ACTION_ABOVE,
        _XA_NET_WM_ACTION_BELOW,
        _XA_NET_WM_ACTION_CHANGE_DESKTOP,
        _XA_NET_WM_ACTION_CLOSE,
        _XA_NET_WM_ACTION_FULLSCREEN,
        _XA_NET_WM_ACTION_MAXIMIZE_HORZ,
        _XA_NET_WM_ACTION_MAXIMIZE_VERT,
        _XA_NET_WM_ACTION_MINIMIZE,
        _XA_NET_WM_ACTION_MOVE,
        _XA_NET_WM_ACTION_RESIZE,
        _XA_NET_WM_ACTION_SHADE,
        _XA_NET_WM_ACTION_STICK,
        _XA_NET_WM_ALLOWED_ACTIONS,
//      _XA_NET_WM_BYPASS_COMPOSITOR,
        _XA_NET_WM_DESKTOP,
//      _XA_NET_WM_FULL_PLACEMENT,
        _XA_NET_WM_FULLSCREEN_MONITORS,
        _XA_NET_WM_HANDLED_ICONS,           // trivial support
//      _XA_NET_WM_ICON_GEOMETRY,
        _XA_NET_WM_ICON_NAME,
        _XA_NET_WM_ICON,
        _XA_NET_WM_MOVERESIZE,
        _XA_NET_WM_NAME,
//      _XA_NET_WM_OPAQUE_REGION,
        _XA_NET_WM_PID,                     // trivial support
        _XA_NET_WM_PING,
        _XA_NET_WM_STATE,
        _XA_NET_WM_STATE_ABOVE,
        _XA_NET_WM_STATE_BELOW,
        _XA_NET_WM_STATE_DEMANDS_ATTENTION,
        _XA_NET_WM_STATE_FOCUSED,
        _XA_NET_WM_STATE_FULLSCREEN,
        _XA_NET_WM_STATE_HIDDEN,
        _XA_NET_WM_STATE_MAXIMIZED_HORZ,
        _XA_NET_WM_STATE_MAXIMIZED_VERT,
        _XA_NET_WM_STATE_MODAL,
        _XA_NET_WM_STATE_SHADED,
        _XA_NET_WM_STATE_SKIP_PAGER,        // trivial support
        _XA_NET_WM_STATE_SKIP_TASKBAR,
        _XA_NET_WM_STATE_STICKY,            // trivial support
        _XA_NET_WM_STRUT,
        _XA_NET_WM_STRUT_PARTIAL,           // trivial support
//      _XA_NET_WM_SYNC_REQUEST,
//      _XA_NET_WM_SYNC_REQUEST_COUNTER,
        _XA_NET_WM_USER_TIME,
        _XA_NET_WM_USER_TIME_WINDOW,
        _XA_NET_WM_VISIBLE_ICON_NAME,       // trivial support
        _XA_NET_WM_VISIBLE_NAME,            // trivial support
        _XA_NET_WM_WINDOW_OPACITY,
        _XA_NET_WM_WINDOW_TYPE,
        _XA_NET_WM_WINDOW_TYPE_COMBO,
        _XA_NET_WM_WINDOW_TYPE_DESKTOP,
        _XA_NET_WM_WINDOW_TYPE_DIALOG,
        _XA_NET_WM_WINDOW_TYPE_DND,
        _XA_NET_WM_WINDOW_TYPE_DOCK,
        _XA_NET_WM_WINDOW_TYPE_DROPDOWN_MENU,
        _XA_NET_WM_WINDOW_TYPE_MENU,
        _XA_NET_WM_WINDOW_TYPE_NORMAL,
        _XA_NET_WM_WINDOW_TYPE_NOTIFICATION,
        _XA_NET_WM_WINDOW_TYPE_POPUP_MENU,
        _XA_NET_WM_WINDOW_TYPE_SPLASH,
        _XA_NET_WM_WINDOW_TYPE_TOOLBAR,
        _XA_NET_WM_WINDOW_TYPE_TOOLTIP,
        _XA_NET_WM_WINDOW_TYPE_UTILITY,
        _XA_NET_WORKAREA
    };
    int net_count = int ACOUNT(net_proto);

    if ((showTaskBar & taskBarEnableSystemTray) == false) {
        for (int k = net_count; 0 < k--; ) {
            if (net_proto[k] == _XA_NET_SYSTEM_TRAY_MESSAGE_DATA ||
                net_proto[k] == _XA_NET_SYSTEM_TRAY_OPCODE ||
                net_proto[k] == _XA_NET_SYSTEM_TRAY_ORIENTATION ||
                net_proto[k] == _XA_NET_SYSTEM_TRAY_VISUAL)
            {
                int keep = --net_count - k;
                if (keep > 0) {
                    size_t size = keep * sizeof(Atom);
                    memmove(&net_proto[k], &net_proto[k + 1], size);
                }
            }
        }
    }

    XChangeProperty(xapp->display(), desktop->handle(),
                    _XA_NET_SUPPORTED, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)net_proto, net_count);
}

static void registerNetProperties(Window xid) {
    XChangeProperty(xapp->display(), xid,
                    _XA_NET_SUPPORTING_WM_CHECK, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&xid, 1);

    XID pid = getpid();

    XChangeProperty(xapp->display(), xid,
                    _XA_NET_WM_PID, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&pid, 1);

    const char wmname[] = "IceWM " VERSION " (" HOSTOS "/" HOSTCPU ")";

    XChangeProperty(xapp->display(), xid,
                    _XA_NET_WM_NAME, _XA_UTF8_STRING, 8,
                    PropModeReplace, (unsigned char *)wmname,
                    strnlen(wmname, sizeof(wmname)));

    XChangeProperty(xapp->display(), desktop->handle(),
                    _XA_NET_SUPPORTING_WM_CHECK, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&xid, 1);
}

static void registerProtocols2(Window xid) {
    registerWinProtocols(xid);
    registerWinProperties(xid);
    registerNetProtocols(xid);
    registerNetProperties(xid);
}

void YWMApp::unregisterProtocols() {
    if (managerWindow) {
        YAtom wmSx("WM_S", true);
        if (managerWindow == XGetSelectionOwner(xapp->display(), wmSx)) {
            XSelectInput(xapp->display(), xapp->root(), None);
            XDeleteProperty(xapp->display(), xapp->root(), _XA_WIN_PROTOCOLS);
            XSetSelectionOwner(xapp->display(), wmSx, None, CurrentTime);
        }
        managerWindow = None;
        xapp->sync();
    }
}

void YWMApp::initIconSize() {
    XIconSize *is = XAllocIconSize();
    if (is) {
        unsigned sizes[] = {
            menuIconSize,
            smallIconSize,
            largeIconSize,
            hugeIconSize
        };
        unsigned count = 4;
        for (unsigned i = 1; i < count; ++i) {
            for (unsigned j = i; j > 0 && sizes[j-1] > sizes[j]; --j)
                swap(sizes[j], sizes[j-1]);
        }
        unsigned delta = 0;
        for (unsigned i = 1; i < count; ++i) {
            unsigned gap = sizes[i] - sizes[i - 1];
            if (0 < gap && (delta == 0 || gap < delta))
                delta = gap;
        }
        is->min_width = is->min_height = int(sizes[0]);
        is->max_width = is->max_height = int(sizes[count - 1]);
        is->width_inc = is->height_inc = int(delta);
        XSetIconSizes(xapp->display(), desktop->handle(), is, 1);
        XFree(is);
    }
}

void YWMApp::initIcons() {
    defaultAppIcon = YIcon::getIcon("app");
}

ref<YIcon> YWMApp::getDefaultAppIcon() {
    return defaultAppIcon;
}

CtrlAltDelete* YWMApp::getCtrlAltDelete() {
    if (ctrlAltDelete == nullptr) {
        ctrlAltDelete = new CtrlAltDelete(this, desktop);
    }
    return ctrlAltDelete;
}

SwitchWindow* YWMApp::getSwitchWindow() {
    if (switchWindow == nullptr && quickSwitch) {
        switchWindow = new SwitchWindow(desktop, nullptr, quickSwitchVertical);
    }
    return switchWindow;
}

void YWMApp::initPointers() {
    osmart<YCursorLoader> l(YCursor::newLoader());

    sizeRightPointer       = l->load("sizeR.xpm",   XC_right_side);
    sizeTopRightPointer    = l->load("sizeTR.xpm",  XC_top_right_corner);
    sizeTopPointer         = l->load("sizeT.xpm",   XC_top_side);
    sizeTopLeftPointer     = l->load("sizeTL.xpm",  XC_top_left_corner);
    sizeLeftPointer        = l->load("sizeL.xpm",   XC_left_side);
    sizeBottomLeftPointer  = l->load("sizeBL.xpm",  XC_bottom_left_corner);
    sizeBottomPointer      = l->load("sizeB.xpm",   XC_bottom_side);
    sizeBottomRightPointer = l->load("sizeBR.xpm",  XC_bottom_right_corner);
    scrollLeftPointer      = l->load("scrollL.xpm", XC_sb_left_arrow);
    scrollRightPointer     = l->load("scrollR.xpm", XC_sb_right_arrow);
    scrollUpPointer        = l->load("scrollU.xpm", XC_sb_up_arrow);
    scrollDownPointer      = l->load("scrollD.xpm", XC_sb_down_arrow);
}

void LogoutMenu::updatePopup() {
    if (itemCount())
        return;

    if (showLogoutMenu) {
        setShared(true); /// !!! get rid of this (refcount objects)
        if (showLogoutSubMenu) {
            addItem(_("_Logout"), -2, null, actionLogout);
            addItem(_("_Cancel logout"), -2, null, actionCancelLogout)->setEnabled(false);
            addSeparator();

            int const oldItemCount = itemCount();
            if (canLock())
                addItem(_("Lock _Workstation"), -2, null, actionLock, "lock");
            if (canShutdown(Reboot))
                addItem(_("Re_boot"), -2, null, actionReboot, "reboot");
            if (canShutdown(Shutdown))
                addItem(_("Shut_down"), -2, null, actionShutdown, "shutdown");
            if (couldRunCommand(suspendCommand))
                addItem(_("_Sleep mode"), -2, null, actionSuspend, "suspend");

            if (itemCount() != oldItemCount)
                addSeparator();

            addItem(_("Restart _Icewm"), -2, null, actionRestart, "restart");

            addItem(_("Restart _Xterm"), -2, null, actionRestartXterm, TERM);

        }
    }
}

void LayerMenu::updatePopup() {
    if (itemCount())
        return;

    addItem(_("_Menu"),       -2, null, layerActionSet[WinLayerMenu]);
    addItem(_("_Above Dock"), -2, null, layerActionSet[WinLayerAboveDock]);
    addItem(_("_Dock"),       -2, null, layerActionSet[WinLayerDock]);
    addItem(_("_OnTop"),      -2, null, layerActionSet[WinLayerOnTop]);
    addItem(_("_Normal"),     -2, null, layerActionSet[WinLayerNormal]);
    addItem(_("_Below"),      -2, null, layerActionSet[WinLayerBelow]);
    addItem(_("D_esktop"),    -2, null, layerActionSet[WinLayerDesktop]);
}

void MoveMenu::updatePopup() {
    if (itemCount())
        return;

    for (int w = 1; w <= workspaceCount; w++) {
        char s[128];
        snprintf(s, sizeof s, "%2d.  %s ", w, workspaceNames[w - 1]);
        addItem(s, 1,
                w ==  1 ? KEY_NAME(gKeySysWorkspace1TakeWin)  :
                w ==  2 ? KEY_NAME(gKeySysWorkspace2TakeWin)  :
                w ==  3 ? KEY_NAME(gKeySysWorkspace3TakeWin)  :
                w ==  4 ? KEY_NAME(gKeySysWorkspace4TakeWin)  :
                w ==  5 ? KEY_NAME(gKeySysWorkspace5TakeWin)  :
                w ==  6 ? KEY_NAME(gKeySysWorkspace6TakeWin)  :
                w ==  7 ? KEY_NAME(gKeySysWorkspace7TakeWin)  :
                w ==  8 ? KEY_NAME(gKeySysWorkspace8TakeWin)  :
                w ==  9 ? KEY_NAME(gKeySysWorkspace9TakeWin)  :
                w == 10 ? KEY_NAME(gKeySysWorkspace10TakeWin) :
                w == 11 ? KEY_NAME(gKeySysWorkspace11TakeWin) :
                w == 12 ? KEY_NAME(gKeySysWorkspace12TakeWin) :
                "", workspaceActionMoveTo[w - 1]);
    }
}

YMenu* YWMApp::getWindowMenu() {
    if (windowMenu)
        return windowMenu;

    windowMenu = new YMenu();
    windowMenu->setShared(true);

    if (strchr(winMenuItems, 'r'))
        windowMenu->addItem(_("_Restore"),  -2, KEY_NAME(gKeyWinRestore), actionRestore);
    if (strchr(winMenuItems, 'm'))
        windowMenu->addItem(_("_Move"),     -2, KEY_NAME(gKeyWinMove), actionMove);
    if (strchr(winMenuItems, 's'))
        windowMenu->addItem(_("_Size"),     -2, KEY_NAME(gKeyWinSize), actionSize);
    if (strchr(winMenuItems, 'n'))
        windowMenu->addItem(_("Mi_nimize"), -2, KEY_NAME(gKeyWinMinimize), actionMinimize);
    if (strchr(winMenuItems, 'x')) {
        windowMenu->addItem(_("Ma_ximize"), -2, KEY_NAME(gKeyWinMaximize), actionMaximize);
        windowMenu->addItem(_("Maximize_Vert"), -2, KEY_NAME(gKeyWinMaximizeVert), actionMaximizeVert);
        windowMenu->addItem(_("MaximizeHori_z"), -2, KEY_NAME(gKeyWinMaximizeHoriz), actionMaximizeHoriz);
    }
    if (strchr(winMenuItems,'f') && allowFullscreen)
        windowMenu->addItem(_("_Fullscreen"), -2, KEY_NAME(gKeyWinFullscreen), actionFullscreen);

    if (strchr(winMenuItems, 'h'))
        windowMenu->addItem(_("_Hide"),     -2, KEY_NAME(gKeyWinHide), actionHide);
    if (strchr(winMenuItems, 'u'))
        windowMenu->addItem(_("Roll_up"),   -2, KEY_NAME(gKeyWinRollup), actionRollup);
    if (strchr(winMenuItems, 'a') ||
        strchr(winMenuItems,'l') ||
        strchr(winMenuItems,'y') ||
        strchr(winMenuItems,'t'))
        windowMenu->addSeparator();
    if (strchr(winMenuItems, 'a'))
        windowMenu->addItem(_("R_aise"),    -2, KEY_NAME(gKeyWinRaise), actionRaise);
    if (strchr(winMenuItems, 'l'))
        windowMenu->addItem(_("_Lower"),    -2, KEY_NAME(gKeyWinLower), actionLower);
    if (strchr(winMenuItems, 'y'))
        windowMenu->addSubmenu(_("La_yer"), -2, layerMenu);

    if (strchr(winMenuItems, 't') && workspaceCount > 1) {
        windowMenu->addSeparator();
        windowMenu->addSubmenu(_("Move _To"), -2, moveMenu);
        windowMenu->addItem(_("Occupy _All"), -2, KEY_NAME(gKeyWinOccupyAll), actionOccupyAllOrCurrent);
    }

    /// this should probably go away, cause fullscreen will do mostly the same thing
#if DO_NOT_COVER_OLD
    if (!limitByDockLayer)
        windowMenu->addItem(_("Limit _Workarea"), -2, null, actionDoNotCover);
#endif

    if (strchr(winMenuItems, 'i') && taskBarShowTray)
        windowMenu->addItem(_("Tray _icon"), -2, null, actionToggleTray);

    if (strchr(winMenuItems, 'c') || strchr(winMenuItems, 'k'))
        windowMenu->addSeparator();
    if (strchr(winMenuItems, 'c'))
        windowMenu->addItem(_("_Close"), -2, KEY_NAME(gKeyWinClose), actionClose);
    if (strchr(winMenuItems, 'k'))
        windowMenu->addItem(_("_Kill Client"), -2, null, actionKill);
    if (strchr(winMenuItems, 'w')) {
        windowMenu->addSeparator();
        windowMenu->addItem(_("_Window list"), -2, KEY_NAME(gKeySysWindowList), actionWindowList);
    }

    return windowMenu;
}

bool YWMApp::handleTimer(YTimer *timer) {
    if (timer == errorTimer) {
        errorTimer = null;
        if (errorRequestCode == X_SetInputFocus && errorFrame != nullptr) {
            if (errorFrame == manager->getFocus()) {
                if (errorFrame->client()) {
                    errorFrame->client()->testDestroyed();
                }
                manager->setFocus(nullptr);
                manager->focusLastWindow();
            }
        }
        errorRequestCode = Success;
    }
    else if (timer == splashTimer) {
        splashTimer = null;
        splashWindow = null;
    }
    return false;
}

int YWMApp::handleError(XErrorEvent *xev) {

    if (initializing &&
        xev->request_code == X_ChangeWindowAttributes &&
        xev->error_code == BadAccess)
    {
        msg(_("Another window manager already running, exiting..."));
        ::exit(1);
    }

    if (xev->error_code == BadWindow) {
        YWindow* ywin = windowContext.find(xev->resourceid);
        if (ywin) {
            if (ywin->destroyed())
                return Success;
            else
                ywin->setDestroyed();
        }
        if (xev->request_code == X_SetInputFocus) {
            errorRequestCode = xev->request_code;
            errorFrame = manager->getFocus();
            if (errorFrame)
                errorTimer->setTimer(0, this, true);
        }
    }
    if (xev->request_code == X_GetWindowAttributes) {
        return Success;
    }

    return BadRequest;
}

#ifdef DEBUG
void dumpZorder(const char *oper, YFrameWindow *w, YFrameWindow *a) {
    msg("--- %s ", oper);
    for (YFrameWindow *p = manager->top(w->getActiveLayer()); p; p = p->next()) {
        if (p && p->client()) {
            mstring cs(p->client()->windowTitle());
            msg(" %c %c 0x%lX: %s", (p == w) ? '*' : ' ',  (p == a) ? '#' : ' ', p->client()->handle(), cs.c_str());
        } else
            msg("?? 0x%lX", p->handle());
        PRECONDITION(p->next() != p);
        PRECONDITION(p->prev() != p);
        if (p->next()) {
            PRECONDITION(p->next()->prev() == p);
        }
    }
}
#endif

void YWMApp::runRestart(const char *path, char *const *args) {
    XSelectInput(xapp->display(), desktop->handle(), 0);
    XFlush(xapp->display());
    ///!!! problem with repeated SIGHUP for restart...
    resetSignals();

    closeFiles();

    if (path) {
        if (args) {
            execvp(path, args);
        } else {
            execlp(path, path, nullptr);
        }
    } else {
        if (mainArgv[0][0] == '/' ||
            (strchr(mainArgv[0], '/') != nullptr &&
             access(mainArgv[0], X_OK) == 0))
        {
            execv(mainArgv[0], mainArgv);
            fail("execv %s", mainArgv[0]);
        }
        execvp(ICEWMEXE, mainArgv);
        fail("execvp %s", ICEWMEXE);
    }

    xapp->alert();

    die(13, _("Could not restart: %s\nDoes $PATH lead to %s?"),
         strerror(errno), path ? path : ICEWMEXE);
}

void YWMApp::restartClient(const char *cpath, char *const *cargs) {
    csmart path(newstr(cpath));
    YStringArray sargs((const char**) cargs);
    char *const *args = (cargs == nullptr) ? nullptr : sargs.getCArray();

    wmapp->signalGuiEvent(geRestart);
    manager->unmanageClients();
    unregisterProtocols();

    runRestart(path, args);

    // icesm knows how to restart.
    if (notifyParent && notifiedParent && kill(notifiedParent, 0) == 0)
        ::exit(ICESM_EXIT_RESTART);

    /* somehow exec failed, try to recover */
    managerWindow = registerProtocols1(mainArgv, mainArgc);
    registerProtocols2(managerWindow);
    manager->manageClients();
}

int YWMApp::runProgram(const char *path, const char *const *args) {
    mstring command;
    YTraceProg trace;
    if (trace.tracing()) {
        command = path;
        if (nonempty(*args)) {
            for (int i = 1; args[i]; ++i) {
                command = mstring(command, " ", args[i]);
            }
        }
        trace.init(command);
    }
    return YApplication::runProgram(path, args);
}

void YWMApp::runOnce(const char *resource, long *pid,
                     const char *path, char *const *args)
{
    if (0 < *pid && mapClientByPid(resource, *pid))
        return;

    if (mapClientByResource(resource, pid))
        return;

    *pid = runProgram(path, args);
}

void YWMApp::runCommand(const char *cmdline) {
    const char shell[] = "&();<>`{}|";
    wordexp_t exp = {};
    if (strpbrk(cmdline, shell) == nullptr &&
        wordexp(cmdline, &exp, WRDE_NOCMD) == 0)
    {
        runProgram(exp.we_wordv[0], exp.we_wordv);
        wordfree(&exp);
    }
    else {
        char const * argv[] = { "/bin/sh", "-c", cmdline, nullptr };
        runProgram(argv[0], argv);
    }
}

void YWMApp::runCommandOnce(const char *resource, const char *cmdline, long *pid) {
    if (0 < *pid && mapClientByPid(resource, *pid))
        return;

    if (mapClientByResource(resource, pid))
        return;

    const char shell[] = "&();<>`{}|";
    wordexp_t exp = {};
    if (strpbrk(cmdline, shell) == nullptr &&
        wordexp(cmdline, &exp, WRDE_NOCMD) == 0)
    {
        *pid = runProgram(exp.we_wordv[0], exp.we_wordv);
        wordfree(&exp);
    }
    else {
        char const *const argv[] = { "/bin/sh", "-c", cmdline, nullptr };
        *pid = runProgram(argv[0], argv);
    }
}

bool YWMApp::mapClientByPid(const char* resource, long pid) {
    if (isEmpty(resource))
        return false;

    bool found = false;

    for (YFrameIter frame = manager->focusedIterator(); ++frame; ) {
        long tmp = 0;
        if (frame->client()->getNetWMPid(&tmp) && tmp == pid) {
            if (frame->client()->classHint()->match(resource)) {
                frame->setWorkspace(manager->activeWorkspace());
                frame->activateWindow(true);
                found = true;
                break;
            }
        }
    }

    return found;
}

bool YWMApp::mapClientByResource(const char* resource, long *pid) {
    if (isEmpty(resource))
        return false;

    Window win(manager->findWindow(resource));
    if (win) {
        YFrameWindow* frame(manager->findFrame(win));
        if (frame) {
            frame->setWorkspace(manager->activeWorkspace());
            frame->activateWindow(true);
            frame->client()->getNetWMPid(pid);
        }
        else {
            XMapRaised(xapp->display(), win);
        }
        return true;
    }
    return false;
}

void YWMApp::setFocusMode(FocusModels mode) {
    focusMode = mode;
    initFocusMode();
    WMConfig::setDefaultFocus(mode);
}

void YWMApp::actionPerformed(YAction action, unsigned int /*modifiers*/) {
    if (action == actionLogout) {
        doLogout(Logout);
    } else if (action == actionCancelLogout) {
        cancelLogout();
    } else if (action == actionLock) {
        runCommand(lockCommand);
    } else if (action == actionShutdown) {
        handleSMAction(ICEWM_ACTION_SHUTDOWN);
    } else if (action == actionSuspend) {
        handleSMAction(ICEWM_ACTION_SUSPEND);
    } else if (action == actionReboot) {
        handleSMAction(ICEWM_ACTION_REBOOT);
    } else if (action == actionRestart) {
#if defined(DEBUG) || defined(PRECON)
        // Prefer a return from main for cleanup checking; icesm restarts.
        if (notifyParent && notifiedParent && kill(notifiedParent, 0) == 0)
            this->exit(ICESM_EXIT_RESTART);
        else
#endif
            restartClient(nullptr, nullptr);
    }
    else if (action == actionRestartXterm) {
        struct t_executor : public YMsgBoxListener {
            YSMListener *listener;
            t_executor(YSMListener* x) : listener(x) {}
            virtual void handleMsgBox(YMsgBox *msgbox, int operation) {
                if (msgbox)
                    manager->unmanageClient(msgbox);
                if (operation == YMsgBox::mbOK)
                    listener->restartClient(TERM, nullptr);
            }
        };
        static t_executor delegate(this);
        YFrameWindow::wmConfirmKill(_("Kill IceWM, replace with Xterm"), &delegate);
    }
    else if (action == actionRun) {
        runCommand(runDlgCommand);
    } else if (action == actionExit) {
        manager->unmanageClients();
        unregisterProtocols();
        exit(0);
    } else if (action == actionFocusClickToFocus) {
        setFocusMode(FocusClick);
    } else if (action == actionFocusMouseSloppy) {
        setFocusMode(FocusSloppy);
    } else if (action == actionFocusExplicit) {
        setFocusMode(FocusExplicit);
    } else if (action == actionFocusMouseStrict) {
        setFocusMode(FocusStrict);
    } else if (action == actionFocusQuietSloppy) {
        setFocusMode(FocusQuiet);
    } else if (action == actionFocusCustom) {
        setFocusMode(FocusCustom);
    } else if (action == actionRefresh) {
        osmart<YWindow> w(new YWindow());
        if (w) {
            w->setGeometry(desktop->geometry());
            w->raise();
            w->show();
            w->hide();
        }
    } else if (action == actionAbout) {
        if (aboutDlg == nullptr)
            aboutDlg = new AboutDlg();
        else
            aboutDlg->getFrame()->setWorkspace(manager->activeWorkspace());
        if (aboutDlg)
            aboutDlg->showFocused();
    } else if (action == actionTileVertical ||
               action == actionTileHorizontal)
    {
        YFrameWindow **w = nullptr;
        int count = 0;

        manager->getWindowsToArrange(&w, &count);
        if (w) {
            manager->tileWindows(w, count,
                                 (action == actionTileVertical) ? true : false);
            delete [] w;
        }
    } else if (action == actionArrange) {
        YFrameWindow **w = nullptr;
        int count = 0;
        manager->getWindowsToArrange(&w, &count);
        if (w) {
            manager->smartPlace(w, count);
            delete [] w;
        }
    } else if (action == actionArrangeIcons) {
        manager->arrangeIcons();
    } else if (action == actionHideAll || action == actionMinimizeAll) {
        YFrameWindow **w = nullptr;
        int count = 0;
        manager->getWindowsToArrange(&w, &count);
        if (w) {
            manager->setWindows(w, count, action);
            delete [] w;
        }
    } else if (action == actionShowDesktop) {
        YFrameWindow **w = nullptr;
        int count = 0;
        manager->getWindowsToArrange(&w, &count, true, true);
        if (w && count > 0) {
            manager->setWindows(w, count, actionMinimizeAll);
            manager->setShowingDesktop(true);
        } else {
            manager->undoArrange();
            manager->setShowingDesktop(false);
        }
        delete [] w;
    } else if (action == actionCascade) {
        YFrameWindow **w = nullptr;
        int count = 0;
        manager->getWindowsToArrange(&w, &count);
        if (w) {
            manager->cascadePlace(w, count);
            delete [] w;
        }
    } else if (action == actionUndoArrange) {
        manager->undoArrange();
    } else if (action == actionWindowList) {
        if (windowList->visible())
            windowList->getFrame()->wmHide();
        else
            windowList->showFocused(-1, -1);
    } else if (action == actionWinOptions) {
        loadWinOptions(findConfigFile("winoptions"));
    } else if (action == actionReloadKeys) {
        keyProgs.clear();
        MenuLoader(this, this, this).loadMenus(findConfigFile("keys"), nullptr);
        if (manager && !initializing) {
            if (manager->wmState() == YWindowManager::wmRUNNING) {
                manager->grabKeys();
            }
        }
    } else if (action == actionCollapseTaskbar && taskBar) {
        taskBar->handleCollapseButton();
    } else {
        for (int w = 0; w < workspaceCount; w++) {
            if (workspaceActionActivate[w] == action) {
                manager->activateWorkspace(w);
                return ;
            }
        }
    }
}

void YWMApp::initFocusCustom() {
    cfoption focus_prefs[] = {
        OBV("ClickToFocus",              &clickFocus,                ""),
        OBV("FocusOnAppRaise",           &focusOnAppRaise,           ""),
        OBV("RequestFocusOnAppRaise",    &requestFocusOnAppRaise,    ""),
        OBV("RaiseOnFocus",              &raiseOnFocus,              ""),
        OBV("FocusOnClickClient",        &focusOnClickClient,        ""),
        OBV("RaiseOnClickClient",        &raiseOnClickClient,        ""),
        OBV("FocusChangesWorkspace",     &focusChangesWorkspace,     ""),
        OBV("FocusCurrentWorkspace",     &focusCurrentWorkspace,     ""),
        OBV("FocusOnMap",                &focusOnMap,                ""),
        OBV("FocusOnMapTransient",       &focusOnMapTransient,       ""),
        OBV("FocusOnMapTransientActive", &focusOnMapTransientActive, ""),
        OK0()
    };

    YConfig::findLoadConfigFile(this, focus_prefs, configFile);
    YConfig::findLoadConfigFile(this, focus_prefs, "prefoverride");
}

void YWMApp::initFocusMode() {
    switch (focusMode) {

    case FocusCustom: /* custom */
        initFocusCustom();
        break;

    case FocusClick: /* click to focus */
        clickFocus = true;
        // focusOnAppRaise = false;
        requestFocusOnAppRaise = true;
        raiseOnFocus = true;
        raiseOnClickClient = true;
        focusOnMap = true;
        mapInactiveOnTop = true;
        focusChangesWorkspace = false;
        focusOnMapTransient = false;
        focusOnMapTransientActive = true;
        break;

    case FocusSloppy:  /* sloppy mouse focus */
        clickFocus = false;
        // focusOnAppRaise = false;
        requestFocusOnAppRaise = true;
        raiseOnFocus = false;
        raiseOnClickClient = true;
        focusOnMap = true;
        mapInactiveOnTop = true;
        focusChangesWorkspace = false;
        focusOnMapTransient = false;
        focusOnMapTransientActive = true;
        break;

    case FocusExplicit: /* explicit focus */
        clickFocus = true;
        // focusOnAppRaise = false;
        requestFocusOnAppRaise = false;
        raiseOnFocus = false;
        raiseOnClickClient = false;
        focusOnMap = false;
        mapInactiveOnTop = true;
        focusChangesWorkspace = false;
        focusOnMapTransient = false;
        focusOnMapTransientActive = true;
        break;

    case FocusStrict:  /* strict mouse focus */
        clickFocus = false;
        // focusOnAppRaise = false;
        requestFocusOnAppRaise = false;
        raiseOnFocus = true;
        raiseOnClickClient = true;
        focusOnMap = false;
        mapInactiveOnTop = false;
        focusChangesWorkspace = false;
        focusOnMapTransient = false;
        focusOnMapTransientActive = true;
        break;

    case FocusQuiet:  /* quiet sloppy focus */
        clickFocus = false;
        // focusOnAppRaise = false;
        requestFocusOnAppRaise = false;
        raiseOnFocus = false;
        raiseOnClickClient = true;
        focusOnMap = true;
        mapInactiveOnTop = true;
        focusChangesWorkspace = false;
        focusOnMapTransient = false;
        focusOnMapTransientActive = true;
        break;

    default:
        warn("Erroneous focus mode %d.", focusMode);
    }
}

void YWMApp::loadFocusMode() {
    const char* focusMode = nullptr;
    cfoption focus_prefs[] = {
        OSV("FocusMode", &focusMode,
            "Focus mode (0=custom, 1=click, 2=sloppy"
            ", 3=explicit, 4=strict, 5=quiet)"),
        OK0()
    };

    YConfig::findLoadConfigFile(this, focus_prefs, "focus_mode");
    if (focusMode) {
        static const struct {
            FocusModels num;
            const char* str;
        } models[] = {
            { FocusCustom,   "custom"   },
            { FocusClick,    "click"    },
            { FocusSloppy,   "sloppy"   },
            { FocusExplicit, "explicit" },
            { FocusStrict,   "strict"   },
            { FocusQuiet,    "quiet"    },
        };
        if (ASCII::isDigit(*focusMode)) {
            int mode = atoi(focusMode);
            for (int i = 0; i < int ACOUNT(models); ++i) {
                if (mode == models[i].num) {
                    this->focusMode = models[i].num;
                    break;
                }
            }
        }
        else {
            mstring mode(mstring(focusMode).lower());
            for (int i = 0; i < int ACOUNT(models); ++i) {
                if (mode == models[i].str) {
                    this->focusMode = models[i].num;
                    break;
                }
            }
        }
        delete[] const_cast<char *>(focusMode);
    }
}

static void showExtensions() {
    struct {
        const char* str;
        YExtension* ext;
    } xs[] = {
        { "composite", &composite },
        { "damage",    &damage    },
        { "fixes",     &fixes     },
        { "render",    &render    },
        { "shapes",    &shapes    },
        { "xrandr",    &xrandr    },
        { "xinerama",  &xinerama  },
    };
    printf("[name]   [ver] [ev][err]\n");
    for (int i = 0; i < int ACOUNT(xs); ++i) {
        const char* s = xs[i].str;
        YExtension* x = xs[i].ext;
        if (x->versionMajor | x->versionMinor) {
            printf("%-9s %d.%-2d (%2d, %3d)\n", s,
                    x->versionMajor, x->versionMinor,
                    x->eventBase, x->errorBase);
        }
        if (!x->supported) {
            printf("%-9s unsupported\n", s);
        }
    }
}

static int restartWM(const char* displayName, const char* overrideTheme) {
    Display* display = XOpenDisplay(displayName);
    if (display) {
        if (nonempty(overrideTheme)) {
            WMConfig::setDefaultTheme(overrideTheme);
        }
        XClientMessageEvent message = {
            ClientMessage, 0UL, False, nullptr, DefaultRootWindow(display),
            XInternAtom(display, "_ICEWM_ACTION", False), 32,
        };
        message.data.l[0] = CurrentTime;
        message.data.l[1] = ICEWM_ACTION_RESTARTWM;
        XSendEvent(display, DefaultRootWindow(display), False,
                   SubstructureNotifyMask, (XEvent *) &message);
        XSync(display, False);
        XCloseDisplay(display);
        return EXIT_SUCCESS;
    }
    else {
        msg(_("Can't open display: %s. X must be running and $DISPLAY set."),
            displayName ? displayName : _("<none>"));
        return EXIT_FAILURE;
    }
}

YWMApp::YWMApp(int *argc, char ***argv, const char *displayName,
                bool notifyParent, const char *splashFile,
                const char *configFile, const char *overrideTheme) :
    YSMApplication(argc, argv, displayName),
    mainArgv(*argv),
    mainArgc(*argc),
    configFile(configFile),
    notifyParent(notifyParent),
    notifiedParent(0),
    fLogoutMsgBox(nullptr),
    aboutDlg(nullptr),
    ctrlAltDelete(nullptr),
    switchWindow(nullptr),
    windowMenu(nullptr),
    errorRequestCode(0),
    errorFrame(nullptr),
    splashWindow(splash(splashFile)),
    focusMode(FocusClick),
    managerWindow(None)
{
    wmapp = this;

    WMConfig::loadConfiguration(this, configFile);
    if (themeName != nullptr) {
        MSG(("themeName=%s", themeName));

        bool ok = WMConfig::loadThemeConfiguration(this, themeName);
        if (ok == false && strcmp(themeName, CONFIG_DEFAULT_THEME)) {
            themeName = CONFIG_DEFAULT_THEME;
            ok = WMConfig::loadThemeConfiguration(this, themeName);
        }
        if (ok == false && strpcmp(themeName, "default", "/")) {
            themeName = "default/default.theme";
            ok = WMConfig::loadThemeConfiguration(this, themeName);
        }
    }
    loadFocusMode();
    WMConfig::loadConfiguration(this, "prefoverride");
    if (focusMode != FocusCustom)
        initFocusMode();

    DEPRECATE(warpPointer == true);
    DEPRECATE(focusRootWindow == true);
    DEPRECATE(replayMenuCancelClick == true);
    //DEPRECATE(manualPlacement == true);
    //DEPRECATE(strongPointerFocus == true);
    DEPRECATE(showPopupsAbovePointer == true);
    DEPRECATE(considerHorizBorder == true);
    DEPRECATE(considerVertBorder == true);
    DEPRECATE(sizeMaximized == true);
    DEPRECATE(dontRotateMenuPointer == false);
    DEPRECATE(lowerOnClickWhenRaised == true);

    catchSignal(SIGINT);
    catchSignal(SIGTERM);
    catchSignal(SIGQUIT);
    catchSignal(SIGHUP);
    catchSignal(SIGCHLD);
    catchSignal(SIGUSR2);
    catchSignal(SIGPIPE);

    actionPerformed(actionWinOptions, 0);
    actionPerformed(actionReloadKeys, 0);

    initPointers();

    if (post_preferences)
        WMConfig::printPrefs(focusMode, wmapp_preferences);
    if (show_extensions)
        showExtensions();

    delete desktop;

    managerWindow = registerProtocols1(*argv, *argc);

    manager = new YWindowManager(
        this, this, this, nullptr, root());
    PRECONDITION(desktop != nullptr);

    registerProtocols2(managerWindow);

    initIcons();
    initIconSize();
    WPixRes::initPixmaps();

    if (scrollBarWidth == 0) {
        switch(wmLook) {
            case lookWarp4:
                scrollBarWidth = 14;
                break;

            case lookMotif:
            case lookGtk:
                scrollBarWidth = 15;
                break;

            case lookNice:
            case lookWin95:
            case lookWarp3:
            case lookPixmap:
                scrollBarWidth = 16;
                break;

            case lookFlat:
            case lookMetal:
                scrollBarWidth = 17;
                break;
        }
    }

    if (scrollBarHeight == 0) {
        switch(wmLook) {
            case lookWarp4:
                scrollBarHeight = 20;
                break;

            case lookMotif:
            case lookGtk:
                scrollBarHeight = scrollBarWidth;
                break;

            case lookNice:
            case lookWin95:
            case lookWarp3:
            case lookPixmap:
                scrollBarHeight = scrollBarWidth;
                break;

            case lookFlat:
            case lookMetal:
                scrollBarHeight = scrollBarWidth;
                break;
        }
    }

    manager->initWorkspaces();

    manager->grabKeys();

    manager->setupRootProxy();

#ifdef CONFIG_SESSION
    if (haveSessionManager())
        loadWindowInfo();
#endif

    initializing = false;
}

YWMApp::~YWMApp() {
    if (fLogoutMsgBox) {
        manager->unmanageClient(fLogoutMsgBox);
        fLogoutMsgBox = nullptr;
    }
    if (aboutDlg) {
        manager->unmanageClient(aboutDlg);
        aboutDlg = nullptr;
    }

    delete switchWindow; switchWindow = nullptr;
    delete ctrlAltDelete; ctrlAltDelete = nullptr;
    delete taskBar; taskBar = nullptr;

    if (statusMoveSize)
        statusMoveSize = null;
    if (statusWorkspace)
        statusWorkspace = null;

    rootMenu = null;
    windowList = null;

    if (windowMenu) {
        windowMenu->setShared(false);
        delete windowMenu; windowMenu = nullptr;
    }

    // shared menus last
    if (logoutMenu) {
        logoutMenu->setShared(false);
        logoutMenu = null;
    }
    windowListMenu = null;
    layerMenu = null;
    moveMenu = null;

    keyProgs.clear();
    workspaces.reset();
    WPixRes::freePixmaps();

    extern void freeTitleColorsFonts();
    freeTitleColorsFonts();

    //!!!XFreeGC(display(), outlineGC); lazy init in movesize.cc
    //!!!XFreeGC(display(), clipPixmapGC); in ypaint.cc

    XFlush(display());
}

int YWMApp::mainLoop() {
    signalGuiEvent(geStartup);
    manager->manageClients();

    if (notifyParent) {
        notifiedParent = getppid();
        if (kill(notifiedParent, SIGUSR1)) {
            notifiedParent = 0;
            notifyParent = false;
            fail("notify parent");
        }
    }

    return super::mainLoop();
}

void YWMApp::handleSignal(int sig) {
    switch (sig) {
    case SIGINT:
    case SIGTERM:
        actionPerformed(actionExit, 0);
        break;

    case SIGQUIT:
        actionPerformed(actionLogout, 0);
        break;

    case SIGHUP:
        actionPerformed(actionRestart, 0);
        break;

    case SIGUSR2:
        tlog("logEvents %s", boolstr(toggleLogEvents()));
        break;

    case SIGPIPE:
        if (ferror(stdout) || ferror(stderr))
            this->exit(1);
        break;

    default:
        YApplication::handleSignal(sig);
        break;
    }
}

bool YWMApp::handleIdle() {
    static int qbits;
    bool busy = YSMApplication::handleIdle();

    if ((QLength(display()) >> qbits) > 0) {
        ++qbits;
    }
    else if (taskBar == nullptr && showTaskBar) {
        createTaskBar();
        busy = true;
    }
    else if (splashWindow) {
        splashWindow = null;
        splashTimer = null;
    }
    else if (taskBar) {
        taskBar->relayoutNow();
    }
    return busy;
}

void YWMApp::signalGuiEvent(GUIEvent ge) {
    /*
     * The first event must be geStartup.
     * Ignore all other events before that.
     */
    static bool started;
    if (ge == geStartup)
        started = true;
    else if (started == false)
        return;

    /*
     * Because there is no event buffering,
     * when multiple events occur in a burst,
     * only signal the first event of the burst.
     */
    timeval now = monotime();
    static timeval next;
    if (now < next && ge != geStartup) {
        return;
    }
    next = now + millitime(100L);

    unsigned char num = (unsigned char)ge;

    XChangeProperty(xapp->display(), desktop->handle(),
                    _XA_ICEWM_GUIEVENT, _XA_ICEWM_GUIEVENT,
                    8, PropModeReplace, &num, 1);
}

bool YWMApp::filterEvent(const XEvent &xev) {
    if (xev.type == SelectionClear) {
        if (xev.xselectionclear.window == managerWindow) {
            manager->unmanageClients();
            unregisterProtocols();
            exit(0);
        }
    }
    return YSMApplication::filterEvent(xev);
}

void YWMApp::afterWindowEvent(XEvent &xev) {
    static XEvent lastKeyEvent = { 0 };

    if (xev.type == KeyRelease && lastKeyEvent.type == KeyPress) {
        KeySym k1 = XkbKeycodeToKeysym(xapp->display(), xev.xkey.keycode, 0, 0);
        unsigned int m1 = KEY_MODMASK(lastKeyEvent.xkey.state);
        KeySym k2 = XkbKeycodeToKeysym(xapp->display(), lastKeyEvent.xkey.keycode, 0, 0);

        if (m1 == 0 && xapp->WinMask && win95keys) {
            if (k1 == xapp->Win_L && k2 == xapp->Win_L) {
                manager->popupStartMenu(desktop);
            }
            else if (k1 == xapp->Win_R && k2 == xapp->Win_R) {
                actionPerformed(actionWindowList, 0);
            }
        }
    }

    if (xev.type == KeyPress ||
        xev.type == KeyRelease ||
        xev.type == ButtonPress ||
        xev.type == ButtonRelease)
        lastKeyEvent = xev;
}

static void print_usage(const char *argv0) {
    const char *usage_client_id =
#ifdef CONFIG_SESSION
             _("  --client-id=ID      Client id to use when contacting session manager.\n");
#else
             "";
#endif
    const char *usage_debug =
#ifdef DEBUG
             _("\n"
             "  --debug             Print generic debug messages.\n"
             "  --debug-z           Print debug messages regarding window stacking.\n");
#else
             "";
#endif

    const char *usage_preferences =
             _("\n"
             "  -a, --alpha         Use a 32-bit visual for translucency.\n"
             "  -c, --config=FILE   Load preferences from FILE.\n"
             "  -t, --theme=FILE    Load theme from FILE.\n"
             "  -s, --splash=IMAGE  Briefly show IMAGE on startup.\n"
             "  -p, --postpreferences  Print preferences after all processing.\n"
             "  --trace=conf,icon   Trace paths used to load configuration.\n"
             );

    printf(_("Usage: %s [OPTIONS]\n"
             "Starts the IceWM window manager.\n"
             "\n"
             "Options:\n"
             "  -d, --display=NAME  NAME of the X server to use.\n"
             "%s"
             "  --sync              Synchronize X11 commands.\n"
             "%s"
             "\n"
             "  -V, --version       Prints version information and exits.\n"
             "  -h, --help          Prints this usage screen and exits.\n"
             "%s"
             "\n"
             "  --replace           Replace an existing window manager.\n"
             "  -r, --restart       Tell the running icewm to restart itself.\n"
             "\n"
             "  --configured        Print the compile time configuration.\n"
             "  --directories       Print the configuration directories.\n"
             "  -l, --list-themes   Print a list of all available themes.\n"
             "\n"
             "Environment variables:\n"
             "  ICEWM_PRIVCFG=PATH  Directory for user configuration files,\n"
             "                      \"$XDG_CONFIG_HOME/icewm\" if exists or\n"
             "                      \"$HOME/.icewm\" by default.\n"
             "  DISPLAY=NAME        Name of the X server to use.\n"
             "  MAIL=URL            Location of your mailbox.\n"
             "\n"
             "To report bugs, support requests, comments please visit:\n"
             "%s\n\n"),
             argv0,
             usage_client_id,
             usage_preferences,
             usage_debug,
             PACKAGE_BUGREPORT[0] ? PACKAGE_BUGREPORT :
             PACKAGE_URL[0] ? PACKAGE_URL :
             "https://ice-wm.org/");
    exit(0);
}

static void print_themes_list() {
    themeName = nullptr;
    ref<YResourcePaths> res(YResourcePaths::subdirs(null, true));
    for (int i = 0; i < res->getCount(); ++i) {
        for (sdir dir(res->getPath(i)); dir.next(); ) {
            upath thmp(dir.path() + dir.entry());
            if (thmp.dirExists()) {
                for (sdir thmdir(thmp); thmdir.nextExt(".theme"); ) {
                    upath theme(thmdir.path() + thmdir.entry());
                    puts(theme.string());
                }
            }
        }
    }
    exit(0);
}

static void print_confdir(const char *name, const char *path) {
    printf("%s=%s\n", name, path);
}

static void print_directories(const char *argv0) {
    upath priv(YApplication::getPrivConfDir());
    printf(_("%s configuration directories:\n"), argv0);
    print_confdir("PrivConfDir", priv.string());
    print_confdir("CFGDIR", CFGDIR);
    print_confdir("LIBDIR", LIBDIR);
    print_confdir("LOCDIR", LOCDIR);
    print_confdir("DOCDIR", DOCDIR);
    exit(0);
}

static void print_configured(const char *argv0) {
    static const char compile_time_configured_options[] =
    /* Sorted alphabetically: */
#ifdef ENABLE_ALSA
    " alsa"
#endif
#ifdef ENABLE_AO
    " ao"
#endif
#ifdef CONFIG_COREFONTS
    " corefonts"
#endif
#ifdef DEBUG
    " debug"
#endif
#ifdef CONFIG_FDO_MENUS
    " fdomenus"
#endif
#ifdef CONFIG_FRIBIDI
    " fribidi"
#endif
#ifdef CONFIG_GDK_PIXBUF_XLIB
    " gdkpixbuf"
#endif
#ifdef CONFIG_I18N
    " i18n"
#endif
#ifdef CONFIG_LIBICONV
    " libiconv"
#endif
#ifdef CONFIG_LIBJPEG
    " libjpeg"
#endif
#ifdef CONFIG_LIBPNG
    " libpng"
#endif
#ifdef CONFIG_LIBRSVG
    " librsvg"
#endif
#ifdef CONFIG_XPM
    " libxpm"
#endif
#ifdef LOGEVENTS
    " logevents"
#endif
#ifdef ENABLE_NLS
    " nls"
#endif
#ifdef ENABLE_OSS
    " oss"
#endif
#ifdef CONFIG_SESSION
    " session"
#endif
#ifdef CONFIG_SHAPE
    " shape"
#endif
#ifdef CONFIG_UNICODE_SET
    " unicodeset"
#endif
#ifdef CONFIG_XFREETYPE
    " xfreetype" QUOTE(CONFIG_XFREETYPE)
#endif
#ifdef XINERAMA
    " xinerama"
#endif
#ifdef CONFIG_XRANDR
    " xrandr"
#endif
    "\n";
    printf(_("%s configured options:%s\n"), argv0,
            compile_time_configured_options);
    exit(0);
}

static void loadStartup(const char* configFile)
{
    upath prefs(YApplication::locateConfigFile(configFile));
    if (prefs.nonempty()) {
        YConfig::loadConfigFile(wmapp_preferences, prefs);
    }

    YXApplication::alphaBlending |= alphaBlending;
    YXApplication::synchronizeX11 |= synchronizeX11;
    if (tracingModules && YTrace::tracingConf() == nullptr) {
        YTrace::tracing(tracingModules);
    }

    upath theme(YApplication::locateConfigFile("theme"));
    if (theme.nonempty()) {
        unsigned last = ACOUNT(wmapp_preferences) - 2;
        YConfig::loadConfigFile(wmapp_preferences + last, theme);
    }
}

int main(int argc, char **argv) {
    YLocale locale;
    bool restart_wm(false);
    bool log_events(false);
    bool notify_parent(false);
    const char* configFile(nullptr);
    const char* displayName(nullptr);
    const char* overrideTheme(nullptr);

    for (char ** arg = argv + 1; arg < argv + argc; ++arg) {
        if (**arg == '-') {
            char *value(nullptr);
            if (GetArgument(value, "c", "config", arg, argv+argc))
                configFile = value;
            else if (GetArgument(value, "t", "theme", arg, argv+argc))
                overrideTheme = value;
            else if (is_switch(*arg, "p", "postpreferences"))
                post_preferences = true;
            else if (is_long_switch(*arg, "extensions"))
                show_extensions = true;
            else
#ifdef DEBUG
            if (is_long_switch(*arg, "debug"))
                debug = true;
            else if (is_long_switch(*arg, "debug-z"))
                debug_z = true;
            else
#endif
            if (is_switch(*arg, "r", "restart"))
                restart_wm = true;
            else if (is_long_switch(*arg, "replace"))
                replace_wm = true;
            else if (is_long_switch(*arg, "notify"))
                notify_parent = true;
            else if (is_long_switch(*arg, "configured"))
                print_configured(argv[0]);
            else if (is_long_switch(*arg, "directories"))
                print_directories(argv[0]);
            else if (is_switch(*arg, "l", "list-themes"))
                print_themes_list();
            else if (is_help_switch(*arg))
                print_usage(my_basename(argv[0]));
            else if (is_version_switch(*arg))
                print_version_exit(VERSION);
            else if (is_copying_switch(*arg))
                print_copying_exit();
            else if (is_long_switch(*arg, "sync"))
                YXApplication::synchronizeX11 = true;
            else if (is_long_switch(*arg, "logevents"))
                log_events = true;
            else if (is_switch(*arg, "a", "alpha"))
                YXApplication::alphaBlending = true;
            else if (GetArgument(value, "d", "display", arg, argv+argc))
                displayName = value;
            else if (GetArgument(value, "s", "splash", arg, argv+argc))
                splashFile = value;
            else if (GetLongArgument(value, "trace", arg, argv+argc))
                YTrace::tracing(value);
            else
                warn(_("Unrecognized option '%s'."), *arg);
        }
    }

    if (restart_wm)
        return restartWM(displayName, overrideTheme);

    if (isEmpty(configFile))
        configFile = "preferences";
    loadStartup(configFile);

    if (nonempty(overrideTheme))
        themeName = overrideTheme;
    loggingEvents |= log_events;
    if (loggingEvents)
        initLogEvents();

    YWMApp app(&argc, &argv, displayName,
                notify_parent, splashFile,
                configFile, overrideTheme);

    int rc = app.mainLoop();
    app.signalGuiEvent(geShutdown);
    manager->unmanageClients();
    app.unregisterProtocols();
    YIcon::freeIcons();
    WMConfig::freeConfiguration();
    defOptions = null;
    hintOptions = null;
    return rc;
}

void YWMApp::createTaskBar() {
    if (showTaskBar && taskBar == nullptr) {
        taskBar = new TaskBar(this, desktop, this, this);
        for (YFrameIter frame = manager->focusedIterator(); ++frame; ) {
            frame->updateAppStatus();
        }
        taskBar->showBar();
        taskBar->relayoutNow();
    }
}

void YWMApp::doLogout(RebootShutdown reboot) {
    rebootOrShutdown = reboot;
    if (!confirmLogout)
        logout();
    else {
        if (fLogoutMsgBox == nullptr) {
            YMsgBox *msgbox = new YMsgBox(YMsgBox::mbOK|YMsgBox::mbCancel);
            fLogoutMsgBox = msgbox;
            msgbox->setTitle(_("Confirm Logout"));
            msgbox->setText(_("Logout will close all active applications.\nProceed?"));
            msgbox->autoSize();
            msgbox->setMsgBoxListener(this);
        }
        if (fLogoutMsgBox)
            fLogoutMsgBox->showFocused();
    }
}

void YWMApp::logout() {
    if (logoutMenu) {
        logoutMenu->checkCommand(actionLogout, true);
        logoutMenu->disableCommand(actionLogout);
        logoutMenu->enableCommand(actionCancelLogout);
    }
    if (logoutCommand && logoutCommand[0]) {
        runCommand(logoutCommand);
#ifdef CONFIG_SESSION
    } else if (haveSessionManager()) {
        smRequestShutdown();
#endif
    } else {
        manager->wmCloseSession();
        // should we always do this??
        manager->exitAfterLastClient(true);
    }
}

void YWMApp::cancelLogout() {
    rebootOrShutdown = Logout;
    if (logoutMenu) {
        logoutMenu->checkCommand(actionLogout, false);
        logoutMenu->enableCommand(actionLogout);
        logoutMenu->disableCommand(actionCancelLogout);
    }
    if (logoutCancelCommand && logoutCancelCommand[0]) {
        runCommand(logoutCancelCommand);
#ifdef CONFIG_SESSION
    } else if (haveSessionManager()) { // !!! this doesn't work
        smCancelShutdown();
#endif
    } else {
        // should we always do this??
        manager->exitAfterLastClient(false);
    }
}

void YWMApp::handleMsgBox(YMsgBox *msgbox, int operation) {
    if (msgbox == fLogoutMsgBox && fLogoutMsgBox) {
        if (fLogoutMsgBox) {
            manager->unmanageClient(fLogoutMsgBox);
            fLogoutMsgBox = nullptr;
        }
        if (operation == YMsgBox::mbOK) {
            logout();
        }
    }
}

void YWMApp::handleSMAction(WMAction message) {
    switch (message) {
    case ICEWM_ACTION_LOGOUT:
        wmapp->doLogout(Logout);
        break;
    case ICEWM_ACTION_CANCEL_LOGOUT:
        wmapp->actionPerformed(actionCancelLogout, 0);
        break;
    case ICEWM_ACTION_SHUTDOWN:
        wmapp->doLogout(Shutdown);
        break;
    case ICEWM_ACTION_REBOOT:
        wmapp->doLogout(Reboot);
        break;
    case ICEWM_ACTION_RESTARTWM:
        wmapp->actionPerformed(actionRestart, 0);
        break;
    case ICEWM_ACTION_WINDOWLIST:
        wmapp->actionPerformed(actionWindowList, 0);
        break;
    case ICEWM_ACTION_ABOUT:
        wmapp->actionPerformed(actionAbout, 0);
        break;
    case ICEWM_ACTION_SUSPEND:
        wmapp->runCommand(suspendCommand);
        break;
    case ICEWM_ACTION_WINOPTIONS:
        wmapp->actionPerformed(actionWinOptions, 0);
        break;
    case ICEWM_ACTION_RELOADKEYS:
        wmapp->actionPerformed(actionReloadKeys, 0);
        break;
    }
}

class SplashWindow : public YWindow {
    ref<YImage> image;
public:
    SplashWindow(ref<YImage> image, int depth, Visual* visual) :
        YWindow(nullptr, None, depth, visual),
        image(image)
    {
        setToplevel(true);
        setStyle(wsOverrideRedirect | wsSaveUnder | wsNoExpose);
        place();
        XSelectInput(xapp->display(), handle(), VisibilityChangeMask);
        props();
        show();
        xapp->sync();
    }
    void place() {
        YRect geo(desktop->getScreenGeometry());
        int w = int(image->width());
        int h = int(image->height());
        int x = geo.x() + (geo.width() - w) / 2;
        int y = geo.y() + (geo.height() - h) / 2;
        setGeometry(YRect(x, y, w, h));
        GraphicsBuffer(this).paint();
    }
    void props() {
        setTitle("IceSplash");
        setClassHint("splash", "IceWM");
        setNetOpacity(82 * (0xFFFFFFFF / 100));
        setNetWindowType(_XA_NET_WM_WINDOW_TYPE_SPLASH);
        setProperty(_XA_WIN_LAYER, XA_CARDINAL, 15);
    }
    void repaint() {
    }
    void handleExpose(const XExposeEvent&) {
    }
    void paint(Graphics& g, const YRect&) {
        g.copyImage(image, 0, 0);
    }
    void handleVisibility(const XVisibilityEvent&) {
        raise();
        xapp->sync();
    }
};

YWindow* YWMApp::splash(const char* splashFile) {
    YWindow* window(nullptr);
    if (splashFile && 4 < strlen(splashFile)) {
        upath path(findConfigFile(splashFile));
        if (path.nonempty()) {
            ref<YImage> imag(YImage::load(path));
            if (imag != null) {
                unsigned depth = DefaultDepth(display(), screen());
                Visual* visual = DefaultVisual(display(), screen());
                window = new SplashWindow(imag, depth, visual);
                window->unmanageWindow();
                window->raise();
                splashTimer->setTimer(1000L, this, true);
            }
        }
    }
    return window;
}

// vim: set sw=4 ts=4 et:
