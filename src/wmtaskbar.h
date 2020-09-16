#ifndef TASKBAR_H
#define TASKBAR_H

#include "yaction.h"
#include "ytimer.h"
#include "wmclient.h"
#include "yxtray.h"
#include "applet.h"

class ObjectBar;
class ObjectButton;
class MEMStatus;
class CPUStatusControl;
class NetStatusControl;
class AddressBar;
class KeyboardStatus;
class MailBoxControl;
class MailBoxStatus;
class YButton;
class YClock;
class YApm;
class TaskBarMenu;
class TaskPane;
class TrayPane;
class AWorkspaces;
class WorkspacesPane;
class YXTray;
class YSMListener;
class TaskBar;
class TaskBarApp;
class TrayApp;

class EdgeTrigger: public YWindow, public YTimerListener {
public:
    EdgeTrigger(TaskBar *owner);
    virtual ~EdgeTrigger();

    bool enabled() const;
    void show();
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
    virtual void handleFocus(const XFocusChangeEvent& focus);
    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual void handleExpose(const XExposeEvent &expose) {}

    virtual void actionPerformed(YAction action, unsigned int modifiers);
    virtual void handlePopDown(YPopupWindow *popup);

    void updateWMHints();
    void updateLocation();
    void updateWinLayer();
    virtual void configure(const YRect2 &r);
    virtual void repaint();

    YClock *clock() { return fClock; }

public:
    bool windowTrayRequestDock(Window w);
    void setWorkspaceActive(long workspace, bool active);
    void workspacesRepaint();
    void workspacesUpdateButtons();
    void workspacesRelabelButtons();
    void keyboardUpdate(mstring keyboard);

    void updateFrame(YFrameWindow* frame);
    void delistFrame(YFrameWindow* frame, TaskBarApp* task, TrayApp* tray);
    void removeTasksApp(YFrameWindow* frame);
    TaskBarApp* addTasksApp(YFrameWindow* frame);
    void relayoutTasks();
    void relayoutTray();
    TrayApp* addTrayApp(YFrameWindow* frame);
    void removeTrayApp(YFrameWindow* frame);

    void popupStartMenu();
    void popupWindowListMenu();

    void showAddressBar();
    void showBar();
    void handleCollapseButton();

    void relayout() { fNeedRelayout = true; }
    void relayoutNow();

    void detachDesktopTray();

    bool hidden() const { return fIsCollapsed | fIsHidden | !fIsMapped; }
    bool autoTimer(bool show);
    void updateFullscreen(bool fullscreen);
    Window edgeTriggerWindow() { return fEdgeTrigger->handle(); }
    void switchToPrev();
    void switchToNext();
    void movePrev();
    void moveNext();
    void refresh();

private:
    void popOut();

    AddressBar *addressBar() const { return fAddressBar; }
    TaskPane *taskPane() const { return fTasks; }
    TrayPane *windowTrayPane() const { return fWindowTray; }

    virtual ref<YImage> getGradient() { return fGradient; }
    const YSurface& getSurface() const { return fSurface; }

    void contextMenu(int x_root, int y_root);
    void buttonUpdate();
    void trayChanged();
    YXTray *netwmTray() { return fDesktopTray; }

private:
    YSurface fSurface;
    TaskPane *fTasks;

    ObjectButton *fCollapseButton;
    TrayPane *fWindowTray;
    YClock *fClock;
    KeyboardStatus *fKeyboardStatus;
    MailBoxControl *fMailBoxControl;
    MEMStatus *fMEMStatus;
    CPUStatusControl *fCPUStatus;
    YApm *fApm;
    NetStatusControl *fNetStatus;

    ObjectBar *fObjectBar;
    ObjectButton *fApplications;
    ObjectButton *fWinList;
    ObjectButton *fShowDesktop;
    AddressBar *fAddressBar;
    AWorkspaces *fWorkspaces;
    YXTray *fDesktopTray;
    EdgeTrigger *fEdgeTrigger;
    YActionListener *wmActionListener;
    YSMListener *smActionListener;
    IApp *app;

    lazy<TaskBarMenu> taskBarMenu;
    ref<YImage> fGradient;
    YArray<YFrameWindow*> fUpdates;

    bool fIsHidden;
    bool fFullscreen;
    bool fIsCollapsed;
    bool fIsMapped;
    bool fMenuShown;
    bool fNeedRelayout;
    bool fButtonUpdate;


    friend class WindowList;
    friend class WindowListBox;

    void initApplets();
    void updateLayout(unsigned &size_w, unsigned &size_h);

    class YStrut {
    public:
        Atom left, right, top, bottom;
        YStrut() : left(0), right(0), top(0), bottom(0) { }
        bool operator!=(const YStrut& s) const {
            return left != s.left || right != s.right
                || top != s.top || bottom != s.bottom;
        }
        const Atom* operator&() const { return &left; }
    } fStrut;
};

extern TaskBar *taskBar; // !!! get rid of this

extern YColorName taskBarBg;

#endif

// vim: set sw=4 ts=4 et:
