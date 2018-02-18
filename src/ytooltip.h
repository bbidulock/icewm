#ifndef __YTOOLTIP_H
#define __YTOOLTIP_H

#include "ywindow.h"
#include "ytimer.h"
#include "ypointer.h"

class YToolTipWindow: public YWindow {
public:
    YToolTipWindow(ustring text);

    virtual void paint(Graphics &g, const YRect &r);

    void setText(const ustring &tip);
    void locate(YWindow *w);

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

    void setText(const ustring &tip);
    void locate(YWindow *w, const XCrossingEvent &crossing);

    void hide();
    void repaint();
    bool visible();

private:
    void display();
    YToolTipWindow* window();

    ustring fText;
    YWindow* fLocate;
    osmart<YToolTipWindow> fWindow;
    lazy<YTimer> fTimer;
};
#endif

// vim: set sw=4 ts=4 et:
