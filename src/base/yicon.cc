/*
 * IceWM
 *
 * Copyright (C) 1997-2000 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "ypaint.h"
#include "yapp.h"
#include "sysdep.h"
#include "debug.h"

YIcon::YIcon(const char *fileName) {
    loadedS = loadedL = false;

    fLarge = fSmall = 0;
    fPath = new char [strlen(fileName) + 1];
    if (fPath)
        strcpy(fPath, fileName);
}

YIcon::YIcon(YPixmap *small, YPixmap *large) {
    fSmall = small;
    fLarge = large;
    loadedS = loadedL = true;
    fPath = 0;
    fNext = 0;
}

YIcon::~YIcon() {
    delete fLarge; fLarge = 0;
    delete fSmall; fSmall = 0;
    delete [] fPath; fPath = 0;
}

YPixmap *YIcon::large() {
    if (fLarge == 0 && !loadedL)
        fLarge = loadIcon(ICON_LARGE);
    loadedL = true;
    return fLarge;
}

YPixmap *YIcon::small() {
    if (fSmall == 0 && !loadedS)
        fSmall = loadIcon(ICON_SMALL);
    loadedS = true;
    //return large(); // for testing menus...
    return fSmall;
}

