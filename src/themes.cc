/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"

#ifndef NO_CONFIGURE_MENUS
#include "themes.h"

#include "ymenu.h"
#include "wmmgr.h"
#include "wmprog.h"
#include "ymenuitem.h"
#include "sysdep.h"
#include "base.h"
#include "prefs.h"
#include <dirent.h>

#include "intl.h"

ThemesMenu::ThemesMenu(YWindow *parent): ObjectMenu(parent) {
    const char *homeDir = getenv("HOME");
    char *path;

    path = strJoin(libDir, "/themes/", NULL);
    findThemes(path, this);
    delete path;

    path = strJoin(configDir, "/themes/", NULL);
    findThemes(path, this);
    delete path;

    path = strJoin(homeDir, "/.icewm/themes/", NULL);
    findThemes(path, this);
    delete path;
}

ThemesMenu::~ThemesMenu() {
}

extern char *configArg;

YMenuItem *ThemesMenu::findTheme(YMenu *container, const char *themeName) {
    for (int i = 0; i < container->itemCount(); i++) {
        const char *n = container->item(i)->name();

        if (n && strcmp(themeName, n) == 0)
            return container->item(i);
    }
    return 0;
}

void ThemesMenu::addTheme(YMenu *container, const char *name, const char *name_theme) {
    char **args = (char **)MALLOC(6 * sizeof(char *));
    if (args) {
        args[0] = newstr(ICEWMEXE);
        args[1] = newstr("-t");
        args[2] = newstr(name_theme);
        args[3] = configArg ? newstr("-c") : 0;
        args[4] = newstr(configArg);
        args[5] = 0;
        if (args[0] && args[1] && args[2]) {
            DProgram *re_theme = DProgram::newProgram(name, 0, true, ICEWMEXE, args);
            if (re_theme) {
                YMenuItem *item = container->add(new DObjectMenuItem(re_theme));
                if (themeName && strcmp(themeName, name_theme) == 0)
                    item->setChecked(true);
            }
        }
    }
}

void ThemesMenu::findThemes(const char *path, YMenu *container) {
    DIR *dir;
    const char *tname = "/default.theme";
    char *npath = 0;
    char *dpath = 0;
    int dplen = strlen(path);
    bool first = true;

    if (dplen == 0 || path[dplen - 1] != '/') {
        npath = strJoin(path, "/", NULL);
        dplen++;
    } else {
        dpath = newstr(path);
    }

    if ((dir = opendir(path)) != NULL) {
        struct dirent *de;

        while ((de = readdir(dir)) != NULL) {
            YMenuItem *im = findTheme(container, de->d_name);
            if (im == 0) {
                npath = strJoin(dpath, de->d_name, tname, NULL);
                if (npath && access(npath, R_OK) == 0) {
		    if (first) {
			first = false;
			if (itemCount())
			    addSeparator();
			addLabel(path);
			addSeparator();
		    }
                    addTheme(container, de->d_name, npath + dplen);
		}
                delete [] npath;
            }
            char *subdir = strJoin(dpath, de->d_name, NULL);
            im = findTheme(container, de->d_name);
            if (im && subdir) {
                findThemeAlternatives(subdir, dplen, im);
            } 
        }
        closedir(dir);
    }
    delete [] dpath;
}

void ThemesMenu::findThemeAlternatives(const char *path, int plen, YMenuItem *item) {
    DIR *dir;

    if ((dir = opendir(path)) != NULL) {
        struct dirent *de;

        while ((de = readdir(dir)) != NULL) {
            const char *p;

            if ((p = strstr(de->d_name, ".theme")) != 0 &&
                strcmp(p, ".theme") == 0 &&
                strcmp(de->d_name, "default.theme") != 0)
            {
                char *npath = strJoin(path, "/", de->d_name, NULL);

                if (npath && access(npath, R_OK) == 0) {
                    YMenu *sub = item->submenu();

                    if (sub == 0) {
                        sub = new YMenu();
                        item->setSubmenu(sub);
                    }
                    if (sub) {
                        YMenuItem *im = findTheme(sub, de->d_name);
                        char *tname = newstr(de->d_name, p - de->d_name);
                        if (im == 0 && tname)
                            addTheme(sub, tname, npath + plen);
                        delete [] tname;
                    }
                }
                delete [] npath;
            }
        }
        closedir(dir);
    }
}
#endif
