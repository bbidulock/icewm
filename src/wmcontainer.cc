/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "ylib.h"
#include "wmcontainer.h"

#include "wmframe.h"
#include "yapp.h"
#include "prefs.h"

#include <stdio.h>

YClientContainer::YClientContainer(YWindow *parent, YFrameWindow *frame)
:YWindow(parent)
{
    fFrame = frame;
    fHaveGrab = false;
    fHaveActionGrab = false;

    setStyle(wsManager);
    setPointer(YApplication::leftPointer);
}

YClientContainer::~YClientContainer() {
    releaseButtons();
}

void YClientContainer::handleButton(const XButtonEvent &button) {
    bool doRaise = false;
    bool doActivate = false;
    bool firstClick = false;

    if (!(button.state & ControlMask) &&
        (buttonRaiseMask & (1 << (button.button - 1))) &&
        (!useMouseWheel || (button.button != 4 && button.button != 5)))
    {
        if (focusOnClickClient) {
            if (getFrame()->isFocusable() && !getFrame()->focused())
                firstClick = true;
            doActivate = true;
        }
        if (raiseOnClickClient) {
            doRaise = true;
            if (getFrame()->canRaise())
                firstClick = true;
        }
    }
#if 1
    if (clientMouseActions && 
        ((button.state & (ControlMask | ShiftMask | app->AltMask)) == app->AltMask))

    {
        XAllowEvents(app->display(), AsyncPointer, CurrentTime);
        if (button.button == 1) {
#if 0
            if (getFrame()->canMove()) {
                getFrame()->startMoveSize(1, 1,
                                          0, 0,
                                          button.x + x(), button.y + y());
            }
#else
            int px = button.x + x();
            int py = button.y + y();
            int gx = (px * 5 / (int)width() - 2) / 2;
            int gy = (py * 5 / (int)height() - 2) / 2;
            if (gx < 0) gx = -1;
            if (gx > 0) gx = 1;
            if (gy < 0) gy = -1;
            if (gy > 0) gy = 1;
            bool doMove = (gx == 0 && gy == 0) ? true : false;
            int mx, my;
            if (doMove) {
                mx = px;
                my = py;
            } else {
                mx = button.x_root;
                my = button.y_root;
            }
            if (doMove && getFrame()->canMove() ||
                !doMove && getFrame()->canSize())
            {
                getFrame()->startMoveSize(doMove, 1,
                                          gx, gy,
                                          mx, my);
            }
#endif
        }
        if (button.button == 3) {
        }
        return ;
    }
#endif
    ///!!! it might be nice if this was per-window option (app-request)
    if (!firstClick || passFirstClickToClient)
        XAllowEvents(app->display(), ReplayPointer, CurrentTime);
    else
        XAllowEvents(app->display(), AsyncPointer, CurrentTime);
    XSync(app->display(), 0);
    ///!!! do this first?
    if (doActivate)
        getFrame()->activate();
    if (doRaise)
        getFrame()->wmRaise();
    return ;
}

// manage button grab on frame window to capture clicks to client window
// we want to keep the grab when:
//    focusOnClickClient && not focused
// || raiseOnClickClient && not can be raised

// ('not on top' != 'can be raised')
// the difference is when we have transients and explicitFocus
// also there is the difference with layers and multiple workspaces

void YClientContainer::grabButtons() {
    if (!fHaveActionGrab) {
        fHaveActionGrab = true;
        XGrabButton(app->display(),
                    1, app->AltMask,
                    handle(), True,
                    ButtonPressMask,
                    GrabModeSync, GrabModeAsync, None, None);
        if (app->NumLockMask) {
            XGrabButton(app->display(),
                        1, app->AltMask + app->NumLockMask,
                        handle(), True,
                        ButtonPressMask,
                        GrabModeSync, GrabModeAsync, None, None);
        }
#if 0
        if (app->MetaMask)
            XGrabButton(app->display(),
                        1, app->MetaMask,
                        handle(), True,
                        ButtonPressMask,
                        GrabModeSync, GrabModeAsync, None, None);
#endif
    }
    if (!fHaveGrab && (clickFocus ||
                       focusOnClickClient ||
                       raiseOnClickClient))
    {
        fHaveGrab = true;

        XGrabButton(app->display(),
                    AnyButton, AnyModifier,
                    handle(), True,
                    ButtonPressMask,
                    GrabModeSync, GrabModeAsync, None, None);
    }
}

void YClientContainer::releaseButtons() {
    if (fHaveGrab) {
        fHaveGrab = false;

        XUngrabButton(app->display(), AnyButton, AnyModifier, handle());
        fHaveActionGrab = false;
    }
    if (!fHaveActionGrab) {
        fHaveActionGrab = true;
        XGrabButton(app->display(),
                    1, app->AltMask,
                    handle(), True,
                    ButtonPressMask,
                    GrabModeSync, GrabModeAsync, None, None);
        if (app->NumLockMask) {
            XGrabButton(app->display(),
                        1, app->AltMask + app->NumLockMask,
                        handle(), True,
                        ButtonPressMask,
                        GrabModeSync, GrabModeAsync, None, None);
        }
#if 0
        if (app->MetaMask)
            XGrabButton(app->display(),
                        1, app->MetaMask,
                        handle(), True,
                        ButtonPressMask,
                        GrabModeSync, GrabModeAsync, None, None);
#endif
    }
}

void YClientContainer::handleConfigureRequest(const XConfigureRequestEvent &configureRequest) {
    MSG(("configure request in frame"));

    if (getFrame() &&
        configureRequest.window == getFrame()->client()->handle())
    {
        XConfigureRequestEvent cre = configureRequest;

        getFrame()->configureClient(cre);
    }
}

void YClientContainer::handleMapRequest(const XMapRequestEvent &mapRequest) {
    if (mapRequest.window == getFrame()->client()->handle()) {
        getFrame()->setState(WinStateMinimized |
                             WinStateHidden |
                             WinStateRollup,
                             0);
        getFrame()->focusOnMap();
    }
}

void YClientContainer::handleCrossing(const XCrossingEvent &crossing) {
    if (getFrame() && pointerColormap) {
        if (crossing.type == EnterNotify)
            manager->setColormapWindow(getFrame());
        else if (crossing.type == LeaveNotify &&
                 crossing.detail != NotifyInferior &&
                 crossing.mode == NotifyNormal &&
                 manager->colormapWindow() == getFrame())
        {
            manager->setColormapWindow(0);
        }
    }
}

