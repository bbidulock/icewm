#ifndef __TASKBAR_H
#define __TASKBAR_H

#include "yaction.h"
#include "ybutton.h"
#include "ymenu.h"
#include "ytimer.h"
#include "yconfig.h"
//#include "wmclient.h"
#include "apppstatus.h"
#include "dockaction.h"

/// !!! lose this
#define BASE1 2
#define ADD1 3
#define BASE2 3
#define ADD2 5

class ObjectBar;
#if (defined(linux)||defined(HAVE_KSTAT_H))
class CPUStatus;
#endif
class NetStatus;
class AddressBar;
class MailBoxStatus;
class YClock;
class YApm;
class TaskPane;
class WorkspacesPane;
class YWindowManager;
class DesktopInfo;

class TaskBar;

class TaskBar: public YWindow, public YTimerListener, public YActionListener, public PopDownListener
{
public:
    TaskBar(DesktopInfo *desktopinfo, YWindow *aParent);
    virtual ~TaskBar();

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);
    virtual bool handleKeyEvent(const XKeyEvent &key);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleDrag(const XButtonEvent &down, const XMotionEvent &motion);

    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual bool handleTimer(YTimer *t);

    virtual void actionPerformed(YAction *action, unsigned int modifiers);
    virtual void handlePopDown(YPopupWindow *popup);

    void updateLocation();

    YClock *clock() { return fClock; }

    WorkspacesPane *workspacesPane() const { return fWorkspaces; }

    void popupStartMenu();
    void popupWindowListMenu();

    virtual void handleDNDEnter(int nTypes, Atom *types);
    virtual void handleDNDLeave();
    void popOut();
    void showBar(bool visible);

    AddressBar *addressBar() const { return fAddressBar; }
    TaskPane *taskPane() const { return fTasks; }

    void contextMenu(int x_root, int y_root);
private:
    TaskPane *fTasks;
    //YWindowManager *fRoot;

    YClock *fClock;
    MailBoxStatus *fMailBoxStatus;
#if (defined(linux)||defined(HAVE_KSTAT_H))
    CPUStatus *fCPUStatus;
#endif
    YApm *fApm;
    NetStatus *fNetStatus;

    ObjectBar *fObjectBar;
    YButton *fApplications;
    AddressBar *fAddressBar;
    YButton *fWinList;
    WorkspacesPane *fWorkspaces;

    YPref fTaskBarAutoHide;

    int leftX, rightX;
    bool fIsHidden;
    bool fIsMapped;
    bool fMenuShown;
    YTimer fAutoHideTimer;

    YMenu *taskBarMenu;
    YMenu *startMenu;

    DesktopInfo *fDesktopInfo;

    friend class WindowList;
    friend class WindowListBox;

    static YColorPrefProperty gTaskBarBg;
    static YNumPrefProperty gAutoHideDelay;

    static YPixmapPrefProperty gPixmapStartButton;
    static YPixmapPrefProperty gPixmapWindowsButton;
    static YPixmapPrefProperty gPixmapBackground;
};

#endif
