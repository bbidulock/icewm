#ifndef __YACTIONBUTTON_H
#define __YACTIONBUTTON_H

#include "ybutton.h"
#include "yaction.h"

class YActionButton: public YButton, public YAction {
public:
    YActionButton(YWindow *parent): YButton(parent, this) {
    }
};

#endif
