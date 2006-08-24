/*
 * IceWM
 *
 * Copyright (C) 1998-2003 Marko Macek & Nehal Mistry
 *
 * Changes:
 *
 *      2003/06/14
 *       * created gnome2 support from gnome.cc
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
#include <libgnome/gnome-desktop-item.h>
#include <libgnomevfs/gnome-vfs-init.h>
#include "yarray.h"

char const * ApplicationName = "icewm-menu-gnome2";

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
            printf("prog \"%s\" %s icewm-menu-gnome2 --open \"%s\"\n",
                   item->title,
                   item->icon ? item->icon : "-",
                   item->dentry);
        } else if (item->dentry && item->submenu) {
            printf("menuprog \"%s\" %s icewm-menu-gnome2 --list \"%s\"\n",
                   item->title,
                   item->icon ? item->icon : "-",
                   (!strcmp(my_basename(item->dentry), ".directory") ?
                    g_dirname(item->dentry) : item->dentry));
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

        GnomeDesktopItem *ditem =
            gnome_desktop_item_new_from_file(npath,
                                             (GnomeDesktopItemLoadFlags)0,
                                             NULL);

        struct stat sb;
        const char *type;
        bool isDir = (!stat(npath, &sb) && S_ISDIR(sb.st_mode));
        type = gnome_desktop_item_get_string(ditem,
                                             GNOME_DESKTOP_ITEM_TYPE);
        if (!isDir && type && strstr(type, "Directory")) {
            isDir = 1;
        }

        if (isDir) {
            GnomeMenu *submenu = new GnomeMenu();

            item->title = g_basename(npath);
            item->icon = gnome_pixmap_file("gnome-folder.png");
            item->submenu = submenu;

            char *epath = new char[nlen + sizeof("/.directory")];
            strcpy(epath, npath);
            strcpy(epath + nlen, "/.directory");

            if (stat(epath, &sb) == -1) {
                strcpy(epath, npath);
            }

            ditem = gnome_desktop_item_new_from_file(epath,
                                                     (GnomeDesktopItemLoadFlags)0,
                                                     NULL);
            if (ditem) {
                item->title = gnome_desktop_item_get_localestring(ditem, GNOME_DESKTOP_ITEM_NAME); //LXP FX
                item->icon = gnome_desktop_item_get_string(ditem, GNOME_DESKTOP_ITEM_ICON);
            }
            item->dentry = epath;
        } else {
            if (type && !strstr(type, "Directory")) {
                item->title = gnome_desktop_item_get_localestring(ditem, GNOME_DESKTOP_ITEM_NAME);
                if (gnome_desktop_item_get_string(ditem, GNOME_DESKTOP_ITEM_ICON))
                    item->icon = gnome_desktop_item_get_string(ditem, GNOME_DESKTOP_ITEM_ICON);
                item->dentry = npath;
            }
        }
        if (firstRun || !isDuplicateName(item->title))
            items.append(item);
    }
}

void GnomeMenu::populateMenu(const char *fPath) {
    struct stat sb;
    bool isDir = (!stat(fPath, &sb) && S_ISDIR(sb.st_mode));
    const int plen = strlen(fPath);

    char tmp[256];
    strcpy(tmp, fPath);
    strcat(tmp, "/.directory");

    if (isDir && !stat(tmp, &sb)) { // looks like kde/gnome1 style

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
    } else {   // gnome2 style
        char *category = NULL;
        char dirname[256] = "a";

        if (isDir) {
            strcpy(dirname, fPath);
        } else if (strstr(fPath, "Settings")) {
            strcpy(dirname, "/usr/share/control-center-2.0/capplets/");
        } else if (strstr(fPath, "Advanced")) {
            strcpy(dirname, "/usr/share/control-center-2.0/capplets/");
        } else {
            dirname[0] = '\0';
        }

        if (isDir) {
            DIR *dir = opendir(dirname);
            if (dir != 0) {
                struct dirent *file;

                while ((file = readdir(dir)) != NULL) {
                    if (!strcmp(dirname, fPath) &&
                        (strstr(file->d_name, "Accessibility") ||
                         strstr(file->d_name, "Advanced") ||
                         strstr(file->d_name, "Applications") ||
                         strstr(file->d_name, "Root") ))
                        continue;
                    if (*file->d_name != '.')
                        addEntry(dirname, file->d_name, strlen(dirname), false);
                }
                closedir(dir);
            }
        }

        strcpy(dirname, "/usr/share/applications/");

        if (isDir) {
            category = strdup("ion;Core");
        } else if (strstr(fPath, "Applications")) {
            category = strdup("ion;Merg");
        } else if (strstr(fPath, "Accessories")) {
            category = strdup("ion;Util");
        } else if (strstr(fPath, "Advanced")) {
            category = strdup("ngs;Adva");
            strcpy(dirname, "/usr/share/control-center-2.0/capplets/");
        } else if (strstr(fPath, "Accessibility")) {
            category = strdup("ngs;Acce");
            strcpy(dirname, "/usr/share/control-center-2.0/capplets/");
        } else if (strstr(fPath, "Development")) {
            category = strdup("ion;Deve");
        } else if (strstr(fPath, "Editors")) {
            category = strdup("ion;Text");
        } else if (strstr(fPath, "Games")) {
            category = strdup("ion;Game");
        } else if (strstr(fPath, "Graphics")) {
            category = strdup("ion;Grap");
        } else if (strstr(fPath, "Internet")) {
            category = strdup("ion;Netw");
        } else if (strstr(fPath, "Root")) {
            category = strdup("ion;Core");
        } else if (strstr(fPath, "Multimedia")) {
            category = strdup("ion;Audi");
        } else if (strstr(fPath, "Office")) {
            category = strdup("ion;Offi");
        } else if (strstr(fPath, "Settings")) {
            category = strdup("ion;Sett");
            strcpy(dirname, "/usr/share/control-center-2.0/capplets/");
        } else if (strstr(fPath, "System")) {
            category = strdup("ion;Syst");
        } else {
            category = strdup("xyz");
        }

        if (!strlen(dirname))
            strcpy(dirname, "/usr/share/applications/");

        DIR* dir = opendir(dirname);
        if (dir != 0) {
            struct dirent *file;

            while ((file = readdir(dir)) != NULL) {
                char fullpath[256];
                strcpy(fullpath, dirname);
                strcat(fullpath, file->d_name);
                GnomeDesktopItem *ditem =
                    gnome_desktop_item_new_from_file(fullpath,
                                                     (GnomeDesktopItemLoadFlags)0,
                                                     NULL);
                const char *categories =
                    gnome_desktop_item_get_string(ditem,
                                                  GNOME_DESKTOP_ITEM_CATEGORIES);

                if (categories && strstr(categories, category)) {
                    if (*file->d_name != '.') {
                        if (strstr(fPath, "Settings")) {
                            if (!strstr(categories, "ngs;Adva") && !strstr(categories, "ngs;Acce"))
                                addEntry(dirname, file->d_name, strlen(dirname), false);
                        } else {
                            addEntry(dirname, file->d_name, strlen(dirname), false);
                        }
                    }
                }
            }

            if (strstr(fPath, "Settings")) {
                addEntry("/usr/share/gnome/vfolders/", "Accessibility.directory",
                         strlen("/usr/share/gnome/vfolders/"), false);
                addEntry("/usr/share/gnome/vfolders/", "Advanced.directory",
                         strlen("/usr/share/gnome/vfolders/"), false);
            }
            closedir(dir);
        }
    }
}

int makeMenu(const char *base_directory) {
    GnomeMenu *menu = new GnomeMenu();
    menu->populateMenu(base_directory);
    dumpMenu(menu);
    return 0;
}

int runFile(const char *dentry_path) {

    char arg[32];
    int i;

    GnomeDesktopItem *ditem =
        gnome_desktop_item_new_from_file(dentry_path,
                                         (GnomeDesktopItemLoadFlags)0,
                                         NULL);

    if (ditem == NULL) {
        return 1;
    } else {
//      FIXME: leads to segfault for some reason, so using execlp instead
//      gnome_desktop_item_launch(ditem, NULL, 0, NULL);

        const char *app = gnome_desktop_item_get_string(ditem, GNOME_DESKTOP_ITEM_EXEC);
        for (i = 0; app[i] && app[i] != ' '; ++i) {
            arg[i] = app[i];
        }
        arg[i] = '\0';

        execlp(arg, arg, NULL);
    }

    return 0;
}

int main(int argc, char **argv) {

    gnome_vfs_init();

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
