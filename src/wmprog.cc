/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
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
#ifdef CONFIG_GNOME_MENUS
#include <gnome.h>
#endif
#include "wmtaskbar.h"

extern bool parseKey(const char *arg, KeySym *key, unsigned int *mod);

#include "intl.h"

DObjectMenuItem::DObjectMenuItem(DObject *object):
    YMenuItem(object->getName(), -2, 0, this, 0)
{
    fObject = object;
#ifndef LITE
    if (object->getIcon())
        setIcon(object->getIcon()->small());
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

#ifdef LITE
void ObjectMenu::addContainer(char *name, YIcon */*icon*/, ObjectContainer *container) {
#else
void ObjectMenu::addContainer(char *name, YIcon *icon, ObjectContainer *container) {
#endif
    if (container) {
#ifndef LITE
        YMenuItem *item = 
#endif
		addSubmenu(name, 0, (ObjectMenu *)container);
#ifndef LITE
        if (item && icon)
            item->setIcon(icon->small());
#endif
    }
}

DObject::DObject(const char *name, YIcon *icon) {
    fName = newstr(name);
    fIcon = icon;
}

DObject::~DObject() {
    delete[] fName; fName = 0;
    //delete fIcon;
    fIcon = 0; // !!! icons cached forever
}

void DObject::open() {
}

DProgram::DProgram(const char *name, YIcon *icon, const bool restart,
		   const char *wmclass, const char *exe, const char **args):
    DObject(name, icon), fRestart(restart), fRes(newstr(wmclass)),
    fCmd(newstr(exe)), fArgs(args) {
}

DProgram::~DProgram() {
    if (fArgs)
	for (const char **p = fArgs; p && *p; ++p)
	    delete[] *p;

    delete[] fArgs;
    delete[] fCmd;
    delete[] fRes;
}

void DProgram::open() {
    if (fRestart)
        wmapp->restartClient(fCmd, fArgs);
    else if (fRes)
        wmapp->runOnce(fRes, fCmd, fArgs);
    else
        app->runProgram(fCmd, fArgs);
}

DProgram *DProgram::newProgram(const char *name, YIcon *icon,
			       const bool restart, const char *wmclass,
			       const char *exe, const char **args) {
    char *fullname(NULL);

    if (exe && exe[0] &&  // updates command with full path
        NULL == (fullname = findPath(getenv("PATH"), X_OK, exe))) {
        for (const char **p = args; p && *p; ++p) delete[] *p;
        delete[] args;

        MSG(("Program %s (%s) not found.", name, exe));
        return 0;
    }

    DProgram *p = new DProgram(name, icon, restart, wmclass, fullname, args);

    delete[] fullname;
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

char *getCommandArgs(char *p, char *command, int command_len, const char **&args, int &argCount) {

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
            args = (const char **)
		REALLOC((void *)args, ((argCount) + 2) * sizeof(char *));
            assert(args != NULL);

            args[argCount] = newstr(command);
            assert(args[argCount] != NULL);
            args[argCount + 1] = NULL;

            argCount++;
        }

        args = (const char **)
	    REALLOC((void *)args, ((argCount) + 2) * sizeof(char *));
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

        if (container) {
	    if (strcmp(word, "separator") == 0)
	        container->addSeparator();
	    else if (!(strcmp(word, "prog") &&
		       strcmp(word, "restart") &&
		       strcmp(word, "runonce"))) {
		char name[64];

		p = getArgument(name, sizeof(name), p, false);
		if (p == 0) return p;

		char icons[128];

		p = getArgument(icons, sizeof(icons), p, false);
		if (p == 0) return p;

		char wmclass[256];

		if (word[1] == 'u') {
		    p = getArgument(wmclass, sizeof(wmclass), p, false);
		    if (p == 0) return p;
		}

		char command[256];
		const char **args = 0;
		int argCount = 0;

		p = getCommandArgs(p, command, sizeof(command), args, argCount);
		if (p == 0) {
		    msg(_("Error at prog %s"), name); return p;
		}

		YIcon *icon = 0;
#ifndef LITE
		if (icons[0] != '-') icon = getIcon(icons);
#endif
		DProgram * prog(DProgram::newProgram(name, icon,
		     word[1] == 'e', word[1] == 'u' ? wmclass : 0, 
		     command, args));

		if (prog) container->addObject(prog);
	    } else if (strcmp(word, "menu") == 0) {
		char name[64];

		p = getArgument(name, sizeof(name), p, false);
		if (p == 0) return p;

		char icons[128];

		p = getArgument(icons, sizeof(icons), p, false);
		if (p == 0) return p;

		p = getWord(word, sizeof(word), p);
		if (*p != '{') return 0;
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
		    else
			container->addContainer(name, icon, sub);

		} else
		    return p;
	    } else if (*p == '}')
		return ++p;
	    else
		return 0;
        } else {
	    if (!(strcmp(word, "key") &&
	          strcmp(word, "runonce"))) {
		char key[64];

		p = getArgument(key, sizeof(key), p, false);
		if (p == 0) return p;

		char wmclass[256];

		if (*word == 'r') {
		    p = getArgument(wmclass, sizeof(wmclass), p, false);
		    if (p == 0) return p;
		}

		char command[256];
		const char **args = 0;
		int argCount = 0;

		p = getCommandArgs(p, command, sizeof(command), args, argCount);
		if (p == 0) {
		    msg(_("Error at key %s"), key);
		    return p;
		}

		DProgram *prog(DProgram::newProgram(key, 0,
		    false, *word == 'r' ? wmclass : 0, command, args));

		if (prog) new KProgram(key, prog);
            } else
		return 0;
        }
    }
    return p;
}

