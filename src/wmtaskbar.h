#ifndef __TASKBAR_H
#define __TASKBAR_H

#include "yaction.h"
#include "ybutton.h"
#include "ymenu.h"
#include "ytimer.h"
#include "wmclient.h"
#include "apppstatus.h"
#include "acpustatus.h"

#define THSP 4
#if 0
/// !!! lose this
#define BASE1 0
#define ADD1 0
#define BASE2 0
#define ADD2 0
#endif

class ObjectBar;
#if CONFIG_APPLET_CPU_STATUS
class CPUStatus;
#endif
#ifdef HAVE_NET_STATUS
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

#ifdef CONFIG_TASKBAR
class TaskBar;

class TaskBar:
    public YFrameClient,
    public YTimerListener,
    public YActionListener,
    public YPopDownListener
{
public:
    TaskBar(YWindow *aParent);
    virtual ~TaskBar();

    virtual void paint(Graphics &g, const YRect &r);
    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleDrag(const XButtonEvent &down, const XMotionEvent &motion);

    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual bool handleTimer(YTimer *t);

    virtual void actionPerformed(YAction *action, unsigned int modifiers);
    virtual void handlePopDown(YPopupWindow *popup);

    void updateWMHints();
    void updateLocation();
    void configure(const YRect &r, const bool resized);

#ifdef CONFIG_APPLET_CLOCK
    YClock *clock() { return fClock; }
#endif

    WorkspacesPane *workspacesPane() const { return fWorkspaces; }

    void popupStartMenu();
    void popupWindowListMenu();

    virtual void handleDNDEnter();
    virtual void handleDNDLeave();
    void popOut();
    void showBar(bool visible);

    AddressBar *addressBar() const { return fAddressBar; }
    TaskPane *taskPane() const { return fTasks; }
#ifdef CONFIG_TRAY
    TrayPane *trayPane() const { return fTray; }
#endif

#ifdef CONFIG_GRADIENTS
    virtual class YPixbuf * getGradient() const { return fGradient; }
#endif    

    void contextMenu(int x_root, int y_root);

    void relayout() { fNeedRelayout = true; }
    void relayoutNow();
private:
    TaskPane *fTasks;

#ifdef CONFIG_TRAY
    TrayPane *fTray;
#endif
#ifdef CONFIG_APPLET_CLOCK
    YClock *fClock;
#endif
#ifdef CONFIG_APPLET_MAILBOX
    MailBoxStatus **fMailBoxStatus;
#endif
#ifdef CONFIG_APPLET_CPU_STATUS
    CPUStatus *fCPUStatus;
#endif
#ifdef CONFIG_APPLET_APM
    YApm *fApm;
#endif
#ifdef HAVE_NET_STATUS
    NetStatus **fNetStatus;
#endif

#ifndef NO_CONFIGURE_MENUS
    ObjectBar *fObjectBar;
    YButton *fApplications;
#endif
#ifdef CONFIG_WINMENU
    YButton *fWinList;
#endif
    AddressBar *fAddressBar;
    WorkspacesPane *fWorkspaces;
    YXTray *fTray2;

    int leftX, rightX;
    bool fIsHidden;
    bool fIsMapped;
    bool fMenuShown;
    YTimer *fAutoHideTimer;

    YMenu *taskBarMenu;

    friend class WindowList;
    friend class WindowListBox;
    
#ifdef CONFIG_GRADIENTS
    class YPixbuf * fGradient;
#endif

    bool fNeedRelayout;

    void initMenu();
    void initApplets();
    void updateLayout();
};

extern TaskBar *taskBar; // !!! get rid of this

extern YPixmap *startPixmap;
extern YPixmap *windowsPixmap;

extern YPixmap *taskbackPixmap;
extern YPixmap *taskbuttonPixmap;
extern YPixmap *taskbuttonactivePixmap;
extern YPixmap *taskbuttonminimizedPixmap;

#ifdef CONFIG_GRADIENTS
class YPixbuf;

extern YPixbuf *taskbackPixbuf;
extern YPixbuf *taskbuttonPixbuf;
extern YPixbuf *taskbuttonactivePixbuf;
extern YPixbuf *taskbuttonminimizedPixbuf;
#endif

#endif

#endif
