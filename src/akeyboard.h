#ifndef AKEYBOARD_H
#define AKEYBOARD_H

#include "ywindow.h"
#include "applet.h"
#include "yaction.h"
#include "yicon.h"
#include "ymenu.h"
#include "ypointer.h"

class IAppletContainer;
class IApp;

class KeyboardStatus:
    public IApplet,
    private Picturer,
    private YActionListener
{
public:
    KeyboardStatus(IApp* app, IAppletContainer* taskBar, YWindow *aParent);
    virtual ~KeyboardStatus();

    virtual void actionPerformed(YAction action, unsigned int modifiers);
    virtual void handleClick(const XButtonEvent& up, int count);
    virtual void handleCrossing(const XCrossingEvent& crossing);

    void updateKeyboard(mstring keyboard);
    void getStatus();
    virtual void updateToolTip();

private:
    bool picture();
    mstring detectLayout();
    void fill(Graphics& g);
    void draw(Graphics& g);

    IApp* app;
    IAppletContainer* taskBar;
    mstring fKeyboard;
    mstring fToolTip;
    ref<YIcon> fIcon;
    YFont fFont;
    YColorName fColor;
    osmart<YMenu> fMenu;
    int fIndex;
    bool fInside;
};

#endif

// vim: set sw=4 ts=4 et:
