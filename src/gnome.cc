/*
 * IceWM
 *
 * Copyright (C) 1998-2001 Marko Macek
 *
 * Changes:
 *
 *      2000/10/20 mathias.hasselmann@gmx.de
 *       * read .order files
 *       * icons for submenus
 *       * clean up
 *      2000/09/19 mathias.hasselmann@gmx.de
 *       * localized submenus
 */

#include "config.h"

#ifdef CONFIG_GNOME_MENUS
#include "default.h"
#include "ylib.h"

#include "yapp.h"
#include "sysdep.h"
#include "base.h"
#include <dirent.h>
#include "gnomeapps.h"

YPixmap* GnomeMenu::folder_icon = 0;

DGnomeDesktopEntry::DGnomeDesktopEntry(const char *name, YIcon *icon, GnomeDesktopEntry *dentry):
    DObject(name, icon)
{
    fEntry = dentry;
}

DGnomeDesktopEntry::~DGnomeDesktopEntry() {
    gnome_desktop_entry_free(fEntry);
}

void DGnomeDesktopEntry::open() {
    XSync(app->display(), False);
    gnome_desktop_entry_launch(fEntry);
}

GnomeMenu::GnomeMenu(YWindow *parent,
                     const char *path): ObjectMenu(parent)
{
    fPath = newstr(path);
    fModTime = 0;
}

GnomeMenu::~GnomeMenu() {
    delete fPath; fPath = 0;
}

void GnomeMenu::updatePopup() {
    struct stat sb;

    if (stat(fPath, &sb) != 0)
        removeAll();
    else if (sb.st_mtime > fModTime) {
        fModTime = sb.st_mtime;
        removeAll();

        populateMenu(this);
    }
}

void GnomeMenu::createToplevel(ObjectMenu *menu, const char *path) {
    GnomeMenu *gmenu = new GnomeMenu(0, path);

    if (gmenu != 0) {
        gmenu->populateMenu(menu);
        delete gmenu;
    }
}

void GnomeMenu::createSubmenu(ObjectMenu *menu, const char *path,
                              const char *name, YPixmap *icon) {
    GnomeMenu *gmenu = new GnomeMenu(0, path);

    if (gmenu != 0) {
        YMenuItem *item = menu->addSubmenu(name, 0, gmenu);
        if (icon && item) item->setPixmap(icon);
    }
}

void GnomeMenu::populateMenu(ObjectMenu *target) {
    const int firstEntry = target->itemCount();

#ifdef LITE
    if (folder_icon == 0)
#ifdef CONFIG_IMLIB
        if (gnomeFolderIcon) {
            char const * icon_path(gnome_pixmap_file("gnome-folder.png"));

            if (icon_path != NULL)
                folder_icon = new YPixmap(icon_path, ICON_SMALL, ICON_SMALL);
            g_free(icon_path);
        } else {
#endif
            YIcon * icon(getIcon("folder"));
            if (icon) folder_icon = icon->small();
#ifdef CONFIG_IMLIB
        }
#endif
#endif

    const int plen = strlen(fPath);

    char *opath = new char[plen + sizeof(".order")];
    if (opath) {
        strcpy(opath, fPath);
        strcpy(opath + plen, ".order");

        FILE *order = fopen(opath, "r");
        if (order != 0) {
            char oentry[100];

            while (fgets(oentry, sizeof (oentry), order)) {
                const int oend = strlen(oentry) - 1;

                if (oend > 0 && oentry[oend] == '\n')
                    oentry[oend] = '\0';

                addEntry(oentry, plen, target, firstEntry);
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
                addEntry(file->d_name, plen, target, firstEntry, false);
        }

        closedir(dir);
    }
}

void GnomeMenu::addEntry(const char *name, const int plen, ObjectMenu *target,
                         const int firstItem, const bool firstRun) {
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

        struct stat sb;
        const bool isDir = (!stat(npath, &sb) && S_ISDIR(sb.st_mode));
        GnomeDesktopEntry *dentry;

        if (isDir) {
            YMenu *sub = new GnomeMenu(0, npath);

            if (sub) {
                char *epath(new char[nlen + sizeof("/.directory")]);
                strcpy(epath, npath);
                strcpy(epath + nlen, "/.directory");

                dentry = gnome_desktop_entry_load(epath);
                const char * tname((dentry ? dentry->name : name));

                if (firstRun || !target->findName(tname, firstItem)) {
                    YMenuItem *item = target->addSubmenu(tname, 0, sub);
                    if (item) {
#ifdef CONFIG_IMLIB
                        YPixmap * icon(gnomeFolderIcon && 
				       dentry && dentry->icon ?
			    new YPixmap(dentry->icon,
			    		YIcon::smallSize, YIcon::smallSize) :
                            folder_icon);

                        if (icon) item->setPixmap(icon);
#else
                        if (folder_icon) item->setPixmap(folder_icon);
#endif
                    }
                }

                gnome_desktop_entry_free(dentry);
                delete epath;
            }
        } else if ((dentry = gnome_desktop_entry_load(npath)) != NULL &&
                   (firstRun || !target->findName(dentry->name, firstItem))) {
            DGnomeDesktopEntry *gde =
                new DGnomeDesktopEntry(dentry->name, 0, dentry);

            if (gde) {
                YMenuItem *item = new DObjectMenuItem(gde);
#ifdef CONFIG_IMLIB
                if (dentry->icon) {
                    YPixmap * icon(new YPixmap(dentry->icon, 
			YIcon::smallSize, YIcon::smallSize));

                    if (icon) item->setPixmap(icon);
                }
#endif
                target->add(item);
            }
        }

        delete npath;
    }
}

#endif
