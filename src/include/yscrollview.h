#ifndef __YSCROLLVIEW_H
#define __YSCROLLVIEW_H

#include "ywindow.h"

#pragma interface

class YScrollBar;

class YScrollable {
public:
    virtual int contentWidth() = 0;
    virtual int contentHeight() = 0;

//#warning "remove scrollable interface?"
    virtual YWindow *getWindow() = 0; // !!! hack ?
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
    virtual void configure(const YRect &cr);

private:
    YScrollable *scrollable;
    YScrollBar *scrollVert;
    YScrollBar *scrollHoriz;
private: // not-used
    YScrollView(const YScrollView &);
    YScrollView &operator=(const YScrollView &);
};

#endif
