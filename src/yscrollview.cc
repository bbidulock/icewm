/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "yscrollview.h"
#include "yscrollbar.h"
#include "prefs.h"

static YColorName scrollBarBg(&clrScrollBar);


YScrollView::YScrollView(YWindow *aParent): YWindow(aParent) {
    scrollVert = new YScrollBar(YScrollBar::Vertical, this);
    scrollVert->show();
    scrollHoriz = new YScrollBar(YScrollBar::Horizontal, this);
    scrollHoriz->show();
    scrollable = 0;
}

YScrollView::~YScrollView() {
    delete scrollVert; scrollVert = 0;
    delete scrollHoriz; scrollHoriz = 0;
}

void YScrollView::setView(YScrollable *s) {
    scrollable = s;
}

void YScrollView::getGap(int &dx, int &dy) {
    unsigned const cw(scrollable->contentWidth());
    unsigned const ch(scrollable->contentHeight());

    ///msg("content %d %d this %d %d", cw, ch, width(), height());
    dx = dy = 0;

    if (width() < cw) {
        dy = scrollBarWidth;
        if (height() - dy < ch)
            dx = scrollBarWidth;
    } else if (height() < ch) {
        dx = scrollBarWidth;
        if (width() - dx < cw)
            dy = scrollBarWidth;
    }
}

void YScrollView::layout() {
    if (!scrollable)   // !!! fix
        return ;

    int const w(width()), h(height());
    int dx, dy;

    getGap(dx, dy);
    ///msg("gap %d %d", dx, dy);

    int sw(max(0, w - dx));
    int sh(max(0, h - dy));

    scrollVert->setGeometry(YRect(w - dx, 0, dx, sh));
    scrollHoriz->setGeometry(YRect(0, h - dy, sw, dy));

    if (dx > w) dx = w;
    if (dy > h) dy = h;

    YWindow *ww = scrollable->getWindow(); //!!!
    ww->setGeometry(YRect(0, 0, w - dx, h - dy));
}

void YScrollView::configure(const YRect &r) {
    YWindow::configure(r);
    layout();
}

void YScrollView::paint(Graphics &g, const YRect &r) {
    int dx, dy;

    getGap(dx, dy);

    g.setColor(scrollBarBg);
    if (dx && dy) g.fillRect(width() - dx, height() - dy, dx, dy);

    YWindow::paint(g, r);
}

// vim: set sw=4 ts=4 et:
