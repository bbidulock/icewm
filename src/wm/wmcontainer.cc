/*
 * IceWM
 *
 * Copyright (C) 1997,1998,1999 Marko Macek
 */
#include "config.h"
#include "yxlib.h"
#include "wmcontainer.h"

#include "ybuttonevent.h"
#include "ycrossingevent.h"
#include "wmframe.h"
#include "yapp.h"
#include "base.h"

YBoolPrefProperty YClientContainer::gClientMouseActions("icewm", "ClientWindowMouseActions", true);
YBoolPrefProperty YClientContainer::gUseMouseWheel("icewm", "UseMouseWheel", true);
YBoolPrefProperty YClientContainer::gPointerColormap("icewm", "PointerColormap", false);
YBoolPrefProperty YClientContainer::gPassFirstClickToClient("icewm", "PassFirstClickToClient", true);
YBoolPrefProperty YClientContainer::gRaiseOnClickClient("icewm", "RaiseOnClickClient", true);
YBoolPrefProperty YClientContainer::gFocusOnClickClient("icewm", "FocusOnClickClient", true);
YBoolPrefProperty YClientContainer::gClickFocus("icewm", "ClickFocus", true);

YClientContainer::YClientContainer(YWindow *parent, YFrameWindow *frame)
:YWindow(parent)
{
    fFrame = frame;
    fHaveGrab = false;
    fHaveActionGrab = false;

    setStyle(wsManager);
}

YClientContainer::~YClientContainer() {
    releaseButtons();
}

bool YClientContainer::eventButton(const YButtonEvent &button) {
    bool doRaise = false;
    bool doActivate = false;
    bool firstClick = false;

    if (!(button.isCtrl()) &&
        getFrame()->shouldRaise(button) &&
        (!gUseMouseWheel.getBool() || (button.getButton() != 4 && button.getButton() != 5)))
    {
        if (gFocusOnClickClient.getBool()) {
            if (getFrame()->isFocusable() && !getFrame()->focused())
                firstClick = true;
            doActivate = true;
        }
        if (gRaiseOnClickClient.getBool()) {
            doRaise = true;
            if (getFrame()->canRaise())
                firstClick = true;
        }
    }
#if 1
    if (gClientMouseActions.getBool() &&
        button.getButton() == 1 &&
        button.isCtrl() && button.isAlt() && !button.isShift())
    {
        XAllowEvents(app->display(), AsyncPointer, CurrentTime);
        if (getFrame()->canMove()) {
            getFrame()->startMoveSize(1, 1,
                                      0, 0,
                                      button.x() + x(), button.y() + y());
        }
        return true;
    }
#endif
    ///!!! it might be nice if this was per-window option (app-request)
    if (!firstClick || gPassFirstClickToClient.getBool())
        XAllowEvents(app->display(), ReplayPointer, CurrentTime);
    else
        XAllowEvents(app->display(), AsyncPointer, CurrentTime);
    XSync(app->display(), 0);
    // ??? do this first
    if (!(getFrame()->frameOptions() & YFrameWindow::foIgnoreFocusClick)) {
        if (doActivate)
            getFrame()->activate();
        if (doRaise)
            getFrame()->wmRaise();
    }
    return true;
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
        int mod;

        if (app->XMod(YEvent::mCtrl | YEvent::mCtrl, mod))
            XGrabButton(app->display(),
                        1, mod,
                        handle(), True,
                        ButtonPressMask,
                        GrabModeSync, GrabModeAsync, None, None);
#if 1
        if (app->XMod(YEvent::mWin, mod))
            XGrabButton(app->display(),
                        1, mod,
                        handle(), True,
                        ButtonPressMask,
                        GrabModeSync, GrabModeAsync, None, None);
#endif
    }
    if (!fHaveGrab && (gClickFocus.getBool() ||
                       gFocusOnClickClient.getBool() ||
                       gRaiseOnClickClient.getBool()))
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
        int mod;

        fHaveActionGrab = true;
        if (app->XMod(YEvent::mAlt | YEvent::mCtrl, mod))
            XGrabButton(app->display(),
                        1, mod,
                        handle(), True,
                        ButtonPressMask,
                        GrabModeSync, GrabModeAsync, None, None);
#if 1
        if (app->XMod(YEvent::mWin, mod))
            XGrabButton(app->display(),
                        1, mod,
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

bool YClientContainer::handleCrossing(const XCrossingEvent &crossing) {
    if (getFrame() && gPointerColormap.getBool()) {
        if (crossing.type == EnterNotify)
            getFrame()->getRoot()->setColormapWindow(getFrame());
        else if (crossing.type == LeaveNotify &&
                 crossing.detail != NotifyInferior &&
                 crossing.mode == NotifyNormal &&
                 getFrame()->getRoot()->colormapWindow() == getFrame())
        {
            getFrame()->getRoot()->setColormapWindow(0);
        }
    }
    return YWindow::handleCrossing(crossing);
}
