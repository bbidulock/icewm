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

YScrollView::YScrollView(YWindow *aParent, YScrollable* scroll):
    YWindow(aParent),
    scrollable(scroll),
    scrollVert(new YScrollBar(YScrollBar::Vertical, this)),
    scrollHoriz(new YScrollBar(YScrollBar::Horizontal, this))
{
    addStyle(wsNoExpose);
    setTitle("ScrollView");
    setBackground(scrollBarBg);
}

void YScrollView::setView(YScrollable *s) {
    scrollable = s;
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
        scrollVert->setGeometry(YRect(w - dx, 0, dx, h > dy ? h - dy : 1));
        scrollVert->enable();
    } else {
        scrollVert->hide();
        dx = 0;
    }
    if (dy > 0 && h > dy) {
        scrollHoriz->setGeometry(YRect(0, h - dy, w > dx ? w - dx : 1, dx));
        scrollHoriz->enable();
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
    }
}

bool YScrollView::handleScrollKeys(const XKeyEvent& key) {
    return scrollVert->handleScrollKeys(key)
        || scrollHoriz->handleScrollKeys(key);
}

// vim: set sw=4 ts=4 et:
