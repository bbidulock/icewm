#ifndef __ADDRBAR_H
#define __ADDRBAR_H

#include "yinputline.h"

class IApp;

class AddressBar: public YInputLine {
public:
    AddressBar(IApp *app, YWindow *parent = nullptr);
    virtual ~AddressBar();

    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleFocus(const XFocusChangeEvent &focus);

    void showNow();
    void hideNow();

private:
    void changeLocation(int newLocation);
    bool handleReturn(int mask);
    bool appendCommand(const char* cmd, class YStringArray& args);

    IApp *app;
    MStringArray history;
    int location;
};

#endif

// vim: set sw=4 ts=4 et:
