#ifndef __WMWINMENU_H
#define __WMWINMENU_H

class WindowListMenu: public YMenu {
public:
    WindowListMenu(IApp *app, YWindow *parent = 0);
    virtual void updatePopup();
private:
    IApp *app;
};

#endif

// vim: set sw=4 ts=4 et:
