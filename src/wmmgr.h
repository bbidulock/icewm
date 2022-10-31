#ifndef WMMGR_H
#define WMMGR_H

#include "WinMgr.h"
#include "ylist.h"
#include "ymsgbox.h"
#include "ypopup.h"
#include "workspaces.h"

extern YAction layerActionSet[WinLayerCount];

class YStringList;
class YWindowManager;
class YFrameClient;
class YFrameWindow;
class YSMListener;
class SwitchWindow;
class MiniIcon;
class DockApp;
class IApp;

class EdgeSwitch: public YDndWindow, public YTimerListener {
public:
    EdgeSwitch(YWindowManager *manager, int delta, bool vertical);
    virtual ~EdgeSwitch();

    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual bool handleTimer(YTimer *t);
    virtual void handleDNDEnter();
    virtual void handleDNDLeave();
    void setGeometry();
    int destination();
private:
    YWindowManager *fManager;
    Cursor fCursor;
    int fDelta;
    bool fVert;

    lazy<YTimer> fEdgeSwitchTimer;
};

class YProxyWindow: public YWindow {
public:
    YProxyWindow(YWindow *parent);
    virtual ~YProxyWindow();

    virtual void handleButton(const XButtonEvent &button);
};

class YArrange {
public:
    YArrange(YFrameWindow** w, int n) : win(w), count(n) { }
    void discard() { delete[] win; win = nullptr; count = 0; }
    int size() const { return count; }
    YFrameWindow** begin() const { return win; }
    YFrameWindow** end() const { return win + count; }
    YFrameWindow* operator[](int index) const { return win[index]; }
    operator bool() const { return win && count; }
private:
    YFrameWindow** win;
    int count;
};

class YTopWindow : public YWindow {
public:
    YTopWindow();
    bool handleKey(const XKeyEvent& key) override;
    void setFrame(YFrameWindow* frame);

private:
    YFrameWindow* fFrame;
    Window fHandle;
};

