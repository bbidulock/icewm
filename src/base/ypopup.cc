/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#pragma implementation
#include "config.h"

#include "yxlib.h" // !!! remove

#include "ypopup.h"

#include "ybuttonevent.h"
#include "ymotionevent.h"
#include "yapp.h"
#include "base.h"

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

    fFlags = flags;

    updatePopup();

    sizePopup();

    if ((x + width() > desktop->width()) || (fFlags & pfFlipHorizontal)) {
        if (fFlags & (pfCanFlipHorizontal | pfFlipHorizontal)) {
            x -= width() + x_delta;
            fFlags |= pfFlipHorizontal;
        } else
            x = desktop->width() - width();
    }
    if ((y + height() > desktop->height()) || (fFlags & pfFlipVertical)) {
        if (fFlags & (pfCanFlipVertical | pfFlipVertical)) {
            y -= height() + y_delta;
            fFlags |= pfFlipVertical;
        } else
            y = desktop->height() - height();
    }
    if (x < 0 && (x + width() < desktop->width() / 2)) {
        if (fFlags & pfCanFlipHorizontal)
            x += width() + x_delta;
        else
            x = 0;
    }
    if (y < 0 && (y + height() < desktop->height() / 2)) {
        if ((fFlags & pfCanFlipVertical))
            y += height() + y_delta;
        else
            y = 0;
    }
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
    if (fForWindow) {
        fForWindow->donePopup(this);
    } else {
        popdown();
    }
}

void YPopupWindow::finishPopup() {
    while (app->popup())
        app->popup()->cancelPopup();
}

bool YPopupWindow::eventKey(const YKeyEvent &/*key*/) {
    return true;
}

bool YPopupWindow::eventButton(const YButtonEvent &button) {
    if (button.x_root() >= x() &&
        button.y_root() >= y() &&
        button.x_root() < int (x() + width()) &&
        button.y_root() < int (y() + height()) &&
        button.getWindow() == handle())
    {
        return YWindow::eventButton(button);
    } else {
        if (fForWindow) {
#warning "fix fForWindow"

            int nx = 0;
            int ny = 0;
            Window child;
//#if 0
            //XEvent xev;

            //xev.xbutton = button;
            if (XTranslateCoordinates(app->display(),
                                      handle(), button.getWindow(),
                                      button.x(), button.y(),
                                      &nx, &ny, &child) == True)
            {
                //xev.xbutton.window = win->handle();

                YButtonEvent btn(button.type(),
                                 button.getButton(),
                                 nx,
                                 ny,
                                 button.x_root(),
                                 button.y_root(),
                                 button.getModifiers(),
                                 button.getTime(),
                                 fForWindow->handle());

                return fForWindow->eventButton(btn);
            }


//            app->handleGrabEvent(fForWindow, xev);
//#endif
        } else {
            if (false) { ///replayMenuCancelClick) {
                app->replayEvent();
                popdown();
            } else {
                if (button.type() == YEvent::etButtonRelease) {
                    popdown();
                }
            }
        }
    }
    return false;
}

bool YPopupWindow::eventMotion(const YMotionEvent &motion) {
    if (motion.x_root() >= x() &&
        motion.y_root() >= y() &&
        motion.x_root() < int (x() + width()) &&
        motion.y_root() < int (y() + height()) &&
        motion.getWindow() == handle())
    {
        return YWindow::eventMotion(motion);
    } else {
        if (fForWindow) {
#warning "fix fForWindow"
#if 1
            int nx = 0;
            int ny = 0;
            Window child;
//            XEvent xev;

            //xev.xbutton = button;
            if (XTranslateCoordinates(app->display(),
                                      handle(), motion.getWindow(),
                                      motion.x(), motion.y(),
                                      &nx, &ny, &child) == True)
            {
                //xev.xbutton.window = win->handle();

                YMotionEvent mtn(motion.type(),
                                 //motion.getButton(),
                                 nx,
                                 ny,
                                 motion.x_root(),
                                 motion.y_root(),
                                 motion.getModifiers(),
                                 motion.getTime(),
                                 fForWindow->handle());

                return fForWindow->eventMotion(mtn);
            }


//            xev.xmotion = motion;

//            app->handleGrabEvent(fForWindow, xev);
#endif
        }
    }
    return false;
}
