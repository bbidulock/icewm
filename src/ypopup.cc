/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#include "config.h"
#include "ykey.h"
#include "ymenu.h"

#include "yapp.h"
#include "prefs.h"

bool YApplication::popup(YWindow *forWindow, YPopupWindow *popup) {
    PRECONDITION(popup != 0);
    if (fPopup == 0) {
        Cursor changePointer = None; //!!!(popup->popupFlags() & YPopupWindow::pfNoPointerChange) ? None : rightPointer;

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
    PRECONDITION(fUp == false);
}

void YPopupWindow::updatePopup() {
}

void YPopupWindow::sizePopup() {
}

void YPopupWindow::activatePopup() {
}

void YPopupWindow::deactivatePopup() {
}

bool YPopupWindow::popup(YWindow *forWindow,
                         PopDownListener *popDown,
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
                         PopDownListener *popDown,
                         int x, int y, int x_delta, int y_delta, unsigned int flags) {

    if ((flags & pfPopupMenu) && showPopupsAbovePointer)
        flags |= pfFlipVertical;

    fFlags = flags;

    updatePopup();

    sizePopup();

    /* !!! FIX this to maximize visible area */
    if ((x + width() > desktop->width()) || (fFlags & pfFlipHorizontal))
        if (fFlags & (pfCanFlipHorizontal | pfFlipHorizontal)) {
            x -= width() + x_delta;
            fFlags |= pfFlipHorizontal;
        } else
            x = desktop->width() - width();
    if ((y + height() > desktop->height()) || (fFlags & pfFlipVertical))
        if (fFlags & (pfCanFlipVertical | pfFlipVertical)) {
            y -= height() + y_delta;
            fFlags |= pfFlipVertical;
        } else
            y = desktop->height() - height();
    if (x < 0 && (x + width() < desktop->width() / 2))
        if (fFlags & pfCanFlipHorizontal)
            x += width() + x_delta;
        else
            x = 0;
    if (y < 0 && (y + height() < desktop->height() / 2))
        if ((fFlags & pfCanFlipVertical))
            y += height() + y_delta;
        else
            y = 0;

    if (forWindow == 0) {
        if ((x + width() > desktop->width()))
            x = desktop->width() - width();

        if (x < 0)
            x = 0;

        if ((y + height() > desktop->height()))
            y = desktop->height() - height();

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
    if (button.x_root >= x() &&
        button.y_root >= y() &&
        button.x_root < int (x() + width()) &&
        button.y_root < int (y() + height()) &&
        button.window == handle())
    {
        YWindow::handleButton(button);
    } else {
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

