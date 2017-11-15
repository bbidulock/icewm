#ifndef __OBJMENU_H
#define __OBJMENU_H

#include "ymenu.h"
#include "ymenuitem.h"
#include "yaction.h"

#include "obj.h"

class DObject;
class YSMListener;

class DObjectMenuItem: public YMenuItem {
public:
    DObjectMenuItem(DObject *object);
    virtual ~DObjectMenuItem();

    virtual void actionPerformed(YActionListener *listener, YAction action, unsigned int modifiers);
private:
    DObject *fObject;
};

class ObjectMenu: public YMenu, public ObjectContainer {
public:
    ObjectMenu(YActionListener *wmActionListener, YWindow *parent = 0);
    virtual ~ObjectMenu();

    virtual void addObject(DObject *object);
    virtual void addObject(DObject *object, const char *icons);
    virtual void addSeparator();
    virtual void addContainer(const ustring &name, ref<YIcon> icon, ObjectMenu *container);
protected:
    YActionListener *wmActionListener;
};

#endif

// vim: set sw=4 ts=4 et:
