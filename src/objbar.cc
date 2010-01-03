/*
 * IceWM
 *
 * Copyright (C) 1999-2002 Marko Macek
 */
#include "config.h"

#ifndef NO_CONFIGURE_MENUS
#include "objmenu.h"
#endif

#ifdef CONFIG_TASKBAR
#include "objbar.h"
#include "objbutton.h"
#include "ybutton.h"
#include "prefs.h"
#include "wmtaskbar.h"
#include "wmapp.h"
#include "yrect.h"
#include "yicon.h"

YColor * ObjectBar::bgColor(NULL);

ref<YFont> ObjectButton::font;
YColor * ObjectButton::bgColor(NULL);
YColor * ObjectButton::fgColor(NULL);

ref<YPixmap> toolbuttonPixmap;

#ifdef CONFIG_GRADIENTS
ref<YImage> toolbuttonPixbuf;
#endif

ObjectBar::ObjectBar(YWindow *parent): YWindow(parent) {
    if (bgColor == 0)
        bgColor = new YColor(clrDefaultTaskBar);
    setSize(1, 1);
}

ObjectBar::~ObjectBar() {
}

void ObjectBar::addButton(const ustring &name, ref<YIcon> icon, YButton *button) {
    button->setToolTip(name);
#ifndef LITE
    if (icon != null) {
        button->setIcon(icon, YIcon::smallSize());
        button->setSize(button->width() + 4, button->width() + 4);
    } else
#endif
        button->setText(name);

    button->setPosition(width(), 0);
    int h = button->height();
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
#ifdef CONFIG_GRADIENTS
    ref<YImage> gradient(parent()->getGradient());

    if (gradient != null)
        g.drawImage(gradient, this->x(), this->y(), width(), height(), 0, 0);
    else
#endif
        if (taskbackPixmap != null)
            g.fillPixmap(taskbackPixmap, 0, 0, width(), height());
        else {
            g.setColor(bgColor);
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

void ObjectBar::addContainer(const ustring &name, ref<YIcon> icon, ObjectContainer *container) {
    if (container) {
        YButton *button = new ObjectButton(this, (ObjectMenu*) container);
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

YColor * ObjectButton::getColor() {
    return *clrToolButtonText
        ? fgColor ? fgColor : fgColor = new YColor(clrToolButtonText)
        : YButton::getColor();
}

YSurface ObjectButton::getSurface() {
    if (bgColor == 0)
        bgColor = new YColor(*clrToolButton ? clrToolButton : clrNormalButton);

#ifdef CONFIG_GRADIENTS
    return YSurface(bgColor, toolbuttonPixmap, toolbuttonPixbuf);
#else
    return YSurface(bgColor, toolbuttonPixmap);
#endif
}

void ObjectButton::actionPerformed(YAction * action, unsigned modifiers) {
#ifdef CONFIG_GUIEVENTS
    wmapp->signalGuiEvent(geLaunchApp);
#endif
    if (fObject) fObject->open();
    else YButton::actionPerformed(action, modifiers);
}

#endif /* CONFIG_TASKBAR */

#ifndef NO_CONFIGURE_MENUS
ObjectMenu *rootMenu(NULL);
#endif
