#ifndef __YSCROLLVIEW_H
#define __YSCROLLVIEW_H

#include "ywindow.h"

class YScrollBar;

class YScrollable {
public:
    virtual int contentWidth() = 0;
    virtual int contentHeight() = 0;

    virtual YWindow *getWindow() = 0; // !!! hack ?
protected:
    virtual ~YScrollable() {};
};

class YScrollView: public YWindow {
public:
    YScrollView(YWindow *aParent);
    virtual ~YScrollView();

    void setView(YScrollable *l);

    YScrollBar *getVerticalScrollBar() { return scrollVert; }
    YScrollBar *getHorizontalScrollBar() { return scrollHoriz; }
    YScrollable *getScrollable() { return scrollable; }
    
    void layout();
    virtual void configure(const YRect &r,
                           const bool resized);
    virtual void paint(Graphics &g, const YRect &r);

protected:
    void getGap(int &dx, int &dy);

private:
    YScrollable *scrollable;
    YScrollBar *scrollVert;
    YScrollBar *scrollHoriz;
};

#endif
