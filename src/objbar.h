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

    void configure(const YRect &r);

    virtual void addObject(DObject *object);
    virtual void addSeparator();
    virtual void addContainer(const ustring &name, ref<YIcon> icon, ObjectMenu *container);

    virtual void paint(Graphics &g, const YRect &r);

    void addButton(const ustring &name, ref<YIcon> icon, YButton *button);

private:
    YObjectArray<YButton> objects;
};

#endif

// vim: set sw=4 ts=4 et:
