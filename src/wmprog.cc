/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"

#ifndef NO_CONFIGURE_MENUS

#define NEED_TIME_H

#include "objmenu.h"
#include "wmprog.h"
#include "wmoption.h"
#include "wmaction.h"
#include "wmconfig.h"
#include "ybutton.h"
#include "objbutton.h"
#include "objbar.h"
#include "prefs.h"
#include "wmapp.h"
#include "sysdep.h"
#include "base.h"
#include "wmmgr.h"
#include "ymenuitem.h"
#include "themes.h"
#include "browse.h"
#include "wmtaskbar.h"
#include "ypaths.h"
#include "intl.h"
#include "appnames.h"
#include "ascii.h"
#include "argument.h"

DObjectMenuItem::DObjectMenuItem(DObject *object):
    YMenuItem(object->getName(), -3, null, this, 0)
{
    fObject = object;
#ifndef LITE
    if (object->getIcon() != null)
        setIcon(object->getIcon());
#endif
}

DObjectMenuItem::~DObjectMenuItem() {
    delete fObject;
}

void DObjectMenuItem::actionPerformed(YActionListener * /*listener*/, YAction * /*action*/, unsigned int /*modifiers*/) {
#ifdef CONFIG_GUIEVENTS
    wmapp->signalGuiEvent(geLaunchApp);
#endif
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

#ifndef LITE
void ObjectMenu::addObject(DObject *fObject, const char *icons) {
    add(new DObjectMenuItem(fObject), icons);
}
#endif

void ObjectMenu::addSeparator() {
    YMenu::addSeparator();
}

#ifdef LITE
void ObjectMenu::addContainer(const ustring &name, ref<YIcon> /*icon*/, ObjectContainer *container) {
#else
void ObjectMenu::addContainer(const ustring &name, ref<YIcon> icon, ObjectContainer *container) {
#endif
    if (container) {
#ifndef LITE
        YMenuItem *item =
#endif
            addSubmenu(name, -3, (ObjectMenu *)container);

#ifndef LITE
        if (item && icon != null)
            item->setIcon(icon);
#endif
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

#ifndef NO_CONFIGURE_MENUS
YObjectArray<KProgram> keyProgs;

KProgram::KProgram(const char *key, DProgram *prog)
    : fKey(NoSymbol), fMod(0), fProg(prog)
{
    YConfig::parseKey(key, &fKey, &fMod);
    keyProgs.append(this);
}
#endif

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
#ifndef LITE
                if (icons[0] != '-') icon = YIcon::getIcon(icons);
#endif
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
#ifndef LITE
                if (icons[0] != '-')
                    icon = YIcon::getIcon(icons);
#endif

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
#ifndef LITE
                if (icons[0] != '-')
                    icon = YIcon::getIcon(icons);
#endif
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
#ifndef LITE
                if (icons[0] != '-')
                    icon = YIcon::getIcon(icons);
#endif
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
#ifndef LITE
                if (icons[0] != '-')
                    icon = YIcon::getIcon(icons);
#endif
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
            if (!(strcmp(word, "key") &&
                  strcmp(word, "runonce")))
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

                if (prog) new KProgram(key, prog);
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
        YAction *action;
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

HelpMenu::HelpMenu(
    IApp *app,
    YSMListener *smActionListener,
    YActionListener *wmActionListener)
    : ObjectMenu(wmActionListener)
{
    struct HelpMenuItem {
        const char *name;
        const char *menu;
    } help[] = {
        { ICEHELPIDX, _("_Manual") },
        { "icewm.1.html", _("_Icewm(1)") },
        { "icewmbg.1.html", _("Icewm_Bg(1)") },
        { "icesound.1.html", _("Ice_Sound(1)") },
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
            "browser.IceHelp",
            ICEHELPEXE,
            args);

        if (prog)
#ifdef LITE
            addObject(prog);
#else
            addObject(prog, "help");
#endif
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
#ifndef LITE
        ref<YIcon> folder = YIcon::getIcon("folder");
#endif
        for (unsigned int i = 0; i < ACOUNT(path); i++) {
            upath& p = path[i];

            sub = new BrowseMenu(app, smActionListener, wmActionListener, p);
            DFile *file = new DFile(app, p, null, p);
            YMenuItem *item = add(new DObjectMenuItem(file));
            if (item && sub) {
                item->setSubmenu(sub);
#ifndef LITE
                if (folder != null)
                    item->setIcon(folder);
#endif
            }
            else if (sub) {
                delete sub;
            }
        }
        addSeparator();
    }

#ifdef CONFIG_WINLIST
    int const oldItemCount = itemCount();
#endif

    if (showPrograms) {
        ObjectMenu *programs = new MenuFileMenu(app, smActionListener, wmActionListener, "programs", 0);
        ///    if (programs->itemCount() > 0)
#ifdef LITE
        addSubmenu(_("Programs"), 0, programs);
#else
        addSubmenu(_("Programs"), 0, programs, "programs");
#endif
    }

    if (showRun) {
        if (runDlgCommand && runDlgCommand[0])
#ifdef LITE
            addItem(_("_Run..."), -2, "", actionRun);
#else
            addItem(_("_Run..."), -2, "", actionRun, "run");
#endif
    }

#ifdef CONFIG_WINLIST
#ifdef CONFIG_WINMENU
    if (itemCount() != oldItemCount) addSeparator();
    if (showWindowList)
#ifdef LITE
        addItem(_("_Windows"), -2, actionWindowList, windowListMenu);
#else
        addItem(_("_Windows"), -2, actionWindowList, windowListMenu, "windows");
#endif
#endif
#endif

    if (
#ifndef LITE
#ifdef CONFIG_TASKBAR
        (!showTaskBar && showAbout) ||
#endif
        showHelp ||
#endif
        showSettingsMenu
    )
#ifdef CONFIG_WINLIST
#ifdef CONFIG_WINMENU
        if (showWindowList)
#endif
#endif
        addSeparator();

#ifndef LITE
#ifdef CONFIG_TASKBAR
    if (!showTaskBar) {
        if (showAbout)
#ifdef LITE
            addItem(_("_About"), -2, actionAbout, 0);
#else
            addItem(_("_About"), -2, actionAbout, 0, "about");
#endif
    }
#endif

    if (showHelp) {
        HelpMenu *helpMenu =
            new HelpMenu(app, smActionListener, wmActionListener);
#ifdef LITE
            addSubmenu(_("_Help"), -2, helpMenu);
#else
            addSubmenu(_("_Help"), -2, helpMenu, "help");
#endif
    }
#endif

    if (showSettingsMenu) {
        // When we have only 2 entries (focus + themes) then
        // it doesn't make sense to create a whole new YMenu.
        // Therefore we will reuse 'this' as value for settings.
        YMenu *settings = this; // new YMenu();

        if (showFocusModeMenu) {
            FocusMenu *focus = new FocusMenu();
#ifdef LITE
            settings->addSubmenu(_("_Focus"), -2, focus);
#else
            settings->addSubmenu(_("_Focus"), -2, focus, "focus");
#endif
        }


        if (showThemesMenu) {
            YMenu *themes = new ThemesMenu(app, smActionListener, wmActionListener);
            if (themes)
#ifdef LITE
                settings->addSubmenu(_("_Themes"), -2, themes);
#else
                settings->addSubmenu(_("_Themes"), -2, themes, "themes");
#endif
        }

        // Only add a menu if we created one:
        if (this != settings)
#ifdef LITE
        addSubmenu(_("Se_ttings"), -2, settings);
#else
        addSubmenu(_("Se_ttings"), -2, settings, "settings");
#endif
    }

    if (logoutMenu) {
        addSeparator();
        if (showLogoutSubMenu)
#ifdef LITE
            addItem(_("_Logout..."), -2, actionLogout, logoutMenu);
#else
            addItem(_("_Logout..."), -2, actionLogout, logoutMenu, "logout");
#endif
        else
#ifdef LITE
            addItem(_("_Logout..."), -2, null, actionLogout);
#else
            addItem(_("_Logout..."), -2, null, actionLogout, "logout");
#endif
    }
}
#endif
