/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"

#ifndef LITE

#include "yscrollview.h"

#include "ylistbox.h"
#include "yscrollbar.h"

#include "yapp.h"
#include "prefs.h"

extern YColor *scrollBarBg;


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

    int sw(max(0, w - dx));
    int sh(max(0, h - dy));

    scrollVert->setGeometry(w - dx, 0, dx, sh);
    scrollHoriz->setGeometry(0, h - dy, sw, dy);

    if (dx > w) dx = w;
    if (dy > h) dy = h;

    YWindow *ww = scrollable->getWindow(); //!!! ???
    ww->setGeometry(0, 0, w - dx, h - dy);
}

void YScrollView::configure(const int x, const int y, 
			    const unsigned width, const unsigned height, 
			    const bool resized) {
    YWindow::configure(x, y, width, height, resized);
    if (resized) layout();
}

void YScrollView::paint(Graphics &g, int x, int y, unsigned w, unsigned h) {
    int dx, dy;
    
    getGap(dx, dy);
    
    g.setColor(scrollBarBg);
    if (dx && dy) g.fillRect(width() - dx, height() - dy, dx, dy);

    YWindow::paint(g, x, y, w, h);
}

#endif
