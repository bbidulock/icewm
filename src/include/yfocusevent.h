#ifndef __YFOCUSEVENT_H
#define __YFOCUSEVENT_H

#include "yevent.h"

class YFocusEvent: public YEvent {
public:
    YFocusEvent(): YEvent() {}

    YFocusEvent(int aType, long window):
        YEvent(aType), fWindow(window)
    {
    }
    unsigned long getWindow() const { return fWindow; }
private:
    long fWindow;
};

#endif
