#ifndef __OBJ_H
#define __OBJ_H

#include "ypaint.h"

class DObject {
public:
    DObject(const ustring &name, YIcon *icon);
    virtual ~DObject();

    ustring getName() { return fName; }
    YIcon *getIcon() { return fIcon; }

    virtual void open();

private:
    ustring fName;
    YIcon *fIcon;
};

class ObjectContainer {
public:
    virtual void addObject(DObject *object) = 0;
    virtual void addSeparator() = 0;
    virtual void addContainer(const ustring &name, YIcon *icon, ObjectContainer *container) = 0;
};


#endif
