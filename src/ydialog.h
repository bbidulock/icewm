#ifndef YDIALOG_H
#define YDIALOG_H

#include "wmclient.h"

class YDialog: public YFrameClient {
public:
    YDialog();
    virtual ~YDialog();

    virtual void paint(Graphics &g, const YRect &r);
    virtual void repaint();
    virtual void configure(const YRect2& r2);
    virtual void handleExpose(const XExposeEvent &expose) {}
    virtual bool handleKey(const XKeyEvent &key);
    virtual ref<YImage> getGradient();

private:
    ref<YImage> fGradient;
    YSurface fSurface;

    const YSurface& getSurface();
};

#endif

// vim: set sw=4 ts=4 et:
