#ifndef __YDIALOG_H
#define __YDIALOG_H

#include "ywindow.h"
#include "ytopwindow.h"
#include "yconfig.h"

#pragma interface

class YDialog: public YTopWindow {
public:
    YDialog(YWindow *owner = 0);

    void paint(Graphics &g, const YRect &er);
    virtual bool eventKey(const YKeyEvent &key);

    YWindow *getOwner() const { return fOwner; }
private:
    YWindow *fOwner;

    static YColorPrefProperty gDialogBg;
    static YPixmapPrefProperty gPixmapBackground;
private: // not-used
    YDialog(const YDialog &);
    YDialog &operator=(const YDialog &);
};

#endif
