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

void YScrollView::layout() {
    if (!scrollable)   // !!! fix
        return ;
    int cw = scrollable->contentWidth();
    int ch = scrollable->contentHeight();
    int dx = 0;
    int dy = 0;
    int w = width();
    int h = height();

    if (w - dx < cw) {
        dy = SCROLLBAR_SIZE;
        if (h - dy < ch)
            dx = SCROLLBAR_SIZE;
    } else if (h - dy < ch) {
        dx = SCROLLBAR_SIZE;
        if (w - dx < cw)
            dy = SCROLLBAR_SIZE;
    }
    int sw = w - dx;
    int sh = h - dy;
    if (sw < 0) sw = 0;
    if (sh < 0) sh = 0;
    scrollVert->setGeometry(w - dx, 0, dx, sh);
    scrollHoriz->setGeometry(0, h - dy, sw, dy);
    if (dx > w) dx = w;
    if (dy > h) dy = h;
    YWindow *ww = scrollable->getWindow(); //!!! ???
    ww->setGeometry(0, 0, w - dx, h - dy);
}

void YScrollView::configure(int x, int y, unsigned int width, unsigned int height) {
    YWindow::configure(x, y, width, height);
    layout();
}
#endif
