#ifndef __WMMGR_H
#define __WMMGR_H

#include <X11/Xproto.h>
#include "yaction.h"
#include "ywindow.h"
#include "ymenu.h"
#include "WinMgr.h"
#include "ytimer.h"

#define MAXWORKSPACES 64
#define INVALID_WORKSPACE 0xFFFFFFFF

extern long gWorkspaceCount;
extern char *gWorkspaceNames[MAXWORKSPACES];
extern YAction *workspaceActionActivate[MAXWORKSPACES];
extern YAction *workspaceActionMoveTo[MAXWORKSPACES];

extern YAction *layerActionSet[WinLayerCount];

class YWindowManager;
class YFrameClient;
class YFrameWindow;
class WindowList;
class TaskBar;
class SwitchWindow;
class MoveSizeStatus;
class CtrlAltDelete;
class AboutDlg;

class EdgeSwitch: public YWindow, public YTimerListener {
public:
    EdgeSwitch(YWindowManager *manager, int delta);
    virtual ~EdgeSwitch();

    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual bool handleTimer(YTimer *t);
private:
    YWindowManager *fManager;
    int fDelta;
    Cursor cursor;

    static YTimer *fEdgeSwitchTimer;
};

class YProxyWindow: public YWindow {
public:
    YProxyWindow(YWindow *parent);
    virtual ~YProxyWindow();

    virtual void handleButton(const XButtonEvent &button);
};

class YWindowManager: public YDesktop, public YActionListener {
public:
    YWindowManager(YWindow *parent, Window win = 0);
    virtual ~YWindowManager();

    void grabKeys();

    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual bool handleKeySym(const XKeyEvent &key, KeySym ksym, int vmod);

    virtual void handleConfigureRequest(const XConfigureRequestEvent &configureRequest);
    virtual void handleMapRequest(const XMapRequestEvent &mapRequest);
    virtual void handleUnmap(const XUnmapEvent &unmap);
    virtual void handleDestroyWindow(const XDestroyWindowEvent &destroyWindow);
    virtual void handleClientMessage(const XClientMessageEvent &message);
    virtual void handleProperty(const XPropertyEvent &property);
    virtual void actionPerformed(YAction *action, unsigned int /*modifiers*/);

    void manageClients();
    void unmanageClients();

    virtual void manageWindow(YWindow *w, bool mapWindow);
    virtual void unmanageWindow(YWindow *w);
    YFrameWindow *findFrame(Window win);
    YFrameClient *findClient(Window win);
    YFrameWindow *manageClient(Window win, bool mapClient = false);
    void unmanageClient(Window win, bool mapClient = false);
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

    int calcCoverage(bool down, YFrameWindow *frame, int x, int y, int w, int h);
    void tryCover(bool down, YFrameWindow *frame, int x, int y, int w, int h,
                  int &px, int &py, int &cover);
    bool getSmartPlace(bool down, YFrameWindow *frame, int &x, int &y, int w, int h);
    void getNewPosition(YFrameWindow *frame, int &x, int &y, int w, int h);
    void placeWindow(YFrameWindow *frame, int x, int y, int newClient, bool &canActivate);

    YFrameWindow *top(long layer) const { return fTop[layer]; }
    void setTop(long layer, YFrameWindow *top);
    YFrameWindow *bottom(long layer) const { return fBottom[layer]; }
    void setBottom(long layer, YFrameWindow *bottom) { fBottom[layer] = bottom; }

    YFrameWindow *topLayer(long layer = WinLayerCount - 1);
    YFrameWindow *bottomLayer(long layer = 0);

    void restackWindows(YFrameWindow *win);
    void focusTopWindow();
    bool focusTop(YFrameWindow *f);
    void relocateWindows(int dx, int dy);
    void updateClientList();

    YMenu *createWindowMenu(YMenu *menu, long workspace);
    int windowCount(long workspace);

    long activeWorkspace() const { return fActiveWorkspace; }
    void activateWorkspace(long workspace);
    long workspaceCount() const { return gWorkspaceCount; }
    const char *workspaceName(long workspace) const { return gWorkspaceNames[workspace]; }

    void putWorkArea();
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

    void switchToWorkspace(long nw, bool takeCurrent);
    void switchToPrevWorkspace(bool takeCurrent);
    void switchToNextWorkspace(bool takeCurrent);

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
    void registerProtocols();
    void unregisterProtocols();
    void initIconSize();
    void initWorkspaces();

    void showWindowList(int x, int y);

    //WindowList *getWindowList() const { return fWindowList; }
    //TaskBar *getTaskBar() const { return fTaskBar; }
    SwitchWindow *getSwitchWindow() const { return fSwitchWindow; }
    MoveSizeStatus *getMoveSizeStatus() const { return fMoveSizeStatus; }

private:
    struct WindowPosState {
        int x, y, w, h;
        long state;
        YFrameWindow *frame;
    };

    YFrameWindow *fFocusWin;
    YFrameWindow *fTop[WinLayerCount];
    YFrameWindow *fBottom[WinLayerCount];
    long fActiveWorkspace;
    YFrameWindow *fColormapWindow;
    int fMinX, fMinY, fMaxX, fMaxY;
    EdgeSwitch *fLeftSwitch, *fRightSwitch;
    bool fShuttingDown;
    int fArrangeCount;
    WindowPosState *fArrangeInfo;
    YProxyWindow *rootProxy;
    YWindow *fTopWin;
    bool fWorkAreaMoveWindows;

    //WindowList *fWindowList;
    MoveSizeStatus *fMoveSizeStatus;
    SwitchWindow *fSwitchWindow;
    //TaskBar *fTaskBar;
    //CtrlAltDelete *fCtrlAltDelete;
    //AboutDlg *fAboutDlg;
};

void dumpZorder(const char *oper, YFrameWindow *w, YFrameWindow *a = 0);

extern YIcon *defaultAppIcon;

#endif
