#ifndef __OBJBUTTON_H
#define __OBJBUTTON_H

#include "ybutton.h"
#include "ypointer.h"

class DObject;
class LazyMenu;
class YColorName;
class YFont;
class YMenu;

class ObjectButton: public YButton {
public:
    ObjectButton(YWindow *parent, DObject *object):
        YButton(parent, actionNull), fObject(object), fRealized(false)
        { obinit(); }

    ObjectButton(YWindow *parent, YMenu *popup):
        YButton(parent, actionNull, popup), fRealized(false) { obinit(); }

    ObjectButton(YWindow *parent, LazyMenu *menu):
        YButton(parent, actionNull), fMenu(menu), fRealized(false) { obinit(); }

    ObjectButton(YWindow *parent, YAction action):
        YButton(parent, action), fRealized(false) { obinit(); }

    virtual ~ObjectButton() {}

    virtual void actionPerformed(YAction action, unsigned int modifiers);
    virtual void configure(const YRect2 &r);
    virtual void handleExpose(const XExposeEvent &expose) {}
    virtual void paint(Graphics &g, const YRect &r);
    virtual void popupMenu();
    virtual void repaint();
    virtual void requestFocus(bool requestUserFocus);
    virtual void handleButton(const XButtonEvent& up);
    virtual void updateToolTip();

    virtual YFont getFont();
    virtual YColor   getColor();
    virtual YSurface getSurface();
    static void freeFont();
    void realize() { fRealized = true; repaint(); show(); }
    bool realized() const { return fRealized; }

private:
    void obinit() { addStyle(wsNoExpose | wsToolTipping); setParentRelative(); }

    osmart<DObject> fObject;
    osmart<LazyMenu> fMenu;
    bool fRealized;

    static YFont font;
    static YColorName bgColor;
    static YColorName fgColor;
    static ref<YImage> gradients[4];
};

#endif

// vim: set sw=4 ts=4 et:
