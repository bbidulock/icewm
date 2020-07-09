#ifndef __WMAPP_H
#define __WMAPP_H

#include "ysmapp.h"
#include "ymenu.h"
#include "ymsgbox.h"
#include "guievent.h"

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
    FocusModelLast = FocusQuiet
};

class YSMListener {
public:
    virtual void handleSMAction(WMAction message) = 0;
    virtual void restartClient(const char *path, char *const *args) = 0;
    virtual int runProgram(const char *path, const char *const *args) = 0;
    virtual void runCommand(const char *prog) = 0;
    virtual void runOnce(const char *resource, long *pid,
                         const char *path, char *const *args) = 0;
    virtual void runCommandOnce(const char *resource, const char *cmdline, long *pid) = 0;
protected:
    virtual ~YSMListener() {}
};

class YWMApp:
    public YSMApplication,
    public YActionListener,
    public YMsgBoxListener,
    public YSMListener,
    public YTimerListener
{
    typedef YSMApplication super;

public:
    YWMApp(int *argc, char ***argv, const char *displayName,
            bool notifyParent, const char *splashFile,
            const char *configFile, const char *overrideTheme);
    ~YWMApp();
    void signalGuiEvent(GUIEvent ge);
    int mainLoop();

    virtual void afterWindowEvent(XEvent &xev);
    virtual void handleSignal(int sig);
    virtual bool handleIdle();
    virtual bool filterEvent(const XEvent &xev);
    virtual void actionPerformed(YAction action, unsigned int modifiers);

    virtual void handleMsgBox(YMsgBox *msgbox, int operation);
    virtual void handleSMAction(WMAction message);

    void doLogout(RebootShutdown reboot);
    void logout();
    void cancelLogout();

#ifdef CONFIG_SESSION
    virtual void smSaveYourselfPhase2();
    virtual void smDie();
#endif

    void setFocusMode(FocusModels mode);
    void initFocusMode();
    void initFocusCustom();
    void loadFocusMode();

    virtual void restartClient(const char *path, char *const *args);
    virtual int runProgram(const char *path, const char *const *args);
    virtual void runOnce(const char *resource, long *pid,
                         const char *path, char *const *args);
    virtual void runCommand(const char *prog);
    virtual void runCommandOnce(const char *resource, const char *cmdline, long *pid);
    bool mapClientByPid(const char* resource, long pid);
    bool mapClientByResource(const char* resource, long *pid);

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

    ref<YIcon> getDefaultAppIcon();

    bool hasCtrlAltDelete() const { return ctrlAltDelete != nullptr; }
    CtrlAltDelete* getCtrlAltDelete();
    bool hasSwitchWindow() const { return switchWindow != nullptr; }
    SwitchWindow* getSwitchWindow();
    const char* getConfigFile() const { return configFile; }
    FocusModels getFocusMode() const { return focusMode; }
    YMenu* getWindowMenu();

    void unregisterProtocols();

private:
    char** mainArgv;
    int mainArgc;
    const char* configFile;
    bool notifyParent;
    pid_t notifiedParent;

    // XXX: these pointers are PITA because they can become wild when objects
    // are destroyed independently by manager. What we need is something like std::weak_ptr...
    YMsgBox *fLogoutMsgBox;
    AboutDlg* aboutDlg;

    CtrlAltDelete* ctrlAltDelete;
    SwitchWindow* switchWindow;
    YMenu* windowMenu;
    int errorRequestCode;
    YFrameWindow* errorFrame;
    lazy<YTimer> errorTimer;
    lazy<YTimer> splashTimer;
    lazy<YWindow> splashWindow;

    void createTaskBar();
    YWindow* splash(const char* splashFile);
    virtual bool handleTimer(YTimer *timer);
    virtual int handleError(XErrorEvent *xev);
    void runRestart(const char *path, char *const *args);

    FocusModels focusMode;
    Window managerWindow;
    ref<YIcon> defaultAppIcon;

    void initPointers();
    void initIcons();
    void initIconSize();
};

extern YWMApp * wmapp;

#ifdef __WMPROG_H
class RootMenu  : public StartMenu {
public:
    RootMenu() : StartMenu(wmapp, wmapp, wmapp, "menu") {
        setShared(true);
    }
};
extern lazily<RootMenu> rootMenu;
#endif

#ifdef __WMWINMENU_H
class SharedWindowList  : public WindowListMenu {
public:
    SharedWindowList() : WindowListMenu(wmapp) {
        setShared(true);
    }
};
extern lazily<SharedWindowList> windowListMenu;
#endif

class LogoutMenu : public YMenu {
public:
    LogoutMenu() {
        setShared(true);
    }
    void updatePopup();
};
extern lazy<LogoutMenu> logoutMenu;

class LayerMenu : public YMenu {
public:
    LayerMenu() {
        setShared(true);
    }
    void updatePopup();
};
extern lazy<LayerMenu> layerMenu;

class MoveMenu : public YMenu {
public:
    MoveMenu() {
        setShared(true);
    }
    void updatePopup();
};
extern lazy<MoveMenu> moveMenu;

class KProgram;
typedef YObjectArray<KProgram> KProgramArrayType;
typedef KProgramArrayType::IterType KProgramIterType;
extern KProgramArrayType keyProgs;

extern RebootShutdown rebootOrShutdown;

#endif

// vim: set sw=4 ts=4 et:
