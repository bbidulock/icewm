#ifndef __YBUTTONEVENT_H
#define __YBUTTONEVENT_H

#include "ywindowevent.h"

class YButtonEvent: public YWindowEvent {
public:
    YButtonEvent(): YWindowEvent() {}
    YButtonEvent(int aType, int button, int x, int y, int x_root, int y_root, int modifiers, long time, unsigned long window):
        YWindowEvent(aType, modifiers, time, window),
        fX(x), fY(y), fXroot(x_root), fYroot(y_root), fButton(button)
    {
    }

    int x() const { return fX; }
    int y() const { return fY; }
    int x_root() const { return fXroot; }
    int y_root() const { return fYroot; }

    int getButton() const { return fButton; }

    bool leftButton() const { return fButton == 1; }
    bool middleButton() const { return fButton == 2; }
    bool rightButton() const { return fButton == 3; }
private:
    int fX;
    int fY;
    int fXroot;
    int fYroot;
    int fButton;
};

class YClickEvent: public YButtonEvent {
public:
    YClickEvent(const YButtonEvent &button, int count):
        YButtonEvent(button.type(),
                     button.getButton(),
                     button.x(),
                     button.y(),
                     button.x_root(),
                     button.y_root(),
                     button.getModifiers(),
                     button.getTime(),
                     button.getWindow()),
        fCount(count)
    {
    }

    bool isSingleClick() const { return getCount() == 1; }
    bool isDoubleClick() const { return (getCount() % 2) == 0; }

    int getCount() const { return fCount; }

private:
    int fCount;
};

#endif
