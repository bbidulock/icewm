#ifndef __YDIALOG_H
#define __YDIALOG_H

#include "ywindow.h"

class YDialog: public YWindow {
public:
    YDialog(YWindow *owner = 0);

    void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);
    virtual bool handleKeySym(const XKeyEvent &key, KeySym ksym, int vmod);

    YWindow *getOwner() const { return fOwner; }
private:
    YWindow *fOwner;
};

#endif
