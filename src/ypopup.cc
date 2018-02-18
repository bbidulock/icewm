/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "ypopup.h"
#include "base.h"
#include "yxapp.h"
#include "yprefs.h"

bool YXApplication::popup(YWindow *forWindow, YPopupWindow *popup) {
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

void YXApplication::popdown(YPopupWindow *popdown) {
    PRECONDITION(popdown != 0);
    PRECONDITION(fPopup != 0);
    PRECONDITION(fPopup == popdown);
    if (popdown != fPopup) {
        MSG(("popdown: 0x%p  fPopup: 0x%p", popdown, fPopup));
        return ;
    }
    fPopup = fPopup->prevPopup();

    if (fPopup == 0) {
        releaseEvents();
    }
}

YPopupWindow::YPopupWindow(YWindow *aParent): YWindow(aParent) {
    fForWindow = 0;
    fPopDownListener = 0;
    fPrevPopup = 0;
    fOwner = 0;
    fFlags = 0;
    fUp = false;
    fXiScreen = -1;
    setStyle(wsSaveUnder | wsOverrideRedirect);
}

YPopupWindow::~YPopupWindow() {
    //    PRECONDITION(fUp == false); // ^C whilest a popup is open breaks this...
}

void YPopupWindow::updatePopup() {
}

void YPopupWindow::sizePopup(int /*hspace*/) {
}

void YPopupWindow::activatePopup(int /*flags*/) {
}

void YPopupWindow::deactivatePopup() {
}

bool YPopupWindow::popup(YWindow *owner,
                         YWindow *forWindow,
                         YPopDownListener *popDown,
                         int xiScreen,
                         unsigned int flags)
{
    PRECONDITION(fUp == false);
    fPrevPopup = 0;
    fFlags = flags;
    fForWindow = forWindow;
    fPopDownListener = popDown;
    fOwner = owner;
    fXiScreen = xiScreen;

    raise();
    show();

    if (xapp->popup(forWindow, this)) {
        fUp = true;
        activatePopup(flags);
        return true;
    } else {
        hide();
        fFlags = 0;
        fForWindow = 0;
        return false;
    }
}

bool YPopupWindow::popup(YWindow *owner,
                         YWindow *forWindow,
                         YPopDownListener *popDown,
                         int x, int y, int x_delta, int y_delta,
                         int xiScreen,
                         unsigned int flags)
{

    if ((flags & pfPopupMenu) && showPopupsAbovePointer)
        flags |= pfFlipVertical;

    fFlags = flags;

    updatePopup();

/// TODO #warning "FIXME: this logic needs rethink"
    MSG(("x: %d y: %d x_delta: %d y_delta: %d", x, y, x_delta, y_delta));

    int dx, dy;
    unsigned uw, uh;
    desktop->getScreenGeometry(&dx, &dy, &uw, &uh, xiScreen);
    int dw = int(uw), dh = int(uh);

    { // check available space on left and right
        int spaceRight = dx + dw - x;
        int spaceLeft = x - x_delta - dx;


        int hspace = (spaceLeft < spaceRight) ? spaceRight : spaceLeft;

        sizePopup(hspace);
    }

    int rspace = dw - x;
    int lspace = x - x_delta;
    int tspace = dh - y;
    int bspace = y - y_delta;

    /* !!! FIX this to maximize visible area */
    if ((x + int(width()) > dx + dw) || (fFlags & pfFlipHorizontal)) {
        if (//(lspace >= rspace) &&
            (fFlags & (pfCanFlipHorizontal | pfFlipHorizontal)))
        {
            x -= int(width()) + x_delta;
            fFlags |= pfFlipHorizontal;
        } else
            x = dx + dw - int(width());
    }
    if ((y + int(height()) > dy + dh) || (fFlags & pfFlipVertical)) {
        if (//(tspace >= bspace) &&
            (fFlags & (pfCanFlipVertical | pfFlipVertical)))
        {
            y -= int(height()) + y_delta;
            fFlags |= pfFlipVertical;
        } else
            y = dy + dh - int(height());
    }
    if (x < dx && (x + int(width()) < dx + dw / 2)) {
        if ((rspace >= lspace) &&
            (fFlags & pfCanFlipHorizontal))
            x += int(width()) + x_delta;
        else
            x = dx;
    }
    if (y < dy && (y + int(height()) < dy + dh / 2)) {
        if ((bspace >= tspace) &&
            (fFlags & pfCanFlipVertical))
            y += int(height()) + y_delta;
        else
            y = dy;
    }

    if (forWindow == 0) {
        if ((x + int(width()) > dx + dw))
            x = dw - int(width());

        if (x < dx)
            x = dx;

        if ((y + int(height()) > dy + dh))
            y = dh - int(height());

        if (y < dy)
            y = dy;
    }

    setPosition(x, y);

    return popup(owner, forWindow, popDown, xiScreen, fFlags);
}

bool YPopupWindow::popup(YWindow *owner,
                         YWindow *forWindow,
                         YPopDownListener *popDown,
                         int x, int y,
                         unsigned int flags)
{
    int xiScreen = desktop->getScreenForRect(x, y, 1, 1);
    return popup(owner, forWindow, popDown, x, y, -1, -1, xiScreen, flags);
}

void YPopupWindow::popdown() {
    if (fUp) {
        if (fPopDownListener)
            fPopDownListener->handlePopDown(this);

        deactivatePopup();
        xapp->popdown(this);
        hide();
        fUp = false;

        fFlags = 0;
        fForWindow = 0;

        if (fOwner) {
            fOwner->handleEndPopup(this);
        }
    }
}

void YPopupWindow::cancelPopup() { // !!! rethink these two (cancel,finish)
    if (fForWindow)
        fForWindow->donePopup(this);
    else
        popdown();
}

void YPopupWindow::finishPopup() {
    while (xapp->popup())
        xapp->popup()->cancelPopup();
}

bool YPopupWindow::handleKey(const XKeyEvent &/*key*/) {
    return true;
}

void YPopupWindow::handleButton(const XButtonEvent &button) {
    if ((button.x_root >= x() &&
         button.y_root >= y() &&
         button.x_root  < x() + int(width()) &&
         button.y_root  < y() + int(height()) &&
         button.window == handle()) /*|
         button.button == Button4 ||
         button.button == Button5*/)
        YWindow::handleButton(button);
    else {
        if (fForWindow) {
            XEvent xev;

            xev.xbutton = button;

            xapp->handleGrabEvent(fForWindow, xev);
        } else {
            if (replayMenuCancelClick) {
                xapp->replayEvent();
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
        motion.x_root  < x() + int(width()) &&
        motion.y_root  < y() + int(height()) &&
        motion.window == handle())
    {
        YWindow::handleMotion(motion);
        dispatchMotionOutside(true, motion);
    } else {
        handleMotionOutside(true, motion);
        if (fForWindow) {
            XEvent xev;

            xev.xmotion = motion;

            xapp->handleGrabEvent(fForWindow, xev);
        }
    }
}

void YPopupWindow::handleMotionOutside(bool /*top*/, const XMotionEvent &/*motion*/) {
}

void YPopupWindow::dispatchMotionOutside(bool /*top*/, const XMotionEvent &motion) {
    if (fPrevPopup) {
        fPrevPopup->handleMotionOutside(false, motion);
        fPrevPopup->dispatchMotionOutside(false, motion);
    }
}

// vim: set sw=4 ts=4 et:
