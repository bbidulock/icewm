#ifndef __WMPROG_H
#define __WMPROG_H

#include "objmenu.h"

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
    friend class MenuProgSwitchItems;
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
        upath exe,
        YStringArray &args);

protected:
    DProgram(
        IApp *app,
        YSMListener *smActionListener,
        const ustring &name,
        ref<YIcon> icon,
        const bool restart,
        const char *wmclass,
        upath exe,
        YStringArray &args);

private:
    const bool fRestart;
    const char *fRes;
    long fPid;
    upath fCmd;
    YStringArray fArgs;
    YSMListener *smActionListener;
};

class DFile: public DObject {
public:
    DFile(IApp *app, const ustring &name, ref<YIcon> icon, upath path);
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
        ustring name,
        YWindow *parent = 0);
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
        ustring name,
        upath command,
        YStringArray &args,
        long timeout = 60L,
        YWindow *parent = 0);

    virtual ~MenuProgMenu();
    virtual void updatePopup();
    virtual void refresh();

private:
    ustring fName;
    upath fCommand;
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
        YWindow *parent = 0);

    virtual bool handleKey(const XKeyEvent &key);
    virtual void updatePopup();
    virtual void refresh();

private:
    YSMListener *smActionListener;
    YActionListener *wmActionListener;
};

/**
 * Management item which wraps DProgram and holds the trigger key information.
 */
class KProgram {
public:
    KProgram(const char *key, DProgram *prog, bool bIsDynSwitchMenuProg);
    ~KProgram() { delete fProg; }

    bool isKey(KeySym key, unsigned int mod) {
        return (key == fKey && mod == fMod);
    }
    void open(unsigned mods);
    KeySym key() { return fKey; }
    unsigned int modifiers() { return fMod; }

private:
    KeySym fKey;
    unsigned int fMod;
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
