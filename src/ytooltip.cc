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
    fLocate(0)
{
}

void YToolTipWindow::configure(const YRect2& r) {
    if (r.resized()) {
        repaint();
    }
}

void YToolTipWindow::repaint() {
    GraphicsBuffer(this).paint();
}

void YToolTipWindow::paint(Graphics &g, const YRect &/*r*/) {
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

void YToolTip::setText(const ustring &tip) {
    fText = tip;
    if (fWindow)
        fWindow->setText(tip);
}

bool YToolTip::visible() {
    return fWindow && fWindow->visible();
}

void YToolTip::repaint() {
    if (visible())
        fWindow->repaint();
}

void YToolTipWindow::setText(const ustring &tip) {
    fText = tip;
    if (fText != null) {
        YDimension const size(toolTipFont->multilineAlloc(fText));
        setSize(size.w + 2 * TTXMargin, size.h + 3 + 2 * TTYMargin);

        //!!! merge with below code in locate
        int screen = desktop->getScreenForRect(x(), y(), width(), height());
        YRect geo(desktop->getScreenGeometry(screen));
        int xpos = clamp(x(), geo.x(), int(geo.width() - width()));
        int ypos = clamp(y(), geo.y(), int(geo.height() - height()));
        setPosition(xpos, ypos);
    }
    repaint();
}

bool YToolTip::handleTimer(YTimer *timer) {
    timer == fTimer ? hide() : display();
    return false;
}

YToolTipWindow* YToolTip::window() {
    if (fWindow == nullptr) {
        fWindow = new YToolTipWindow();
        if (fWindow) {
            fWindow->setText(fText);
            if (fLocate)
                fWindow->locate(fLocate);
        }
    }
    return fWindow;
}

void YToolTip::display() {
    window()->raise();
    window()->show();
    if (ToolTipTime)
        fTimer->setTimer(ToolTipTime, this, true);
}

void YToolTip::hide() {
    fWindow = 0;
    fTimer = null;
}

void YToolTip::locate(YWindow *w, const XCrossingEvent &/*crossing*/) {
    fLocate = w;
    if (fWindow)
        fWindow->locate(fLocate);
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
