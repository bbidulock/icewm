#ifndef __WMMGR_H
#define __WMMGR_H

#include <X11/Xproto.h>
#include "ywindow.h"
#include "ymenu.h"
#include "WinMgr.h"
#include "ytimer.h"

#define MAXWORKSPACES 64
#define INVALID_WORKSPACE 0xFFFFFFFF

extern long workspaceCount;
extern char *workspaceNames[MAXWORKSPACES];
extern YAction *workspaceActionActivate[MAXWORKSPACES];
extern YAction *workspaceActionMoveTo[MAXWORKSPACES];
extern YAction *layerActionSet[WinLayerCount];

class YWindowManager;
class YFrameClient;
class YFrameWindow;

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

    static YTimer *fEdgeSwitchTimer;
};

class YProxyWindow: public YWindow {
public:
    YProxyWindow(YWindow *parent);
    virtual ~YProxyWindow();

    virtual void handleButton(const XButtonEvent &button);
};

class YWindowManager: public YDesktop {
public:
    YWindowManager(YWindow *parent, Window win = 0);
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

    YFrameWindow *top(long layer) const { return fTop[layer]; }
    void setTop(long layer, YFrameWindow *top);
    YFrameWindow *bottom(long layer) const { return fBottom[layer]; }
    void setBottom(long layer, YFrameWindow *bottom) { fBottom[layer] = bottom; }

    YFrameWindow *topLayer(long layer = WinLayerCount - 1);
    YFrameWindow *bottomLayer(long layer = 0);

    YFrameWindow *firstFrame() { return fFirst; }
    YFrameWindow *lastFrame() { return fLast; }
    void setFirstFrame(YFrameWindow *f) { fFirst = f; }
    void setLastFrame(YFrameWindow *f) { fLast = f; }

    YFrameWindow *firstFocusFrame() { return fFirstFocus; }
    YFrameWindow *lastFocusFrame() { return fLastFocus; }
    void setFirstFocusFrame(YFrameWindow *f) { fFirstFocus = f; }
    void setLastFocusFrame(YFrameWindow *f) { fLastFocus = f; }

    void restackWindows(YFrameWindow *win);
    void focusTopWindow();
    YFrameWindow *getLastFocus(bool skipSticky = false, long workspace = -1);
    void focusLastWindow();
    bool focusTop(YFrameWindow *f);
    void relocateWindows(long workspace, int screen, int dx, int dy);
    void updateClientList();

    YMenu *createWindowMenu(YMenu *menu, long workspace);
    int windowCount(long workspace);
#ifdef CONFIG_WINMENU
    void popupWindowListMenu(YWindow *owner, int x, int y);
#endif

    long activeWorkspace() const { return fActiveWorkspace; }
    long lastWorkspace() const { return fLastWorkspace; }
    void activateWorkspace(long workspace);
    long workspaceCount() const { return ::workspaceCount; }
    const char *workspaceName(long workspace) const { return ::workspaceNames[workspace]; }

    void announceWorkArea();
    void setWinWorkspace(long workspace);
    void updateWorkArea();
    void resizeWindows();

    void getIconPosition(YFrameWindow *frame, int *iconX, int *iconY);

    void wmCloseSession();
    void exitAfterLastClient(bool shuttingDown);
    void checkLogout();

    virtual void resetColormap(bool active);

    void switchFocusTo(YFrameWindow *frame, bool reorderFocus = true);
    void switchFocusFrom(YFrameWindow *frame);
    void notifyFocus(YFrameWindow *frame);

    void popupStartMenu(YWindow *owner);
#ifdef CONFIG_WINMENU
    void popupWindowListMenu(YWindow *owner);
#endif

    void switchToWorkspace(long nw, bool takeCurrent);
    void switchToPrevWorkspace(bool takeCurrent);
    void switchToNextWorkspace(bool takeCurrent);
    void switchToLastWorkspace(bool takeCurrent);

    void tilePlace(YFrameWindow *w, int tx, int ty, int tw, int th);
    void tileWindows(YFrameWindow **w, int count, bool vertical);
    void smartPlace(YFrameWindow **w, int count);
    void getCascadePlace(YFrameWindow *frame, int &lastX, int &lastY, int &x, int &y, int w, int h);
    void cascadePlace(YFrameWindow **w, int count);
    void setWindows(YFrameWindow **w, int count, YAction *action);

    void getWindowsToArrange(YFrameWindow ***w, int *count, bool sticky = false, bool skipNonMinimizable = false);

    void saveArrange(YFrameWindow **w, int count);
    void undoArrange();

    bool haveClients();
    void setupRootProxy();

    void setWorkAreaMoveWindows(bool m) { fWorkAreaMoveWindows = m; }

    void updateFullscreenLayer();
    void updateFullscreenLayerEnable(bool enable);
    int getScreen();

    void doWMAction(long action);
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
private:
    struct WindowPosState {
        int x, y, w, h;
        long state;
        YFrameWindow *frame;
    };

    void updateArea(long workspace, int screen_number, int l, int t, int r, int b);
    bool handleWMKey(const XKeyEvent &key, KeySym k, unsigned int m, unsigned int vm);

    YFrameWindow *fFocusWin;
    YFrameWindow *fTop[WinLayerCount];
    YFrameWindow *fBottom[WinLayerCount];
    YFrameWindow *fFirst, *fLast; // creation order
    YFrameWindow *fFirstFocus, *fLastFocus; // focus order
    YFrameWindow **fFocusedWindow;
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
};

extern YWindowManager *manager;

void dumpZorder(const char *oper, YFrameWindow *w, YFrameWindow *a = 0);

extern Atom _XA_WIN_PROTOCOLS;
extern Atom _XA_WIN_WORKSPACE;
extern Atom _XA_WIN_WORKSPACE_COUNT;
extern Atom _XA_WIN_WORKSPACE_NAMES;
extern Atom _XA_WIN_WORKAREA;
extern Atom _XA_WIN_LAYER;
#ifdef CONFIG_TRAY
extern Atom _XA_WIN_TRAY;
#endif
extern Atom _XA_WIN_ICONS;
extern Atom _XA_WIN_HINTS;
extern Atom _XA_WIN_STATE;
extern Atom _XA_WIN_SUPPORTING_WM_CHECK;
extern Atom _XA_WIN_CLIENT_LIST;
extern Atom _XA_WIN_DESKTOP_BUTTON_PROXY;
extern Atom _XA_WIN_AREA;
extern Atom _XA_WIN_AREA_COUNT;
extern Atom _XA_WM_CLIENT_LEADER;
extern Atom _XA_WM_WINDOW_ROLE;
extern Atom _XA_WINDOW_ROLE;
extern Atom _XA_SM_CLIENT_ID;

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
                                                 //*=testnetwmhints
extern Atom _XA_NET_SUPPORTED;                   // OK
extern Atom _XA_NET_CLIENT_LIST;                 // OK (perf: don't update on stacking changes)
extern Atom _XA_NET_CLIENT_LIST_STACKING;        // OK
extern Atom _XA_NET_NUMBER_OF_DESKTOPS;          // implement change request
///extern Atom _XA_NET_DESKTOP_GEOMETRY;         // N/A
///extern Atom _XA_NET_DESKTOP_VIEWPORT;         // N/A
extern Atom _XA_NET_CURRENT_DESKTOP;             // OK
///extern Atom _XA_NET_DESKTOP_NAMES;            // N/A
extern Atom _XA_NET_ACTIVE_WINDOW;               // OK
extern Atom _XA_NET_WORKAREA;                    // OK
extern Atom _XA_NET_SUPPORTING_WM_CHECK;         // OK
///extern Atom _XA_NET_SUPPORTING_WM_CHECK;      // N/A
extern Atom _XA_NET_CLOSE_WINDOW;                // OK
extern Atom _XA_NET_WM_MOVERESIZE;               //*OK
extern Atom _XA_NET_WM_NAME;                     // TODO
extern Atom _XA_NET_WM_VISIBLE_NAME;             // TODO
extern Atom _XA_NET_WM_ICON_NAME;                // TODO
extern Atom _XA_NET_WM_VISIBLE_ICON_NAME;        // TODO
extern Atom _XA_NET_WM_DESKTOP;                  // OK
extern Atom _XA_NET_WM_WINDOW_TYPE;              // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_DESKTOP;      // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_DOCK;         // OK
///extern Atom _XA_NET_WM_WINDOW_TYPE_TOOLBAR;   // N/A
///extern Atom _XA_NET_WM_WINDOW_TYPE_MENU;      // N/A
///extern Atom _XA_NET_WM_WINDOW_TYPE_UTILITY;   // N/A
extern Atom _XA_NET_WM_WINDOW_TYPE_SPLASH;       // N/A
extern Atom _XA_NET_WM_WINDOW_TYPE_DIALOG;       // TODO
extern Atom _XA_NET_WM_WINDOW_TYPE_NORMAL;       // TODO
extern Atom _XA_NET_WM_STATE;                    // OK
extern Atom _XA_NET_WM_STATE_MODAL;              // TODO (broken)
///extern Atom _XA_NET_WM_STATE_STICKY;          // N/A
extern Atom _XA_NET_WM_STATE_MAXIMIZED_VERT;     // OK
extern Atom _XA_NET_WM_STATE_MAXIMIZED_HORZ;     // OK
extern Atom _XA_NET_WM_STATE_SHADED;             // OK
extern Atom _XA_NET_WM_STATE_SKIP_TASKBAR;       // OK
///extern Atom _XA_NET_WM_STATE_SKIP_PAGER;      // N/A
extern Atom _XA_NET_WM_STATE_HIDDEN;             // TODO
extern Atom _XA_NET_WM_STATE_FULLSCREEN;         //*OK
extern Atom _XA_NET_WM_STATE_ABOVE;              //*OK
extern Atom _XA_NET_WM_STATE_BELOW;              //*OK

extern Atom _XA_NET_WM_ALLOWED_ACTIONS;          // TODO
extern Atom _XA_NET_WM_ACTION_MOVE;              // TODO
extern Atom _XA_NET_WM_ACTION_RESIZE;            // TODO
extern Atom _XA_NET_WM_ACTION_SHADE;             // TODO
///extern Atom _XA_NET_WM_ACTION_STICK;          // N/A
extern Atom _XA_NET_WM_ACTION_MAXIMIZE_HORZ;     // TODO
extern Atom _XA_NET_WM_ACTION_MAXIMIZE_VERT;     // TODO
extern Atom _XA_NET_WM_ACTION_CHANGE_DESKTOP;    // TODO
extern Atom _XA_NET_WM_ACTION_CLOSE;             // TODO

extern Atom _XA_NET_WM_STRUT;                    // OK
///extern Atom _XA_NET_WM_ICON_GEOMETRY;         // N/A
extern Atom _XA_NET_WM_ICON;                     // TODO
extern Atom _XA_NET_WM_PID;                      // TODO
extern Atom _XA_NET_WM_HANDLED_ICONS;            // TODO -> toggle minimizeToDesktop
extern Atom _XA_NET_WM_PING;                     // TODO

extern Atom _XA_NET_WM_USER_TIME;                // TODO
extern Atom _XA_NET_WM_STATE_DEMANDS_ATTENTION;  // TODO

// TODO extra:
// original geometry of maximized window
//

/* KDE specific */
extern Atom _XA_KWM_WIN_ICON;
extern Atom _XA_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR;

extern Atom XA_IcewmWinOptHint;

extern ref<YIcon> defaultAppIcon;

#endif
