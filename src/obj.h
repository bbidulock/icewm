#ifndef __OBJ_H
#define __OBJ_H

#include "ypaint.h"

class DObject {
public:
    DObject(const char *name, YIcon *icon);
    virtual ~DObject();

    const char *getName() { return fName; }
    YIcon *getIcon() { return fIcon; }

    virtual void open();

private:
    char *fName;
    YIcon *fIcon;
};

class ObjectContainer {
public:
    virtual void addObject(DObject *object) = 0;
    virtual void addSeparator() = 0;
    virtual void addContainer(char *name, YIcon *icon, ObjectContainer *container) = 0;
protected:
    virtual ~ObjectContainer() {};
};


#endif
