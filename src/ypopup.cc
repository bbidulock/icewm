/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "ykey.h"
#include "ymenu.h"

#include "yapp.h"
#include "prefs.h"

bool YApplication::popup(YWindow *forWindow, YPopupWindow *popup) {
    PRECONDITION(popup != 0);
    if (fPopup == 0) {
//        Cursor changePointer = None; //!!!(popup->popupFlags() & YPopupWindow::pfNoPointerChange) ? None : rightPointer;
        Cursor changePointer = (dontRotateMenuPointer ||
	    (popup->popupFlags() & YPopupWindow::pfNoPointerChange) ?
	     None : rightPointer.handle());

        if (!grabEvents(forWindow ? forWindow : popup, changePointer,
                        ButtonPressMask | ButtonReleaseMask |
                        (menuMouseTracking ? PointerMotionMask : ButtonMotionMask)))
        {
            return false;
        }
    }
    popup->setPrevPopup(fPopup);
    fPopup = popup;
    return true;
}

void YApplication::popdown(YPopupWindow *popdown) {
    PRECONDITION(popdown != 0);
    PRECONDITION(fPopup != 0);
    PRECONDITION(fPopup == popdown);
    if (popdown != fPopup) {
        MSG(("popdown: 0x%lX  fPopup: 0x%lX", popdown, fPopup));
        return ;
    }
    fPopup = fPopup->prevPopup();

    if (fPopup == 0) {
        releaseEvents();
    }
}

YPopupWindow::YPopupWindow(YWindow *aParent): YWindow(aParent) {
    fForWindow = 0;
    fPrevPopup = 0;
    fFlags = 0;
    fUp = false;
    setStyle(wsSaveUnder | wsOverrideRedirect);
}

YPopupWindow::~YPopupWindow() {
//    PRECONDITION(fUp == false); // ^C whilest a popup is open breaks this...
}

void YPopupWindow::updatePopup() {
}

void YPopupWindow::sizePopup(int /*hspace*/) {
}

void YPopupWindow::activatePopup() {
}

void YPopupWindow::deactivatePopup() {
}

bool YPopupWindow::popup(YWindow *forWindow,
                         YPopDownListener *popDown,
                         unsigned int flags)
{
    PRECONDITION(fUp == false);
    fPrevPopup = 0;
    fFlags = flags;
    fForWindow = forWindow;
    fPopDownListener = popDown;

    raise();
    show();

    if (app->popup(forWindow, this)) {
        fUp = true;
        activatePopup();
        return true;
    } else {
        hide();
        fFlags = 0;
        fForWindow = 0;
        return false;
    }
}

bool YPopupWindow::popup(YWindow *forWindow,
                         YPopDownListener *popDown,
                         int x, int y, int x_delta, int y_delta, unsigned int flags) {

    if ((flags & pfPopupMenu) && showPopupsAbovePointer)
        flags |= pfFlipVertical;

    fFlags = flags;

    updatePopup();

    int xiscreen = desktop->getScreenForRect(x, y, 1, 1);
    int dx, dy, dw, dh;
    desktop->getScreenGeometry(&dx, &dy, &dw, &dh, xiscreen);

    { // check available space on left and right
        int spaceRight = dw - x;
        int spaceLeft = x - x_delta;


        int hspace = (spaceLeft < spaceRight) ? spaceRight : spaceLeft;

        sizePopup(hspace);
    }

    int rspace = dw - x;
    int lspace = x - x_delta;
    int tspace = dh - y;
    int bspace = y - y_delta;

    /* !!! FIX this to maximize visible area */
    if ((x + width() > dw) || (fFlags & pfFlipHorizontal))
        if (//(lspace >= rspace) &&
            (fFlags & (pfCanFlipHorizontal | pfFlipHorizontal)))
        {
            x -= width() + x_delta;
            fFlags |= pfFlipHorizontal;
        } else
            x = dw - width();
    if ((y + height() > dh) || (fFlags & pfFlipVertical))
        if (//(tspace >= bspace) &&
            (fFlags & (pfCanFlipVertical | pfFlipVertical)))
        {
            y -= height() + y_delta;
            fFlags |= pfFlipVertical;
        } else
            y = dh - height();
    if (x < 0 && (x + width() < dw / 2))
        if ((rspace >= lspace) &&
            (fFlags & pfCanFlipHorizontal))
            x += width() + x_delta;
        else
            x = 0;
    if (y < 0 && (y + height() < dh / 2))
        if ((bspace >= tspace) &&
            (fFlags & pfCanFlipVertical))
            y += height() + y_delta;
        else
            y = 0;

    if (forWindow == 0) {
        if ((x + width() > dw))
            x = dw - width();

        if (x < 0)
            x = 0;

        if ((y + height() > dh))
            y = dh - height();

        if (y < 0)
            y = 0;
    }

    setPosition(x, y);

    return popup(forWindow, popDown, fFlags);
}

void YPopupWindow::popdown() {
    if (fUp) {
        if (fPopDownListener)
            fPopDownListener->handlePopDown(this);

        deactivatePopup();
        app->popdown(this);
        hide();
        fUp = false;

        fFlags = 0;
        fForWindow = 0;
    }
}

void YPopupWindow::cancelPopup() { // !!! rethink these two (cancel,finish)
    if (fForWindow)
        fForWindow->donePopup(this);
    else
        popdown();
}

void YPopupWindow::finishPopup() {
    while (app->popup())
        app->popup()->cancelPopup();
}

bool YPopupWindow::handleKey(const XKeyEvent &/*key*/) {
    return true;
}

void YPopupWindow::handleButton(const XButtonEvent &button) {
    if ((button.x_root >= x() &&
         button.y_root >= y() &&
         button.x_root < int (x() + width()) &&
         button.y_root < int (y() + height()) &&
         button.window == handle()) /*|
	 button.button == Button4 ||
	 button.button == Button5*/)
        YWindow::handleButton(button);
    else {
        if (fForWindow) {
            XEvent xev;

            xev.xbutton = button;

            app->handleGrabEvent(fForWindow, xev);
        } else {
            if (replayMenuCancelClick) {
                app->replayEvent();
                popdown();
            } else {
                if (button.type == ButtonRelease) {
                    popdown();
                }
            }
        }
    }
}

void YPopupWindow::handleMotion(const XMotionEvent &motion) {
    if (motion.x_root >= x() &&
        motion.y_root >= y() &&
        motion.x_root < int (x() + width()) &&
        motion.y_root < int (y() + height()) &&
       motion.window == handle())
    {
        YWindow::handleMotion(motion);
    } else {
        if (fForWindow) {
            XEvent xev;

            xev.xmotion = motion;

            app->handleGrabEvent(fForWindow, xev);
        }
    }
}

