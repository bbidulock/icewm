#ifndef __ROOTMENU_H
#define __ROOTMENU_H

#include "ywindow.h"

#include "objmenu.h"

class DesktopHandler: public YWindow {
public:
    DesktopHandler();

    virtual void handleClick(const XButtonEvent &up, int count);

    ObjectMenu *getRootMenu();
private:
    ObjectMenu *fRootMenu;
private: // not-used
    DesktopHandler(const DesktopHandler &);
    DesktopHandler &operator=(const DesktopHandler &);
};

#endif
