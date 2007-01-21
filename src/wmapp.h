#ifndef __WMAPP_H
#define __WMAPP_H

#include "ysmapp.h"
#include "ymenu.h"
#include "ymsgbox.h"
#ifdef CONFIG_GUIEVENTS
#include "guievent.h"
#endif

class YWindowManager;

class YWMApp: public YSMApplication, public YActionListener, public YMsgBoxListener {
public:
    YWMApp(int *argc, char ***argv, const char *displayName = 0);
    ~YWMApp();
#ifdef CONFIG_GUIEVENTS
    void signalGuiEvent(GUIEvent ge);
#endif

    virtual void afterWindowEvent(XEvent &xev);
    virtual void handleSignal(int sig);
    virtual bool handleIdle();
    virtual bool filterEvent(const XEvent &xev);
    virtual void actionPerformed(YAction *action, unsigned int modifiers);

    virtual void handleMsgBox(YMsgBox *msgbox, int operation);

    void doLogout();
    void logout();
    void cancelLogout();

#ifdef CONFIG_SESSION
    virtual void smSaveYourselfPhase2();
    virtual void smDie();
#endif

    void setFocusMode(int mode);

    void restartClient(const char *path, char *const *args);
    void runOnce(const char *resource, const char *path, char *const *args);
    void runCommandOnce(const char *resource, const char *cmdline);

    static YCursor sizeRightPointer;
    static YCursor sizeTopRightPointer;
    static YCursor sizeTopPointer;
    static YCursor sizeTopLeftPointer;
    static YCursor sizeLeftPointer;
    static YCursor sizeBottomLeftPointer;
    static YCursor sizeBottomPointer;
    static YCursor sizeBottomRightPointer;
    static YCursor scrollLeftPointer;
    static YCursor scrollRightPointer;
    static YCursor scrollUpPointer;
    static YCursor scrollDownPointer;
private:
    YWindowManager *fWindowManager;
    YMsgBox *fLogoutMsgBox;

    void runRestart(const char *path, char *const *args);
private:
    Window managerWindow;
};

extern YWMApp * wmapp;

extern YMenu *windowMenu;
extern YMenu *occupyMenu;
extern YMenu *layerMenu;
extern YMenu *moveMenu;

#ifdef CONFIG_TRAY
extern YMenu *trayMenu;
#endif

#ifdef CONFIG_WINMENU
extern YMenu *windowListMenu;
#endif

#ifdef CONFIG_WINLIST
extern YMenu *windowListPopup;
extern YMenu *windowListAllPopup;
#endif

extern YMenu *maximizeMenu;
extern YMenu *logoutMenu;

#ifndef NO_CONFIGURE_MENUS
class ObjectMenu;
extern ObjectMenu *rootMenu;
#endif
class CtrlAltDelete;
extern CtrlAltDelete *ctrlAltDelete;
extern int rebootOrShutdown;

#endif
