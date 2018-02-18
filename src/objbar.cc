/*
 * IceWM
 *
 * Copyright (C) 1999-2002 Marko Macek
 */
#include "config.h"
#include "objmenu.h"
#include "objbar.h"
#include "objbutton.h"
#include "ybutton.h"
#include "prefs.h"
#include "wmtaskbar.h"
#include "wmapp.h"
#include "wpixmaps.h"
#include "yrect.h"
#include "yicon.h"

ref<YFont> ObjectButton::font;
YColorName ObjectButton::bgColor(&clrToolButton);
YColorName ObjectButton::fgColor(&clrToolButtonText);

ObjectBar::ObjectBar(YWindow *parent): YWindow(parent) {
    setSize(1, 1);
}

ObjectBar::~ObjectBar() {
}

void ObjectBar::addButton(const ustring &name, ref<YIcon> icon, YButton *button) {
    button->setToolTip(name);
    if (icon != null) {
        button->setIcon(icon, YIcon::smallSize());
        button->setSize(button->width() + 4, button->width() + 4);
    } else
        button->setText(name);

    button->setPosition(width(), 0);
    unsigned h = button->height();
    if (h < height())
        h = height();

    if (h < height())
        h = height();

    button->setSize(button->width(), h);
    setSize(width() + button->width(), h);
    button->show();

    objects.append(button);
}

void ObjectBar::paint(Graphics &g, const YRect &/*r*/) {
    ref<YImage> gradient(parent()->getGradient());

    if (gradient != null)
        g.drawImage(gradient, this->x(), this->y(), width(), height(), 0, 0);
    else
    if (taskbackPixmap != null)
        g.fillPixmap(taskbackPixmap, 0, 0, width(), height());
    else {
        g.setColor(taskBarBg);
        g.fillRect(0, 0, width(), height());
    }
}

void ObjectBar::addObject(DObject *object) {
    YButton *button = new ObjectButton(this, object);
    addButton(object->getName(), object->getIcon(), button);
}

void ObjectBar::addSeparator() {
    setSize(width() + 4, height());
    objects.append(0);
}

void ObjectBar::addContainer(const ustring &name, ref<YIcon> icon, ObjectMenu *container) {
    if (container) {
        YButton *button = new ObjectButton(this, container);
        addButton(name, icon, button);
    }
}

void ObjectBar::configure(const YRect &r) {
    YWindow::configure(r);


    int left = 0;
    for (int i = 0; i < objects.getCount(); i++) {
        YButton *obj = objects[i];
        if (obj) {
            obj->setGeometry(YRect(left, 0, obj->width(), height()));
            left += obj->width();
        } else
            left += 4;
    }
}

ref<YFont> ObjectButton::getFont() {
    return font != null ? font : font =
        (*toolButtonFontName ? YFont::getFont(XFA(toolButtonFontName))
         : YButton::getFont());
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

ObjectMenu *rootMenu(NULL);

// vim: set sw=4 ts=4 et:
