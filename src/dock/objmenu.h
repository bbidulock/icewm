#ifndef __OBJMENU_H
#define __OBJMENU_H

#include "ymenu.h"
#include "ymenuitem.h"
#include "yaction.h"

#include "obj.h"

class DObject;

class DObjectMenuItem: public YMenuItem, public YAction {
public:
    DObjectMenuItem(DObject *object);
    virtual ~DObjectMenuItem();

    virtual void actionPerformed(YActionListener *listener, YAction *action, unsigned int modifiers);
private:
    DObject *fObject;
};

class ObjectMenu: public YMenu, public ObjectContainer {
public:
    ObjectMenu(YWindow *parent = 0);
    virtual ~ObjectMenu();

    virtual void addObject(DObject *object);
    virtual void addSeparator();
    virtual void addContainer(char *name, YIcon *icon, ObjectContainer *container);
};

#endif
