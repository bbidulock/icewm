#ifndef __YDIALOG_H
#define __YDIALOG_H

#include "wmclient.h" // !!! broken, should be ywindow

class YDialog: public YFrameClient {
public:
    YDialog(YWindow *owner = 0);

    void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);
    virtual bool handleKey(const XKeyEvent &key);

    YWindow *getOwner() const { return fOwner; }
private:
    YWindow *fOwner;
};

#endif
