#ifndef __WMWINMENU_H
#define __WMWINMENU_H

class WindowListMenu: public YMenu {
public:
    WindowListMenu(YWindow *parent = 0);
    virtual void updatePopup();
};

#endif
