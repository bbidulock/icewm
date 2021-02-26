/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"

#include "wmprog.h"
#include "prefs.h"
#include "wmwinmenu.h"
#include "wmapp.h"
#include "themes.h"
#include "browse.h"
#include "appnames.h"
#include "wmpref.h"
#include "wmswitch.h"
#include "intl.h"
#include <sys/stat.h>
#include <time.h>

DObjectMenuItem::DObjectMenuItem(DObject *object):
    YMenuItem(object->getName(), -3, null, YAction(), nullptr)
{
    fObject = object;
    if (object->getIcon() != null)
        setIcon(object->getIcon());
}

DObjectMenuItem::~DObjectMenuItem() {
    delete fObject;
}

void DObjectMenuItem::actionPerformed(YActionListener * /*listener*/, YAction /*action*/, unsigned int /*modifiers*/) {
    wmapp->signalGuiEvent(geLaunchApp);
    fObject->open();
}

DFile::DFile(IApp *app, const mstring &name, ref<YIcon> icon, upath path): DObject(app, name, icon) {
    this->app = app;
    fPath = path;
}

DFile::~DFile() {
}

void DFile::open() {
    const char *args[] = { openCommand, fPath.string(), nullptr };
    app->runProgram(openCommand, args);
}

ObjectMenu::ObjectMenu(YActionListener *actionListener, YWindow *parent):
        YMenu(parent),
        wmActionListener(nullptr) {
    setActionListener(actionListener);
}

void ObjectMenu::addObject(DObject *fObject) {
    add(new DObjectMenuItem(fObject));
}

void ObjectMenu::addObject(DObject *fObject, const char *icons) {
    add(new DObjectMenuItem(fObject), icons);
}

void ObjectMenu::addSeparator() {
    YMenu::addSeparator();
}

void ObjectMenu::addContainer(mstring name, ref<YIcon> icon, ObjectMenu *container) {
    if (container) {
        YMenuItem *item =
            addSubmenu(name, -3, container);

        if (item && icon != null)
            item->setIcon(icon);
    }
}

DObject::DObject(IApp *app, const mstring &name, ref<YIcon> icon):
    fName(name), fIcon(icon)
{
    this->app = app;
}

DObject::~DObject() {
    fIcon = null;
}

void DObject::open() {
}

DProgram::DProgram(
    IApp *app,
    YSMListener *smActionListener,
    const mstring &name,
    ref<YIcon> icon,
    const bool restart,
    const char *wmclass,
    upath exe,
    YStringArray &args)
    : DObject(app, name, icon),
    fRestart(restart),
    fRes(newstr(wmclass)),
    fPid(0),
    fCmd(exe),
    fArgs(args)
{
    this->app = app;
    this->smActionListener = smActionListener;
    if (fArgs.isEmpty() || fArgs.getString(fArgs.getCount() - 1))
        fArgs.append(nullptr);
}

DProgram::~DProgram() {
    delete[] fRes;
}

void DProgram::open() {
    if (fRestart)
        smActionListener->restartClient(fCmd.string(), fArgs.getCArray());
    else if (fRes)
        smActionListener->runOnce(fRes, &fPid, fCmd.string(), fArgs.getCArray());
    else
        app->runProgram(fCmd.string(), fArgs.getCArray());
}

DProgram *DProgram::newProgram(
    IApp *app,
    YSMListener *smActionListener,
    const char *name,
    ref<YIcon> icon,
    const bool restart,
    const char *wmclass,
    upath exe,
    YStringArray &args)
{
    DProgram* program = nullptr;
    if (exe != null) {
        const char* exestr = exe.string();
        MSG(("LOOKING FOR: %s\n", exestr));
        csmart path(path_lookup(exestr));
        if (path) {
            program = new DProgram(app, smActionListener, name, icon,
                                   restart, wmclass, upath(path), args);
        } else {
            MSG(("Program %s (%s) not found.", name, exestr));
        }
    }
    return program;
}

