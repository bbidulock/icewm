#ifndef __YDIALOG_H
#define __YDIALOG_H

#include "wmclient.h" // !!! broken, should be ywindow

class YDialog: public YFrameClient {
public:
    YDialog(YWindow *owner = 0);
    virtual ~YDialog();

    void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);
    virtual bool handleKey(const XKeyEvent &key);

#ifdef CONFIG_GRADIENTS
    virtual class YPixbuf * getGradient() const { return fGradient; }
#endif
    YWindow *getOwner() const { return fOwner; }

private:
    YWindow *fOwner;

#ifdef CONFIG_GRADIENTS
    class YPixbuf * fGradient;
#endif
};

extern YPixmap * dialogbackPixmap;

#ifdef CONFIG_GRADIENTS
extern class YPixbuf * dialogbackPixbuf;
#endif

#endif
