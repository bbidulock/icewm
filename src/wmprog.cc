/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"

#define NEED_TIME_H

#include "wmprog.h"
#include "prefs.h"
#include "wmapp.h"
#include "sysdep.h"
#include "wmmgr.h"
#include "themes.h"
#include "browse.h"
#include "appnames.h"
#include "ascii.h"
#include "argument.h"
#include "wmswitch.h"
#include "ypointer.h"
#include <regex.h>
#include "intl.h"

DObjectMenuItem::DObjectMenuItem(DObject *object):
    YMenuItem(object->getName(), -3, null, YAction(), 0)
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

DFile::DFile(IApp *app, const ustring &name, ref<YIcon> icon, upath path): DObject(app, name, icon) {
    this->app = app;
    fPath = path;
}

DFile::~DFile() {
}

void DFile::open() {
    const char *args[] = { openCommand, fPath.string(), 0 };
    app->runProgram(openCommand, args);
}

ObjectMenu::ObjectMenu(YActionListener *actionListener, YWindow *parent): YMenu(parent) {
    setActionListener(actionListener);
}

ObjectMenu::~ObjectMenu() {
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

void ObjectMenu::addContainer(const ustring &name, ref<YIcon> icon, ObjectMenu *container) {
    if (container) {
        YMenuItem *item =
            addSubmenu(name, -3, container);

        if (item && icon != null)
            item->setIcon(icon);
    }
}

DObject::DObject(IApp *app, const ustring &name, ref<YIcon> icon):
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
    const ustring &name,
    ref<YIcon> icon,
    const bool restart,
    const char *wmclass,
    upath exe,
    YStringArray &args)
    : DObject(app, name, icon),
    fRestart(restart),
    fRes(newstr(wmclass)),
    fCmd(exe),
    fArgs(args)
{
    this->app = app;
    this->smActionListener = smActionListener;
    if (fArgs.isEmpty() || fArgs.getString(fArgs.getCount() - 1))
        fArgs.append(0);
}

DProgram::~DProgram() {
    delete[] fRes;
}

void DProgram::open() {
    if (fRestart)
        smActionListener->restartClient(fCmd.string(), fArgs.getCArray());
    else if (fRes)
        smActionListener->runOnce(fRes, fCmd.string(), fArgs.getCArray());
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
    if (exe != null) {
        MSG(("LOOKING FOR: %s\n", exe.string().c_str()));
        upath fullname = findPath(getenv("PATH"), X_OK, exe);
        if (fullname == null) {
            MSG(("Program %s (%s) not found.", name, exe.string().c_str()));
            return 0;
        }

        DProgram *program =
            new DProgram(app, smActionListener, name, icon, restart, wmclass, fullname, args);

        return program;
    }
    return NULL;
}

static char *getWord(char *word, int maxlen, char *p) {
    while (ASCII::isWhiteSpace(*p))
        p++;
    while (ASCII::isAlnum(*p) && maxlen > 1) {
        *word++ = *p++;
        maxlen--;
    }
    *word++ = 0;
    return p;
}

static char *getCommandArgs(char *source, Argument *command,
                            YStringArray &args)
{
    char *p = source;
    p = YConfig::getArgument(command, p);
    if (p == 0) {
        msg(_("Missing command argument"));
        return p;
    }
    args.append(*command);

    Argument argx;
    while (*p) {

//push to the next word or line end to get the arg
        while (*p && (*p == ' ' || *p == '\t'))
            p++;
//stop on EOL
        if (*p == '\n')
            break;

        // parse the argument into argx and set the new position
        p = YConfig::getArgument(&argx, p);
        if (p == 0) {
            msg(_("Bad argument %d to command '%s'"),
                    args.getCount() + 1, command->cstr());
            return p;
        }

        args.append(argx);
        MSG(("ARG: %s\n", argx.cstr()));
    }
    args.append(0);

    return p;
}

YObjectArray<KProgram> keyProgs;

KProgram::KProgram(const char *key, DProgram *prog, bool bIsDynSwitchMenuProg)
    : fKey(NoSymbol), fMod(0), bIsDynSwitchMenu(bIsDynSwitchMenuProg), fProg(prog), pSwitchWindow(0)
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
        menu = new MenuProgMenu(wmapp, wmapp, NULL /* no wmaction handling*/,
                "switch popup internal menu", prog->fCmd, prog->fArgs, 0);
    }
    virtual void updateList() OVERRIDE
            {
        menu->refresh(wmapp, 0);
        zTarget = 0;
            }
    virtual int getCount() OVERRIDE {
        return menu->itemCount();
    }
    virtual bool isKey(KeySym k, unsigned int mod) OVERRIDE {
        return k == this->key && mod == this->mod;
    }

    // move the focused target up or down and return the new focused element
    virtual int moveTarget(bool zdown) OVERRIDE {
        int count = menu->itemCount();
        zTarget = (zTarget + count + (zdown?1:-1)) % count;
        // no further gimmicks
        return zTarget;
    }
    /// Show changed focus preview to user
    virtual void displayFocusChange(int idxFocused) OVERRIDE {}
    // set target cursor and implementation specific stuff in the beginning
    virtual void begin(bool zdown) OVERRIDE {
        updateList();
        moveTarget(zdown);
    }
    virtual void cancel() OVERRIDE {
    }
    virtual void accept(IClosablePopup *parent) OVERRIDE {
        YMenuItem* item=menu->getItem(zTarget);
        if (!item) return;
        // even through all the obscure "abstraction" it should just run DObjectMenuItem::actionPerformed
        item->actionPerformed(0, actionRun, 0);
        parent->close();
    }

    virtual int getActiveItem() OVERRIDE {
        return zTarget;
    }
    virtual ustring getTitle(int idx) OVERRIDE {
        if (idx<0 || idx>=this->getCount())
            return null;
        return menu->getItem(idx)->getName();
    }
    virtual ref<YIcon> getIcon(int idx) OVERRIDE {
        if (idx<0 || idx>=this->getCount())
            return null;
        return menu->getItem(idx)->getIcon();
    }

    // Manager notification about windows disappearing under the fingers
    virtual void destroyedItem(void* framePtr) OVERRIDE {
    }

};

void KProgram::open(unsigned mods) {
    if (!fProg) return;

    if (bIsDynSwitchMenu) {
        if (!pSwitchWindow) {
            pSwitchWindow = new SwitchWindow(manager, new MenuProgSwitchItems(fProg, fKey, fMod), quickSwitchVertical);
        }
        pSwitchWindow->begin(true, mods);
    }
    else
        fProg->open();
}

ustring guessIconNameFromExe(const char* exe)
{
    upath fullname(exe);
    char buf[1024];
    for (int i=7; i; --i)
    {
        fullname = findPath(getenv("PATH"), X_OK, fullname);
        if (fullname == null)
            return "-";
        ssize_t linkLen = readlink(fullname.string().c_str(), buf, ACOUNT(buf));
        if (linkLen < 0)
            break;
        fullname = upath(buf, linkLen);
    }
    // crop to the generic name
    ustring s(fullname);
    int spos = s.lastIndexOf('/');
    if (spos >= 0)
        s = s.remove(0, spos + 1);
    // scripts have a suffix sometimes which is not part of the icon name
    spos = s.indexOf('.');
    if (spos >= 0)
        s = s.substring(0, spos);
    return s;
}

char *parseIncludeStatement(
        IApp *app,
        YSMListener *smActionListener,
        YActionListener *wmActionListener,
        char *p,
        ObjectContainer *container)
{
    Argument filename;

    p = YConfig::getArgument(&filename, p);
    if (p == 0) {
        warn(_("Missing filename argument to include statement"));
        return p;
    }

    upath path(filename);

    if (!path.isAbsolute())
        path = app->findConfigFile(path);

    if (path != null)
        loadMenus(app, smActionListener, wmActionListener, path, container);

    return p;
}

char *parseMenus(
        IApp *app,
        YSMListener *smActionListener,
        YActionListener *wmActionListener,
        char *data,
        ObjectContainer *container)
{
    char *p = data;
    char word[32];

    while (p && *p) {
        if (ASCII::isWhiteSpace(*p)) {
            p++;
            continue;
        }
        if (*p == '#') {
            while (*p && *p != '\n')
                p++;
            continue;
        }
        p = getWord(word, sizeof(word), p);

        if (container) {
            if (!strcmp(word, "separator"))
                container->addSeparator();
            else if (!(strcmp(word, "prog") &&
                       strcmp(word, "restart") &&
                       strcmp(word, "runonce")))
            {
                Argument name;

                p = YConfig::getArgument(&name, p);
                if (p == 0) return p;

                Argument icons;

                p = YConfig::getArgument(&icons, p);
                if (p == 0) return p;

                Argument wmclass;

                if (word[1] == 'u') {
                    p = YConfig::getArgument(&wmclass, p);
                    if (p == 0) return p;
                }

                Argument command;
                YStringArray args;

                p = getCommandArgs(p, &command, args);
                if (p == 0) {
                    msg(_("Error at %s '%s'"), word, name.cstr());
                    return p;
                }

                ref<YIcon> icon;

                if (icons[0] == '!')
                {
                    ustring iconName = guessIconNameFromExe(command);
                    if (iconName.charAt(0) != '-')
                        icon = YIcon::getIcon(cstring(iconName));

                }
                else if (icons[0] != '-')
                    icon = YIcon::getIcon(icons);

                DProgram * prog = DProgram::newProgram(
                    app,
                    smActionListener,
                    name,
                    icon,
                    word[1] == 'e',
                    word[1] == 'u' ? wmclass.cstr() : NULL,
                    command.cstr(),
                    args);

                if (prog) container->addObject(prog);
            } else if (!strcmp(word, "menu")) {
                Argument name;

                p = YConfig::getArgument(&name, p);
                if (p == 0) return p;

                Argument icons;

                p = YConfig::getArgument(&icons, p);
                if (p == 0) return p;

                p = getWord(word, sizeof(word), p);
                if (*p != '{') return 0;
                p++;

                ref<YIcon> icon;
                if (icons[0] != '-')
                    icon = YIcon::getIcon(icons);

                ObjectMenu *sub = new ObjectMenu(wmActionListener);

                if (sub) {
                    p = parseMenus(app, smActionListener, wmActionListener, p, sub);

                    if (sub->itemCount() == 0)
                        delete sub;
                    else
                        container->addContainer(name.cstr(), icon, sub);

                } else {
                    msg(_("Unexepected menu keyword: '%s'"), name.cstr());
                    return p;
                }
            } else if (!strcmp(word, "menufile")) {
                Argument name;

                p = YConfig::getArgument(&name, p);
                if (p == 0) return p;

                Argument icons;

                p = YConfig::getArgument(&icons, p);
                if (p == 0) return p;

                Argument menufile;

                p = YConfig::getArgument(&menufile, p);
                if (p == 0) return p;

                ref<YIcon> icon;
                if (icons[0] != '-')
                    icon = YIcon::getIcon(icons);
                ObjectMenu *filemenu = new MenuFileMenu(
                        app, smActionListener, wmActionListener,
                        menufile.cstr(), 0);

                if (menufile)
                    container->addContainer(name.cstr(), icon, filemenu);
            } else if (!strcmp(word, "menuprog")) {
                Argument name;

                p = YConfig::getArgument(&name, p);
                if (p == 0) return p;

                Argument icons;

                p = YConfig::getArgument(&icons, p);
                if (p == 0) return p;

                Argument command;
                YStringArray args;

                p = getCommandArgs(p, &command, args);
                if (p == 0) {
                    msg(_("Error at menuprog '%s'"), name.cstr());
                    return p;
                }

                ref<YIcon> icon;
                if (icons[0] != '-')
                    icon = YIcon::getIcon(icons);
                MSG(("menuprog %s %s", name.cstr(), command.cstr()));

                upath fullPath = findPath(getenv("PATH"), X_OK, command.cstr());
                if (fullPath != null) {
                    ObjectMenu *progmenu = new MenuProgMenu(
                            app, smActionListener, wmActionListener,
                            name.cstr(), command.cstr(), args, 0);
                    if (progmenu)
                        container->addContainer(name.cstr(), icon, progmenu);
                }
            } else if (!strcmp(word, "menuprogreload")) {
                Argument name;

                p = YConfig::getArgument(&name, p);
                if (p == 0) return p;

                Argument icons;

                p = YConfig::getArgument(&icons, p);
                if (p == 0) return p;

                Argument timeoutStr;

                p = YConfig::getArgument(&timeoutStr, p);
                if (p == 0) return p;
                time_t timeout = (time_t) atol(timeoutStr);

                Argument command;
                YStringArray args;

                p = getCommandArgs(p, &command, args);
                if (p == 0) {
                    msg(_("Error at %s: '%s'"), word, name.cstr());
                    return p;
                }

                ref<YIcon> icon;
                if (icons[0] != '-')
                    icon = YIcon::getIcon(icons);
                MSG(("menuprogreload %s %s", name.cstr(), command.cstr()));

                upath fullPath = findPath(getenv("PATH"), X_OK, command.cstr());
                if (fullPath != null) {
                    ObjectMenu *progmenu = new MenuProgReloadMenu(
                            app, smActionListener, wmActionListener,
                            name, timeout, command, args, 0);
                    if (progmenu)
                        container->addContainer(name.cstr(), icon, progmenu);
                }
            } else if (!strcmp(word, "include")) {
                p = parseIncludeStatement(app, smActionListener,
                        wmActionListener, p, container);
            } else if (*p == '}')
                return ++p;
            else {
                msg(_("Unknown keyword '%s'"), word);
                return 0;
            }
        } else {
            if (0 == strcmp(word, "key")
                    || 0 == strcmp(word, "runonce")
                    || 0 == strcmp(word, "switchkey"))
            {
                Argument key;

                p = YConfig::getArgument(&key, p);
                if (p == 0) return p;

                Argument wmclass;

                if (*word == 'r') {
                    p = YConfig::getArgument(&wmclass, p);
                    if (p == 0) return p;
                }

                Argument command;
                YStringArray args;

                p = getCommandArgs(p, &command, args);
                if (p == 0) {
                    msg(_("Error at keyword '%s' for %s"), word, key.cstr());
                    return p;
                }

                DProgram *prog = DProgram::newProgram(
                    app,
                    smActionListener,
                    key,
                    null,
                    false,
                    *word == 'r' ? wmclass.cstr() : NULL,
                    command.cstr(),
                    args);

                if (prog) new KProgram(key, prog, *word == 's');
            } else {
                msg(_("Unknown keyword for a non-container: '%s'.\n"
                      "Expected either 'key' or 'runonce' here.\n"), word);
                return 0;
            }
        }
    }
    return p;
}

void loadMenus(
    IApp *app,
    YSMListener *smActionListener,
    YActionListener *wmActionListener,
    upath menufile,
    ObjectContainer *container)
{
    if (menufile.isEmpty())
        return;

    MSG(("menufile: %s", menufile.string().c_str()));
    char *buf = load_text_file(menufile.string());
    if (buf) {
        parseMenus(app, smActionListener, wmActionListener, buf, container);
        delete[] buf;
    }
}

MenuFileMenu::MenuFileMenu(
    IApp *app,
    YSMListener *smActionListener,
    YActionListener *wmActionListener,
    ustring name,
    YWindow *parent)
    : ObjectMenu(wmActionListener, parent), fName(name)
{
    this->app = app;
    this->smActionListener = smActionListener;
    fName = name;
    fPath = null;
    fModTime = 0;
    ///    updatePopup();
    ///    refresh();
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
        loadMenus(app, smActionListener, wmActionListener, fPath, this);
}

static void loadMenusProg(
    IApp *app,
    YSMListener *smActionListener,
    YActionListener *wmActionListener,
    const char *command,
    char *const argv[],
    ObjectContainer *container)
{
    FILE *fpt = tmpfile();
    if (fpt == 0) {
        fail("tmpfile");
        return;
    }

    int tfd = fileno(fpt);
    int status = 0;
    pid_t child_pid = fork();

    if (child_pid == -1) {
        fail("Forking '%s' failed", command);
    }
    else if (child_pid == 0) {
        int devnull = open("/dev/null", O_RDONLY);
        if (devnull > 0) {
            dup2(devnull, 0);
            close(devnull);
        }
        if (dup2(tfd, 1) == 1) {
            if (tfd > 2) close(tfd);
            execvp(command, argv);
        }
        fail("Exec '%s' failed", command);
        _exit(99);
    }
    else if (waitpid(child_pid, &status, 0) == 0 && status != 0) {
        warn("'%s' exited with code %d.", command, status);
    }
    else if (lseek(tfd, (off_t) 0L, SEEK_SET) == (off_t) -1) {
        fail("lseek failed");
    }
    else {
        char *buf = load_fd(tfd);
        if (buf && *buf) {
            parseMenus(app, smActionListener, wmActionListener, buf, container);
        }
        else {
            warn(_("'%s' produces no output"), command);
        }
        delete[] buf;
    }
    fclose(fpt);
}

