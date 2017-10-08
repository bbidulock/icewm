#ifndef __YACTIONBUTTON_H
#define __YACTIONBUTTON_H

#include "ybutton.h"
#include "yaction.h"

class YActionButton: public YAction, public YButton {
public:
    YActionButton(YWindow *parent):
        YAction(),
        YButton(parent, YAction(*this))
    {
    }
};

#endif

// vim: set sw=4 ts=4 et:
