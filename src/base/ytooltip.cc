/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#pragma implementation
#include "config.h"
#include "ytooltip.h"

//#include "base.h"

#include "deffonts.h"

#include "ypaint.h"
#include "ycstring.h"
#include <string.h>


YColorPrefProperty YToolTip::gToolTipBg("system", "ColorToolTip", "rgb:E0/E0/00");
YColorPrefProperty YToolTip::gToolTipFg("system", "ColorToolTipText", "rgb:00/00/00");
YFontPrefProperty YToolTip::gToolTipFont("system", "ToolTipFontName", FONT(120));
YTimer *YToolTip::fToolTipVisibleTimer = 0;

unsigned int YToolTip::ToolTipTime = 5000;

YToolTip::YToolTip(YWindow *aParent):
    YWindow(aParent), fText(0)
{
    setStyle(wsOverrideRedirect);
}

YToolTip::~YToolTip() {
    delete fText; fText = 0;
    if (fToolTipVisibleTimer) {
        if (fToolTipVisibleTimer->getTimerListener() == this) {
            fToolTipVisibleTimer->setTimerListener(0);
            fToolTipVisibleTimer->stopTimer();
        }
    }
}

void YToolTip::paint(Graphics &g, const YRect &/*er*/) {
    g.setColor(gToolTipBg);
    g.fillRect(0, 0, width(), height());
    g.setColor(YColor::black);
    g.drawRect(0, 0, width() - 1, height() - 1);
    if (fText) {
        int y = gToolTipFont.getFont()->ascent() + 2;
        g.setFont(gToolTipFont);
        g.setColor(gToolTipFg);
        g.drawChars(*fText, 0, fText->length(), 3, y);
    }
}

#if 0
void YToolTip::_setText(const char *tip) {
    CStr *s = CStr::newstr(tip);
    setText(s);
    delete s;
}
#endif

void YToolTip::setText(const CStr *tip) {
    delete fText; fText = 0;
    if (tip) {
        fText = tip->clone();
        if (fText) {
            int w = gToolTipFont.getFont()->textWidth(*fText);
            int h = gToolTipFont.getFont()->ascent();

            setSize(w + 6, h + 7);
        } else {
            setSize(20, 20);
        }

        //!!! merge with below code in locate
        int x = this->x();
        int y = this->y();
        if (x + width() >= desktop->width())
            x = desktop->width() - width();
        if (y + height() >= desktop->height())
            y = desktop->height() - height();
        if (y < 0)
            y = 0;
        if (x < 0)
            x = 0;
        setPosition(x, y);
    }
    repaint();
}

bool YToolTip::handleTimer(YTimer *t) {
    if (t == fToolTipVisibleTimer && fToolTipVisibleTimer)
        hide();
    else
        display();
    return false;
}

void YToolTip::display() {
    raise();
    show();
    if (!fToolTipVisibleTimer)
        fToolTipVisibleTimer = new YTimer(this, ToolTipTime);
    if (fToolTipVisibleTimer) {
        fToolTipVisibleTimer->setTimerListener(this);
        fToolTipVisibleTimer->startTimer();
    }
}

void YToolTip::locate(YWindow *w, int /* x_root */, int /* y_root */) {
    int x, y;

    x = w->width() / 2;
    y = w->height();
    w->mapToGlobal(x, y);
    x -= width() / 2;
    if (x + width() >= desktop->width())
        x = desktop->width() - width();
    if (y + height() >= desktop->height())
        y -= height() + w->height();
    if (y < 0)
        y = 0;
    if (x < 0)
        x = 0;
    setPosition(x, y);
}
