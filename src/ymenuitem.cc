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
#include "yprefs.h"
#include "prefs.h"
#include "ypixbuf.h"
#include "yicon.h"

#include <string.h>

extern ref<YFont> menuFont;

YMenuItem::YMenuItem(const ustring &name, int aHotCharPos, const ustring &param,
                     YAction *action, YMenu *submenu) :
    fName(name), fParam(param), fAction(action),
    fHotCharPos(aHotCharPos), fSubmenu(submenu), fIcon(null),
    fChecked(false), fEnabled(true) {
    
    if (fName != null && (fHotCharPos == -2 || fHotCharPos == -3)) {
        int i = fName.indexOf('_');
        if (i != -1) {
            fHotCharPos = i;
            fName = fName.remove(i, 1);
#if 0
        char *hotChar = strchr(fName, '_');
        if (hotChar != NULL) {
            fHotCharPos = (hotChar - fName);
            memmove(hotChar, hotChar + 1, strlen(hotChar));
#endif
        } else {
            if (fHotCharPos == -3)
                fHotCharPos = 0;
            else
                fHotCharPos = -1;
        }
    }
    
    if (fName == null || fHotCharPos >= fName.length() || fHotCharPos < -1)
        fHotCharPos = -1;
}

YMenuItem::YMenuItem(const ustring &name) :
    fName(name), fParam(null), fAction(NULL), fHotCharPos (-1),
    fSubmenu(0), fIcon(null), fChecked(false), fEnabled(true) {
}

YMenuItem::YMenuItem():
    fName(null), fParam(null), fAction(0), fHotCharPos(-1),
    fSubmenu(0), fIcon(null), fChecked(false), fEnabled(false) {
}

YMenuItem::~YMenuItem() {
    if (fSubmenu && !fSubmenu->isShared())
        delete fSubmenu;
    fSubmenu = 0;
}

void YMenuItem::setChecked(bool c) {
    fChecked = c;
}

void YMenuItem::setIcon(ref<YIcon> icon) {
    fIcon = icon;
}

void YMenuItem::actionPerformed(YActionListener *listener, YAction *action, unsigned int modifiers) {
    if (listener && action)
        listener->actionPerformed(action, modifiers);
}

int YMenuItem::queryHeight(int &top, int &bottom, int &pad) const {
    top = bottom = pad = 0;

    if (getName() != null || getSubmenu()) {
        int fontHeight = max(16, menuFont->height() + 1);
        int ih = fontHeight;

#ifndef LITE
        if (YIcon::smallSize() > ih)
            ih = YIcon::smallSize();
#endif

        if (wmLook == lookWarp4 || wmLook == lookWin95) {
            top = bottom = 0;
            pad = 1;
        } else if (wmLook == lookMetal || wmLook == lookFlat) {
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
        top = 0;
        bottom = 0;
        pad = 1;

        return ((wmLook == lookMetal || wmLook == lookFlat) ? 3 : 4);
    }
}

int YMenuItem::getIconWidth() const {
#ifdef LITE
    return 0;
#else
    ref<YIcon> icon = getIcon();
    return icon != null ? YIcon::smallSize(): 0;
#endif
}

int YMenuItem::getNameWidth() const {
    ustring name = getName();
    return name != null ? menuFont->textWidth(name) : 0;
}

int YMenuItem::getParamWidth() const {
    ustring param = getParam();
    return  param != null ? menuFont->textWidth(param) : 0;
}
