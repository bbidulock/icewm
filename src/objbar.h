#ifndef OBJBAR_H
#define OBJBAR_H

#include "objmenu.h"

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
    virtual void handleButton(const XButtonEvent& up);

    virtual void addObject(DObject *object);
    virtual void addSeparator();
    virtual void addContainer(mstring name, ref<YIcon> icon, ObjectMenu *container);

    virtual void paint(Graphics &g, const YRect &r);

    void addButton(mstring name, ref<YIcon> icon, ObjectButton *button);
    void refresh();
    bool nonempty() const { return objects.nonempty(); }

private:
    ArrayType objects;
};

#endif

// vim: set sw=4 ts=4 et:
