#ifndef __YDIALOG_H
#define __YDIALOG_H

#include "wmclient.h" // !!! broken, should be ywindow

class YDialog: public YFrameClient {
public:
    YDialog(YWindow *owner = 0);
    virtual ~YDialog();

    void paint(Graphics &g, const YRect &r);
    virtual bool handleKey(const XKeyEvent &key);

#ifdef CONFIG_GRADIENTS
    virtual ref<YImage> getGradient() const { return fGradient; }
#endif
    YWindow *getOwner() const { return fOwner; }

private:
    YWindow *fOwner;

#ifdef CONFIG_GRADIENTS
    ref<YImage> fGradient;
#endif
};

extern ref<YPixmap> dialogbackPixmap;

#ifdef CONFIG_GRADIENTS
extern ref<YImage> dialogbackPixbuf;
#endif

#endif
