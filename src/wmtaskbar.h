#ifndef __TASKBAR_H
#define __TASKBAR_H

#include "yaction.h"
#include "ybutton.h"
#include "ymenu.h"
#include "ytimer.h"
#include "wmclient.h"
#include "apppstatus.h"

/// !!! lose this
#define BASE1 2
#define ADD1 3
#define BASE2 3
#define ADD2 5

class ObjectBar;
#if (defined(linux)||defined(HAVE_KSTAT_H))
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
class WorkspacesPane;

#ifdef CONFIG_TASKBAR
class TaskBar;

class TaskBar: public YFrameClient, public YTimerListener, public YActionListener, public PopDownListener
{
public:
    TaskBar(YWindow *aParent);
    virtual ~TaskBar();

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);
    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleDrag(const XButtonEvent &down, const XMotionEvent &motion);

    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual bool handleTimer(YTimer *t);

    virtual void actionPerformed(YAction *action, unsigned int modifiers);
    virtual void handlePopDown(YPopupWindow *popup);

    void updateLocation();

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

#ifdef CONFIG_GRADIENTS
    virtual class YPixbuf const * getGradient() const { return fGradient; }
#endif    

    void contextMenu(int x_root, int y_root);
private:
    TaskPane *fTasks;

#ifdef CONFIG_APPLET_CLOCK
    YClock *fClock;
#endif
#ifdef CONFIG_APPLET_MAILBOX
    MailBoxStatus **fMailBoxStatus;
#endif
#ifdef CONFIG_APPLET_CPU_STATUS
#if (defined(linux)||defined(HAVE_KSTAT_H))
    CPUStatus *fCPUStatus;
#endif
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
    YButton *fWinList;
    AddressBar *fAddressBar;
    WorkspacesPane *fWorkspaces;

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
