#ifndef __WMPROG_H
#define __WMPROG_H

#ifndef NO_CONFIGURE_MENUS

#include "objmenu.h"
#include <sys/time.h>

class ObjectContainer;

void loadMenus(const char *fileName, ObjectContainer *container);

class DProgram: public DObject {
public:
    virtual ~DProgram();

    virtual void open();

    static DProgram *newProgram(const char *name, YIcon *icon, bool restart, const char *exe, char **args);

private:
    DProgram(const char *name, YIcon *icon, bool restart, const char *exe, char **args);

    bool fRestart;
    char *fCmd;
    char **fArgs;
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
    time_t fModTime;
};

class StartMenu: public MenuFileMenu {
public:
    StartMenu(const char *name, YWindow *parent = 0);
    virtual void refresh();
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

#endif

// !!! move there somewhere else probably
int findPath(const char *path, int mode, const char *name, char **fullname, bool path_relative = false);

typedef struct {
    const char **root;
    const char *rdir;
    const char **sub;
}  pathelem;

extern pathelem icon_paths[10];

char *joinPath(pathelem *pe, const char *base, const char *name);
void verifyPaths(pathelem *search, const char *base);

int is_reg(const char *path);

void loadPixmap(pathelem *pe, const char *base, const char *name, YPixmap **pixmap);

extern KProgram *keyProgs;

#endif
