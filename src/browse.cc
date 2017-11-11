/*
 * IceWM
 *
 * Copyright (C) 1998-2001 Marko Macek
 */

#include "config.h"
#include "obj.h"
#include "objmenu.h"
#include "browse.h"
#include "wmmgr.h"
#include "wmprog.h"
#include "yicon.h"
#include "sysdep.h"
#include "base.h"
#include "udir.h"

BrowseMenu::BrowseMenu(
    IApp *app,
    YSMListener *smActionListener,
    YActionListener *wmActionListener,
    upath path,
    YWindow *parent): ObjectMenu(wmActionListener, parent)
{
    this->app = app;
    this->smActionListener = smActionListener;
    fPath = path;
    fModTime = 0;
}

BrowseMenu::~BrowseMenu() {
}

void BrowseMenu::updatePopup() {
    struct stat sb;

    if (fPath.stat(&sb) != 0)
        removeAll();
    else if (sb.st_mtime > fModTime) {
        fModTime = sb.st_mtime;

        removeAll();

        ref<YIcon> file = YIcon::getIcon("file");
        ref<YIcon> folder = YIcon::getIcon("folder");

        for (adir dir(fPath.string()); dir.next(); ) {
            ustring entry(dir.entry());
            upath npath(fPath + entry);

            YMenu *sub = 0;
            if (npath.dirExists())
                sub = new BrowseMenu(app, smActionListener, wmActionListener, npath);

            DFile *pfile = new DFile(app, entry, null, npath);
            YMenuItem *item = add(new DObjectMenuItem(pfile));
            if (item) {
                item->setSubmenu(sub);
                if (sub) {
                    if (folder != null)
                        item->setIcon(folder);
                } else {
                    if (file != null)
                        item->setIcon(file);
                }
            }
            else if (sub) {
                delete sub;
            }
        }
    }
}

// vim: set sw=4 ts=4 et:
