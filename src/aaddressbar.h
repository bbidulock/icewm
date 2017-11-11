#ifndef __ADDRBAR_H
#define __ADDRBAR_H

#include "yinputline.h"

class IApp;

class AddressBar: public YInputLine {
public:
    AddressBar(IApp *app, YWindow *parent = 0);
    virtual ~AddressBar();

    virtual bool handleKey(const XKeyEvent &key);

    void showNow();
    void hideNow();
private:
    IApp *app;
};

#endif

// vim: set sw=4 ts=4 et:
