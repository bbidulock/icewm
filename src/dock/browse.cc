/*
 * IceWM
 *
 * Copyright (C) 1998 Marko Macek
 */

#include "config.h"

#include "obj.h"
#include "objmenu.h"
#include "browse.h"
//#include "wmmgr.h"
#include "wmprog.h"
#include "sysdep.h"
#include "base.h"
#include "yapp.h"
#include <dirent.h>

BrowseMenu::BrowseMenu(const char *path,
                         YWindow *parent): ObjectMenu(parent)
{
    fPath = newstr(path);
    fModTime = 0;
}

BrowseMenu::~BrowseMenu() {
    delete fPath; fPath = 0;
}

void BrowseMenu::updatePopup() {
    struct stat sb;

    if (stat(fPath, &sb) != 0)
        removeAll();
    else if (sb.st_mtime > fModTime) {
        fModTime = sb.st_mtime;
        
        removeAll();

        DIR *dir;
        int plen = strlen(fPath);

        if ((dir = opendir(fPath)) != NULL) {
            struct dirent *de;
            bool isDir;
            int nlen;
            char *npath;
            YMenu *sub;
            char *name;

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
                    
                    sub = 0;
                    if (isDir)
                        sub = new BrowseMenu(npath);

                    name = de->d_name;
                    
                    if (name) {
                        DFile *pfile = new DFile(name, 0, npath);
                        YMenuItem *item = add(new DObjectMenuItem(pfile));
                        if (item) {
                            static YIcon *file, *folder;
                            if (file == 0)
                                file = app->getIcon("file");
                            if (folder == 0)
                                folder = app->getIcon("folder");
                            item->setSubmenu(sub);
                            if (sub) {
                                if (folder)
                                    item->setPixmap(folder->small());
                            } else {
                                if (file)
                                    item->setPixmap(file->small());
                            }
                        }
                    }
                }
            }
            closedir(dir);
        }
    }
}
