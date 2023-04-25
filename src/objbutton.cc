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
ref<YImage> ObjectButton::gradients[4];

void ObjectButton::freeFont() {
    font = null;
    for (int i = 0; i < 4; ++i)
        gradients[i] = null;
}

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
    ref<YImage> grad;
    if (toolbuttonPixbuf != null) {
        int i = 0;
        for (; i < 4; ++i)
            if (gradients[i] == null ||
                (gradients[i]->width() == width() &&
                 gradients[i]->height() == height()))
                break;
        if (i == 4 || gradients[i] == null)
            grad = toolbuttonPixbuf->scale(width(), height());
        else
            grad = gradients[i];
        for (; 0 < i; --i)
            if (i < 4)
                gradients[i] = gradients[i - 1];
        gradients[0] = grad;
    }
    return YSurface(bgColor ? bgColor : YButton::normalButtonBg,
                    toolbuttonPixmap, grad);
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
    if (fRealized) {
        GraphicsBuffer(this).paint();
    }
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
    if (hasToolTip() == false) {
        if (fObject) {
            setToolTip(fObject->getName(), fObject->getIcon());
        } else {
            xsmart<char> name;
            if (fetchTitle(&name)) {
                setToolTip(mstring(name), getIcon());
            }
        }
    }
}

// vim: set sw=4 ts=4 et:
