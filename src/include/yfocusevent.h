#ifndef __YFOCUSEVENT_H
#define __YFOCUSEVENT_H

#include "ywindowevent.h"

class YFocusEvent: public YWindowEvent {
public:
    YFocusEvent(): YWindowEvent() {}

    YFocusEvent(int aType, int modifiers, long time, long window):
        YWindowEvent(aType, modifiers, time, window)
    {
    }
private:
};

#endif