KProgramArrayType keyProgs;

KProgram::KProgram(const char *key, DProgram *prog, bool bIsDynSwitchMenuProg)
    : fKey(NoSymbol), fMod(0), bIsDynSwitchMenu(bIsDynSwitchMenuProg), fProg(prog), pSwitchWindow(nullptr)
{
    YConfig::parseKey(key, &fKey, &fMod);
    keyProgs.append(this);
}

class MenuProgSwitchItems: public ISwitchItems {
    MenuProgMenu *menu;
    int zTarget;

    KeySym key;
    unsigned int mod;

public:
    MenuProgSwitchItems(DProgram* prog, KeySym key, unsigned keymod) : ISwitchItems()
        , zTarget(0), key(key), mod(keymod) {
        menu = new MenuProgMenu(wmapp, wmapp, nullptr /* no wmaction handling*/,
                "switch popup internal menu", prog->fCmd, prog->fArgs);
    }
    virtual void updateList() override {
        menu->refresh();
        zTarget = 0;
    }
    virtual int getCount() override {
        return menu->itemCount();
    }
    virtual bool isKey(KeySym k, unsigned int mod) override {
        return k == this->key && mod == this->mod;
    }
    virtual void setWMClass(char* wmclass) override {
        if (wmclass) free(wmclass); // unimplemented
    }

    // move the focused target up or down and return the new focused element
    virtual int moveTarget(bool zdown) override {
        int count = menu->itemCount();
        zTarget += zdown ? 1 : -1;
        if (zTarget >= count)
            zTarget = 0;
        if (zTarget < 0)
            zTarget = max(0, count - 1);
        // no further gimmicks
        return zTarget;
    }
    // move the focused target up or down and return the new focused element
        virtual int setTarget(int where) override {
            int count = menu->itemCount();
            return zTarget = inrange(where, 0, count) ? zTarget : 0;
        }

    /// Show changed focus preview to user
    virtual void displayFocusChange(int idxFocused) override {}
    // set target cursor and implementation specific stuff in the beginning
    virtual void begin(bool zdown) override {
        updateList();
        moveTarget(zdown);
    }
    virtual void reset() override {
        zTarget = 0;
    }
    virtual void cancel() override {
    }
    virtual void accept(IClosablePopup *parent) override {
        YMenuItem* item = menu->getItem(zTarget);
        if (!item) return;
        // even through all the obscure "abstraction" it should just run DObjectMenuItem::actionPerformed
        item->actionPerformed(nullptr, actionRun, 0);
        parent->close();
    }

    virtual int getActiveItem() override {
        return zTarget;
    }
    virtual mstring getTitle(int idx) override {
        if (idx<0 || idx>=this->getCount())
            return null;
        return menu->getItem(idx)->getName();
    }
    virtual ref<YIcon> getIcon(int idx) override {
        if (idx<0 || idx>=this->getCount())
            return null;
        return menu->getItem(idx)->getIcon();
    }

    // Manager notification about windows disappearing under the fingers
    virtual void destroyedItem(void* framePtr) override {
    }

};

void KProgram::open(unsigned mods) {
    if (!fProg) return;

    if (bIsDynSwitchMenu) {
        if (!pSwitchWindow) {
            pSwitchWindow = new SwitchWindow(desktop, new MenuProgSwitchItems(fProg, fKey, fMod), quickSwitchVertical);
        }
        pSwitchWindow->begin(true, mods);
    }
    else
        fProg->open();
}

MenuFileMenu::MenuFileMenu(
    IApp *app,
    YSMListener *smActionListener,
    YActionListener *wmActionListener,
    mstring name,
    YWindow *parent)
    :
    ObjectMenu(wmActionListener, parent),
    MenuLoader(app, smActionListener, wmActionListener),
    fName(name),
    fModTime(0),
    app(app)
{
}

MenuFileMenu::~MenuFileMenu() {
}

void MenuFileMenu::updatePopup() {
    if (!autoReloadMenus && fPath != null)
        return;

    upath np = app->findConfigFile(upath(fName));
    bool rel = false;


    if (fPath == null) {
        fPath = np;
        rel = true;
    } else {
        if (np == null || np.equals(fPath)) {
            fPath = np;
            rel = true;
        } else
            np = null;
    }

    if (fPath == null) {
        refresh();
    } else {
        struct stat sb;
        if (stat(fPath.string(), &sb) != 0) {
            fPath = null;
            refresh();
        } else if (sb.st_mtime > fModTime || rel) {
            fModTime = sb.st_mtime;
            refresh();
        }
    }
}

void MenuFileMenu::refresh() {
    removeAll();
    if (fPath != null)
        loadMenus(fPath, this);
}

MenuProgMenu::MenuProgMenu(
    IApp *app,
    YSMListener *smActionListener,
    YActionListener *wmActionListener,
    mstring name,
    upath command,
    YStringArray &args,
    long timeout,
    YWindow *parent)
    :
    ObjectMenu(wmActionListener, parent),
    MenuLoader(app, smActionListener, wmActionListener),
    fName(name),
    fCommand(command),
    fArgs(args),
    fModTime(0),
    fTimeout(timeout)
{
}

MenuProgMenu::~MenuProgMenu() {
}

void MenuProgMenu::updatePopup() {
    time_t now = time(nullptr);
    if (fModTime == 0 || (0 < fTimeout && now >= fModTime + fTimeout)) {
        refresh();
        fModTime = now;
    }
}

void MenuProgMenu::refresh()
{
    removeAll();
    if (fCommand != null)
        progMenus(fCommand.string(), fArgs.getCArray(), this);
}

StartMenu::StartMenu(
    IApp *app,
    YSMListener *smActionListener,
    YActionListener *wmActionListener,
    const char *name,
    YWindow *parent)
    : MenuFileMenu(app, smActionListener, wmActionListener, name, parent),
    smActionListener(smActionListener),
    wmActionListener(wmActionListener)
{
}

bool StartMenu::handleKey(const XKeyEvent &key) {
    // If meta key, close the popup
    if (key.type == KeyPress) {
        KeySym k = keyCodeToKeySym(key.keycode);
        int m = KEY_MODMASK(key.state);

        if ((k == xapp->Win_L ||
             k == XK_Multi_key ||
             k == xapp->Win_R) && m == 0)
        {
            cancelPopup();
            return true;
        }
    }
    return MenuFileMenu::handleKey(key);
}

void StartMenu::updatePopup() {
    MenuFileMenu::updatePopup();
}

FocusMenu::FocusMenu() {
    struct FocusModelNameAction {
        FocusModel mode;
        const char *name;
        YAction action;
    } foci[] = {
        { FocusClick, _("_Click to focus"), actionFocusClickToFocus },
        { FocusExplicit, _("_Explicit focus"), actionFocusExplicit },
        { FocusSloppy, _("_Sloppy mouse focus"), actionFocusMouseSloppy },
        { FocusStrict, _("S_trict mouse focus"), actionFocusMouseStrict },
        { FocusQuiet, _("_Quiet sloppy focus"), actionFocusQuietSloppy },
        { FocusCustom, _("Custo_m"), actionFocusCustom },
    };
    for (size_t k = 0; k < ACOUNT(foci); ++k) {
        YMenuItem *item = addItem(foci[k].name, -2, null, foci[k].action);
        if (wmapp->getFocusMode() == foci[k].mode) {
            item->setEnabled(false);
            item->setChecked(true);
        }
    }
}

