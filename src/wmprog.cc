/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#include "config.h"

#ifndef NO_CONFIGURE_MENUS

#include "ykey.h"
#include "objmenu.h"
#include "wmprog.h"
#include "wmoption.h"
#include "wmaction.h"
#include "ybutton.h"
#include "objbutton.h"
#include "objbar.h"
#include "prefs.h"
#include "wmapp.h"
#include "sysdep.h"
#include "base.h"
#include "wmmgr.h"
#include "ymenuitem.h"
#include "gnomeapps.h"
#include "themes.h"
#include "browse.h"
#ifdef GNOME
#include <gnome.h>
#endif
#include "wmtaskbar.h"

extern bool parseKey(const char *arg, KeySym *key, unsigned int *mod);

#include "intl.h"

DObjectMenuItem::DObjectMenuItem(DObject *object):
    YMenuItem(object->getName(), 0, 0, this, 0)
{
    fObject = object;
#ifndef LITE
    if (object->getIcon())
        setPixmap(object->getIcon()->small());
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

DFile::DFile(const char *name, YIcon *icon, const char *path): DObject(name, icon) {
    fPath = strdup(path);
}

DFile::~DFile() {
    delete fPath;
}

void DFile::open() {
    const char *args[3];
    args[0] = openCommand;
    args[1] = fPath;
    args[2] = 0;
    app->runProgram(openCommand, args);
}

ObjectMenu::ObjectMenu(YWindow *parent): YMenu(parent) {
    setActionListener(wmapp);
}

ObjectMenu::~ObjectMenu() {
}

void ObjectMenu::addObject(DObject *fObject) {
    add(new DObjectMenuItem(fObject));
}

void ObjectMenu::addSeparator() {
    YMenu::addSeparator();
}

void ObjectMenu::addContainer(char *name, YIcon *icon, ObjectContainer *container) {
    if (container) {
        YMenuItem *item = addSubmenu(name, 0, (ObjectMenu *)container);
#ifndef LITE
        if (item && icon)
            item->setPixmap(icon->small());
#endif
    }
}

ObjectButton::ObjectButton(YWindow *parent, DObject *object): YButton(parent, 0)
{
    fObject = object;
}

void ObjectButton::actionPerformed(YAction * /*action*/, unsigned int /*modifiers*/) {
#ifdef CONFIG_GUIEVENTS
    wmapp->signalGuiEvent(geLaunchApp);
#endif
    fObject->open();
}

ObjectMenu *rootMenu = 0;

#define ACOUNT(x) (sizeof(x)/sizeof(x[0]))

DObject::DObject(const char *name, YIcon *icon) {
    fName = newstr(name);
    fIcon = icon;
}

DObject::~DObject() {
    delete fName; fName = 0;
    //delete fIcon;
    fIcon = 0; // !!! icons cached forever
}

void DObject::open() {
}

DProgram::DProgram(const char *name, YIcon *icon, bool restart, const char *exe, char **args):
    DObject(name, icon)
{
    fCmd = newstr(exe);
    fArgs = args;
    fRestart = restart;
}

DProgram::~DProgram() {
    delete fCmd; fCmd = 0;
    if (fArgs) {
        char **p = fArgs;
        while (*p) {
            delete *p;
            p++;
        }
    }
    delete [] fArgs; fArgs = 0;
}

void DProgram::open() {
    if (fRestart) {
        wmapp->restartClient(fCmd, fArgs);
    } else
        app->runProgram(fCmd, fArgs);
}

DProgram *DProgram::newProgram(const char *name, YIcon *icon, bool restart, const char *exe, char **args) {
    char *fullname = 0;

    if (exe && exe[0] && findPath(getenv("PATH"), X_OK, exe, &fullname) == 0) { // updates command with full path
        char **p = args;
        while (p && *p) {
            delete *p;
            p++;
        }
        FREE(args);

	MSG(("Program %s (%s) not found.", name, exe));
        return 0;
    }
    DProgram *p = new DProgram(name, icon, restart, fullname, args);
    delete fullname;
    return p;
}

char *getWord(char *word, int maxlen, char *p) {
    while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'))
        p++;
    while (*p && isalnum(*p) && maxlen > 1) {
        *word++ = *p++;
        maxlen--;
    }
    *word++ = 0;
    return p;
}

char *getCommandArgs(char *p, char *command, int command_len, char **&args, int &argCount) {

    p = getArgument(command, command_len, p, false);
    if (p == 0) {
        msg(_("Missing command argument"));
        return p;
    }

    while (*p) {
        char argx[256];

        while (*p && (*p == ' ' || *p == '\t'))
            p++;

        if (*p == '\n')
            break;

        p = getArgument(argx, sizeof(argx), p, false);
        if (p == 0) {
            msg(_("Bad argument %d"), argCount + 1);
            return p;
        }

        if (args == 0) {
            args = (char **)REALLOC((void *)args, ((argCount) + 2) * sizeof(char *));
            assert(args != NULL);

            args[argCount] = newstr(command);
            assert(args[argCount] != NULL);
            args[argCount + 1] = NULL;

            argCount++;
        }

        args = (char **)REALLOC((void *)args, ((argCount) + 2) * sizeof(char *));
        assert(args != NULL);

        args[argCount] = newstr(argx);
        assert(args[argCount] != NULL);
        args[argCount + 1] = NULL;

        argCount++;
    }
    return p;
}

KProgram *keyProgs = 0;

KProgram::KProgram(const char *key, DProgram *prog) {
    parseKey(key, &fKey, &fMod);
    fProg = prog;
    fNext = keyProgs;
    keyProgs = this;
}

char *parseMenus(char *data, ObjectContainer *container) {
    char *p = data;
    char word[32];

    while (p && *p) {
        while (*p == ' ' || *p == '\t' || *p == '\n')
            p++;
        if (*p == '#') {
            while (*p && *p != '\n')
                p++;
            continue;
        }
        p = getWord(word, sizeof(word), p);

        if (container && strcmp(word, "separator") == 0) {
            if (container)
                container->addSeparator();
        } else if (container && strcmp(word, "prog") == 0 || 
				strcmp(word, "restart") == 0) {
            char name[64];
            char icons[128];
            bool restart = (strcmp(word, "restart") == 0) ? true : false;

            p = getArgument(name, sizeof(name), p, false);
            if (p == 0)
                return p;

            p = getArgument(icons, sizeof(icons), p, false);
            if (p == 0)
                return p;

            char command[256];
            char **args = 0;
            int argCount = 0;

            p = getCommandArgs(p, command, sizeof(command), args, argCount);
            if (p == 0) {
                msg(_("Error at prog %s"), name);
                return p;
            }
            if (!p)
                msg(_("Missing 2nd argument for prog %s"), name);
            else {
                YIcon *icon = 0;
#ifndef LITE
                if (icons[0] != '-')
                    icon = getIcon(icons);
#endif
                DProgram *prog = DProgram::newProgram(name, icon, restart, command, args);
                if (prog && container)
                    container->addObject(prog);
            }
        } else if (!container && strcmp(word, "key") == 0) {
            char key[64];

            p = getArgument(key, sizeof(key), p, false);
            if (p == 0)
                return p;

            char command[256];
            char **args = 0;
            int argCount = 0;

            p = getCommandArgs(p, command, sizeof(command), args, argCount);
            if (p == 0) {
                msg(_("Error at key %s"), key);
                return p;
            }

            DProgram *prog = DProgram::newProgram(key, 0, false, command, args);
            if (prog) {
                new KProgram(key, prog);
            }
        } else if (container && strcmp(word, "menu") == 0) {
            char name[64];
            char icons[128];

            p = getArgument(name, sizeof(name), p, false);

            p = getArgument(icons, sizeof(icons), p, false);
            if (p == 0)
                return p;

            p = getWord(word, sizeof(word), p);
            if (*p != '{')
                return 0;
            p++;

            YIcon *icon = 0;

#ifndef LITE
            if (icons[0] != '-')
                icon = getIcon(icons);
#endif

            ObjectMenu *sub = new ObjectMenu();
            if (sub) {
                p = parseMenus(p, sub);
                if (sub->itemCount() == 0)
                    delete sub;
                else if (container)
                    container->addContainer(name, icon, sub);
            } else
                return p;
        } else if (*p == '}') {
            p++;
            return p;
        } else {
            return 0;
        }
    }
    return p;
}

void loadMenus(const char *menuFile, ObjectContainer *container) {
    if (menuFile == 0)
        return ;

    int fd = open(menuFile, O_RDONLY | O_TEXT);

    if (fd == -1)
        return ;

    struct stat sb;

    if (fstat(fd, &sb) == -1)
        return ;

    int len = sb.st_size;

    char *buf = new char[len + 1];
    if (buf == 0)
        return ;

    if (read(fd, buf, len) != len)
        return;

    buf[sb.st_size] = 0;
    close(fd);

    parseMenus(buf, container);

    delete buf;
}

MenuFileMenu::MenuFileMenu(const char *name, YWindow *parent): ObjectMenu(parent) {
    fName = newstr(name);
    fPath = 0;
    fModTime = 0;
    updatePopup();
    refresh();
}

MenuFileMenu::~MenuFileMenu() {
    delete fPath; fPath = 0;
    delete fName; fName = 0;
}

void MenuFileMenu::updatePopup() {
    if (!autoReloadMenus && fPath != 0)
        return;

    struct stat sb;
    char *np = app->findConfigFile(fName);
    bool rel = false;


    if (fPath == 0) {
        fPath = np;
        rel = true;
    } else {
        if (!np || strcmp(np, fPath) != 0) {
            delete [] fPath;
            fPath = np;
            rel = true;
        } else
            delete np;
    }

    if (!autoReloadMenus)
        return;

    if (fPath == 0) {
        refresh();
    } else {
        if (stat(fPath, &sb) != 0) {
            delete [] fPath;
            fPath = 0;
            refresh();
        } else if (sb.st_mtime != fModTime || rel) {
            fModTime = sb.st_mtime;
            refresh();
        }
    }
}

void MenuFileMenu::refresh() {
    removeAll();
    if (fPath)
        loadMenus(fPath, this);
}

StartMenu::StartMenu(const char *name, YWindow *parent): MenuFileMenu(name, parent) {
    updatePopup();
    refresh();
}

bool StartMenu::handleKey(const XKeyEvent &key) {
    // If meta key, close the popup
    if (key.type == KeyPress) {
        KeySym k = XKeycodeToKeysym(app->display(), key.keycode, 0);
        int m = KEY_MODMASK(key.state);
        
        if (((k == XK_Meta_L) || (k == XK_Alt_L)) && m == 0) {
            cancelPopup();
            return true;
        }
    }
    return MenuFileMenu::handleKey(key);
}

void StartMenu::refresh() {
    MenuFileMenu::refresh();

#ifndef NO_CONFIGURE_MENUS
    addSeparator();
#ifdef GNOME
    {
        YPixmap *gnomeicon = 0;
#ifdef IMLIB
        char *gnome_logo =
	    gnome_pixmap_file("gnome-logo-icon-transparent.png");
        if (gnome_logo)
            gnomeicon = new YPixmap(gnome_logo, ICON_SMALL, ICON_SMALL);
#endif

        char * gnomeApps = gnome_datadir_file("gnome/apps/");

        if (access(gnomeApps, X_OK | R_OK) == 0)  {
            YMenu *sub = new GnomeMenu(0, gnomeApps);
            YMenuItem *item = addSubmenu(_("Gnome"), 0, sub);
            if (gnomeicon && item)
                item->setPixmap(gnomeicon);
        }

        const char *homeDir = getenv("HOME");
        char *gnomeUserApps;

        gnomeUserApps = strJoin(homeDir, "/.gnome/apps", NULL);

        if (access(gnomeUserApps, X_OK | R_OK) == 0) {
            YMenu *sub2 = new GnomeMenu(0, gnomeUserApps);
            YMenuItem *item2 = addSubmenu(_("Gnome User Apps"), 0, sub2);
            if (gnomeicon && item2)
                item2->setPixmap(gnomeicon);
        }
    }
#endif
    ObjectMenu *programs = new MenuFileMenu("programs", 0);

    if (programs->itemCount() > 0)
        addSubmenu(_("Programs"), 0, programs);
#else
#endif

    if (openCommand && openCommand[0]) {
        const char *path[2];
        YMenu *sub;
#ifndef LITE
        YIcon *folder = getIcon("folder");
#endif
        path[0] = "/";
        path[1] = getenv("HOME");

        for (unsigned int i = 0; i < sizeof(path)/sizeof(path[0]); i++) {
            const char *p = path[i];

            sub = new BrowseMenu(p);
            DFile *file = new DFile(p, 0, p);
            YMenuItem *item = add(new DObjectMenuItem(file));
            if (item && sub) {
                item->setSubmenu(sub);
#ifndef LITE
                if (folder)
                    item->setPixmap(folder->small());
#endif
            }
        }
    }
#ifdef CONFIG_WINLIST
    addSeparator();
    addItem(_("Windows"), 0, actionWindowList, windowListMenu);
#endif

    if (runDlgCommand && runDlgCommand[0])
        addItem(_("Run..."), 0, "", actionRun);
    addSeparator();

    if (showThemesMenu) {
        YMenu *themes = new ThemesMenu();
        if (themes->itemCount() > 1)
            addSubmenu(_("Themes"), 0, themes);
    }
    addItem(_("Logout..."), 0, actionLogout, logoutMenu);
}
#endif
