#ifndef __WMMGR_H
#define __WMMGR_H

#include <X11/Xproto.h>
#include "ywindow.h"
#include "ymenu.h"
#include "WinMgr.h"
#include "ytimer.h"

#ifdef CONFIG_WM_SESSION
#include "yarray.h"

#define PROC_WM_SESSION "/proc/wm-session"
#endif

#define MAXWORKSPACES 64
#define INVALID_WORKSPACE 0xFFFFFFFF

extern long workspaceCount;
extern char *workspaceNames[MAXWORKSPACES];
extern YAction *workspaceActionActivate[MAXWORKSPACES];
extern YAction *workspaceActionMoveTo[MAXWORKSPACES];

extern YAction *layerActionSet[WinLayerCount];

#ifdef CONFIG_TRAY
extern YAction *trayOptionActionSet[WinTrayOptionCount];
#endif

class YWindowManager;
class YFrameClient;
class YFrameWindow;

class EdgeSwitch:
public YWindow,
public YTimer::Listener {
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

    void registerProtocols();
    void unregisterProtocols();
    void initWorkspaces();
    void grabKeys();

    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual bool handleKey(const XKeyEvent &key);

    virtual void handleConfigureRequest(const XConfigureRequestEvent &configureRequest);
    virtual void handleMapRequest(const XMapRequestEvent &mapRequest);
    virtual void handleUnmap(const XUnmapEvent &unmap);
    virtual void handleDestroyWindow(const XDestroyWindowEvent &destroyWindow);
    virtual void handleClientMessage(const XClientMessageEvent &message);
    virtual void handleProperty(const XPropertyEvent &property);

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
    void loseFocus(YFrameWindow *window,
                   YFrameWindow *next,
                   YFrameWindow *prev);
    void activate(YFrameWindow *frame, bool canWarp = false);

    void installColormap(Colormap cmap);
    void setColormapWindow(YFrameWindow *frame);
    YFrameWindow *colormapWindow() { return fColormapWindow; }

    void removeClientFrame(YFrameWindow *frame);

    int minX(long layer) const;
    int minY(long layer) const;
    int maxX(long layer) const;
    int maxY(long layer) const;
    int maxWidth(long layer) const { return maxX(layer) - minX(layer); }
    int maxHeight(long layer) const { return maxY(layer) - minY(layer); }

    int minX(YFrameWindow const *frame) const;
    int minY(YFrameWindow const *frame) const;
    int maxX(YFrameWindow const *frame) const;
    int maxY(YFrameWindow const *frame) const;
    int maxWidth(YFrameWindow const *frame) const {
	return maxX(frame) - minX(frame); }
    int maxHeight(YFrameWindow const *frame) const {
	return maxY(frame) - minY(frame); }

    int calcCoverage(bool down, YFrameWindow *frame, int x, int y, int w, int h);
    void tryCover(bool down, YFrameWindow *frame, int x, int y, int w, int h,
                  int &px, int &py, int &cover);
    bool getSmartPlace(bool down, YFrameWindow *frame, int &x, int &y, int w, int h);
    void getNewPosition(YFrameWindow *frame, int &x, int &y, int w, int h);
    void placeWindow(YFrameWindow *frame, int x, int y, bool newClient, bool &canActivate);

    YFrameWindow *top(long layer) const { return fTop[layer]; }
    void top(long layer, YFrameWindow *top);
    YFrameWindow *bottom(long layer) const { return fBottom[layer]; }
    void bottom(long layer, YFrameWindow *bottom) { fBottom[layer] = bottom; }

    YFrameWindow *topLayer(long layer = WinLayerCount - 1);
    YFrameWindow *bottomLayer(long layer = 0);

    YFrameWindow *firstCreated() const { return fFirstCreated; }
    YFrameWindow *lastCreated() const { return fLastCreated; }
    void firstCreated(YFrameWindow *first) { fFirstCreated = first; }
    void lastCreated(YFrameWindow *last) { fLastCreated = last; }

    void restackWindows(YFrameWindow *win);
    void focusTopWindow();
    bool focusTop(YFrameWindow *f);
    void relocateWindows(int dx, int dy);
    void updateClientList();

    YMenu *createWindowMenu(YMenu *menu, long workspace);
    int windowCount(long workspace);
#ifdef CONFIG_WINMENU
    void popupWindowListMenu(int x, int y);
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

    void switchFocusTo(YFrameWindow *frame);
    void switchFocusFrom(YFrameWindow *frame);

    void popupStartMenu();
#ifdef CONFIG_WINMENU
    void popupWindowListMenu();
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

    void getWindowsToArrange(YFrameWindow ***w, int *count);

    void saveArrange(YFrameWindow **w, int count);
    void undoArrange();

    bool haveClients();
    void setupRootProxy();

    void setWorkAreaMoveWindows(bool m) { fWorkAreaMoveWindows = m; }

#ifdef CONFIG_WM_SESSION
    void setTopLevelProcess(pid_t p);
    void removeLRUProcess();
#endif

    static char const * getName();

public:
    static XContext frameContext;
    static XContext clientContext;

private:
    struct WindowPosState {
        int x, y, w, h;
        long state;
        YFrameWindow *frame;
    };

    bool fShuttingDown;
    bool fWorkAreaMoveWindows;

    YFrameWindow *fFocusWin;
    YFrameWindow *fTop[WinLayerCount];
    YFrameWindow *fBottom[WinLayerCount];
    YFrameWindow *fFirstCreated;
    YFrameWindow *fLastCreated;
    YFrameWindow *fColormapWindow;
    YProxyWindow *fRootProxy;
    YWindow *fTopWin;

    long fActiveWorkspace;
    long fLastWorkspace;
    int fMinX, fMinY, fMaxX, fMaxY;

    EdgeSwitch *fLeftSwitch, *fRightSwitch;
    EdgeSwitch *fTopSwitch, *fBottomSwitch;

    int fArrangeCount;
    WindowPosState *fArrangeInfo;

#ifdef CONFIG_WM_SESSION
    YStackSet<pid_t> fProcessList;
#endif
};

extern YWindowManager *manager;

void dumpZorder(const char *oper, YFrameWindow *w, YFrameWindow *a = 0);

extern YIcon *defaultAppIcon;

#endif