void loadMenus(const char *menuFile, ObjectContainer *container) {
    if (menuFile == 0)
        return ;


    int fd(open(menuFile, O_RDONLY | O_TEXT));
    if (fd == -1) return ;

    struct stat sb;
    if (fstat(fd, &sb) == -1) { close(fd); return; }

    char *buf = new char[sb.st_size + 1];
    if (buf == 0) { close(fd); return; }

    read(fd, buf, sb.st_size);
    buf[sb.st_size] = '\0';
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
        } else if (sb.st_mtime > fModTime || rel) {
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
    fHasGnomeAppsMenu = 
    fHasGnomeUserMenu = 
    fHasKDEMenu = false;

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

void StartMenu::updatePopup() {
    MenuFileMenu::updatePopup();

#ifdef CONFIG_GNOME_MENUS
    if (autoReloadMenus) {
        char *gnomeAppsMenu = gnome_datadir_file("gnome/apps/");
        char *gnomeUserMenu = gnome_util_home_file("apps/");
        const char *kdeMenu = strJoin(kdeDataDir, "/applnk", 0);

        struct stat sb;
        bool dirty = false;

// !!! we need a better architecture with inlined submenus...

        if (gnomeAppsMenuAtToplevel) {
            if (stat(gnomeAppsMenu, &sb) != 0) {
                dirty = fHasGnomeAppsMenu;
                fHasGnomeAppsMenu = false;
            } else {
                if (sb.st_mtime > fModTime || !fHasGnomeAppsMenu)
                    fModTime = sb.st_mtime;
                fHasGnomeAppsMenu = dirty = true;
            }
        }

        if (gnomeUserMenuAtToplevel) {
            if (stat(gnomeUserMenu, &sb) != 0) {
                dirty = fHasGnomeUserMenu;
                fHasGnomeUserMenu = false;
            } else {
                if (sb.st_mtime > fModTime || !fHasGnomeUserMenu)
                    fModTime = sb.st_mtime;
                fHasGnomeUserMenu = dirty = true;
            }
        }

        if (kdeMenuAtToplevel) {
            if (stat(kdeMenu, &sb) != 0) {
                dirty = fHasKDEMenu;
                fHasKDEMenu = false;
            } else {
                if (sb.st_mtime != fModTime || !fHasKDEMenu)
                    fModTime = sb.st_mtime;
                fHasKDEMenu = dirty = true;
            }
        }

        g_free(gnomeAppsMenu);
        g_free(gnomeUserMenu);
        delete kdeMenu;

        if (dirty) refresh();
    }
#endif    
}

void StartMenu::refresh() {
    MenuFileMenu::refresh();

#ifndef NO_CONFIGURE_MENUS
    if (itemCount()) addSeparator();
    int const oldItemCount(itemCount());
#ifdef CONFIG_GNOME_MENUS
    {
        YIcon *gnomeIcon = 0;
        YIcon *kdeIcon = 0;

        char *gnomeAppsMenu = gnome_datadir_file("gnome/apps/");
        char *gnomeUserMenu = gnome_util_home_file("apps/");
        const char *kdeMenu = strJoin(kdeDataDir, "/applnk", 0);

        fHasGnomeAppsMenu = showGnomeAppsMenu &&
			    !access(gnomeAppsMenu, X_OK | R_OK);
        fHasGnomeUserMenu = showGnomeUserMenu &&
			    !access(gnomeUserMenu, X_OK | R_OK);
        fHasKDEMenu       = showKDEMenu &&
			    !access(kdeMenu, X_OK | R_OK);

	if (fHasGnomeAppsMenu || fHasGnomeUserMenu)
	    gnomeIcon = getIcon("gnome");

	if (fHasGnomeAppsMenu)
	    kdeIcon = getIcon("kde");

        if (fHasGnomeAppsMenu)
            if (gnomeAppsMenuAtToplevel)
                GnomeMenu::createToplevel(this, gnomeAppsMenu);
            else
                GnomeMenu::createSubmenu(this, gnomeAppsMenu,
		    _("Gnome"), gnomeIcon ? gnomeIcon->small () : 0);

        if (fHasGnomeAppsMenu && fHasGnomeUserMenu &&
            (gnomeAppsMenuAtToplevel || gnomeUserMenuAtToplevel))
            addSeparator();

        if (fHasGnomeUserMenu)
            if (gnomeUserMenuAtToplevel)
                GnomeMenu::createToplevel(this, gnomeUserMenu);
            else
                GnomeMenu::createSubmenu(this, gnomeUserMenu,
		    _("Gnome User Apps"), gnomeIcon ? gnomeIcon->small () : 0);

        if (fHasGnomeUserMenu && fHasKDEMenu &&
            (gnomeUserMenuAtToplevel || kdeMenuAtToplevel))
            addSeparator();

        if (fHasKDEMenu)
            if (kdeMenuAtToplevel)
                GnomeMenu::createToplevel(this, kdeMenu);
            else
                GnomeMenu::createSubmenu(this, kdeMenu,
		    _("KDE"), kdeIcon ? kdeIcon->small () : 0);

        g_free(gnomeAppsMenu);
        g_free(gnomeUserMenu);
        delete kdeMenu;
	
//	delete gnomeIcon;
//	delete kdeIcon;
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
                    item->setIcon(folder->small());
#endif
            }
        }
    }
#ifdef CONFIG_WINLIST
    if (itemCount() != oldItemCount) addSeparator();
    addItem(_("_Windows"), -2, actionWindowList, windowListMenu);
#endif

    if (runDlgCommand && runDlgCommand[0])
        addItem(_("_Run..."), -2, "", actionRun);

    addSeparator();

#ifndef LITE
    if (!showTaskBar)
        addItem(_("_About"), -2, actionAbout, 0);

    if (showHelp) {
	const char ** args = new const char*[3];
	args[0] = newstr(ICEHELPEXE);
	args[1] = newstr(ICEHELPIDX);
	args[2] = 0;

	DProgram *help(DProgram::newProgram
	    (_("_Help"), NULL, false, "browser.IceHelp", ICEHELPEXE, args));

	if (help) addObject(help);
    }
#endif

    if (showThemesMenu) {
        YMenu *themes = new ThemesMenu();
        if (themes->itemCount() > 1)
            addSubmenu(_("_Themes"), -2, themes);
    }
    
    if (logoutMenu)
	addItem(_("_Logout..."), -2, actionLogout, logoutMenu);
}
#endif
