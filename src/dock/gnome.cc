/*
 * IceWM
 *
 * Copyright (C) 1998 Marko Macek
 */

/* just a quick hack (on top of a quick hack!) */
#include "config.h"
#ifdef GNOME
#include "ylib.h"

#include "yapp.h"
#include "sysdep.h"
#include "base.h"
#include <dirent.h>
#include "gnomeapps.h"

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

        DIR *dir;
        int plen = strlen(fPath);

        static YPixmap *folder = 0;

#ifdef CONFIG_IMLIB
        if (folder == 0) {
            char *folder_icon = gnome_pixmap_file("gnome-folder.png");
            if (folder_icon)
                folder = new YPixmap(folder_icon, ICON_SMALL, ICON_SMALL);
        }
#endif
        if ((dir = opendir(fPath)) != NULL) {
            struct dirent *de;
            bool isDir;
            int nlen;
            char *npath;
            YMenu *sub;
            GnomeDesktopEntry *dentry;

            while ((de = readdir(dir)) != NULL) {
                nlen = plen + 1 + strlen(de->d_name) + 1;
                npath = new char[nlen];

                if (npath && de->d_name[0] != '.') {
                    strcpy(npath, fPath);
                    if (plen == 0 || npath[plen - 1] != '/') {
                        strcpy(npath + plen, "/");
                        strcpy(npath + plen + 1, de->d_name);
                    } else {
                        strcpy(npath + plen, de->d_name);
                    }

                    isDir = false;

                    if (stat(npath, &sb) == 0)
                        if (S_ISDIR(sb.st_mode))
                            isDir = true;

                    if (isDir) {
                        sub = new GnomeMenu(0, npath);

                        if (sub) {
                            YMenuItem *item = addSubmenu(de->d_name, 0, sub);
                            if (folder && item) item->setPixmap(folder);
                        }
                    } else if ((dentry = gnome_desktop_entry_load(npath))) {
                        DGnomeDesktopEntry *gde = new DGnomeDesktopEntry(dentry->name, 0, dentry);
                        if (gde) {
                            YMenuItem *item = new DObjectMenuItem(gde);
                            if (dentry->icon) {
                                YPixmap *menuicon = 0;
#ifdef CONFIG_IMLIB
                                menuicon = new YPixmap(dentry->icon,
                                                       ICON_SMALL, ICON_SMALL);
#endif
                                if (menuicon)
                                    item->setPixmap(menuicon);
                            }
                            add(item);
                        }
                    }
                }
            }
            closedir(dir);
        }
    }
}
#endif
