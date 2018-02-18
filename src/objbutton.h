#ifndef __OBJBUTTON_H
#define __OBJBUTTON_H

#include "ybutton.h"
#include "ymenu.h"
#include "ypointer.h"

class Program;

class ObjectButton: public YButton {
public:
    ObjectButton(YWindow *parent, DObject *object):
        YButton(parent, actionNull), fObject(object) {}
    ObjectButton(YWindow *parent, YMenu *popup):
        YButton(parent, actionNull, popup), fObject(NULL) {}
    ObjectButton(YWindow *parent, YAction action):
        YButton(parent, action, 0), fObject(NULL) { /* hack */ }

    virtual ~ObjectButton() {}

    virtual void actionPerformed(YAction action, unsigned int modifiers);
    virtual ref<YFont> getFont();
    virtual YColor   getColor();
    virtual YSurface getSurface();

private:
    osmart<DObject> fObject;

    static ref<YFont> font;
    static YColorName bgColor;
    static YColorName fgColor;
};

#endif

// vim: set sw=4 ts=4 et:
