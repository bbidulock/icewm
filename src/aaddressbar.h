#ifndef __ADDRBAR_H
#define __ADDRBAR_H

#ifdef CONFIG_ADDRESSBAR

#include "yinputline.h"

class AddressBar: public YInputLine {
public:
    AddressBar(YWindow *parent = 0);
    virtual ~AddressBar();

    virtual bool handleKey(const XKeyEvent &key);

    void showNow();
    void hideNow();
};

#endif

#endif
