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

    virtual ~ObjectButton() {}

    virtual void actionPerformed(YAction *action, unsigned int modifiers);
    virtual void handleClick(const XButtonEvent &up, int count);

    virtual YFont * getFont();
    virtual YColor * getColor();
    virtual YSurface getSurface();

private:
    DObject *fObject;

    static YFont *font;
    static YColor *bgColor;
    static YColor *fgColor;
};

extern YPixmap *toolbuttonPixmap;

#ifdef CONFIG_GRADIENTS
extern class YPixbuf *toolbuttonPixbuf;
#endif

#endif
