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

YScrollView::YScrollView(YWindow *aParent):
    YWindow(aParent),
    scrollable(nullptr),
    scrollVert(new YScrollBar(YScrollBar::Vertical, this)),
    scrollHoriz(new YScrollBar(YScrollBar::Horizontal, this))
{
    scrollVert->show();
    scrollHoriz->show();
    addStyle(wsNoExpose);
    setTitle("ScrollView");
    setBackground(scrollBarBg);
}

YScrollView::~YScrollView() {
    delete scrollVert; scrollVert = nullptr;
    delete scrollHoriz; scrollHoriz = nullptr;
}

void YScrollView::setView(YScrollable *s) {
    scrollable = s;
    scrollVert->raise();
    scrollHoriz->raise();
}

void YScrollView::setListener(YScrollBarListener* l) {
    scrollVert->setScrollBarListener(l);
    scrollHoriz->setScrollBarListener(l);
}

void YScrollView::getGap(int& dx, int& dy) {
    unsigned const cw(scrollable ? scrollable->contentWidth() : 0);
    unsigned const ch(scrollable ? scrollable->contentHeight() : 0);
    dx = (height() < ch || (width() < cw && height() < ch + scrollBarHeight))
        ? scrollBarWidth : 0;
    dy = (width() < cw || (height() < ch && width() < cw + scrollBarWidth))
        ? scrollBarHeight : 0;
}

void YScrollView::layout() {
    int const w(width()), h(height());
    int dx, dy;
    getGap(dx, dy);

    if (dx > 0 && w > dx) {
        scrollVert->show();
        scrollVert->setGeometry(YRect(w - dx, 0, dx, h > dy ? h - dy : 1));
    } else {
        scrollVert->hide();
        dx = 0;
    }
    if (dy > 0 && h > dy) {
        scrollHoriz->show();
        scrollHoriz->setGeometry(YRect(0, h - dy, w > dx ? w - dx : 1, dx));
    } else {
        scrollHoriz->hide();
        dy = 0;
    }
    if (scrollable) {
        YWindow* ywin = dynamic_cast<YWindow *>(scrollable);
        if (ywin) {
            if (w > dx && h > dy) {
                ywin->setGeometry(YRect(0, 0, w - dx, h - dy));
                ywin->show();
            }
        }
    }
}

void YScrollView::configure(const YRect2& r) {
    if (r.resized()) {
        layout();
        repaint();
    }
}

void YScrollView::paint(Graphics &g, const YRect &r) {
    g.setColor(scrollBarBg);
    g.fillRect(r.x(), r.y(), r.width(), r.height());
}

void YScrollView::repaint() {
    GraphicsBuffer(this).paint();
}

// vim: set sw=4 ts=4 et:
