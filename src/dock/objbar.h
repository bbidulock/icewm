#ifndef __OBJBAR_H
#define __OBJBAR_H

#include "ywindow.h"
#include "ybutton.h"
#include "obj.h"

class Program;

class ObjectBar: public YWindow, public ObjectContainer {
public:
    ObjectBar(YWindow *parent);
    virtual ~ObjectBar();

    virtual void addObject(DObject *object);
    virtual void addSeparator();
    virtual void addContainer(char *name, YIcon *icon, ObjectContainer *container);

    virtual void paint(Graphics &g, const YRect &er);

    void __addButton(const char *name, YIcon *icon, YButton *button);
};

#endif
