#ifndef __YDIALOG_H
#define __YDIALOG_H

#include "ywindow.h"
#include "yconfig.h"

class YDialog: public YWindow {
public:
    YDialog(YWindow *owner = 0);

    void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);
    virtual bool handleKeySym(const XKeyEvent &key, KeySym ksym, int vmod);

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
