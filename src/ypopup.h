#ifndef _YPOPUP_H
#define _YPOPUP_H

#include "ywindow.h"

class YPopDownListener {
public:
    virtual void handlePopDown(YPopupWindow *popup) = 0;
protected:
    virtual ~YPopDownListener() {};
};

class YPopupWindow: public YWindow {
public:
    YPopupWindow(YWindow *aParent);
    virtual ~YPopupWindow();

    virtual void sizePopup(int hspace);

    bool popup(YWindow *owner,
               YWindow *forWindow,
               YPopDownListener *popDown,
               int xiScreen,
               unsigned int flags);
    bool popup(YWindow *owner,
               YWindow *forWindow,
               YPopDownListener *popDown,
               int x, int y,
               unsigned int flags);
private:
    bool popup(YWindow *owner,
               YWindow *forWindow,
               YPopDownListener *popDown,
               int x, int y,
               int x_delta, int y_delta,
               int xiScreen,
               unsigned int flags);
    friend class YMenu;
    friend class YButton;
public:
    void popdown();

    virtual void updatePopup();
    void finishPopup();
    void cancelPopup();

    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleMotion(const XMotionEvent &motion);
    virtual void handleMotionOutside(bool top, const XMotionEvent &motion);
    void dispatchMotionOutside(bool top, const XMotionEvent &motion);

    virtual void activatePopup(int flags);
    virtual void deactivatePopup();

    unsigned int popupFlags() const { return fFlags; }

    YPopupWindow *prevPopup() const { return fPrevPopup; }
    void setPrevPopup(YPopupWindow *prevPopup) { fPrevPopup = prevPopup; }

    enum {
        pfButtonDown        = 1 << 0,
        pfCanFlipVertical   = 1 << 1,
        pfCanFlipHorizontal = 1 << 2,
        pfFlipVertical      = 1 << 3,
        pfFlipHorizontal    = 1 << 4,
        pfNoPointerChange   = 1 << 5,
        pfPopupMenu         = 1 << 6
    } PopupFlags;

    YWindow *owner() { return fOwner; }
    int getXiScreen() { return fXiScreen; }
private:
    unsigned int fFlags;
    YWindow *fForWindow;
    YPopDownListener *fPopDownListener;
    YPopupWindow *fPrevPopup;
    YWindow *fOwner;
    bool fUp;
    int fXiScreen;
};

#endif