HelpMenu::HelpMenu(
    IApp *app,
    YSMListener *smActionListener,
    YActionListener *wmActionListener)
    : ObjectMenu(wmActionListener)
{
    struct HelpMenuItem {
        const char *name;
        const char *menu;
        const char *clas;
    } help[] = {
        { ICEHELPIDX, _("_Manual"), "browser.Manual" },
        { "icewm.1.html", _("_Icewm(1)"), "browser.IceWM" },
        { "icewmbg.1.html", _("Icewm_Bg(1)"), "browser.IcewmBg" },
        { "icesound.1.html", _("Ice_Sound(1)"), "browser.IceSound" },
    };
    for (size_t k = 0; k < ACOUNT(help); ++k) {
        YStringArray args(3);
        args.append(ICEHELPEXE);
        if (k == 0) {
            args.append(help[k].name);
        } else {
            upath path = upath(ICEHELPIDX).parent() + help[k].name;
            args.append(path.string());
        }
        args.append(nullptr);

        DProgram *prog = DProgram::newProgram(
            app,
            smActionListener,
            help[k].menu,
            null,
            false,
            help[k].clas,
            ICEHELPEXE,
            args);

        if (prog)
            addObject(prog, "help");
    }
}

void StartMenu::refresh() {
    MenuFileMenu::refresh();

    addSeparator();

/// TODO #warning "make this into a menuprog (ala gnome.cc), and use mime"
    if (nonempty(openCommand)) {
        upath path[] = { upath::root(), YApplication::getHomeDir() };
        YMenu *sub;
        ref<YIcon> folder = YIcon::getIcon("folder");
        for (unsigned int i = 0; i < ACOUNT(path); i++) {
            upath& p = path[i];

            sub = new BrowseMenu(app, smActionListener, wmActionListener, p);
            DFile *file = new DFile(app, p, null, p);
            YMenuItem *item = add(new DObjectMenuItem(file));
            if (item && sub) {
                item->setSubmenu(sub);
                if (folder != null)
                    item->setIcon(folder);
            }
            else {
                delete sub;
            }
        }
        addSeparator();
    }

    if (showPrograms) {
        ObjectMenu *programs = new MenuFileMenu(app, smActionListener,
                                    wmActionListener, "programs", nullptr);
        ///    if (programs->itemCount() > 0)
        addSubmenu(_("Programs"), 0, programs, "programs");
    }

    if (showRun && nonempty(runDlgCommand)) {
        addItem(_("_Run..."), -2, null, actionRun, "run");
    }

    if (showWindowList) {
        addItem(_("_Windows"), -2, actionWindowList, windowListMenu, "windows");
    }

    YMenu* settings = new YMenu();

    if (!showTaskBar && showAbout) {
        settings->addItem(_("_About"), -2, actionAbout, nullptr, "about");
    }

    if (showHelp) {
        HelpMenu* help = new HelpMenu(app, smActionListener, wmActionListener);
        settings->addSubmenu(_("_Help"), -2, help, "help");
    }

    if (showFocusModeMenu) {
        FocusMenu *focus = new FocusMenu();
        settings->addSubmenu(_("_Focus"), -2, focus, "focus");
    }

    if (showSettingsMenu) {
        PrefsMenu *prefs = new PrefsMenu();
        settings->addSubmenu(_("_Preferences"), -2, prefs, "pref");
    }

    if (showThemesMenu) {
        YMenu *themes = new ThemesMenu(app, smActionListener, wmActionListener);
        settings->addSubmenu(_("_Themes"), -2, themes, "themes");
    }

    if (settings->itemCount()) {
        addSubmenu(_("Se_ttings"), -2, settings, "settings");
    } else {
        delete settings;
    }

    if (showLogoutMenu) {
        addSeparator();
        if (showLogoutSubMenu)
            addItem(_("_Logout..."), -2, actionLogout, logoutMenu, "logout");
        else
            addItem(_("_Logout..."), -2, null, actionLogout, "logout");
    }
}

// vim: set sw=4 ts=4 et:
