#ifndef __ADDRBAR_H
#define __ADDRBAR_H

#include "yinputline.h"

class AddressBar: public YInputLine {
public:
    AddressBar(YWindow *parent = 0);
    virtual ~AddressBar();

    virtual bool handleKeySym(const XKeyEvent &key, KeySym ksym, int vmod);
};

#endif
