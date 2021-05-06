#ifndef YSCROLLVIEW_H
#define YSCROLLVIEW_H

#include "ywindow.h"
#include "ypointer.h"

class YScrollBar;
class YScrollBarListener;

class YScrollable {
public:
    virtual unsigned contentWidth() = 0;
    virtual unsigned contentHeight() = 0;
protected:
    virtual ~YScrollable() {}
};

class YScrollView: public YWindow {
public:
    YScrollView(YWindow* aParent, YScrollable* scroll = nullptr);

    void setView(YScrollable* l);
    void setListener(YScrollBarListener* l);

    YScrollBar* getVerticalScrollBar() { return scrollVert; }
    YScrollBar* getHorizontalScrollBar() { return scrollHoriz; }
    YScrollable* getScrollable() { return scrollable; }

    void layout();
    void configure(const YRect2& r) override ;
    void paint(Graphics& g, const YRect& r) override { }
    void repaint() override { }
    void handleExpose(const XExposeEvent& expose) override {}
    bool handleScrollKeys(const XKeyEvent& key);

protected:
    void getGap(int& dx, int& dy);

private:
    YScrollable* scrollable;
    osmart<YScrollBar> scrollVert;
    osmart<YScrollBar> scrollHoriz;
};

#endif

// vim: set sw=4 ts=4 et:
