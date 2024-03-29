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
    if (fPopup == nullptr) {
        Cursor cursor = dontRotateMenuPointer ||
                         (popup->popupFlags() & YPopupWindow::pfNoPointerChange)
                      ? None : getRightPointer();

        if (!grabEvents(forWindow ? forWindow : popup, cursor,
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

    if (fPopup == nullptr) {
        releaseEvents();
    }
}

YPopupWindow::YPopupWindow(YWindow *aParent): YWindow(aParent) {
    fForWindow = nullptr;
    fPopDownListener = nullptr;
    fPrevPopup = nullptr;
    fOwner = nullptr;
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

void YPopupWindow::sizePopup(int hspace) {
}

void YPopupWindow::activatePopup(int flags) {
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
    fPrevPopup = nullptr;
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
        fForWindow = nullptr;
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

    if (forWindow == nullptr) {
        if ((x + int(width()) > dx + dw))
            x = dw - int(width());

        if (x < dx)
            x = dx;

        if ((y + int(height()) > dy + dh))
            y = dh - int(height());

        if (y < dy)
            y = dy;
    }
    else {
        YWindow* parent = forWindow->parent();
        char* title = nullptr;
        if (parent && parent->fetchTitle(&title)) {
            if (0 == strcmp(title, "TitleBar")) {
                int tx = 0;
                int ty = parent->y();
                parent->mapToGlobal(tx, ty);
                int tw = int(parent->width());
                if (x + int(width()) > tx + tw &&
                    tx + tw - int(width()) < dx + int(uw))
                    x = max(dx, max(tx, tx + tw - int(width())));
            }
            XFree(title);
        }
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
        deactivatePopup();
        xapp->popdown(this);
        hide();
        fUp = false;

        fFlags = 0;
        fForWindow = nullptr;

        if (fPopDownListener)
            fPopDownListener->handlePopDown(this);
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

bool YPopupWindow::handleKey(const XKeyEvent& key) {
    return true;
}

void YPopupWindow::handleButton(const XButtonEvent &button) {
    if (geometry().contains(button.x_root, button.y_root) &&
        button.window == handle())
    {
        YWindow::handleButton(button);
    }
    else if (fForWindow) {
        XEvent xev;
        xev.xbutton = button;
        xapp->handleGrabEvent(fForWindow, xev);
    }
    else if (replayMenuCancelClick) {
        xapp->replayEvent();
        popdown();
    }
    else if (button.type == ButtonRelease) {
        popdown();
    }
}

void YPopupWindow::handleMotion(const XMotionEvent &motion) {
    if (geometry().contains(motion.x_root, motion.y_root) &&
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

void YPopupWindow::handleMotionOutside(bool top, const XMotionEvent& motion) {
}

void YPopupWindow::dispatchMotionOutside(bool top, const XMotionEvent& motion) {
    if (fPrevPopup) {
        fPrevPopup->handleMotionOutside(false, motion);
        fPrevPopup->dispatchMotionOutside(false, motion);
    }
}

// vim: set sw=4 ts=4 et:
