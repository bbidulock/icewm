#ifndef __YACTIONBUTTON_H
#define __YACTIONBUTTON_H

#include "ybutton.h"
#include "yaction.h"

class YActionButton: public YButton {
public:
    YActionButton(YWindow *parent):
        YButton(parent, YAction())
    {
    }

    operator YAction() const { return getAction(); }
};

#endif

// vim: set sw=4 ts=4 et:
