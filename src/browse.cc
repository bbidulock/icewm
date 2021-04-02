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
        loadItems();
    }
}

void BrowseMenu::loadItems() {
    removeAll();
    for (sdir dir(fPath); dir.next(); ) {
        upath npath(fPath + dir.entry());
        ObjectMenu *sub = nullptr;
        if (npath.dirExists())
            sub = new BrowseMenu(app, smActionListener,
                                 getActionListener(), npath);
        DFile *pfile = new DFile(app, dir.entry(), null, npath);
        addObject(pfile, sub ? "folder" : "file", sub);
    }
}

// vim: set sw=4 ts=4 et:
