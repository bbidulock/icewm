#ifndef _YPOPUP_H
#define _YPOPUP_H

#include "ywindow.h"

class PopDownListener {
public:
    virtual void handlePopDown(YPopupWindow *popup) = 0;
};

class YPopupWindow: public YWindow {
public:
    YPopupWindow(YWindow *aParent);
    virtual ~YPopupWindow();

    virtual void sizePopup(int hspace);

    bool popup(YWindow *forWindow,
               PopDownListener *popDown,
               int x, int y,
               int x_delta, int y_delta,
               unsigned int flags);
    bool popup(YWindow *forWindow,
               PopDownListener *popDown,
               unsigned int flags);
    void popdown();

    virtual void updatePopup();
    void finishPopup();
    void cancelPopup();

    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleMotion(const XMotionEvent &motion);

    virtual void activatePopup();
    virtual void deactivatePopup();

    unsigned popupFlags() const { return fFlags; }

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

private:
    unsigned int fFlags;
    YWindow *fForWindow;
    PopDownListener *fPopDownListener;
    YPopupWindow *fPrevPopup;
    bool fUp;
};

#endif
