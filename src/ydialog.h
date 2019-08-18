#ifndef __YDIALOG_H
#define __YDIALOG_H

#include "wmclient.h" // !!! broken, should be ywindow

class YDialog: public YFrameClient {
public:
    YDialog(YWindow *owner = 0);
    virtual ~YDialog();

    virtual void paint(Graphics &g, const YRect &r);
    virtual void repaint();
    virtual void configure(const YRect2& r2);
    virtual void handleExpose(const XExposeEvent &expose) {}
    virtual bool handleKey(const XKeyEvent &key);

    virtual ref<YImage> getGradient() const { return fGradient; }
    YWindow *getOwner() const { return fOwner; }

private:
    YWindow *fOwner;

    ref<YImage> fGradient;
    YSurface fSurface;

    const YSurface& getSurface() const { return fSurface; }
};

#endif

// vim: set sw=4 ts=4 et:
