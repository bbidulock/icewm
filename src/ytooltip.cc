/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "ytooltip.h"

#include "base.h"
#include "prefs.h"
#include "yprefs.h"
#include "yrect.h"

#include <string.h>

enum ToolTipMargins {
    TTXMargin = 5,
    TTYMargin = 3,
};

YToolTipWindow::YToolTipWindow() :
    toolTipBg(&clrToolTip),
    toolTipFg(&clrToolTipText),
    toolTipFont(YFont::getFont(XFA(toolTipFontName)))
{
    setStyle(wsToolTip | wsOverrideRedirect | wsSaveUnder | wsNoExpose);
    setNetWindowType(_XA_NET_WM_WINDOW_TYPE_TOOLTIP);
    setClassHint("tooltip", "IceWM");
    setTitle("Tooltip");
}

YToolTip::YToolTip() :
    fLocate(nullptr)
{
}

void YToolTipWindow::repaint() {
    GraphicsBuffer(this).paint();
}

void YToolTipWindow::paint(Graphics& g, const YRect& /*r*/) {
    g.setColor(toolTipBg);
    g.fillRect(0, 0, width(), height());
    g.setColor(YColor::black);
    g.drawRect(0, 0, width() - 1, height() - 1);
    if (fText != null) {
        int y = toolTipFont->ascent() + TTYMargin;
        g.setFont(toolTipFont);
        g.setColor(toolTipFg);
        g.drawStringMultiline(TTXMargin, y, fText);
    }
}

void YToolTipWindow::setText(const mstring& tip) {
    fText = tip;
    YDimension size(toolTipFont->multilineAlloc(fText));
    setSize(size.w + 2 * TTXMargin, size.h + 3 + 2 * TTYMargin);
}

void YToolTip::setText(const mstring& tip) {
    fText = tip;
    if (fWindow) {
        fWindow->setText(tip);
        fWindow->locate(fLocate);
        fWindow->repaint();
    }
}

bool YToolTip::visible() {
    return fWindow;
}

bool YToolTip::handleTimer(YTimer *timer) {
    if (timer == fTimer) {
        if (fWindow) {
            fWindow = null;
        }
        else {
            fWindow->setText(fText);
            fWindow->locate(fLocate);
            fWindow->repaint();
            fWindow->show();
            if (ToolTipTime)
                fTimer->setTimer(ToolTipTime, this, true);
        }
    }
    return false;
}

void YToolTip::enter(YWindow* client) {
    fLocate = client;
    fTimer->setTimer(ToolTipDelay, this, true);
}

void YToolTip::leave() {
    fWindow = null;
    fTimer = null;
}

void YToolTipWindow::locate(YWindow *wfor) {
    int x = wfor->x(), y = wfor->y();
    for (YWindow* parent = wfor->parent(); parent; parent = parent->parent()) {
        if (parent == desktop || hasbit(parent->getStyle(), wsManager)) {
            break;
        } else {
            x += parent->x();
            y += parent->y();
        }
    }
    int screen = desktop->getScreenForRect(x, y, wfor->width(), wfor->height());
    YRect scgeo(desktop->getScreenGeometry(screen));
    int xbest = x + int(wfor->width() / 2) - int(width() / 2);
    xbest = clamp(xbest, scgeo.x(), scgeo.x() + int(scgeo.width() - width()));
    YRect above(xbest, y - int(height()), width(), height());
    YRect below(xbest, y + int(wfor->height()), width(), height());
    unsigned upper(above.overlap(scgeo));
    unsigned lower(below.overlap(scgeo));
    if (lower < upper) {
        setPosition(above.x(), above.y());
    } else {
        setPosition(below.x(), below.y());
    }
}

// vim: set sw=4 ts=4 et:
