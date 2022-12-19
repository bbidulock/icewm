#ifndef WMAPP_H
#define WMAPP_H

#include "ysmapp.h"
#include "ymsgbox.h"
#include "guievent.h"
#include "yicon.h"

class YWindowManager;
class AboutDlg;
class CtrlAltDelete;

enum FocusModel {
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
    virtual int runProgram(const char *path, const char *const *args, int fd) = 0;
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
    virtual void actionPerformed(YAction action, unsigned int modifiers = 0);

    virtual void handleMsgBox(YMsgBox *msgbox, int operation);
    virtual void handleSMAction(WMAction message);

    void doLogout(RebootShutdown reboot);
    void logout();
    void cancelLogout();

#ifdef CONFIG_SESSION
    virtual void smSaveYourselfPhase2();
    virtual void smDie();
#endif

    void setFocusMode(FocusModel mode);
    void initFocusMode();
    void initFocusCustom();
    FocusModel loadFocusMode();

    virtual void restartClient(const char *path, char *const *args);
    virtual int runProgram(const char *path, const char *const *args, int fd = -1);
    virtual void runOnce(const char *resource, long *pid,
                         const char *path, char *const *args);
    virtual void runCommand(const char *prog);
    virtual void runCommandOnce(const char *resource, const char *cmdline, long *pid);
    bool mapClientByPid(const char* resource, long pid);
    bool mapClientByResource(const char* resource, long *pid);

    static YCursor leftPointer;
    static YCursor rightPointer;
    static YCursor movePointer;
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
    const char* getConfigFile() const { return configFile; }
    FocusModel getFocusMode() const { return focusMode; }

    AToolTip* newToolTip();
    YMenu* getWindowMenu();
    void subdirs(const char* subdir, bool themeOnly, MStringArray& paths);
    void unregisterProtocols();
    void refreshDesktop();

private:
    char** mainArgv;
    int mainArgc;
    const char* configFile;
    bool notifyParent;
    pid_t notifiedParent;

    // XXX: these pointers are PITA because they can become wild when objects
    // are destroyed independently by manager. What we need is something like std::weak_ptr...
    YMsgBox *fLogoutMsgBox;
    YMsgBox* fRestartMsgBox;
    AboutDlg* aboutDlg;

    CtrlAltDelete* ctrlAltDelete;
    YMenu* windowMenu;
    int errorRequestCode;
    YFrameWindow* errorFrame;
    lazy<YTimer> errorTimer;
    lazy<YTimer> pathsTimer;
    lazy<YTimer> splashTimer;
    lazy<YWindow> splashWindow;
    lazy<GuiSignaler> guiSignaler;

    void createTaskBar();
    YWindow* splash(const char* splashFile);
    virtual Cursor getRightPointer() const { return rightPointer; }
    virtual bool handleTimer(YTimer *timer);
    virtual int handleError(XErrorEvent *xev);
    void runRestart(const char *path, char *const *args);

    FocusModel focusMode;
    Window managerWindow;
    ref<YIcon> defaultAppIcon;

    MStringArray resourcePaths;
    YArray<bool> themeOnlyPath;

    void freePointers();
    void initPointers();
    void initIcons();
    void initIconSize();
};

extern YWMApp * wmapp;

#ifdef WMPROG_H
class RootMenu  : public StartMenu {
public:
    RootMenu() : StartMenu(wmapp, wmapp, wmapp, "menu") {
        setShared(true);
    }
};
extern lazily<RootMenu> rootMenu;
#endif

#ifdef WMWINMENU_H
class SharedWindowList  : public WindowListMenu {
public:
    SharedWindowList() : WindowListMenu(wmapp) {
        setShared(true);
    }
};
extern lazily<SharedWindowList> windowListMenu;
#endif

class SharedMenu : public YMenu {
public:
    SharedMenu() { setShared(true); }
};

class LogoutMenu : public SharedMenu {
public:
    void updatePopup();
};
extern lazy<LogoutMenu> logoutMenu;

class LayerMenu : public SharedMenu {
public:
    void updatePopup();
};
extern lazy<LayerMenu> layerMenu;

class MoveMenu : public SharedMenu {
public:
    void updatePopup();
};
extern lazy<MoveMenu> moveMenu;

class TileMenu : public SharedMenu {
public:
    void updatePopup();
};
extern lazy<TileMenu> tileMenu;

class TabsMenu : public SharedMenu {
public:
    void updatePopup();
};
extern lazy<TabsMenu> tabsMenu;

class KProgram;
typedef YObjectArray<KProgram> KProgramArrayType;
typedef KProgramArrayType::IterType KProgramIterType;
extern KProgramArrayType keyProgs;

extern RebootShutdown rebootOrShutdown;

#endif

// vim: set sw=4 ts=4 et:
