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

YColor * ObjectBar::bgColor(NULL);

YFont * ObjectButton::font(NULL);
YColor * ObjectButton::bgColor(NULL);
YColor * ObjectButton::fgColor(NULL);

YPixmap *toolbuttonPixmap(NULL);

#ifdef CONFIG_GRADIENTS
class YPixbuf *toolbuttonPixbuf(NULL);
#endif

ObjectBar::ObjectBar(YWindow *parent): YWindow(parent) {
    if (bgColor == 0)
        bgColor = new YColor(clrDefaultTaskBar);
    setSize(1, 1);
}

ObjectBar::~ObjectBar() {
}

void ObjectBar::addButton(const char *name, YIcon *icon, YButton *button) {
    button->setToolTip(name);
    if (icon && icon->small()) {
        button->setImage(icon->small());
        button->setSize(button->width() + 4, button->width() + 4);
    } else
        button->setText(name);

    button->setPosition(width(), 0);
    int h = button->height();
    if (h < height())
        h = height();

    if (h < height())
        h = height();

    button->setSize(button->width(), h);
    setSize(width() + button->width() + 1, h);
    button->show();
}

void ObjectBar::paint(Graphics &g, const YRect &/*r*/) {
#ifdef CONFIG_GRADIENTS
    class YPixbuf * gradient(parent()->getGradient());

    if (gradient)
	g.copyPixbuf(*gradient, this->x(), this->y(), width(), height(), 0, 0);
    else 
#endif	    
    if (taskbackPixmap)
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
}

void ObjectBar::addContainer(char *name, YIcon *icon, ObjectContainer *container) {
    if (container) {
        YButton *button = new ObjectButton(this, (ObjectMenu*) container);
        addButton(name, icon, button);
    }
}

YFont * ObjectButton::getFont() {
    return font ? font : font =
        (*toolButtonFontName ? YFont::getFont(toolButtonFontName)
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
