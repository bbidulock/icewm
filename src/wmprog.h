#ifndef WMPROG_H
#define WMPROG_H

#include "objmenu.h"
#include "wmkey.h"

class ObjectContainer;
class YSMListener;
class YActionListener;
class SwitchWindow;
class MenuProgSwitchItems;

class MenuLoader {
public:
    MenuLoader(IApp *app, YSMListener *smActionListener,
               YActionListener *wmActionListener) :
        app(app),
        smActionListener(smActionListener),
        wmActionListener(wmActionListener)
    {
    }

    void loadMenus(upath fileName, ObjectContainer *container);
    void progMenus(const char *command, char *const argv[],
                   ObjectContainer *container);

private:
    char* parseIncludeStatement(char *p, ObjectContainer *container);
    char* parseIncludeProgStatement(char *p, ObjectContainer *container);
    char* parseMenus(char *data, ObjectContainer *container);
    char* parseAMenu(char *data, ObjectContainer *container);
    char* parseMenuFile(char *data, ObjectContainer *container);
    char* parseMenuProg(char *data, ObjectContainer *container);
    char* parseMenuProgReload(char *data, ObjectContainer *container);
    char* parseKey(char *word, char *p);
    char* parseProgram(char *word, char *p, ObjectContainer *container);
    char* parseWord(char *word, char *p, ObjectContainer *container);

    IApp *app;
    YSMListener *smActionListener;
    YActionListener *wmActionListener;
};

class DProgram: public DObject {
public:
    virtual ~DProgram();

    virtual void open();

    static char *fullname(const char *exe);
    static DProgram *newProgram(
        IApp *app,
        YSMListener *smActionListener,
        const char *name,
        ref<YIcon> icon,
        const bool restart,
        const char *wmclass,
        const char *exe,
        YStringArray &args);

    const char* cmd() const { return fCmd; }
    YStringArray& args() { return fArgs; }

protected:
    DProgram(
        IApp *app,
        YSMListener *smActionListener,
        const mstring &name,
        ref<YIcon> icon,
        const bool restart,
        const char *wmclass,
        const char *exe,
        YStringArray &args);

private:
    const bool fRestart;
    const char *fRes;
    long fPid;
    const char* fCmd;
    YStringArray fArgs;
    YSMListener *smActionListener;
};

class DFile: public DObject {
public:
    DFile(IApp *app, const mstring &name, ref<YIcon> icon, upath path);
    virtual ~DFile();

    virtual void open();
private:
    upath fPath;
};

class MenuFileMenu: public ObjectMenu, private MenuLoader {
public:
    MenuFileMenu(
        IApp *app,
        YSMListener *smActionListener,
        YActionListener *wmActionListener,
        mstring name,
        YWindow *parent = nullptr);
    virtual ~MenuFileMenu();
    virtual void updatePopup();
    virtual void refresh();
private:
    mstring fName;
    upath fPath;
    time_t fModTime;
protected:
    IApp *app;
};

class MenuProgMenu: public ObjectMenu, private MenuLoader {
public:
    MenuProgMenu(
        IApp *app,
        YSMListener *smActionListener,
        YActionListener *wmActionListener,
        mstring name,
        const char* command,
        YStringArray &args,
        long timeout = 60L,
        YWindow *parent = nullptr);

    virtual ~MenuProgMenu();
    virtual void updatePopup();
    virtual void refresh();

private:
    mstring fName;
    const char* fCommand;
    YStringArray fArgs;
    time_t fModTime;
    long fTimeout;
};

class FocusMenu: public YMenu {
public:
    FocusMenu();
};

class HelpMenu: public ObjectMenu {
public:
    HelpMenu(IApp *app,
            YSMListener *smActionListener,
            YActionListener *wmActionListener);
};

class StartMenu: public MenuFileMenu {
public:
    StartMenu(
        IApp *app,
        YSMListener *smActionListener,
        YActionListener *wmActionListener,
        const char *name,
        YWindow *parent = nullptr);

    virtual bool handleKey(const XKeyEvent &key);
    virtual void updatePopup();
    virtual void refresh();

private:
    YSMListener *smActionListener;
    YActionListener *wmActionListener;
};

/**
 * Management item that wraps DProgram and holds the trigger key information.
 */
class KProgram {
public:
    KProgram(const char *key, DProgram *prog, bool bIsDynSwitchMenuProg);
    ~KProgram();

    void parse() { wm.parse(); }
    bool isKey(KeySym key, unsigned mod) const {
        return wm.eq(key, mod);
    }
    void open(unsigned mods);
    KeySym key() const { return wm.key; }
    unsigned modifiers() const { return wm.mod; }

private:
    WMKey wm;

    // not a program starter but custom switch menu
    // use as bool to fit into memory wasted wit 64bit alignment
    unsigned int bIsDynSwitchMenu;
    DProgram *fProg;
    // For dynswitch mode, keep the persistent handler window until its destroyed.
    // The instance is NOT deleted because there is apparently interference with ywindows cleanup
    // sequence and this object here is cached over process lifetime anyway.
    SwitchWindow *pSwitchWindow;
};

#endif

// vim: set sw=4 ts=4 et:
