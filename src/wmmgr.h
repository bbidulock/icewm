#ifndef __WMMGR_H
#define __WMMGR_H

#include "WinMgr.h"
#include "ylist.h"
#include "yaction.h"

#define MAXWORKSPACES     20

extern long workspaceCount;
extern char *workspaceNames[MAXWORKSPACES];
extern YAction workspaceActionActivate[MAXWORKSPACES];
extern YAction workspaceActionMoveTo[MAXWORKSPACES];
extern YAction layerActionSet[WinLayerCount];

class YWindowManager;
class YFrameClient;
class YFrameWindow;
class YSMListener;
class IApp;

/*
 * X11 time state to support _NET_WM_USER_TIME.
 * Keep track of the time in seconds when we receive a X11 time stamp.
 * Only compare two X11 time stamps if they are in a time interval.
 */
class UserTime {
private:
    unsigned long xtime;
    bool valid;
    long since;
    enum {
        XTimeMask = 0xFFFFFFFFUL,       // milliseconds
        XTimeRange = 0x7FFFFFFFUL,      // milliseconds
        SInterval = 0x3FFFFFFFUL / 1000,     // seconds
    };
public:
    UserTime() : xtime(0), valid(false), since(0) { }
    explicit UserTime(unsigned long xtime, bool valid = true) :
        xtime(xtime & XTimeMask), valid(valid), since(seconds()) { }
    unsigned long time() const { return xtime; }
    bool good() const { return valid; }
    long when() const { return since; }
    bool update(unsigned long xtime, bool valid = true) {
        UserTime u(xtime, valid);
        return *this < u || xtime == 0 ? (*this = u, true) : false;
    }
    bool operator<(const UserTime& u) const {
        if (since == 0 || u.since == 0) return u.since != 0;
        if (valid == false || u.valid == false) return u.valid;
        if (since < u.since && u.since - since > SInterval) return true;
        if (since > u.since && since - u.since > SInterval) return false;
        if (xtime < u.xtime) return u.xtime - xtime <= XTimeRange;
        if (xtime > u.xtime) return xtime - u.xtime >  XTimeRange;
        return false;
    }
    bool operator==(const UserTime& u) const {
        return !(*this < u) && !(u < *this);
    }
};

class EdgeSwitch: public YWindow, public YTimerListener {
public:
    EdgeSwitch(YWindowManager *manager, int delta, bool vertical);
    virtual ~EdgeSwitch();

    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual bool handleTimer(YTimer *t);
private:
    YWindowManager *fManager;
    YCursor & fCursor;
    int fDelta;
    bool fVert;

    static lazy<YTimer> fEdgeSwitchTimer;
};

class YProxyWindow: public YWindow {
public:
    YProxyWindow(YWindow *parent);
    virtual ~YProxyWindow();

    virtual void handleButton(const XButtonEvent &button);
};

class YWindowManager: public YDesktop {
public:
    YWindowManager(
        IApp *app,
        YActionListener *wmActionListener,
        YSMListener *smListener,
        YWindow *parent,
        Window win = 0);

    virtual ~YWindowManager();

    virtual void grabKeys();

    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual bool handleKey(const XKeyEvent &key);

    virtual void handleConfigure(const XConfigureEvent &configure);
    virtual void handleConfigureRequest(const XConfigureRequestEvent &configureRequest);
    virtual void handleMapRequest(const XMapRequestEvent &mapRequest);
    virtual void handleUnmapNotify(const XUnmapEvent &unmap);
    virtual void handleDestroyWindow(const XDestroyWindowEvent &destroyWindow);
    virtual void handleClientMessage(const XClientMessageEvent &message);
    virtual void handleProperty(const XPropertyEvent &property);
    virtual void handleFocus(const XFocusChangeEvent &focus);
#ifdef CONFIG_XRANDR
    virtual void handleRRScreenChangeNotify(const XRRScreenChangeNotifyEvent &xrrsc);
#endif

    void manageClients();
    void unmanageClients();

    Window findWindow(char const * resource);
    Window findWindow(Window root, char const * wmInstance,
                      char const * wmClass);

    YFrameWindow *findFrame(Window win);
    YFrameClient *findClient(Window win);
    YFrameWindow *manageClient(Window win, bool mapClient = false);
    void unmanageClient(Window win, bool mapClient = false,
                        bool reparent = true);
    void destroyedClient(Window win);
    YFrameWindow *mapClient(Window win);

    void setFocus(YFrameWindow *f, bool canWarp = false);
    YFrameWindow *getFocus() { return fFocusWin; }

    void loseFocus(YFrameWindow *window);
    void activate(YFrameWindow *frame, bool raise, bool canWarp = false);

    void installColormap(Colormap cmap);
    void setColormapWindow(YFrameWindow *frame);
    YFrameWindow *colormapWindow() { return fColormapWindow; }

    void removeClientFrame(YFrameWindow *frame);

    void UpdateScreenSize(XEvent *event);
    void getWorkArea(YFrameWindow *frame, int *mx, int *my, int *Mx, int *My, int xiscreen = -1) const;
    void getWorkAreaSize(YFrameWindow *frame, int *Mw,int *Mh);

    int calcCoverage(bool down, YFrameWindow *frame, int x, int y, int w, int h);
    void tryCover(bool down, YFrameWindow *frame, int x, int y, int w, int h,
                  int &px, int &py, int &cover, int xiscreen);
    bool getSmartPlace(bool down, YFrameWindow *frame, int &x, int &y, int w, int h, int xiscreen);
    void getNewPosition(YFrameWindow *frame, int &x, int &y, int w, int h, int xiscreen);
    void placeWindow(YFrameWindow *frame, int x, int y, int cw, int ch, bool newClient, bool &canActivate);

    YFrameWindow *top(long layer) const;
    void setTop(long layer, YFrameWindow *top);
    YFrameWindow *bottom(long layer) const;
    void setBottom(long layer, YFrameWindow *bottom);

    YFrameWindow *topLayer(long layer = WinLayerCount - 1);
    YFrameWindow *bottomLayer(long layer = 0);

    void setAbove(YFrameWindow* frame, YFrameWindow* above);
    void setBelow(YFrameWindow* frame, YFrameWindow* below);
    void removeLayeredFrame(YFrameWindow *);
    void appendCreatedFrame(YFrameWindow *f);
    void removeCreatedFrame(YFrameWindow *f);

    YFrameIter focusedIterator() { return fFocusedOrder.iterator(); }
    YFrameIter focusedReverseIterator() { return fFocusedOrder.reverseIterator(); }
    int focusedCount() const { return fFocusedOrder.count(); }
    void insertFocusFrame(YFrameWindow* frame, bool focused);
    void removeFocusFrame(YFrameWindow* frame);
    void lowerFocusFrame(YFrameWindow* frame);
    void raiseFocusFrame(YFrameWindow* frame);

    void restackWindows(YFrameWindow *win);
    void focusTopWindow();
    YFrameWindow *getLastFocus(bool skipAllWorkspaces = false, long workspace = -1);
    void focusLastWindow();
    bool focusTop(YFrameWindow *f);
    void relocateWindows(long workspace, int screen, int dx, int dy);
    void updateClientList();
    void updateUserTime(const UserTime& userTime);

    YMenu *createWindowMenu(YMenu *menu, long workspace);
    int windowCount(long workspace);
    void popupWindowListMenu(YWindow *owner, int x, int y);

    void initWorkspaces();

    long activeWorkspace() const { return fActiveWorkspace; }
    long lastWorkspace() const { return fLastWorkspace; }
    void activateWorkspace(long workspace);
    long workspaceCount() const { return ::workspaceCount; }
    const char *workspaceName(long workspace) const { return ::workspaceNames[workspace]; }

    void appendNewWorkspace();
    void removeLastWorkspace();
    void updateWorkspaces(bool increase);

    void setShowingDesktop();
    void setShowingDesktop(bool setting);

    void updateTaskBarNames();
    void updateMoveMenu();

    bool readCurrentDesktop(long &workspace);
    void setDesktopGeometry();
    bool compareDesktopNames(char **strings, int count);
    bool readDesktopLayout();
    bool readDesktopNames();
    bool readNetDesktopNames();
    bool readWinDesktopNames();
    void setWinDesktopNames(long count);
    void setNetDesktopNames(long count);
    void setDesktopNames(long count);
    void setDesktopNames();
    void setDesktopCount();
    void setDesktopViewport();

    void announceWorkArea();
    void setWinWorkspace(long workspace);
    void updateWorkArea();
    void resizeWindows();

    void getIconPosition(YFrameWindow *frame, int *iconX, int *iconY);

    void wmCloseSession();
    void exitAfterLastClient(bool shuttingDown);
    static void execAfterFork(const char *command);
    void checkLogout();

    virtual void resetColormap(bool active);

    void switchFocusTo(YFrameWindow *frame, bool reorderFocus = true);
    void switchFocusFrom(YFrameWindow *frame);
    void notifyActive(YFrameWindow *frame);

    void popupStartMenu(YWindow *owner);
    void popupWindowListMenu(YWindow *owner);

    void switchToWorkspace(long nw, bool takeCurrent);
    void switchToPrevWorkspace(bool takeCurrent);
    void switchToNextWorkspace(bool takeCurrent);
    void switchToLastWorkspace(bool takeCurrent);

    void tilePlace(YFrameWindow *w, int tx, int ty, int tw, int th);
    void tileWindows(YFrameWindow **w, int count, bool vertical);
    void smartPlace(YFrameWindow **w, int count);
    void getCascadePlace(YFrameWindow *frame, int &lastX, int &lastY, int &x, int &y, int w, int h);
    void cascadePlace(YFrameWindow **w, int count);
    void setWindows(YFrameWindow **w, int count, YAction action);

    void getWindowsToArrange(YFrameWindow ***w, int *count, bool sticky = false, bool skipNonMinimizable = false);

    void saveArrange(YFrameWindow **w, int count);
    void undoArrange();

    bool haveClients();
    void setupRootProxy();

    void setWorkAreaMoveWindows(bool m) { fWorkAreaMoveWindows = m; }

    void updateFullscreenLayer();
    void updateFullscreenLayerEnable(bool enable);
    int getScreen();

    static void doWMAction(WMAction action);
    void lockFocus() {
        //MSG(("lockFocus %d", lockFocusCount));
        lockFocusCount++;
    }
    void unlockFocus() {
        lockFocusCount--;
        //MSG(("unlockFocus %d", lockFocusCount));
    }
    bool focusLocked() { return lockFocusCount != 0; }

    enum WMState { wmSTARTUP, wmRUNNING, wmSHUTDOWN };

    WMState wmState() const { return fWmState; }
    bool fullscreenEnabled() { return fFullscreenEnabled; }
    void setFullscreenEnabled(bool enable) { fFullscreenEnabled = enable; }
    const UserTime& lastUserTime() const { return fLastUserTime; }

    struct DesktopLayout {
        int orient;
        int columns;
        int rows;
        int corner;
    };

    const DesktopLayout& layout() const { return fLayout; }

private:
    struct WindowPosState {
        int x, y, w, h;
        long state;
        YFrameWindow *frame;
    };

    void updateArea(long workspace, int screen_number, int l, int t, int r, int b);
    bool handleWMKey(const XKeyEvent &key, KeySym k, unsigned int m, unsigned int vm);

    IApp *app;
    YActionListener *wmActionListener;
    YSMListener *smActionListener;
    Window fActiveWindow;
    YFrameWindow *fFocusWin;
    YLayeredList fLayers[WinLayerCount];
    YCreatedList fCreationOrder;  // frame creation order
    YFocusedList fFocusedOrder;   // focus order: old -> now
    YFrameWindow *fFocusedWindow[MAXWORKSPACES];

    long fActiveWorkspace;
    long fLastWorkspace;
    YFrameWindow *fColormapWindow;

    long fWorkAreaWorkspaceCount;
    int fWorkAreaScreenCount;
    struct WorkAreaRect {
        int fMinX, fMinY, fMaxX, fMaxY;
    } **fWorkArea;

    EdgeSwitch *fLeftSwitch, *fRightSwitch, *fTopSwitch, *fBottomSwitch;
    bool fShuttingDown;
    int fArrangeCount;
    WindowPosState *fArrangeInfo;
    YProxyWindow *rootProxy;
    YWindow *fTopWin;
    bool fWorkAreaMoveWindows;
    bool fOtherScreenFocused;
    int lockFocusCount;
    bool fFullscreenEnabled;

    WMState fWmState;
    UserTime fLastUserTime;
    bool fShowingDesktop;
    bool fCreatedUpdated;
    bool fLayeredUpdated;

    DesktopLayout fLayout;
};

extern YWindowManager *manager;

void dumpZorder(const char *oper, YFrameWindow *w, YFrameWindow *a = 0);

extern Atom _XA_WIN_APP_STATE;
extern Atom _XA_WIN_AREA_COUNT;
extern Atom _XA_WIN_AREA;
extern Atom _XA_WIN_CLIENT_LIST;
extern Atom _XA_WIN_DESKTOP_BUTTON_PROXY;
extern Atom _XA_WIN_EXPANDED_SIZE;
extern Atom _XA_WIN_HINTS;
extern Atom _XA_WIN_ICONS;
extern Atom _XA_WIN_LAYER;
extern Atom _XA_WIN_PROTOCOLS;
extern Atom _XA_WIN_STATE;
extern Atom _XA_WIN_SUPPORTING_WM_CHECK;
extern Atom _XA_WIN_TRAY;
extern Atom _XA_WIN_WORKAREA;
extern Atom _XA_WIN_WORKSPACE_COUNT;
extern Atom _XA_WIN_WORKSPACE_NAMES;
extern Atom _XA_WIN_WORKSPACE;

extern Atom _XA_WM_CLIENT_LEADER;
extern Atom _XA_WM_CLIENT_MACHINE;
extern Atom _XA_WM_WINDOW_ROLE;

extern Atom _XA_WINDOW_ROLE;
extern Atom _XA_SM_CLIENT_ID;
extern Atom _XA_UTF8_STRING;

extern Atom _XA_ICEWM_ACTION;

/// _SET would be nice to have
#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1
#define _NET_WM_STATE_TOGGLE 2

#define _NET_WM_MOVERESIZE_SIZE_TOPLEFT      0
#define _NET_WM_MOVERESIZE_SIZE_TOP          1
#define _NET_WM_MOVERESIZE_SIZE_TOPRIGHT     2
#define _NET_WM_MOVERESIZE_SIZE_RIGHT        3
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT  4
#define _NET_WM_MOVERESIZE_SIZE_BOTTOM       5
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT   6
#define _NET_WM_MOVERESIZE_SIZE_LEFT         7
#define _NET_WM_MOVERESIZE_MOVE              8 /* Movement only */
#define _NET_WM_MOVERESIZE_SIZE_KEYBOARD     9
#define _NET_WM_MOVERESIZE_MOVE_KEYBOARD    10
#define _NET_WM_MOVERESIZE_CANCEL           11

#define _NET_WM_ORIENTATION_HORZ    0
#define _NET_WM_ORIENTATION_VERT    1
#define _NET_WM_TOPLEFT     0
#define _NET_WM_TOPRIGHT    1
#define _NET_WM_BOTTOMRIGHT 2
#define _NET_WM_BOTTOMLEFT  3


extern Atom _XA_NET_ACTIVE_WINDOW;                  // OK
extern Atom _XA_NET_CLIENT_LIST;                    // OK
extern Atom _XA_NET_CLIENT_LIST_STACKING;           // OK
extern Atom _XA_NET_CLOSE_WINDOW;                   // OK
extern Atom _XA_NET_CURRENT_DESKTOP;                // OK
extern Atom _XA_NET_DESKTOP_GEOMETRY;               // OK
extern Atom _XA_NET_DESKTOP_LAYOUT;                 // OK
extern Atom _XA_NET_DESKTOP_NAMES;                  //*OK
extern Atom _XA_NET_DESKTOP_VIEWPORT;               // OK (trivial)
extern Atom _XA_NET_FRAME_EXTENTS;                  // OK
extern Atom _XA_NET_MOVERESIZE_WINDOW;              //*OK
extern Atom _XA_NET_NUMBER_OF_DESKTOPS;             //*OK
extern Atom _XA_NET_PROPERTIES;                     // N/A (obsolete)
extern Atom _XA_NET_REQUEST_FRAME_EXTENTS;          // OK
extern Atom _XA_NET_RESTACK_WINDOW;                 // OK
extern Atom _XA_NET_SHOWING_DESKTOP;                // OK
extern Atom _XA_NET_STARTUP_ID;                     // OK
extern Atom _XA_NET_STARTUP_INFO;                   // TODO
extern Atom _XA_NET_STARTUP_INFO_BEGIN;             // TODO
extern Atom _XA_NET_SUPPORTED;                      // OK
extern Atom _XA_NET_SUPPORTING_WM_CHECK;            // OK
extern Atom _XA_NET_SYSTEM_TRAY_MESSAGE_DATA;       // OK
extern Atom _XA_NET_SYSTEM_TRAY_OPCODE;             // OK
extern Atom _XA_NET_SYSTEM_TRAY_ORIENTATION;        // TODO
extern Atom _XA_NET_SYSTEM_TRAY_VISUAL;             // TODO
extern Atom _XA_NET_VIRTUAL_ROOTS;                  // N/A
extern Atom _XA_NET_WM_ACTION_ABOVE;                // OK
extern Atom _XA_NET_WM_ACTION_BELOW;                // OK
extern Atom _XA_NET_WM_ACTION_CHANGE_DESKTOP;       // OK
extern Atom _XA_NET_WM_ACTION_CLOSE;                // OK
extern Atom _XA_NET_WM_ACTION_FULLSCREEN;           // OK
extern Atom _XA_NET_WM_ACTION_MAXIMIZE_HORZ;        // OK
extern Atom _XA_NET_WM_ACTION_MAXIMIZE_VERT;        // OK
extern Atom _XA_NET_WM_ACTION_MINIMIZE;             // OK
extern Atom _XA_NET_WM_ACTION_MOVE;                 // OK
extern Atom _XA_NET_WM_ACTION_RESIZE;               // OK
extern Atom _XA_NET_WM_ACTION_SHADE;                // OK
extern Atom _XA_NET_WM_ACTION_STICK;                // OK
extern Atom _XA_NET_WM_ALLOWED_ACTIONS;             // OK
extern Atom _XA_NET_WM_BYPASS_COMPOSITOR;           // N/A
extern Atom _XA_NET_WM_DESKTOP;                     // OK
extern Atom _XA_NET_WM_FULL_PLACEMENT;              // OK
extern Atom _XA_NET_WM_FULLSCREEN_MONITORS;         // OK
extern Atom _XA_NET_WM_HANDLED_ICONS;               // TODO -> toggle minimizeToDesktop
extern Atom _XA_NET_WM_ICON_GEOMETRY;               // N/A
extern Atom _XA_NET_WM_ICON_NAME;                   // OK
extern Atom _XA_NET_WM_ICON;                        // OK
extern Atom _XA_NET_WM_MOVERESIZE;                  //*OK
extern Atom _XA_NET_WM_NAME;                        // OK
extern Atom _XA_NET_WM_OPAQUE_REGION;               // TODO
extern Atom _XA_NET_WM_PID;                         // OK (trivial)
extern Atom _XA_NET_WM_PING;                        // OK
extern Atom _XA_NET_WM_STATE;                       // OK
extern Atom _XA_NET_WM_STATE_ABOVE;                 // OK
extern Atom _XA_NET_WM_STATE_BELOW;                 // OK
extern Atom _XA_NET_WM_STATE_DEMANDS_ATTENTION;     // OK
extern Atom _XA_NET_WM_STATE_FOCUSED;               // OK
extern Atom _XA_NET_WM_STATE_FULLSCREEN;            // OK
extern Atom _XA_NET_WM_STATE_HIDDEN;                // OK
extern Atom _XA_NET_WM_STATE_MAXIMIZED_HORZ;        // OK
extern Atom _XA_NET_WM_STATE_MAXIMIZED_VERT;        // OK
extern Atom _XA_NET_WM_STATE_MODAL;                 // OK (state only)
extern Atom _XA_NET_WM_STATE_SHADED;                // OK
extern Atom _XA_NET_WM_STATE_SKIP_PAGER;            // OK (trivial)
extern Atom _XA_NET_WM_STATE_SKIP_TASKBAR;          // OK
extern Atom _XA_NET_WM_STATE_STICKY;                // OK (trivial)
extern Atom _XA_NET_WM_STRUT;                       // OK
extern Atom _XA_NET_WM_STRUT_PARTIAL;               // OK (minimal)
extern Atom _XA_NET_WM_SYNC_REQUEST;                // TODO
extern Atom _XA_NET_WM_SYNC_REQUEST_COUNTER;        // TODO
extern Atom _XA_NET_WM_USER_TIME;                   // OK
extern Atom _XA_NET_WM_USER_TIME_WINDOW;            // OK
extern Atom _XA_NET_WM_VISIBLE_ICON_NAME;           // OK
extern Atom _XA_NET_WM_VISIBLE_NAME;                // OK
extern Atom _XA_NET_WM_WINDOW_OPACITY;              // OK
extern Atom _XA_NET_WM_WINDOW_TYPE;                 // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_COMBO;           // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_DESKTOP;         // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_DIALOG;          // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_DND;             // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_DOCK;            // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_DROPDOWN_MENU;   // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_MENU;            // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_NORMAL;          // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_NOTIFICATION;    // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_POPUP_MENU;      // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_SPLASH;          // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_TOOLBAR;         // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_TOOLTIP;         // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_UTILITY;         // OK
extern Atom _XA_NET_WORKAREA;                       // OK

// TODO extra:
// original geometry of maximized window
//

/* KDE specific */
extern Atom _XA_KWM_DOCKWINDOW;
extern Atom _XA_KWM_WIN_ICON;

extern Atom _XA_KDE_NET_SYSTEM_TRAY_WINDOWS;
extern Atom _XA_KDE_NET_WM_FRAME_STRUT;
extern Atom _XA_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR;
extern Atom _XA_KDE_NET_WM_WINDOW_TYPE_OVERRIDE;
extern Atom _XA_KDE_SPLASH_PROGRESS;
extern Atom _XA_KDE_WM_CHANGE_STATE;

extern Atom XA_IcewmWinOptHint;

#endif

// vim: set sw=4 ts=4 et:
