/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#include "config.h"
#include "ykey.h"
#include "ymenuitem.h"
#include "ymenu.h"
#include "yaction.h"

#include "yapp.h"
#include "prefs.h"

#include <string.h>

YMenuItem::YMenuItem(const char *name, int aHotCharPos, const char *param, YAction *action, YMenu *submenu) {
    fName = newstr(name);
    fParam = newstr(param);
    fAction = action;
    //fCommand = command;
    //fContext = context;
    fSubmenu = submenu;
    fEnabled = 1;
    fHotCharPos = aHotCharPos;
    fPixmap = 0;
    fChecked = 0;
    if (!fName || fHotCharPos >= int(strlen(fName)) || fHotCharPos < -1)
        fHotCharPos = -1;
}

YMenuItem::~YMenuItem() {
    if (fSubmenu && !fSubmenu->isShared())
        delete fSubmenu;
    fSubmenu = 0;
    delete fName; fName = 0;
    delete fParam; fParam = 0;
}

void YMenuItem::setChecked(bool c) {
    fChecked = c;
}

void YMenuItem::setPixmap(YPixmap *pixmap) {
    fPixmap = pixmap;
}

void YMenuItem::actionPerformed(YActionListener *listener, YAction *action, unsigned int modifiers) {
    if (listener && action)
        listener->actionPerformed(action, modifiers);
}
