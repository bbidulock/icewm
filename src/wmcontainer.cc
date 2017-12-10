/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "ylib.h"
#include <X11/keysym.h>
#include "wmcontainer.h"

#include "wmframe.h"
#include "yxapp.h"
#include "prefs.h"

#include <stdio.h>

YClientContainer::YClientContainer(YWindow *parent, YFrameWindow *frame)
:YWindow(parent)
{
    fFrame = frame;
    fHaveGrab = false;
    fHaveActionGrab = false;

    setStyle(wsManager);
    setDoubleBuffer(false);
    setPointer(YXApplication::leftPointer);
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
            if (!getFrame()->isTypeDock()) {
                doActivate = true;
                if (getFrame()->canFocusByMouse() && !getFrame()->focused())
                    firstClick = true;
            }
        }
        if (raiseOnClickClient) {
            doRaise = true;
            if (getFrame()->canRaise())
                firstClick = true;
        }
    }
#if 1
    if (clientMouseActions) {
        unsigned int k = button.button + XK_Pointer_Button1 - 1;
        unsigned int m = KEY_MODMASK(button.state);
        unsigned int vm = VMod(m);

        if (IS_WMKEY(k, vm, gMouseWinSize)) {
            XAllowEvents(xapp->display(), AsyncPointer, CurrentTime);

            int px = button.x + x();
            int py = button.y + y();
            int gx = (px * 3 / (int)width() - 1);
            int gy = (py * 3 / (int)height() - 1);
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
            if ((doMove && getFrame()->canMove()) ||
                (!doMove && getFrame()->canSize()))
            {
                getFrame()->startMoveSize(doMove, true,
                                          gx, gy,
                                          mx, my);
            }
            return ;
        } else if (IS_WMKEY(k, vm, gMouseWinMove)) {
            XAllowEvents(xapp->display(), AsyncPointer, CurrentTime);

            if (getFrame()->canMove()) {
                int px = button.x + x();
                int py = button.y + y();
                getFrame()->startMoveSize(true, true,
                                          0, 0,
                                          px, py);
            }
            return ;
        } else if (IS_WMKEY(k, vm, gMouseWinRaise)) {
            XAllowEvents(xapp->display(), AsyncPointer, CurrentTime);
            getFrame()->wmRaise();
            return ;
        }
    }
#endif
    ///!!! do this first?
    if (doActivate)
        getFrame()->activate();
    if (doRaise)
        getFrame()->wmRaise();
    ///!!! it might be nice if this was per-window option (app-request)
    if (!firstClick || passFirstClickToClient)
        XAllowEvents(xapp->display(), ReplayPointer, CurrentTime);
    else
        XAllowEvents(xapp->display(), AsyncPointer, CurrentTime);
    XSync(xapp->display(), False);
}

// manage button grab on frame window to capture clicks to client window
// we want to keep the grab when:
//    focusOnClickClient && not focused
// || raiseOnClickClient && not can be raised

// ('not on top' != 'can be raised')
// the difference is when we have transients and explicitFocus
// also there is the difference with layers and multiple workspaces

void YClientContainer::grabButtons() {
    grabActions();
    if (!fHaveGrab && (clickFocus ||
                       focusOnClickClient ||
                       raiseOnClickClient))
    {
        fHaveGrab = true;

        XGrabButton(xapp->display(),
                    Button1, AnyModifier,
                    handle(), True,
                    ButtonPressMask,
                    GrabModeSync, GrabModeAsync, None, None);
        XGrabButton(xapp->display(),
                    Button2, AnyModifier,
                    handle(), True,
                    ButtonPressMask,
                    GrabModeSync, GrabModeAsync, None, None);
        XGrabButton(xapp->display(),
                    Button3, AnyModifier,
                    handle(), True,
                    ButtonPressMask,
                    GrabModeSync, GrabModeAsync, None, None);
    }
}

void YClientContainer::releaseButtons() {
    if (fHaveGrab) {
        fHaveGrab = false;

        XUngrabButton(xapp->display(), Button1, AnyModifier, handle());
        XUngrabButton(xapp->display(), Button2, AnyModifier, handle());
        XUngrabButton(xapp->display(), Button3, AnyModifier, handle());
        fHaveActionGrab = false;
    }
    grabActions();
}

void YClientContainer::regrabMouse() {
    XUngrabButton(xapp->display(), Button1, AnyModifier, handle());
    XUngrabButton(xapp->display(), Button2, AnyModifier, handle());
    XUngrabButton(xapp->display(), Button3, AnyModifier, handle());

    if (fHaveActionGrab)  {
        fHaveActionGrab = false;
        grabActions();
    }

    if (fHaveGrab ) {
        fHaveGrab = false;
        grabButtons();
    }
}

void YClientContainer::grabActions() {
    if (clientMouseActions && fHaveActionGrab == false) {
        fHaveActionGrab = true;
        const KeySym minButton = XK_Pointer_Button1;
        const KeySym maxButton = XK_Pointer_Button3;
        const KeySym xkButton0 = XK_Pointer_Button1 - 1;
        if (inrange(gMouseWinMove.key, minButton, maxButton))
            grabVButton(gMouseWinMove.key - xkButton0, gMouseWinMove.mod);
        if (inrange(gMouseWinSize.key, minButton, maxButton))
            grabVButton(gMouseWinSize.key - xkButton0, gMouseWinSize.mod);
        if (inrange(gMouseWinRaise.key, minButton, maxButton))
            grabVButton(gMouseWinRaise.key - xkButton0, gMouseWinRaise.mod);
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
        manager->lockFocus();
        getFrame()->setState(WinStateMinimized |
                             WinStateHidden |
                             WinStateRollup,
                             0);
        manager->unlockFocus();
        bool doActivate = true;
        getFrame()->updateFocusOnMap(doActivate);
        if (doActivate) {
            getFrame()->activateWindow(true);
        }
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


// vim: set sw=4 ts=4 et:
