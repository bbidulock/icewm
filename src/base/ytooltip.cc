/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#include "config.h"
#include "ytooltip.h"

#include "base.h"
#include "prefs.h"

#include <string.h>

YColor *YToolTip::toolTipBg = 0;
YColor *YToolTip::toolTipFg = 0;

YFont *YToolTip::toolTipFont = 0;
YTimer *YToolTip::fToolTipVisibleTimer = 0;

unsigned int YToolTip::ToolTipTime = 5000;

YToolTip::YToolTip(YWindow *aParent): YWindow(aParent) {
    if (toolTipBg == 0)
        toolTipBg = new YColor(clrToolTip);
    if (toolTipFg == 0)
        toolTipFg = new YColor(clrToolTipText);
    if (toolTipFont == 0)
        toolTipFont = YFont::getFont(toolTipFontName);

    fText = 0;
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

void YToolTip::paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
    g.setColor(toolTipBg);
    g.fillRect(0, 0, width(), height());
    g.setColor(YColor::black);
    g.drawRect(0, 0, width() - 1, height() - 1);
    if (fText) {
        int y = toolTipFont->ascent() + 2;
        g.setFont(toolTipFont);
        g.setColor(toolTipFg);
        g.drawChars(fText, 0, strlen(fText), 3, y);
    }
}

void YToolTip::setText(const char *tip) {
    delete fText; fText = 0;
    if (tip) {
        fText = newstr(tip);
        if (fText) {
            int w = toolTipFont->textWidth(fText);
            int h = toolTipFont->ascent();

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
        fToolTipVisibleTimer = new YTimer(ToolTipTime);
    if (fToolTipVisibleTimer) {
        fToolTipVisibleTimer->setTimerListener(this);
        fToolTipVisibleTimer->startTimer();
    }
}

void YToolTip::locate(YWindow *w, const XCrossingEvent &/*crossing*/) {
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
