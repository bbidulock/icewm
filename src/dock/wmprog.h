#ifndef __WMPROG_H
#define __WMPROG_H

#include "objmenu.h"
#include <sys/time.h>

class ObjectContainer;
class YKeyBind;

void loadMenus(const char *fileName, ObjectContainer *container);

class DProgram: public DObject {
public:
    virtual ~DProgram();

    virtual void open();

    static DProgram *newProgram(const char *name, YIcon *icon, bool restart, const char *exe, char **args);
    friend class DProgramConstructor; // !!! just to shut up gcc

private:
    DProgram(const char *name, YIcon *icon, bool restart, const char *exe, char **args);

    bool fRestart;
    char *fCmd;
    char **fArgs;
private: // not-used
    DProgram(const DProgram &);
    DProgram &operator=(const DProgram &);
};

class DFile: public DObject {
public:
    DFile(const char *name, YIcon *icon, const char *path);
    virtual ~DFile();

    virtual void open();
private:
    char *fPath;
private: // not-used
    DFile(const DFile &);
    DFile &operator=(const DFile &);
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
private: // not-used
    MenuFileMenu(const MenuFileMenu &);
    MenuFileMenu &operator=(const MenuFileMenu &);
};

class StartMenu: public MenuFileMenu {
public:
    StartMenu(const char *name, YWindow *parent = 0);
    virtual void refresh();
};

class KProgram {
public:
    KProgram(const char *key, DProgram *prog);

    bool isKey(const YKeyEvent &key); 
    void open() {
        if (fProg)
            fProg->open();
    }
    YKeyBind *keyBind() { return fKeyBind; }

    KProgram *getNext() { return fNext; }
private:
    KProgram *fNext;
    YKeyBind *fKeyBind;
    DProgram *fProg;
private: // not-used
    KProgram(const KProgram &);
    KProgram &operator=(const KProgram &);
};

extern KProgram *keyProgs;

#endif
