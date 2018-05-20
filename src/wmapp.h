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
    FocusModelCount,
    FocusModelLast = FocusModelCount - 1
};

class YSMListener {
public:
    virtual void handleSMAction(WMAction message) = 0;
    virtual void restartClient(const char *path, char *const *args) = 0;
    virtual void runOnce(const char *resource, const char *path, char *const *args) = 0;
    virtual void runCommandOnce(const char *resource, const char *cmdline) = 0;
protected:
    virtual ~YSMListener() {}
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
    void signalGuiEvent(GUIEvent ge);

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

    // drop ties to own clients/windows since those are now destroyed by unmanageClients
    inline void clientsAreUnmanaged() {
        fLogoutMsgBox = 0;
        aboutDlg = 0;
    }

#ifdef CONFIG_SESSION
    virtual void smSaveYourselfPhase2();
    virtual void smDie();
#endif

    void setFocusMode(int mode);
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

    static ref<YIcon> getDefaultAppIcon();

    bool hasCtrlAltDelete() const { return ctrlAltDelete != 0; }
    CtrlAltDelete* getCtrlAltDelete();
    bool hasSwitchWindow() const { return switchWindow != 0; }
    SwitchWindow* getSwitchWindow();

private:
    char** mainArgv;

    // XXX: these pointers are PITA because they can become wild when objects
    // are destroyed independently by manager. What we need is something like std::weak_ptr...
    YMsgBox *fLogoutMsgBox;
    AboutDlg* aboutDlg;

    CtrlAltDelete* ctrlAltDelete;
    SwitchWindow* switchWindow;

    void runRestart(const char *path, char *const *args);

    Window managerWindow;

    static void initAtoms();
    static void initPointers();
    static void initIcons();
    static void termIcons();
    static void initIconSize();
    static void initPixmaps();
};

extern YWMApp * wmapp;

extern YMenu *windowMenu;
extern YMenu *layerMenu;
extern YMenu *moveMenu;
extern YMenu *trayMenu;
extern YMenu *windowListMenu;
extern YMenu *windowListPopup;
extern YMenu *windowListAllPopup;

extern YMenu *logoutMenu;

class ObjectMenu;
extern ObjectMenu *rootMenu;

class KProgram;
extern YObjectArray<KProgram> keyProgs;
extern RebootShutdown rebootOrShutdown;

#endif

// vim: set sw=4 ts=4 et:
