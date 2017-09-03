#ifndef __TASKBAR_H
#define __TASKBAR_H

#include "yaction.h"
#include "ybutton.h"
#include "ymenu.h"
#include "ytimer.h"
#include "wmclient.h"
#include "yxtray.h"

class ObjectBar;
#ifdef CONFIG_APPLET_MEM_STATUS
class MEMStatus;
#endif
#ifdef CONFIG_APPLET_CPU_STATUS
class CPUStatus;
#endif
#ifdef CONFIG_APPLET_NET_STATUS
class NetStatus;
#endif
class AddressBar;
class MailBoxStatus;
class YClock;
class YApm;
class TaskPane;
class TrayPane;
class WorkspacesPane;
class YXTray;
class YSMListener;
class IApp;

class IAppletContainer {
public:
    virtual void relayout() = 0;
    virtual void contextMenu(int x_root, int y_root) = 0;
protected:
    virtual ~IAppletContainer() {}
};

#ifdef CONFIG_TASKBAR
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
    YTimer *fAutoHideTimer;
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

    virtual void actionPerformed(YAction *action, unsigned int modifiers);
    virtual void handlePopDown(YPopupWindow *popup);
    virtual void handleEndPopup(YPopupWindow *popup);

#ifdef WMSPEC_HINTS
    void updateWMHints();
#endif
    void updateLocation();
    void configure(const YRect &r);

#ifdef CONFIG_APPLET_CLOCK
    YClock *clock() { return fClock; }
#endif

    bool windowTrayRequestDock(Window w);
    void setWorkspaceActive(long workspace, int active);

    void removeTasksApp(YFrameWindow *w);
    class TaskBarApp *addTasksApp(YFrameWindow *w);
    void relayoutTasks();

    WorkspacesPane *workspacesPane() const { return fWorkspaces; }

    void popupStartMenu();
    void popupWindowListMenu();

    void popOut();
    void showAddressBar();
    void showBar(bool visible);
    void handleCollapseButton();

    AddressBar *addressBar() const { return fAddressBar; }
    TaskPane *taskPane() const { return fTasks; }
#ifdef CONFIG_TRAY
    TrayPane *windowTrayPane() const { return fWindowTray; }
#endif

#ifdef CONFIG_GRADIENTS
    virtual ref<YImage> getGradient() const { return fGradient; }
#endif    

    void contextMenu(int x_root, int y_root);

    void relayout() { fNeedRelayout = true; }
    void relayoutNow();

    void detachDesktopTray();
    void trayChanged();
    YXTray *netwmTray() { return fDesktopTray; }

    void relayoutTray();
    class TrayApp *addTrayApp(YFrameWindow *w);
    void removeTrayApp(YFrameWindow *w);

    bool autoTimer(bool show);
    void updateFullscreen(bool fullscreen);
    Window edgeTriggerWindow() { return fEdgeTrigger->handle(); }

private:
    TaskPane *fTasks;

    YButton *fCollapseButton;
#ifdef CONFIG_TRAY
    TrayPane *fWindowTray;
#endif
#ifdef CONFIG_APPLET_CLOCK
    YClock *fClock;
#endif
#ifdef CONFIG_APPLET_MAILBOX
    MailBoxStatus **fMailBoxStatus;
#endif
#ifdef CONFIG_APPLET_MEM_STATUS
    MEMStatus *fMEMStatus;
#endif
#ifdef CONFIG_APPLET_CPU_STATUS
    CPUStatus **fCPUStatus;
#endif
#ifdef CONFIG_APPLET_APM
    YApm *fApm;
#endif
#ifdef CONFIG_APPLET_NET_STATUS
    NetStatus **fNetStatus;
#endif

#ifndef NO_CONFIGURE_MENUS
    ObjectBar *fObjectBar;
    YButton *fApplications;
#endif
#ifdef CONFIG_WINMENU
    YButton *fWinList;
#endif
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
    
#ifdef CONFIG_GRADIENTS
    ref<YImage> fGradient;
#endif

    bool fNeedRelayout;

    void initMenu();
    void initApplets();
    void updateLayout(int &size_w, int &size_h);

    EdgeTrigger *fEdgeTrigger;
};

extern TaskBar *taskBar; // !!! get rid of this

class YColor* getTaskBarBg();

#endif

#endif
