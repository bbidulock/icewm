/*
 * IceWM
 *
 * Copyright (C) 1997-2000 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "ypaint.h"
#include "yapp.h"
#include "ywindow.h"
#include "sysdep.h"
#include "prefs.h"
//#include "debug.h"

YPixmap::YPixmap(Pixmap pixmap, Pixmap mask, int w, int h, bool owned) {
    fOwned = owned;
    fWidth = w;
    fHeight = h;
    fPixmap = pixmap;
    fMask = mask;
}

YPixmap::YPixmap(int w, int h, bool mask) {
    fOwned = true;
    fWidth = w;
    fHeight = h;

    fPixmap = XCreatePixmap(app->display(),
                            desktop->handle(),
                            fWidth, fHeight,
                            DefaultDepth(app->display(),
                                         DefaultScreen(app->display())));
    if (mask)
        fMask = XCreatePixmap(app->display(),
                              desktop->handle(),
                              fWidth, fHeight,
                              DefaultDepth(app->display(),
                                           DefaultScreen(app->display())));
    else
        fMask = 0;
}

YPixmap::~YPixmap() {
    if (fOwned && app) {
        if (fPixmap != None)
            XFreePixmap(app->display(), fPixmap);
        if (fMask != None)
            XFreePixmap(app->display(), fMask);
    }
}
