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
    char const *const homeDir = getenv("HOME");
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

YMenuItem * ThemesMenu::newThemeItem(char const *label, char const *theme) {
    char **args(new (char*)[6]);

    args[0] = newstr(ICEWMEXE);
    args[1] = newstr("-t");
    args[2] = newstr(theme);
    args[3] = configArg ? newstr("-c") : 0;
    args[4] = newstr(configArg);
    args[5] = 0;

    if (args[0] && args[1] && args[2]) {
	DProgram *launcher(DProgram::newProgram
	    (label, 0, true, 0, ICEWMEXE, args));

	if (launcher) {
	    YMenuItem *item(new DObjectMenuItem(launcher));

	    if (item) {
	        item->setChecked(themeName && !strcmp(themeName, theme));
		return item;
	    }
	}
	
	delete launcher;
    }

    delete[] args[4];
    delete[] args[3];
    delete[] args[2];
    delete[] args[1];
    delete[] args[0];
    delete[] args;
	
    return NULL;
}

void ThemesMenu::findThemes(const char *path, YMenu *container) {
    char const *const tname("/default.theme");
    bool isFirst(true);

    int dplen(strlen(path));
    char *npath(NULL), *dpath(NULL);

    if (dplen == 0 || path[dplen - 1] != '/') {
        npath = strJoin(path, "/", NULL);
        dplen++;
    } else {
        dpath = newstr(path);
    }

    DIR *dir(opendir(path));

    if (dir != NULL) {
        struct dirent *de;
        while ((de = readdir(dir)) != NULL) {
            YMenuItem *im(NULL);
	    npath = strJoin(dpath, de->d_name, tname, NULL);

            if (npath && access(npath, R_OK) == 0) {
		if (isFirst) {
		    isFirst = false;
		    if (itemCount()) addSeparator();
		    addLabel(path);
		    addSeparator();
		}
		
		im = newThemeItem(de->d_name, npath);
		if (im) container->add(im);
	    }

            delete [] npath;

	    char *subdir(strJoin(dpath, de->d_name, NULL));
	    if (im && subdir) findThemeAlternatives(subdir, im);
	}

	closedir(dir);
    }

    delete [] dpath;
}

void ThemesMenu::findThemeAlternatives(const char *path, YMenuItem *item) {
    DIR *dir(opendir(path));

    if (dir != NULL) {
        struct dirent *de;
        while ((de = readdir(dir)) != NULL) {
            char const *ext(strstr(de->d_name, ".theme"));

            if (ext != NULL && ext[sizeof("theme")] == '\0' &&
                strcmp(de->d_name, "default.theme")) {
                char *npath(strJoin(path, "/", de->d_name, NULL));

                if (npath && access(npath, R_OK) == 0) {
                    YMenu *sub(item->submenu());

                    if (sub == NULL)
                        item->setSubmenu(sub = new YMenu());

                    if (sub) {
                        char *tname(newstr(de->d_name, ext - de->d_name));
			sub->add(newThemeItem(tname, npath));
                        delete[] tname;
                    }
                }
                delete[] npath;
            }
        }
        closedir(dir);
    }
}
#endif
