/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "ykey.h"
#include "ypaint.h"
#include "ymenuitem.h"
#include "ymenu.h"
#include "yaction.h"

#include "yapp.h"
#include "prefs.h"

#include <string.h>

YMenuItem::YMenuItem(const char *name, int aHotCharPos, const char *param, 
		     YAction *action, YMenu *submenu) :
    fName(newstr(name)), fParam(newstr(param)), fAction(action),
    fHotCharPos(aHotCharPos), fSubmenu(submenu), fIcon(NULL), 
    fChecked(false), fEnabled(true) {
    
    if (fName && fHotCharPos == -2) {
        char *hotChar = strchr(fName, '_');
        if (hotChar != NULL) {
            fHotCharPos = (hotChar - fName);
            memmove(hotChar, hotChar + 1, strlen(hotChar));
        } else
            fHotCharPos = 0;
    }
    
    if (!fName || fHotCharPos >= int(strlen(fName)) || fHotCharPos < -1)
        fHotCharPos = -1;
}

YMenuItem::YMenuItem(const char *name) :
    fName(newstr(name)), fParam(NULL), fAction(NULL), fHotCharPos (-1),
    fSubmenu(0), fIcon(NULL), fChecked(false), fEnabled(true) {
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

void YMenuItem::setIcon(YIcon::Image * icon) {
    fIcon = icon;
}

void YMenuItem::actionPerformed(YActionListener *listener, YAction *action, unsigned int modifiers) {
    if (listener && action)
        listener->actionPerformed(action, modifiers);
}
