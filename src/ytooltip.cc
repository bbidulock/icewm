/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "ytooltip.h"

#ifdef CONFIG_TOOLTIP
#include "base.h"
#include "prefs.h"
#include "yprefs.h"

#include <string.h>

YColor *YToolTip::toolTipBg = 0;
YColor *YToolTip::toolTipFg = 0;

ref<YFont> YToolTip::toolTipFont;
YTimer *YToolTip::fToolTipVisibleTimer = 0;

YToolTip::YToolTip(YWindow *aParent): YWindow(aParent), fText(null) {
    if (toolTipBg == 0)
        toolTipBg = new YColor(clrToolTip);
    if (toolTipFg == 0)
        toolTipFg = new YColor(clrToolTipText);
    if (toolTipFont == null)
        toolTipFont = YFont::getFont(XFA(toolTipFontName));

    setStyle(wsOverrideRedirect);
}

YToolTip::~YToolTip() {
    if (fToolTipVisibleTimer) {
        if (fToolTipVisibleTimer->getTimerListener() == this) {
            fToolTipVisibleTimer->setTimerListener(0);
            fToolTipVisibleTimer->stopTimer();
        }
    }
}

void YToolTip::paint(Graphics &g, const YRect &/*r*/) {
    g.setColor(toolTipBg);
    g.fillRect(0, 0, width(), height());
    g.setColor(YColor::black);
    g.drawRect(0, 0, width() - 1, height() - 1);
    if (fText != null) {
        int y = toolTipFont->ascent() + 2;
        g.setFont(toolTipFont);
        g.setColor(toolTipFg);
        g.drawStringMultiline(3, y, fText);
    }
}

void YToolTip::setText(const ustring &tip) {
    fText = tip;
    if (fText != null) {
        YDimension const size(toolTipFont->multilineAlloc(fText));
        setSize(size.w + 6, size.h + 7);

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

    if (!fToolTipVisibleTimer && ToolTipTime > 0)
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
#endif
