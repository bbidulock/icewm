#ifndef __ROOTMENU_H
#define __ROOTMENU_H

#include "ywindow.h"

class DesktopHandler: public YWindow {
public:
    DesktopHandler();

    virtual void eventClick(const YButtonEvent &up);
};

#endif
