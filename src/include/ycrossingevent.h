#ifndef __YCROSSINGEVENT_H
#define __YCROSSINGEVENT_H

#include "ywindowevent.h"

class YCrossingEvent: public YWindowEvent {
public:
    YCrossingEvent(): YWindowEvent() {}

    YCrossingEvent(int aType, int x, int y, int x_root, int y_root, int modifiers, long time, long window):
        YWindowEvent(aType, modifiers, time, window),
        fX(x), fY(y), fXroot(x_root), fYroot(y_root)
    {
    }

    int x() const { return fX; }
    int y() const { return fY; }
    int x_root() const { return fXroot; }
    int y_root() const { return fYroot; }

private:
    int fX;
    int fY;
    int fXroot;
    int fYroot;
};

#endif
