#ifndef __OBJBUTTON_H
#define __OBJBUTTON_H

#include "ybutton.h"
#include "ymenu.h"

class Program;

class ObjectButton: public YButton {
public:
    ObjectButton(YWindow *parent, DObject *object);
    virtual ~ObjectButton() {}

    virtual void actionPerformed(YAction *action, unsigned int modifiers);
private:
    DObject *fObject;
};

#endif
