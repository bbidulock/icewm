#ifndef __WMPROG_H
#define __WMPROG_H

#ifndef NO_CONFIGURE_MENUS

#include "objmenu.h"
#include "yarray.h"
#include <sys/time.h>

class ObjectContainer;

void loadMenus(const char *fileName, ObjectContainer *container);

class DProgram: public DObject {
public:
    virtual ~DProgram();

    virtual void open();
    
    static char *fullname(const char *exe);
    static DProgram *newProgram(const char *name, YIcon *icon,
                                const bool restart, const char *wmclass,
                                const char *exe, YStringArray &args);

protected:
    DProgram(const char *name, YIcon *icon, const bool restart,
             const char *wmclass, const char *exe, YStringArray &args);

private:
    const bool fRestart;
    const char *fRes;
    const char *fCmd;
    YStringArray fArgs;
};

class DFile: public DObject {
public:
    DFile(const char *name, YIcon *icon, const char *path);
    virtual ~DFile();

    virtual void open();
private:
    char *fPath;
};

class MenuFileMenu: public ObjectMenu {
public:
    MenuFileMenu(const char *name, YWindow *parent = 0);
    virtual ~MenuFileMenu();
    virtual void updatePopup();
    virtual void refresh();
private:
    char *fName;
    char *fPath;
protected:
    time_t fModTime;
};

class MenuProgMenu: public ObjectMenu {
public:
    MenuProgMenu(const char *name, const char *command, YStringArray &args, YWindow *parent = 0);
    virtual ~MenuProgMenu();
    virtual void updatePopup();
    virtual void refresh();
private:
    char *fName;
    char *fCommand;
    YStringArray fArgs;
protected:
    time_t fModTime;
};

class MenuProgReloadMenu: public MenuProgMenu {
public:
    MenuProgReloadMenu(const char *name, time_t timeout, const char *command, YStringArray &args, YWindow *parent = 0);
    virtual void updatePopup();
protected:
    time_t fTimeout;
};

class StartMenu: public MenuFileMenu {
public:
    StartMenu(const char *name, YWindow *parent = 0);
    virtual bool handleKey(const XKeyEvent &key);
    virtual void updatePopup();
    virtual void refresh();
    
    bool fHasGnomeAppsMenu;
    bool fHasGnomeUserMenu;
    bool fHasKDEMenu;
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
