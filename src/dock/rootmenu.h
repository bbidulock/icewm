#ifndef __ROOTMENU_H
#define __ROOTMENU_H

#include "ywindow.h"

#include "objmenu.h"

class DesktopHandler: public YWindow {
public:
    DesktopHandler();

    virtual bool eventClick(const YClickEvent &up);

    ObjectMenu *getRootMenu();
private:
    ObjectMenu *fRootMenu;
private: // not-used
    DesktopHandler(const DesktopHandler &);
    DesktopHandler &operator=(const DesktopHandler &);
};

#endif
