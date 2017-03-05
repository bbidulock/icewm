#ifndef __OBJMENU_H
#define __OBJMENU_H

#include "ymenu.h"
#include "ymenuitem.h"
#include "yaction.h"

#include "obj.h"

class DObject;
class YSMListener;

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
    ObjectMenu(YActionListener *wmActionListener, YWindow *parent = 0);
    virtual ~ObjectMenu();

    virtual void addObject(DObject *object);
#ifndef LITE
    virtual void addObject(DObject *object, const char *icons);
#endif
    virtual void addSeparator();
    virtual void addContainer(const ustring &name, ref<YIcon> icon, ObjectContainer *container);
protected: 
    YActionListener *wmActionListener;
};

#endif
