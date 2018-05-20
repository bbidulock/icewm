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

YToolTipWindow::YToolTipWindow(ustring text) :
    toolTipBg(&clrToolTip),
    toolTipFg(&clrToolTipText),
    toolTipFont(YFont::getFont(XFA(toolTipFontName)))
{
    setStyle(wsToolTip | wsOverrideRedirect | wsSaveUnder);
    setText(text);
}

YToolTip::YToolTip() :
    fLocate(0)
{
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

bool YToolTip::handleTimer(YTimer *timer) {
    timer == fTimer ? hide() : display();
    return false;
}

YToolTipWindow* YToolTip::window() {
    if (fWindow == 0) {
        fWindow = new YToolTipWindow(fText);
        if (fLocate)
            fWindow->locate(fLocate);
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

void YToolTipWindow::locate(YWindow *w) {
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

// vim: set sw=4 ts=4 et:
