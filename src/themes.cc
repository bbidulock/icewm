/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"

#ifndef NO_CONFIGURE_MENUS
#include "themes.h"

#include "yapp.h"
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
    char *path;

    path = strJoin(libDir, "/themes/", NULL);
    findThemes(path, this);
    delete path;

    path = strJoin(configDir, "/themes/", NULL);
    findThemes(path, this);
    delete path;

    path = strJoin(YApplication::getPrivConfDir(), "/themes/", NULL);
    findThemes(path, this);
    delete path;
}

ThemesMenu::~ThemesMenu() {
}

extern char *configArg;

YMenuItem * ThemesMenu::newThemeItem(char const *label, char const *theme) {
    YStringArray args(6);

    args.append(app->executable());
    args.append("-t");
    args.append(theme);
    
    if (configArg) {
    	args.append("-c");
    	args.append(configArg);
    }

    if (args[0] && args[1] && args[2]) {
	DProgram *launcher =
	    DProgram::newProgram(label, 0, true, 0, *args, args);

	if (launcher) {
	    YMenuItem *item(new DObjectMenuItem(launcher));

	    if (item) {
	        item->setChecked(themeName && !strcmp(themeName, theme));
		return item;
	    }
	}
	
	delete launcher;
	return 0;
    }

    return NULL;
}

void ThemesMenu::findThemes(const char *path, YMenu *container) {
    char const *const tname("/default.theme");
    bool isFirst(true);

    int dplen(strlen(path));
    char *npath = NULL, *dpath = NULL;

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
            delete [] subdir;
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
                    YMenu *sub(item->getSubmenu());

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
