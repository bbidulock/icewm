/*
 * IceWM
 *
 * Copyright (C) 1999-2002 Marko Macek
 */
#include "config.h"
#include "objmenu.h"
#include "objbar.h"
#include "objbutton.h"
#include "wmtaskbar.h"
#include "wpixmaps.h"

ObjectBar::ObjectBar(YWindow *parent): YWindow(parent) {
    setParentRelative();
}

ObjectBar::~ObjectBar() {
    ObjectButton::freeFont();
}

void ObjectBar::addButton(mstring name, ref<YIcon> icon, ObjectButton *button) {
    if (icon != null) {
        button->setIcon(icon, YIcon::smallSize());
        button->setSize(button->width() + 4, button->width() + 4);
    } else
        button->setText(name);

    IterType iter(objects.reverseIterator());
    while (++iter && !*iter);
    const int extent(iter ? iter->x() + int(iter->width()) : 0);

    button->setPosition(extent, 0);
    if (button->height() < height())
        button->setSize(button->width(), height());
    objects.append(button);

    setSize(extent + button->width(), height());
    button->show();
}

void ObjectBar::paint(Graphics &g, const YRect& r) {
    if (taskbackPixmap == null && getGradient() == null) {
        g.setColor(taskBarBg);
        g.fillRect(r.x(), r.y(), r.width(), r.height());
    }
}

void ObjectBar::addObject(DObject *object) {
    ObjectButton *button = new ObjectButton(this, object);
    button->setTitle(object->getName().c_str());
    addButton(object->getName(), object->getIcon(), button);
}

void ObjectBar::addSeparator() {
    setSize(int(width()) <= 1 ? 4 : width() + 4, height());
    objects.append(0);
}

void ObjectBar::addContainer(mstring name, ref<YIcon> icon, ObjectMenu *container) {
    if (container) {
        ObjectButton *button = new ObjectButton(this, container);
        button->setTitle(name.c_str());
        addButton(name, icon, button);
    }
}

void ObjectBar::configure(const YRect2& r) {
    if (r.resized()) {
        int left = 0;
        for (IterType obj(objects.iterator()); ++obj; ) {
            if (*obj) {
                if (obj->x() != left || obj->y() != 0) {
                    obj->setPosition(left, 0);
                }
                if (obj->height() < r.height()) {
                    obj->setSize(obj->width(), r.height());
                }
                left += obj->width();
            } else
                left += 4;
        }
    }
}

void ObjectBar::handleExpose(const XExposeEvent& e) {
    if (taskbackPixmap != null || getGradient() != null) {
        clearArea(e.x, e.y, e.width, e.height);
    }
}

void ObjectBar::handleButton(const XButtonEvent& up) {
    parent()->handleButton(up);
}

void ObjectBar::refresh() {
    for (IterType iter(objects.iterator()); ++iter; ) {
        if (*iter) {
            iter->repaint();
        }
    }
}

// vim: set sw=4 ts=4 et:
