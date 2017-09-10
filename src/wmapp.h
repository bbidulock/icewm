#ifndef __WMAPP_H
#define __WMAPP_H

#include "ysmapp.h"
#include "ymenu.h"
#include "ymsgbox.h"
#ifdef CONFIG_GUIEVENTS
#include "guievent.h"
#endif

class YWindowManager;
class AboutDlg;
class CtrlAltDelete;
class SwitchWindow;

enum FocusModels {
    FocusCustom,
    FocusClick,
    FocusSloppy,
    FocusExplicit,
    FocusStrict,
    FocusQuiet,
    FocusModelCount,
    FocusModelLast = FocusModelCount - 1
};

class YSMListener {
public:
    virtual void handleSMAction(int message) = 0;
    virtual void restartClient(const char *path, char *const *args) = 0;
    virtual void runOnce(const char *resource, const char *path, char *const *args) = 0;
    virtual void runCommandOnce(const char *resource, const char *cmdline) = 0;
protected:
    virtual ~YSMListener() {}; 
};

class YWMApp: 
    public YSMApplication, 
    public YActionListener, 
    public YMsgBoxListener, 
    public YSMListener
{
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
    virtual void handleSMAction(int message);

    void doLogout();
    void logout();
    void cancelLogout();

#ifdef CONFIG_SESSION
    virtual void smSaveYourselfPhase2();
    virtual void smDie();
#endif

#ifndef NO_CONFIGURE
    void setFocusMode(int mode);
#endif
    void initFocusMode();

    virtual void restartClient(const char *path, char *const *args);
    virtual void runOnce(const char *resource, const char *path, char *const *args);
    virtual void runCommandOnce(const char *resource, const char *cmdline);

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

#ifndef LITE
    static ref<YIcon> getDefaultAppIcon();

    bool hasCtrlAltDelete() const { return ctrlAltDelete != 0; }
    CtrlAltDelete* getCtrlAltDelete();
#endif
    bool hasSwitchWindow() const { return switchWindow != 0; }
    SwitchWindow* getSwitchWindow();

private:
    char** mainArgv;
    YMsgBox *fLogoutMsgBox;
    AboutDlg* aboutDlg;
#ifndef LITE
    CtrlAltDelete* ctrlAltDelete;
#endif
    SwitchWindow* switchWindow;

    void runRestart(const char *path, char *const *args);

    Window managerWindow;

    static void initAtoms();
    static void initPointers();
#ifndef LITE
    static void initIcons();
    static void termIcons();
#endif
    static void initIconSize();
    static void initPixmaps();
};

extern YWMApp * wmapp;

extern YMenu *windowMenu;
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

extern YMenu *logoutMenu;

#ifndef NO_CONFIGURE_MENUS
class ObjectMenu;
extern ObjectMenu *rootMenu;

class KProgram;
extern YObjectArray<KProgram> keyProgs;
#endif
extern int rebootOrShutdown;

#endif
