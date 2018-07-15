#ifndef __TASKBAR_H
#define __TASKBAR_H

#include "yaction.h"
#include "ybutton.h"
#include "ymenu.h"
#include "ytimer.h"
#include "wmclient.h"
#include "yxtray.h"
#include "base.h"
#include "ypointer.h"
#include "applet.h"

class ObjectBar;
class MEMStatus;
class CPUStatusControl;
class NetStatusControl;
class AddressBar;
class MailBoxControl;
class MailBoxStatus;
class YClock;
class YApm;
class TaskPane;
class TrayPane;
class WorkspacesPane;
class YXTray;
class YSMListener;
class IApp;

class TaskBar;

class EdgeTrigger: public YWindow, public YTimerListener {
public:
    EdgeTrigger(TaskBar *owner);
    virtual ~EdgeTrigger();

    void startHide();
    void stopHide();

    virtual void handleDNDEnter();
    virtual void handleDNDLeave();

    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual bool handleTimer(YTimer *t);
private:
    TaskBar *fTaskBar;
    lazy<YTimer> fAutoHideTimer;
    bool fDoShow;
};

class TaskBar:
    public YFrameClient,
    public YActionListener,
    public YPopDownListener,
    public YXTrayNotifier,
    public IAppletContainer
{
public:
    TaskBar(IApp *app, YWindow *aParent, YActionListener *wmActionListener, YSMListener *smActionListener);
    virtual ~TaskBar();

private:
    virtual void paint(Graphics &g, const YRect &r);
    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleDrag(const XButtonEvent &down, const XMotionEvent &motion);
    virtual void handleEndDrag(const XButtonEvent &down, const XButtonEvent &up);

    virtual void handleCrossing(const XCrossingEvent &crossing);
#if false
    virtual bool handleTimer(YTimer *t);
#endif

    virtual void actionPerformed(YAction action, unsigned int modifiers);
    virtual void handlePopDown(YPopupWindow *popup);
    virtual void handleEndPopup(YPopupWindow *popup);

    void updateWMHints();
    void updateLocation();
    void configure(const YRect &r);

    YClock *clock() { return fClock; }

public:
    bool windowTrayRequestDock(Window w);
    void setWorkspaceActive(long workspace, int active);

    void removeTasksApp(YFrameWindow *w);
    class TaskBarApp *addTasksApp(YFrameWindow *w);
    void relayoutTasks();

    WorkspacesPane *workspacesPane() const { return fWorkspaces; }

    void popupStartMenu();
    void popupWindowListMenu();

    void showAddressBar();
    void showBar(bool visible);
    void handleCollapseButton();

    void relayout() { fNeedRelayout = true; }
    void relayoutNow();

    void detachDesktopTray();

    void relayoutTray();
    class TrayApp *addTrayApp(YFrameWindow *w);
    void removeTrayApp(YFrameWindow *w);

    bool autoTimer(bool show);
    void updateFullscreen(bool fullscreen);
    Window edgeTriggerWindow() { return fEdgeTrigger->handle(); }
    void switchToPrev();
    void switchToNext();
    void movePrev();
    void moveNext();

private:
    void popOut();

    AddressBar *addressBar() const { return fAddressBar; }
    TaskPane *taskPane() const { return fTasks; }
    TrayPane *windowTrayPane() const { return fWindowTray; }

    virtual ref<YImage> getGradient() const { return fGradient; }

    void contextMenu(int x_root, int y_root);

    void trayChanged();
    YXTray *netwmTray() { return fDesktopTray; }

private:
    TaskPane *fTasks;

    YButton *fCollapseButton;
    TrayPane *fWindowTray;
    YClock *fClock;
    MailBoxControl *fMailBoxStatus;
    MEMStatus *fMEMStatus;
    CPUStatusControl *fCPUStatus;
    YApm *fApm;
    ref<NetStatusControl> fNetStatus;

    ObjectBar *fObjectBar;
    YButton *fApplications;
    YButton *fWinList;
    YButton *fShowDesktop;
    AddressBar *fAddressBar;
    WorkspacesPane *fWorkspaces;
    YXTray *fDesktopTray;
    YActionListener *wmActionListener;
    YSMListener *smActionListener;
    IApp *app;

    bool fIsHidden;
    bool fFullscreen;
    bool fIsCollapsed;
    bool fIsMapped;
    bool fMenuShown;
#if false
    YTimer *fAutoHideTimer;
#endif

    YMenu *taskBarMenu;

    friend class WindowList;
    friend class WindowListBox;

    ref<YImage> fGradient;

    bool fNeedRelayout;

    void initMenu();
    void initApplets();
    void updateLayout(unsigned &size_w, unsigned &size_h);

    EdgeTrigger *fEdgeTrigger;
};

extern TaskBar *taskBar; // !!! get rid of this

extern YColorName taskBarBg;

#endif

// vim: set sw=4 ts=4 et:
