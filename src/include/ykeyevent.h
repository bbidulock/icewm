#ifndef __YKEYEVENT_H
#define __YKEYEVENT_H

#include "ywindowevent.h"

class YKeyEvent: public YWindowEvent {
public:
    YKeyEvent(): YWindowEvent() {}
    YKeyEvent(int aType, int aKey, int aUnichar, int aModifiers, long aTime, long aWindow):
        YWindowEvent(aType, aModifiers, aTime, aWindow), key(aKey), unichar(aUnichar)
    {}

    int getKey() const { return key; }
    int getUnichar() const { return unichar; }

    bool isModifierKey() const;
private:
    int key;
    int unichar;
};

#endif