class YWindowManager:
    private YDesktop,
    private YMsgBoxListener,
    private YTimerListener,
    public YPopDownListener
{
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
    virtual void handleMapNotify(const XMapEvent& map);
    virtual void handleUnmapNotify(const XUnmapEvent &unmap);
    virtual void handleClientMessage(const XClientMessageEvent &message);
    virtual void handleProperty(const XPropertyEvent &property);
    virtual void handleFocus(const XFocusChangeEvent &focus);
#ifdef CONFIG_XRANDR
    virtual void handleRRScreenChangeNotify(const XRRScreenChangeNotifyEvent &xrrsc);
    virtual void handleRRNotify(const XRRNotifyEvent &notify);
#endif
    virtual void handleMsgBox(YMsgBox *msgbox, int operation);
    virtual void handlePopDown(YPopupWindow* popup);
    virtual bool handleTimer(YTimer* timer);

    void manageClients();
    void unmanageClients();
    void grabServer();
    void ungrabServer();

    Window findWindow(char const* resource);
    Window findWindow(Window root, char const* resource, int maxdepth);
    bool matchWindow(Window win, char const* resource);

    YFrameWindow *findFrame(Window win);
    YFrameClient *findClient(Window win);
    void manageClient(YFrameClient* client, bool mapClient = false);
    void unmanageClient(YFrameClient *client);
    void clientDestroyed(YFrameClient* client);
    void clientTransfered(YFrameClient* client, YFrameWindow* frame);
    void destroyedClient(Window win);
    void mapClient(Window win);

    void setFocus(YFrameWindow *f, bool canWarp = false, bool reorder = true);
    YFrameWindow* getFocus() const { return fFocusWin; }
    ClientData* getFocused() const;

    void installColormap(Colormap cmap);
    void setColormapWindow(YFrameWindow *frame);
    YFrameWindow *colormapWindow() { return fColormapWindow; }

    void removeClientFrame(YFrameWindow *frame);

    void updateScreenSize(XEvent *event);
    void getWorkArea(int *mx, int *my, int *Mx, int *My);
    void getWorkArea(const YFrameWindow *frame, int *mx, int *my, int *Mx, int *My, int xiscreen = -1);
    void getWorkAreaSize(YFrameWindow *frame, int *Mw,int *Mh);

    int calcCoverage(bool down, YFrameWindow *frame, int x, int y, int w, int h);
    void tryCover(bool down, YFrameWindow *frame, int x, int y, int w, int h,
                  int& px, int& py, int& cover, int mx, int my, int Mx, int My);
    bool getSmartPlace(bool down, YFrameWindow *frame, int &x, int &y, int w, int h, int xiscreen);
    void getNewPosition(YFrameWindow *frame, int &x, int &y, int w, int h, int xiscreen);
    void placeWindow(YFrameWindow *frame, int x, int y, int cw, int ch, bool newClient, bool &canActivate);

    YFrameWindow* top(int layer) const;
    void setTop(int layer, YFrameWindow *top);
    YFrameWindow* bottom(int layer) const;
    void setBottom(int layer, YFrameWindow *bottom);
    YWindow* bottomWindow() const { return fBottom; }

    YFrameWindow* topLayer(int layer = WinLayerCount - 1);
    YFrameWindow* bottomLayer(int layer = 0);

    bool setAbove(YFrameWindow* frame, YFrameWindow* above);
    bool setBelow(YFrameWindow* frame, YFrameWindow* below);
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

    void restackWindows();
    void focusTopWindow();
    YFrameWindow *getFrameUnderMouse(long workspace = AllWorkspaces);
    YFrameWindow *getLastFocus(bool skipAllWorkspaces = false, long workspace = AllWorkspaces);
    void focusLastWindow();
    bool focusTop(YFrameWindow *f);
    void updateClientList();
    void updateUserTime(const UserTime& userTime);

    int windowCount(long workspace);
    void popupWindowListMenu(YWindow *owner, int x, int y);

    void initWorkspaces();

    int activeWorkspace() const { return fActiveWorkspace; }
    int lastWorkspace() const { return fLastWorkspace; }
    void activateWorkspace(int workspace);

    void extendWorkspaces(int target);
    void lessenWorkspaces(int target);
    void updateWorkspaces(bool increase);

    void setShowingDesktop();
    void setShowingDesktop(bool setting);

    void updateTaskBarNames();
    void updateMoveMenu();

    bool readCurrentDesktop(long &workspace);
    void setDesktopGeometry();
    bool compareDesktopNames(const YStringList& list);
    bool readDesktopLayout();
    void readDesktopNames(bool init, bool net);
    bool readNetDesktopNames(YStringList& list);
    bool readWinDesktopNames(YStringList& list);
    void setWinDesktopNames(long count);
    void setNetDesktopNames(long count);
    void setDesktopNames(long count);
    void setDesktopNames();
    void setDesktopCount();
    void setDesktopViewport();

    void announceWorkArea();
    void updateWorkArea();
    bool updateWorkAreaInner();
    void debugWorkArea(const char* prefix);
    void workAreaUpdated();
    void resizeWindows();

    void getIconPosition(MiniIcon* iw, int *iconX, int *iconY);

    void wmCloseSession();
    void exitAfterLastClient(bool exitWhenDone);
    void checkLogout();

    virtual void resetColormap(bool active);

    void switchFocusTo(YFrameWindow *frame, bool reorderFocus = true);
    void switchFocusFrom(YFrameWindow *frame);
    void notifyActive(YFrameWindow *frame);

    void popupStartMenu(YWindow *owner);
    void popupWindowListMenu(YWindow *owner);

    void switchToWorkspace(int nw, bool takeCurrent);
    void switchToPrevWorkspace(bool takeCurrent);
    void switchToNextWorkspace(bool takeCurrent);
    void switchToLastWorkspace(bool takeCurrent);

    void tilePlace(YFrameWindow *w, int tx, int ty, int tw, int th);
    void tileWindows(YArrange arrange, bool vertical);
    void smartPlace(YArrange arrange);
    void getCascadePlace(YFrameWindow *frame, int &lastX, int &lastY, int &x, int &y, int w, int h);
    void cascadePlace(YArrange arrange);
    void setWindows(YArrange arrange, YAction action);

    YArrange getWindowsToArrange(bool sticky = false, bool mini = false, bool full = false);

    bool saveArrange(YArrange arrange);
    void undoArrange();
    void arrangeIcons();
    void tileWindows(bool vertical);
    void arrangeWindows();
    void actionWindows(YAction action);
    void toggleDesktop();
    void cascadeWindows();

    bool haveClients();
    void setupRootProxy();
    void setKeyboard(mstring keyboard);
    void setKeyboard(int configIndex);
    mstring getKeyboard();
    void updateKeyboard(int configIndex);
    void reflectKeyboard(int configIndex, mstring keyboard);
    void kbLayout();

    static void doWMAction(WMAction action, bool putback = false);
    void lockFocus() {
        //MSG(("lockFocus %d", lockFocusCount));
        lockFocusCount++;
    }
    void unlockFocus() {
        lockFocusCount--;
        //MSG(("unlockFocus %d", lockFocusCount));
    }
    bool focusLocked() { return lockFocusCount != 0; }
    void lockWorkArea() { ++fWorkAreaLock; }
    void unlockWorkArea() {
        if (0 == --fWorkAreaLock && fWorkAreaUpdate)
            updateWorkArea();
    }
    void requestWorkAreaUpdate() { ++fWorkAreaUpdate; }

    void lockRestack() { fRestackLock++; }
    void unlockRestack() {
        if (0 == --fRestackLock && fRestackUpdate) {
            fRestackUpdate = 0;
            restackWindows();
        }
    }

    enum WMState { wmSTARTUP, wmRUNNING, wmSHUTDOWN };

    WMState wmState() const { return fWmState; }
    bool isStartup() const { return fWmState == wmSTARTUP; }
    bool isRunning() const { return fWmState == wmRUNNING; }
    bool notRunning() const { return fWmState != wmRUNNING; }
    bool notShutting() const { return fWmState != wmSHUTDOWN; }
    bool shuttingDown() const { return fWmState == wmSHUTDOWN; }
    bool fullscreenEnabled() { return fFullscreenEnabled; }
    void setFullscreenEnabled(bool enable) { fFullscreenEnabled = enable; }
    const UserTime& lastUserTime() const { return fLastUserTime; }

    struct DesktopLayout {
        int orient;
        int columns;
        int rows;
        int corner;
        DesktopLayout(int o, int c, int r, int k) :
            orient(o), columns(c), rows(r), corner(k) { }
    };

    const DesktopLayout& layout() const { return fLayout; }
    bool handleSwitchWorkspaceKey(const XKeyEvent& key, KeySym k, unsigned vm);

    int getSwitchScreen();
    bool switchWindowVisible() const;
    SwitchWindow* getSwitchWindow();
    Window netActiveWindow() const { return fActiveWindow; }
    int edgeWorkspace(int x, int y);

private:
    struct WindowPosState {
        int x, y, w, h;
        long state;
        YFrameWindow *frame;
    };

    bool ignoreOverride(Window win, const XWindowAttributes& attr, int* layer);
    YFrameClient* allocateClient(Window win, bool mapClient);
    YFrameWindow* allocateFrame(YFrameClient* client);
    void updateArea(int workspace, int screen_number, int l, int t, int r, int b);
    bool handleWMKey(const XKeyEvent &key, KeySym k, unsigned vm);
    void setWmState(WMState newWmState);
    void refresh();

    IApp *app;
    YActionListener *wmActionListener;
    YSMListener *smActionListener;
    Window fActiveWindow;
    YFrameWindow *fFocusWin;
    YLayeredList fLayers[WinLayerCount];
    YCreatedList fCreationOrder;  // frame creation order
    YFocusedList fFocusedOrder;   // focus order: old -> now

    int fActiveWorkspace;
    int fLastWorkspace;
    YFrameWindow *fColormapWindow;

    int fWorkAreaWorkspaceCount;
    int fWorkAreaScreenCount;
    struct WorkAreaRect {
        int fMinX, fMinY, fMaxX, fMaxY;
        bool operator!=(const WorkAreaRect& o) const {
            return fMinX != o.fMinX || fMinY != o.fMinY
                || fMaxX != o.fMaxX || fMaxY != o.fMaxY;
        }
        bool displaced(const WorkAreaRect& o) const {
            return fMinX != o.fMinX || fMinY != o.fMinY;
        }
        bool resized(const WorkAreaRect& o) const {
            return fMaxX != o.fMaxX || fMaxY != o.fMaxY;
        }
        void operator=(const DesktopScreenInfo& o) {
            fMaxX = o.width + (fMinX = o.x_org);
            fMaxY = o.height + (fMinY = o.y_org);
        }
        int width() const { return fMaxX - fMinX; }
        int height() const { return fMaxY - fMinY; }
        operator YRect() const {
            return YRect(fMinX, fMinY, width(), height());
        }
    } **fWorkArea;

    YObjectArray<EdgeSwitch> edges;
    bool fExitWhenDone;
    int fArrangeCount;
    WindowPosState *fArrangeInfo;
    YProxyWindow *rootProxy;
    YTopWindow *fTopWin;
    YWindow *fBottom;
    int fCascadeX;
    int fCascadeY;
    int fIconColumn;
    int fIconRow;
    int lockFocusCount;
    int fWorkAreaLock;
    int fWorkAreaUpdate;
    int fRestackLock;
    int fRestackUpdate;
    int fServerGrabCount;
    bool fFullscreenEnabled;

    WMState fWmState;
    UserTime fLastUserTime;
    bool fShowingDesktop;
    bool fCreatedUpdated;
    bool fLayeredUpdated;

    DesktopLayout fLayout;
    mstring fCurrentKeyboard;
    int fDefaultKeyboard;
    SwitchWindow* fSwitchWindow;
    lazy<YTimer> fSwitchDownTimer;
    lazy<YTimer> fLayoutTimer;
    DockApp* fDockApp;
};

extern YWindowManager *manager;

class YRestackLock {
public:
    YRestackLock() { manager->lockRestack(); }
    ~YRestackLock() { manager->unlockRestack(); }
};

class YFullscreenLock {
public:
    YFullscreenLock() { manager->setFullscreenEnabled(false); }
    ~YFullscreenLock() { manager->setFullscreenEnabled(true); }
};

void dumpZorder(const char *oper, YFrameWindow *w, YFrameWindow *a = nullptr);

extern Atom _XA_WIN_ICONS;
extern Atom _XA_WIN_LAYER;
extern Atom _XA_WIN_PROTOCOLS;
extern Atom _XA_WIN_TRAY;

extern Atom _XA_WM_CLIENT_LEADER;
extern Atom _XA_WM_CLIENT_MACHINE;
extern Atom _XA_WM_WINDOW_ROLE;

extern Atom _XA_ICEWM_ACTION;
extern Atom _XA_ICEWM_GUIEVENT;
extern Atom _XA_ICEWM_HINT;
extern Atom _XA_ICEWM_FONT_PATH;
extern Atom _XA_ICEWM_TABS;
extern Atom _XA_XROOTPMAP_ID;
extern Atom _XA_XROOTCOLOR_PIXEL;

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
