#ifndef __WMWINMENU_H
#define __WMWINMENU_H

class WindowListMenu: public YMenu {
    typedef YMenu super;
public:
    WindowListMenu(YActionListener *app, YWindow *parent = nullptr);
    virtual void updatePopup();
    virtual void activatePopup(int flags);
};

#endif

// vim: set sw=4 ts=4 et:
