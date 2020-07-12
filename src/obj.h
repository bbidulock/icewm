#ifndef __OBJ_H
#define __OBJ_H

#include "mstring.h"

class IApp;
class YIcon;

class DObject {
public:
    DObject(IApp *app, const mstring &name, ref<YIcon> icon);
    virtual ~DObject();

    mstring getName() { return fName; }
    ref<YIcon> getIcon() { return fIcon; }

    virtual void open();

private:
    mstring fName;
    ref<YIcon> fIcon;
protected:
    IApp *app;
};

class ObjectMenu;

class ObjectContainer {
public:
    virtual void addObject(DObject *object) = 0;
    virtual void addSeparator() = 0;
    virtual void addContainer(const mstring &name, ref<YIcon> icon, ObjectMenu *container) = 0;
protected:
    virtual ~ObjectContainer() {}
};


#endif

// vim: set sw=4 ts=4 et:
