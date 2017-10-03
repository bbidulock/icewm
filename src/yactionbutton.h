#ifndef __YACTIONBUTTON_H
#define __YACTIONBUTTON_H

#include "ybutton.h"
#include "yaction.h"

class YActionButton: public YButton {
public:
    YActionButton(YWindow *parent): YButton(parent, (tActionId) this) {
    }
};

#endif

// vim: set sw=4 tw=4 et:
