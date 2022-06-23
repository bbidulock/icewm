/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "wmcontainer.h"
#include "wmframe.h"
#include "wmmgr.h"
#include "wmapp.h"
#include "prefs.h"

YClientContainer::YClientContainer(YWindow *parent, YFrameWindow *frame,
                                   int depth, Visual *visual, Colormap cmap)
    : YWindow(parent, None, depth, visual, cmap)
{
    fFrame = frame;
    fHaveGrab = false;
    fHaveActionGrab = false;

    setStyle(wsManager | wsNoExpose);
    setPointer(YWMApp::leftPointer);
    setTitle("Container");
    show();
}

YClientContainer::~YClientContainer() {
    if (destroyed() == false)
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
                doActivate = (getFrame() != manager->getFocus());
                if (getFrame()->canFocusByMouse() && !getFrame()->focused())
                    firstClick = true;
            }
        }
        if (raiseOnClickClient && getFrame()->canRaise()) {
            doRaise = true;
            firstClick = true;
        }
    }

    if (clientMouseActions) {
        unsigned int k = button.button + XK_Pointer_Button1 - 1;
        unsigned int m = KEY_MODMASK(button.state);
        unsigned int vm = VMod(m);

        if (gMouseWinSize.eq(k, vm)) {
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
        }
        else if (gMouseWinMove.eq(k, vm)) {
            XAllowEvents(xapp->display(), AsyncPointer, CurrentTime);

            if (getFrame()->canMove()) {
                int px = button.x + x();
                int py = button.y + y();
                getFrame()->startMoveSize(true, true,
                                          0, 0,
                                          px, py);
            }
            return ;
        }
        else if (gMouseWinRaise.eq(k, vm)
            && (gMouseWinRaise != gMouseWinLower || getFrame()->canRaise()))
        {
            XAllowEvents(xapp->display(), AsyncPointer, CurrentTime);
            getFrame()->wmRaise();
            return ;
        }
        else if (gMouseWinLower.eq(k, vm)) {
            XAllowEvents(xapp->display(), AsyncPointer, CurrentTime);
            getFrame()->wmLower();
            return ;
        }
    }

    ///!!! do this first?
    if (doActivate)
        getFrame()->focus();
    if (doRaise && (!doActivate || !raiseOnFocus))
        getFrame()->wmRaise();
    ///!!! it might be nice if this was per-window option (app-request)
    if (!firstClick || passFirstClickToClient)
        XAllowEvents(xapp->display(), ReplayPointer, CurrentTime);
    else
        XAllowEvents(xapp->display(), AsyncPointer, CurrentTime);
    xapp->sync();
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
        for (int button = Button1; button <= Button3; ++button) {
            if (buttonRaiseMask & (1 << (button - Button1))) {
                XGrabButton(xapp->display(), button, AnyModifier,
                            handle(), True, ButtonPressMask,
                            GrabModeSync, GrabModeAsync, None, None);
            }
        }
        fHaveGrab = true;
    }
}

void YClientContainer::releaseButtons() {
    if (fHaveGrab) {
        fHaveGrab = false;
        for (int button = Button1; button <= Button3; ++button) {
            if (buttonRaiseMask & (1 << (button - Button1))) {
                XUngrabButton(xapp->display(), button, AnyModifier, handle());
            }
        }
        fHaveActionGrab = false;
    }
    grabActions();
}

void YClientContainer::regrabMouse() {
    fHaveActionGrab = false;
    releaseButtons();
    grabButtons();
}

void YClientContainer::grabActions() {
    if (clientMouseActions && fHaveActionGrab == false) {
        WMKey grab[] = { gMouseWinMove, gMouseWinSize,
                         gMouseWinRaise, gMouseWinLower };
        const int count = int ACOUNT(grab) - (gMouseWinRaise == gMouseWinLower);
        for (int i = 0; i < count; ++i) {
            int button = int(grab[i].key) - XK_Pointer_Button1 + Button1;
            if (inrange(button, Button1, Button3)) {
                grabVButton(button, grab[i].mod);
            }
        }
        fHaveActionGrab = true;
    }
}

void YClientContainer::handleConfigureRequest(const XConfigureRequestEvent &configureRequest) {
    MSG(("configure request in frame"));

    if (getFrame() &&
        configureRequest.window == getFrame()->client()->handle())
    {
        getFrame()->configureClient(configureRequest);
    }
}

void YClientContainer::handleMapRequest(const XMapRequestEvent &mapRequest) {
    if (mapRequest.window == getFrame()->client()->handle()
        && getFrame()->isPassive() == false)
    {
        if (getFrame()->isUnmapped()) {
            manager->lockFocus();
            getFrame()->makeMapped();
            manager->unlockFocus();
        }
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
            manager->setColormapWindow(nullptr);
        }
    }
}


// vim: set sw=4 ts=4 et:
