/*
 * IceWM
 *
 * Copyright (C) 1998-2001 Marko Macek
 *
 * Changes:
 *
 *      2002/12/14
 *       * rewrite as an external program
 *      2000/10/20 mathias.hasselmann@gmx.de
 *       * read .order files
 *       * icons for submenus
 *       * clean up
 *      2000/09/19 mathias.hasselmann@gmx.de
 *       * localized submenus
 */

#include "config.h"
#ifdef CONFIG_GNOME_MENUS

#include "ylib.h"
#include "default.h"

#include "ypixbuf.h"
#include "yapp.h"
#include "sysdep.h"
#include "base.h"
#include <dirent.h>
#include <string.h>

#include <gnome.h>
#include "yarray.h"

char const * ApplicationName = "icewm-menu-gnome1";

class GnomeMenu;

class GnomeMenuItem {
public:
    GnomeMenuItem() { title = 0; icon = 0; dentry = 0; submenu = 0; }

    const char *title;
    const char *icon;
    const char *dentry;
    GnomeMenu *submenu;
};

class GnomeMenu {
public:
    GnomeMenu() { }

    YObjectArray<GnomeMenuItem> items;

    bool isDuplicateName(const char *name);
    void addEntry(const char *fPath, const char *name, const int plen,
                         const bool firstRun);
    void populateMenu(const char *fPath);
};

void dumpMenu(GnomeMenu *menu) {
    for (unsigned int i = 0; i < menu->items.getCount(); i++) {
        GnomeMenuItem *item = menu->items.getItem(i);

        if (item->dentry && !item->submenu) {
            printf("prog \"%s\" %s icewm-menu-gnome1 --open \"%s\"\n",
                   item->title,
                   item->icon ? item->icon : "-",
                   item->dentry);
        } else if (item->dentry && item->submenu) {
            printf("menuprog \"%s\" %s icewm-menu-gnome1 --list \"%s\"\n",
                   item->title,
                   item->icon ? item->icon : "-",
                   g_dirname(item->dentry));
        }
    }
}

bool GnomeMenu::isDuplicateName(const char *name) {
    for (unsigned int i = 0; i < items.getCount(); i++) {
        GnomeMenuItem *item = items.getItem(i);
        if (strcmp(name, item->title) == 0)
            return 1;
    }
    return 0;
}

void GnomeMenu::addEntry(const char *fPath, const char *name, const int plen,
                         const bool firstRun)
{
    const int nlen = (plen == 0 || fPath[plen - 1] != '/')
        ? plen + 1 + strlen(name)
        : plen + strlen(name);
    char *npath = new char[nlen + 1];

    if (npath) {
        strcpy(npath, fPath);

        if (plen == 0 || npath[plen - 1] != '/') {
            npath[plen] = '/';
            strcpy(npath + plen + 1, name);
        } else
            strcpy(npath + plen, name);

        GnomeMenuItem *item = new GnomeMenuItem();
        item->title = name;

        GnomeDesktopEntry *dentry = 0;

        struct stat sb;
        const bool isDir = (!stat(npath, &sb) && S_ISDIR(sb.st_mode));
        if (isDir) {
            GnomeMenu *submenu = new GnomeMenu();

            item->title = g_basename(npath);
            item->icon = gnome_pixmap_file("gnome-folder.png");
            item->submenu = submenu;

            char *epath = new char[nlen + sizeof("/.directory")];
            strcpy(epath, npath);
            strcpy(epath + nlen, "/.directory");

            dentry = gnome_desktop_entry_load(epath);
            if (dentry) {
                item->title = dentry->name;
                item->icon = dentry->icon;
            }
            item->dentry = epath;
        } else {
            dentry = gnome_desktop_entry_load(npath);
            if (dentry) {
                item->title = newstr(dentry->name);
                if (dentry->icon)
                    item->icon = newstr(dentry->icon);
                item->dentry = npath;
            }
        }
        if (firstRun || !isDuplicateName(item->title))
            items.append(item);
    }
}

void GnomeMenu::populateMenu(const char *fPath) {
    const int plen = strlen(fPath);

    char *opath = new char[plen + sizeof("/.order")];
    if (opath) {
        strcpy(opath, fPath);
        strcpy(opath + plen, "/.order");

        FILE * order(fopen(opath, "r"));

        if (order) {
            char oentry[100];

            while (fgets(oentry, sizeof (oentry), order)) {
                const int oend = strlen(oentry) - 1;

                if (oend > 0 && oentry[oend] == '\n')
                    oentry[oend] = '\0';

                addEntry(fPath, oentry, plen, true);
            }

            fclose(order);
        }

        delete opath;
    }

    DIR *dir = opendir(fPath);
    if (dir != 0) {
        struct dirent *file;

        while ((file = readdir(dir)) != NULL) {
            if (*file->d_name != '.')
                addEntry(fPath, file->d_name, plen, false);
        }
        closedir(dir);
    }
}

int makeMenu(const char *base_directory) {
    GnomeMenu *menu = new GnomeMenu();
    menu->populateMenu(base_directory);
    dumpMenu(menu);
    return 0;
}

int runFile(const char *dentry_path) {
    GnomeDesktopEntry *dentry = 0;

    dentry = gnome_desktop_entry_load(dentry_path);

    if (dentry->exec_length == 0 || dentry->exec == 0) {
        msg("can't launch: %s", dentry->name);
        return 1;
    }

    gnome_desktop_entry_launch(dentry);
    return 0;
}

int main(int argc, char **argv) {
    for (char ** arg = argv + 1; arg < argv + argc; ++arg) {
        if (**arg == '-') {
            char *path = 0;
            if (IS_SWITCH("h", "help"))
                break;
            if ((path = GET_LONG_ARGUMENT("open")) != NULL) {
                return runFile(path);
            } else if ((path = GET_LONG_ARGUMENT("list")) != NULL) {
                return makeMenu(path);
            }
        }
    }
    msg("Usage: %s [ --open PATH | --list PATH ]", argv[0]);
}
#endif
