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
#include "ypixbuf.h"

#include <string.h>

extern YFont *menuFont;

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

YMenuItem::YMenuItem():
    fName(0), fParam(0), fAction(0), fHotCharPos(-1), 
    fSubmenu(0), fIcon(0), fChecked(false), fEnabled(false) {
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

int YMenuItem::queryHeight(int &top, int &bottom, int &pad) const {
    top = bottom = pad = 0;

    if (getName() || getSubmenu()) {
        unsigned fontHeight = max(16U, menuFont->height() + 1);
        unsigned ih = fontHeight;

        if (getIcon() && getIcon()->height() > ih)
            ih = getIcon()->height();

        if (wmLook == lookWarp4 || wmLook == lookWin95) {
            top = bottom = 0;
            pad = 1;
        } else if (wmLook == lookMetal) {
            top = bottom = 1;
            pad = 1;
        } else if (wmLook == lookMotif) {
            top = bottom = 2;
            pad = 0; //1
        } else if (wmLook == lookGtk) {
            top = bottom = 2;
            pad = 0; //1
        } else {
            top = 1;
            bottom = 2;
            pad = 0; //1;
        }

        return (top + pad + ih + pad + bottom);
    } else {
#warning introduce YMenuSeparator (then again, maybe not)
        top = 0;
        bottom = 0;
        pad = 1;

	return (wmLook == lookMetal ? 3 : 4);
    }
}

int YMenuItem::getIconWidth() const {
    YIcon::Image const *icon = getIcon();
    return icon ? icon->width() : 0;
}

int YMenuItem::getNameWidth() const {
    const char *name = getName();
    return name ? menuFont->textWidth(name) : 0;
}

int YMenuItem::getParamWidth() const {
    const char *param = getParam();
    return  param ? menuFont->textWidth(param) : 0;
}

