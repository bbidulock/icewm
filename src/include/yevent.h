#ifndef __YEVENT_H
#define __YEVENT_H

class YEvent {
public:
    enum EventType {
        etNone,
        etKeyPress,
        etKeyRelease,
        etButtonPress,
        etButtonRelease,
        etMotion,
        etPointerIn,
        etPointerOut,
        etFocusIn,
        etFocusOut
    };

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

    YEvent(): fType(etNone) {}
    YEvent(int aType): fType(aType) {}
    virtual ~YEvent() {}

    int type() const { return fType; }
private:
    int fType;
};

#endif
