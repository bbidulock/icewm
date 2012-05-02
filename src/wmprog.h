#ifndef __WMPROG_H
#define __WMPROG_H

#ifndef NO_CONFIGURE_MENUS

#include "upath.h"
#include "objmenu.h"
#include "yarray.h"
#include <sys/time.h>

class ObjectContainer;
class YSMListener;
class YActionListener;

void loadMenus(IApp *app, YSMListener *smActionListener, YActionListener *wmActionListener, upath fileName, ObjectContainer *container);

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

class MenuFileMenu: public ObjectMenu {
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
protected:
    time_t fModTime;
    YSMListener *smActionListener;
    IApp *app;
};

class MenuProgMenu: public ObjectMenu {
public:
    MenuProgMenu(
        IApp *app,
        YSMListener *smActionListener,
        YActionListener *wmActionListener,
        ustring name,
        upath command,
        YStringArray &args,
        YWindow *parent = 0);
        
    virtual ~MenuProgMenu();
    virtual void updatePopup();
    virtual void refresh(
        YSMListener *smActionListener,
        YActionListener *wmActionListener);
private:
    ustring fName;
    upath fCommand;
    YStringArray fArgs;
protected:
    time_t fModTime;
    YSMListener *smActionListener;
    IApp *app;
};

class MenuProgReloadMenu: public MenuProgMenu {
public:
    MenuProgReloadMenu(
        IApp *app,
        YSMListener *smActionListener,
        YActionListener *wmActionListener,
        const char *name,
        time_t timeout,
        const char *command,
        YStringArray &args,
        YWindow *parent = 0);
    virtual void updatePopup();
protected:
    time_t fTimeout;
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

    bool fHasGnomeAppsMenu;
    bool fHasGnomeUserMenu;
    bool fHasKDEMenu;

private:
    YSMListener *smActionListener;
    YActionListener *wmActionListener;  
};

class KProgram {
public:
    KProgram(const char *key, DProgram *prog);

    bool isKey(KeySym key, unsigned int mod) {
        return (key == fKey && mod == fMod) ? true : false;
    }
    void open() {
        if (fProg)
            fProg->open();
    }
    KeySym key() { return fKey; }
    unsigned int modifiers() { return fMod; }

    KProgram *getNext() { return fNext; }
private:
    KProgram *fNext;
    KeySym fKey;
    unsigned int fMod;
    DProgram *fProg;
};

extern KProgram *keyProgs;

#endif /* NO_CONFIGURE_MENUS */

#endif