MenuProgMenu::MenuProgMenu(
    IApp *app,
    YSMListener *smActionListener,
    YActionListener *wmActionListener,
    ustring name,
    upath command,
    YStringArray &args,
    YWindow *parent)
    : ObjectMenu(wmActionListener, parent),
      fName(name), fCommand(command), fArgs(args)
{
    this->app = app;
    this->smActionListener = smActionListener;
    fName = name;
    fCommand = command;
    fArgs.append(0);
    fModTime = 0;
    ///    updatePopup();
    ///    refresh();
}

MenuProgMenu::~MenuProgMenu() {
}

void MenuProgMenu::updatePopup() {
#if 0
    if (!autoReloadMenus && fPath != 0)
        return;
    struct stat sb;
    char *np = app->findConfigFile(fName);
    bool rel = false;


    if (fPath == null) {
        fPath = np;
        rel = true;
    } else {
        if (!np || strcmp(np, fPath) != 0) {
            delete[] fPath;
            fPath = np;
            rel = true;
        } else
            delete[] np;
    }

    if (!autoReloadMenus)
        return;

    if (fPath == 0) {
        refresh();
    } else {
        if (stat(fPath, &sb) != 0) {
            delete[] fPath;
            fPath = null;
            refresh();
        } else if (sb.st_mtime > fModTime || rel) {
            fModTime = sb.st_mtime;
            refresh();
        }
    }
#endif
    if (fModTime == 0)
        refresh(smActionListener, wmActionListener);
    fModTime = time(NULL);
/// TODO #warning "figure out some way for this to work"
}

void MenuProgMenu::refresh(
    YSMListener *smActionListener,
    YActionListener *wmActionListener)
{
    removeAll();
    if (fCommand != null)
        loadMenusProg(app, smActionListener, wmActionListener,
                fCommand.string(), fArgs.getCArray(), this);
}

MenuProgReloadMenu::MenuProgReloadMenu(
    IApp *app,
    YSMListener *smActionListener,
    YActionListener *wmActionListener,
    const char *name,
    time_t timeout,
    const char *command,
    YStringArray &args,
    YWindow *parent)
    : MenuProgMenu(app, smActionListener, wmActionListener,
            name, command, args, parent)
{
    fTimeout = timeout;
}

void MenuProgReloadMenu::updatePopup() {
    if (fModTime == 0 || time(NULL) >= fModTime + fTimeout) {
        refresh(smActionListener, wmActionListener);
        fModTime = time(NULL);
    }
}

StartMenu::StartMenu(
    IApp *app,
    YSMListener *smActionListener,
    YActionListener *wmActionListener,
    const char *name,
    YWindow *parent)
    : MenuFileMenu(app, smActionListener, wmActionListener, name, parent)
{
    this->smActionListener = smActionListener;
    this->wmActionListener = wmActionListener;
    fHasGnomeAppsMenu =
        fHasGnomeUserMenu =
        fHasKDEMenu = false;

    ///    updatePopup();
    ///    refresh();
}

bool StartMenu::handleKey(const XKeyEvent &key) {
    // If meta key, close the popup
    if (key.type == KeyPress) {
        KeySym k = keyCodeToKeySym(key.keycode);
        int m = KEY_MODMASK(key.state);

        if (((k == xapp->Win_L) || (k == xapp->Win_R)) && m == 0) {
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
        FocusModels mode;
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
        YMenuItem *item = addItem(foci[k].name, -2, "", foci[k].action);
        if (focusMode == foci[k].mode) {
            item->setEnabled(false);
            item->setChecked(true);
        }
    }
}

extern cfoption icewm_preferences[];

class PrefsMenu: public YMenu, private YActionListener {
private:
    const int count;
    const YAction actionSaveMod;
    static YArray<int> mods;

public:
    PrefsMenu() :
        count(countPrefs())
    {
        YMenu *fo, *qs, *tb, *sh, *al, *mz, *ke, *ks, *kw, *sc;
        YMenuItem *item;

        addSubmenu("_Focus", -2, fo = new YMenu, "focus");
        addSubmenu("_QuickSwitch", -2, qs = new YMenu, "pref");
        addSubmenu("_TaskBar", -2, tb = new YMenu, "pref");
        addSubmenu("_Show", -2, sh = new YMenu, "pref");
        addSubmenu("_A... - L...", -2, al = new YMenu, "pref");
        addSubmenu("_M... - Z...", -2, mz = new YMenu, "pref");
        addSubmenu("_KeyWin", -2, ke = new YMenu, "key");
        addSubmenu("Key_Sys", -2, ks = new YMenu, "key");
        addSubmenu("KeySys_Workspace", -2, kw = new YMenu, "key");
        addSubmenu("S_calar", -2, sc = new YMenu, "key");

        int index[count];
        for (int i = 0; i < count; ++i) {
            index[i] = i;
        }
        qsort(index, count, sizeof(int), sortPrefs);

        for (int k = 0; k < count; ++k) {
            cfoption* o = &icewm_preferences[index[k]];
            if (o->type == cfoption::CF_BOOL) {
                YMenu* m = 0 == strncmp(o->name, "Task", 4) ? tb :
                           0 == strncmp(o->name, "Quic", 4) ? qs :
                           0 == strncmp(o->name, "Show", 4) ||
                           0 == strncmp(o->name, "Page", 4) ? sh :
                           0 != strstr(o->name, "Focus") ? fo :
                           o->name[0] <= 'L' ? al : mz;
                item = m->addItem(o->name, -2, "", EAction(index[k] + 1));
                if (*o->v.bool_value) {
                    item->setChecked(true);
                }
            }
            else if (o->type == cfoption::CF_KEY) {
                const char* key = o->v.k.key_value->name;
                if (0 == strncmp(o->name, "KeySysWork", 10)) {
                    kw->addItem(o->name, -2, key, actionNull);
                } else if (0 == strncmp(o->name, "KeySys", 6)) {
                    ks->addItem(o->name, -2, key, actionNull);
                } else {
                    ke->addItem(o->name, -2, key, actionNull);
                }
            }
            else if (o->type == cfoption::CF_INT) {
                int val = *o->v.i.int_value;
                sc->addItem(o->name, -2, mstring(val), actionNull);
            }
        }

        addSeparator();
        addItem("Save Modifications", -2, null, actionSaveMod, "save");
        setActionListener(this);
    }

    virtual void actionPerformed(YAction action, unsigned int modifiers) {
        if (action == actionSaveMod)
            return saveModified();

        const int i = action.ident() - 1;
        cfoption* o = i + icewm_preferences;
        if (inrange(i, 0, count - 1) && o->type == cfoption::CF_BOOL) {
            *o->v.bool_value ^= true;
            msg("%s = %d", o->name, *o->v.bool_value);
            int k = find(mods, i);
            return k >= 0 ? mods.remove(k) : mods.append(i);
        }
    }

    static int countPrefs() {
        for (int k = 0; ; ++k)
            if (icewm_preferences[k].type == cfoption::CF_NONE)
                return k;
    }

    static int sortPrefs(const void* p1, const void* p2) {
        const int i1 = *(const int *)p1;
        const int i2 = *(const int *)p2;
        cfoption* o1 = i1 + icewm_preferences;
        cfoption* o2 = i2 + icewm_preferences;
        if (o1->type == cfoption::CF_KEY &&
            o2->type == cfoption::CF_KEY &&
            0 == strncmp(o1->name, "KeySysWork", 10) &&
            0 == strncmp(o2->name, "KeySysWork", 10))
            return i1 - i2;
        return strcmp(o1->name, o2->name);
    }

    static void saveModified() {
        const int n = mods.getCount();
        if (n < 1)
            return;
        qsort(&*mods, n, sizeof(int), sortPrefs);
        upath path(wmapp->findConfigFile("preferences"));
        csmart text(load_text_file(path.string()));
        if (text == 0)
            (text = new char[1])[0] = 0;
        size_t tlen = strlen(text);
        for (int k = 0; k < n; ++k) {
            const int i = mods[k];
            cfoption* o = i + icewm_preferences;
            if (updateOption(o, text))
                continue;
            if (insertOption(o, text))
                continue;
            csmart c(retrieveComment(o));
            char buf[99];
            snprintf(buf, sizeof buf, "%s=%d # 0/1\n",
                     o->name, *o->v.bool_value);
            size_t clen = c ? strlen(c) : 0;
            size_t blen = strlen(buf);
            size_t size = 1 + tlen + clen + blen;
            char* u = new char[size];
            memcpy(u, text, tlen);
            memcpy(u + tlen, c, clen);
            memcpy(u + tlen + clen, buf, blen + 1);
            text = u;
            tlen += clen + blen;
        }
        writePrefs(path, text, tlen);
    }

    static bool updateOption(cfoption* o, char* text) {
        char buf[99];
        snprintf(buf, sizeof buf, "^[ \t]*%s[ \t]*=[ \t]*[!-~]", o->name);
        regex_t pat = {};
        int c = regcomp(&pat, buf, REG_EXTENDED | REG_NEWLINE);
        if (c) {
            char err[99] = "";
            regerror(c, &pat, err, sizeof err);
            warn("regcomp save mod %s: %s", buf, err);
            return true;
        }
        char* s = text;
        regmatch_t m = {};
        while (0 == regexec(&pat, s, 1, &m, 0)) {
            s += m.rm_eo;
            s[-1] = '0' + *o->v.bool_value;
            s = replaceComment(s, "# 0/1");
        }
        regfree(&pat);
        return s > text;
    }

    static char* replaceComment(char* start, const char* comment) {
        char* s = start;

        if (*s && *s != '\n' && *s != '#' && *comment != ' ')
            *s++ = ' ';
        for (const char* c = comment; *s && *s != '\n' && *c; ++c)
            *s++ = *c;
        while (*s && *s != '\n')
            *s++ = ' ';

        return s;
    }

    static bool insertOption(cfoption* o, char* text) {
        char buf[99];
        snprintf(buf, sizeof buf, "^#+[ \t]*%s[ \t]*=[ \t]*[!-~]", o->name);
        regex_t pat = {};
        int c = regcomp(&pat, buf, REG_EXTENDED | REG_NEWLINE);
        if (c) {
            char err[99] = "";
            regerror(c, &pat, err, sizeof err);
            warn("regcomp save mod %s: %s", buf, err);
            return true;
        }
        regmatch_t m = {};
        c = regexec(&pat, text, 1, &m, 0);
        regfree(&pat);
        if (c == 0) {
            char* s = text + m.rm_so;
            snprintf(buf, sizeof buf, "%s=%d", o->name, *o->v.bool_value);
            size_t blen = strlen(buf);
            memcpy(s, buf, blen);
            s += blen;
            s = replaceComment(s, "# 0/1");
        }
        return c == 0;
    }

    static char* retrieveComment(cfoption* o) {
        static const char path[] = LIBDIR "/preferences";
        char* res = 0;
        csmart text(load_text_file(path));
        if (text) {
            char buf[99];
            snprintf(buf, sizeof buf,
                     "(\n(#  .*\n)*)#+[ \t]*%s[ \t]*=[ \t]*[!-~]",
                     o->name);
            regex_t pat = {};
            int c = regcomp(&pat, buf, REG_EXTENDED | REG_NEWLINE);
            if (c == 0) {
                regmatch_t m[2] = {};
                c = regexec(&pat, text, 2, m, 0);
                regfree(&pat);
                if (c == 0) {
                    int len = m[1].rm_eo - m[1].rm_so;
                    res = newstr(text + m[1].rm_so, len);
                }
            } else {
                char err[99] = "";
                regerror(c, &pat, err, sizeof err);
                warn("regcomp save mod %s: %s", buf, err);
            }
        }
        return res ? res : newstr("\n");
    }

    static void writePrefs(const upath dest, const char* text, size_t tlen) {
        const upath temp(dest.path() + ".tmp");
        int fd = temp.open(O_CREAT | O_WRONLY | O_TRUNC, 0600);
        if (fd == -1) {
            fail("Failed to open %s for writing", temp.string().c_str());
        } else {
            ssize_t w = write(fd, text, tlen);
            if (size_t(w) != tlen)
                fail("Failed to write to %s", temp.string().c_str());
            close(fd);
            if (size_t(w) == tlen) {
                if (temp.renameAs(dest))
                    fail("Failed to rename %s to %s",
                         temp.string().c_str(),
                         dest.string().c_str());
                else
                    mods.clear();
            }
        }
    }
};

YArray<int> PrefsMenu::mods;

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
        args.append(0);

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

    if (itemCount())
        addSeparator();

/// TODO #warning "make this into a menuprog (ala gnome.cc), and use mime"
    if (openCommand && openCommand[0]) {
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
            else if (sub) {
                delete sub;
            }
        }
        addSeparator();
    }

    int const oldItemCount = itemCount();

    if (showPrograms) {
        ObjectMenu *programs = new MenuFileMenu(app, smActionListener, wmActionListener, "programs", 0);
        ///    if (programs->itemCount() > 0)
        addSubmenu(_("Programs"), 0, programs, "programs");
    }

    if (showRun) {
        if (runDlgCommand && runDlgCommand[0])
            addItem(_("_Run..."), -2, "", actionRun, "run");
    }

    if (itemCount() != oldItemCount) addSeparator();
    if (showWindowList)
        addItem(_("_Windows"), -2, actionWindowList, windowListMenu, "windows");

    if ( (!showTaskBar && showAbout) ||
        showHelp ||
        showSettingsMenu
    )
    if (showWindowList)
        addSeparator();

    if (!showTaskBar) {
        if (showAbout)
            addItem(_("_About"), -2, actionAbout, 0, "about");
    }

    if (showHelp) {
        HelpMenu *helpMenu =
            new HelpMenu(app, smActionListener, wmActionListener);
            addSubmenu(_("_Help"), -2, helpMenu, "help");
    }

    if (showSettingsMenu) {
        // When we have only 2 entries (focus + themes) then
        // it doesn't make sense to create a whole new YMenu.
        // Therefore we will reuse 'this' as value for settings.
        YMenu *settings = this; // new YMenu();

        if (showFocusModeMenu) {
            FocusMenu *focus = new FocusMenu();
            settings->addSubmenu(_("_Focus"), -2, focus, "focus");
        }

        PrefsMenu *prefs = new PrefsMenu();
        settings->addSubmenu(_("_Preferences"), -2, prefs, "pref");

        if (showThemesMenu) {
            YMenu *themes = new ThemesMenu(app, smActionListener, wmActionListener);
            if (themes)
                settings->addSubmenu(_("_Themes"), -2, themes, "themes");
        }

        // Only add a menu if we created one:
        if (this != settings)
            addSubmenu(_("Se_ttings"), -2, settings, "settings");
    }

    if (logoutMenu) {
        addSeparator();
        if (showLogoutSubMenu)
            addItem(_("_Logout..."), -2, actionLogout, logoutMenu, "logout");
        else
            addItem(_("_Logout..."), -2, null, actionLogout, "logout");
    }
}

// vim: set sw=4 ts=4 et:
