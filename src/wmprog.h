#ifndef WMPROG_H
#define WMPROG_H

#include "objmenu.h"
#include "wmkey.h"

class ObjectContainer;
class YSMListener;
class YActionListener;
class Switcher;
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
    char* parseAKey(char *word, char *p, bool runonce, bool switchkey);
    char* parseProgram(char *word, char *p, bool restart,
                       bool runonce, ObjectContainer *container);
    char* parseWord(char *word, char *p, ObjectContainer *container);

    IApp *app;
    YSMListener *smActionListener;
    YActionListener *wmActionListener;
};

class AProgram {
public:
    virtual void open(YSMListener* smActionListener, unsigned mods = 0) = 0;
};

class RProgram : public AProgram {
public:
    RProgram(bool restart, const char* resource,
             const char* command, YStringArray &args);
    virtual ~RProgram();
    void open(YSMListener* smActionListener, unsigned mods = 0) override;

    const char* cmd() const { return fCmd; }
    YStringArray& args() { return fArgs; }
    const char* resource() const { return fRes; }

private:
    const bool fRestart;
    const char *fRes;
    long fPid;
    const char* fCmd;
    YStringArray fArgs;
};

class KProgram : public RProgram {
public:
    KProgram(const char* key, const char* resource,
             const char* command, YStringArray &args);
    ~KProgram();

    void parse() { wm.parse(); }
    bool isKey(const XKeyEvent& x) const { return wm == x; }
    bool isButton(const XButtonEvent& b) const { return wm == b; }
    void grab(int handle) { wm.grab(handle); }

protected:
    WMKey wm;
};

class SProgram : public KProgram {
public:
    SProgram(const char* key, const char* resource,
             const char* command, YStringArray &args);
    ~SProgram();

    void open(YSMListener* smActionListener, unsigned mods = 0) override;

private:
    // Keep the persistent handler window until it is destroyed.
    // The instance is NOT deleted because there is apparently
    // interference with ywindows cleanup sequence and this
    // object here is cached over process lifetime anyway.
    Switcher *pSwitchWindow;
};

class DProgram: public DObject, public RProgram {
public:
    DProgram(YSMListener* smActionListener,
             const char* name,
             ref<YIcon> icon,
             bool restart,
             const char* wmclass,
             const char* command,
             YStringArray& args);
    void open() override;
private:
    using RProgram::open;
    YSMListener* smActionListener;
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

class HelpMenu: public YMenu {
public:
    HelpMenu();
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

#endif

// vim: set sw=4 ts=4 et:
