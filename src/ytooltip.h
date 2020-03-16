#ifndef YTOOLTIP_H
#define YTOOLTIP_H

#include "ywindow.h"
#include "ytimer.h"
#include "ypointer.h"

class YToolTipWindow: public YWindow {
public:
    YToolTipWindow();

    virtual void paint(Graphics& g, const YRect& r);
    virtual void handleExpose(const XExposeEvent& expose) {}
    virtual void repaint();

    void setText(const ustring& tip);
    void locate(YWindow* w);

private:
    ustring fText;

    YColorName toolTipBg;
    YColorName toolTipFg;
    ref<YFont> toolTipFont;
};

class YToolTip: public YTimerListener {
public:
    YToolTip();

    virtual bool handleTimer(YTimer *t);

    void setText(const ustring& tip);
    void enter(YWindow* w);
    void leave();
    bool visible();

private:
    void expose();
    YToolTipWindow* window();

    ustring fText;
    YWindow* fLocate;
    lazy<YToolTipWindow> fWindow;
    lazy<YTimer> fTimer;
};
#endif

// vim: set sw=4 ts=4 et:
