#ifndef __YWINDOWEVENT_H
#define __YWINDOWEVENT_H

#include "yevent.h"

class YWindowEvent: public YEvent {
public:
    YWindowEvent(): YEvent() {}

    YWindowEvent(int aType, int modifiers, long time, long window): YEvent(aType),
        fModifiers(modifiers), fTime(time), fWindow(window)
    {
    }

    Modifiers getModifiers() const { return (Modifiers)fModifiers; }
    Modifiers getMouseModifiers() const { return (Modifiers)(fModifiers & mMouseMask); }
    Modifiers getKeyModifiers() const { return (Modifiers)(fModifiers & mKeyMask); }

    bool hasLeftButton() const { return (getModifiers() & mMouseMask) == mLeftButton; }
    bool hasRightButton() const { return (getModifiers() & mMouseMask) == mRightButton; }

    bool isAlt() const { return getModifiers() & mAlt; }
    bool isCtrl() const  { return getModifiers() & mCtrl; }
    bool isShift() const  { return getModifiers() & mShift; }
    bool isWin() const  { return getModifiers() & mWin; }
    bool isMeta() const  { return getModifiers() & mMeta; }
    bool isSuper() const  { return getModifiers() & mSuper; }
    bool isHyper() const  { return getModifiers() & mHyper; }

    bool onlyAlt() const { return getKeyModifiers() == mAlt; }
    bool onlyCtrl() const { return getKeyModifiers() == mCtrl; }

    bool isAutoRepeat() const { return getModifiers() & mAutoRepeat; }

    long getTime() const { return fTime; }
    unsigned long getWindow() const { return fWindow; }
private:
    int fModifiers;
    long fTime;
    long fWindow;
};

#endif
