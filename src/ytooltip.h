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

    void setText(mstring tip, ref<YIcon> icon);
    void locate(YWindow* w);

private:
    mstring fText;
    ref<YIcon> fIcon;

    YColorName toolTipBg;
    YColorName toolTipFg;
    YFont toolTipFont;
};

class YToolTip: public YTimerListener {
public:
    YToolTip();

    virtual bool handleTimer(YTimer *t);

    void setText(mstring tip, ref<YIcon> icon);
    void enter(YWindow* w);
    void leave();
    bool visible() const;
    bool nonempty() const;

private:
    void expose();
    YToolTipWindow* window();

    mstring fText;
    YWindow* fLocate;
    ref<YIcon> fIcon;
    lazy<YToolTipWindow> fWindow;
    lazy<YTimer> fTimer;
};
#endif

// vim: set sw=4 ts=4 et:
