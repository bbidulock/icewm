#ifndef __OBJBUTTON_H
#define __OBJBUTTON_H

#include "ybutton.h"
#include "ymenu.h"

class Program;

class ObjectButton: public YButton {
public:
    ObjectButton(YWindow *parent, DObject *object): 
        YButton(parent, 0), fObject(object) {}
    ObjectButton(YWindow *parent, YMenu *popup):
        YButton(parent, 0, popup), fObject(NULL) {}
    ObjectButton(YWindow *parent, YAction *action):
        YButton(parent, action, 0), fObject(NULL) { /* hack */ }

    virtual ~ObjectButton() {}

    virtual void actionPerformed(YAction *action, unsigned int modifiers);
    virtual ref<YFont> getFont();
    virtual YColor * getColor();
    virtual YSurface getSurface();

private:
    DObject *fObject;

    static ref<YFont> font;
    static YColor *bgColor;
    static YColor *fgColor;
};

extern ref<YPixmap> toolbuttonPixmap;

#ifdef CONFIG_GRADIENTS
extern ref<YImage> toolbuttonPixbuf;
#endif

#endif
