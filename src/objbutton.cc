/*
 * IceWM
 *
 * Copyright (C) 1999-2002 Marko Macek
 */
#include "config.h"
#include "objbutton.h"
#include "prefs.h"
#include "wmapp.h"
#include "wpixmaps.h"
#include "yicon.h"
#include "obj.h"

YFont ObjectButton::font;
YColorName ObjectButton::bgColor(&clrToolButton);
YColorName ObjectButton::fgColor(&clrToolButtonText);

YFont ObjectButton::getFont() {
    if (font == null) {
        if (toolButtonFontName.nonempty()) {
            font = toolButtonFontName;
        }
        if (font == null) {
            font = YButton::getFont();
        }
    }
    return font;
}

YColor ObjectButton::getColor() {
    return fgColor ? fgColor : YButton::getColor();
}

YSurface ObjectButton::getSurface() {
    return YSurface(bgColor ? bgColor : YButton::normalButtonBg,
                    toolbuttonPixmap, toolbuttonPixbuf);
}

void ObjectButton::actionPerformed(YAction action, unsigned modifiers) {
    wmapp->signalGuiEvent(geLaunchApp);
    if (fObject) fObject->open();
    else YButton::actionPerformed(action, modifiers);
}

void ObjectButton::configure(const YRect2& r) {
    if (r.resized()) {
        repaint();
    }
}

void ObjectButton::paint(Graphics &g, const YRect &r) {
    if (taskbackPixbuf == null && taskbackPixmap != null) {
        g.fillPixmap(taskbackPixmap, r.x(), r.y(),
                     r.width(), r.height(), r.x() + x(), r.y() + y());
    }
    YButton::paint(g, r);
}

void ObjectButton::repaint() {
    GraphicsBuffer(this).paint();
}

void ObjectButton::requestFocus(bool requestUserFocus) {
    if (fMenu && hasPopup() == false) {
        setPopup(fMenu->ymenu());
    }
    YButton::requestFocus(requestUserFocus);
}

void ObjectButton::popupMenu() {
    if (fMenu && hasPopup() == false) {
        setPopup(fMenu->ymenu());
    }
    YButton::popupMenu();
}

void ObjectButton::handleButton(const XButtonEvent& up) {
    if (up.button == Button1) {
        YButton::handleButton(up);
    } else {
        parent()->handleButton(up);
    }
}

void ObjectButton::updateToolTip() {
    if (getToolTip() == null) {
        xsmart<char> name;
        if (fetchTitle(&name)) {
            setToolTip(mstring(name));
        }
    }
}

// vim: set sw=4 ts=4 et:
