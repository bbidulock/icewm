#ifndef __YACTIONBUTTON_H
#define __YACTIONBUTTON_H

#include "ybutton.h"
#include "yaction.h"

//#warning "add #pragma implementation"
//#pragma interface

class YActionButton: public YButton, public YAction {
public:
    YActionButton(YWindow *parent): YButton(parent, this) {
    }
};

#endif
