/*
 * IceWM
 *
 * Copyright (C) 1998-2001 Marko Macek
 */

#include "config.h"

#ifndef NO_CONFIGURE_MENUS

#include "obj.h"
#include "objmenu.h"
#include "browse.h"
#include "wmmgr.h"
#include "wmprog.h"
#include "yicon.h"
#include "sysdep.h"
#include "base.h"
#include <dirent.h>

BrowseMenu::BrowseMenu(upath path,
                       YWindow *parent): ObjectMenu(parent)
{
    fPath = path;
    fModTime = 0;
}

BrowseMenu::~BrowseMenu() {
}

void BrowseMenu::updatePopup() {
    struct stat sb;

    if (stat(cstring(fPath.path()).c_str(), &sb) != 0)
        removeAll();
    else if (sb.st_mtime > fModTime) {
        fModTime = sb.st_mtime;

        removeAll();

        DIR *dir;

        if ((dir = opendir(cstring(fPath.path()).c_str())) != NULL) {
            struct dirent *de;
            bool isDir;
            YMenu *sub;

            while ((de = readdir(dir)) != NULL) {
                if (de->d_name[0] != '.') {
                    ustring name(de->d_name);
                    upath npath(fPath.relative(name));

                    isDir = npath.dirExists();

                    sub = 0;
                    if (isDir)
                        sub = new BrowseMenu(npath);

                    DFile *pfile = new DFile(name, null, npath);
                    YMenuItem *item = add(new DObjectMenuItem(pfile));
                    if (item) {
#ifndef LITE
                        static ref<YIcon> file, folder;
                        if (file == null)
                            file = YIcon::getIcon("file");
                        if (folder == null)
                            folder = YIcon::getIcon("folder");
#endif
                        item->setSubmenu(sub);
#ifndef LITE
                        if (sub) {
                            if (folder != null)
                                item->setIcon(folder);
                        } else {
                            if (file != null)
                                item->setIcon(file);
                        }
#endif
                    }
                }
            }
            closedir(dir);
        }
    }
}
#endif
