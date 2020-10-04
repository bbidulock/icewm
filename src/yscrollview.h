#ifndef __YSCROLLVIEW_H
#define __YSCROLLVIEW_H

#include "ywindow.h"

class YScrollBar;

class YScrollable {
public:
    virtual unsigned contentWidth() = 0;
    virtual unsigned contentHeight() = 0;

    virtual YWindow *getWindow() = 0; // !!! hack ?
protected:
    virtual ~YScrollable() =default;
};

class YScrollView: public YWindow {
public:
    YScrollView(YWindow *aParent);

    void setView(YScrollable *l);

    YScrollBar *getVerticalScrollBar() { return scrollVert; }
    YScrollBar *getHorizontalScrollBar() { return scrollHoriz; }
    YScrollable *getScrollable() { return scrollable; }

    void layout();
    virtual void configure(const YRect2 &r);
    virtual void paint(Graphics &g, const YRect &r);
    virtual void repaint();
    virtual void handleExpose(const XExposeEvent& expose) {}

protected:
    void getGap(int &dx, int &dy);

private:
    YScrollable* scrollable;
    osmart<YScrollBar> scrollVert;
    osmart<YScrollBar> scrollHoriz;
};

#endif

// vim: set sw=4 ts=4 et:
