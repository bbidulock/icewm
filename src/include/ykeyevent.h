#ifndef __YKEYEVENT_H
#define __YKEYEVENT_H

#include "ywindowevent.h"

class YKeyEvent: public YWindowEvent {
public:
    enum Modifiers {
        mNoButton        = 0x0000,
        mLeftButton      = 0x0001,
        mRightButton     = 0x0002,
        mMiddleButton    = 0x0004,
        mMouseMask       = 0x00FF,

        mAlt             = 0x0100,
        mCtrl            = 0x0200,
        mShift           = 0x0400,
        mWin             = 0x0800,
        mMeta            = 0x1000,
        mSuper           = 0x2000,
        mHyper           = 0x4000,
        mKeyMask         = 0xFF00,

        mNumLock         = 0x01000,
        mCapsLock        = 0x02000,
        mScrollLock      = 0x04000,
        mKeyPad          = 0x08000,
        mAutoRepeat      = 0x10000,
    };

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
