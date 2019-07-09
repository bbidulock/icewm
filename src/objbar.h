#ifndef __OBJBAR_H
#define __OBJBAR_H

#include "ywindow.h"
#include "ybutton.h"
#include "obj.h"

class ObjectButton;

class ObjectBar: public YWindow, public ObjectContainer {
    typedef YObjectArray<ObjectButton> ArrayType;
    typedef ArrayType::IterType IterType;

public:
    ObjectBar(YWindow *parent);
    virtual ~ObjectBar();

    virtual void configure(const YRect2 &r);
    virtual void handleExpose(const XExposeEvent& expose);
    virtual void repaint() {}

    virtual void addObject(DObject *object);
    virtual void addSeparator();
    virtual void addContainer(const ustring &name, ref<YIcon> icon, ObjectMenu *container);

    virtual void paint(Graphics &g, const YRect &r);

    void addButton(const ustring &name, ref<YIcon> icon, ObjectButton *button);

private:
    ArrayType objects;
};

#endif

// vim: set sw=4 ts=4 et:
