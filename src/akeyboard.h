#ifndef AKEYBOARD_H
#define AKEYBOARD_H

#include "ywindow.h"
#include "applet.h"
#include "yaction.h"
#include "yicon.h"
#include "ymenu.h"
#include "ypointer.h"

class IAppletContainer;

class KeyboardStatus:
    public IApplet,
    private Picturer,
    private YActionListener
{
public:
    KeyboardStatus(IAppletContainer* taskBar, YWindow *aParent);
    virtual ~KeyboardStatus();

    virtual void actionPerformed(YAction action, unsigned int modifiers);
    virtual void handleClick(const XButtonEvent& up, int count);

    void updateKeyboard(mstring keyboard);
    void getStatus();
    virtual void updateToolTip();

private:
    bool picture();
    void fill(Graphics& g);
    void draw(Graphics& g);

    IAppletContainer* taskBar;
    mstring fKeyboard;
    ref<YIcon> fIcon;
    ref<YFont> fFont;
    YColorName fColor;
    osmart<YMenu> fMenu;
    int fIndex;
};

#endif

// vim: set sw=4 ts=4 et:
