/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */

#include "config.h"
#include "yfull.h"
#include "wmframe.h"
#include "bindkey.h"

//#include "atasks.h"
#include "wmaction.h"
#include "wmclient.h"
#include "wmcontainer.h"
#include "wmtitle.h"
#include "wmbutton.h"
#include "wmminiicon.h"
#include "wmswitch.h"
//#include "wmtaskbar.h"
//#include "wmwinlist.h"
#include "wmmgr.h"
#include "wmapp.h"
#include "sysdep.h"

static YColor *activeBorderBg = 0;
static YColor *inactiveBorderBg = 0;

YTimer *YFrameWindow::fAutoRaiseTimer = 0;
YTimer *YFrameWindow::fDelayFocusTimer = 0;

extern XContext frameContext;
extern XContext clientContext;

bool YFrameWindow::isButton(char c) {
    if (strchr(titleButtonsSupported, c) == 0)
        return false;
    if (strchr(titleButtonsRight, c) != 0 || strchr(titleButtonsLeft, c) != 0)
        return true;
    return false;
}

YFrameWindow::YFrameWindow(YWindow *parent, YFrameClient *client, YWindowManager *root): YWindow(parent) {
    if (activeBorderBg == 0)
        activeBorderBg = new YColor(clrActiveBorder);
    if (inactiveBorderBg == 0)
        inactiveBorderBg = new YColor(clrInactiveBorder);

    fClient = 0;
    fFocused = false;
    fNextFrame = fPrevFrame = 0;
    fPopupActive = 0;
    fRoot = root;

    normalX = 0;
    normalY = 0;
    normalWidth = 1;
    normalHeight = 1;
    iconX = -1;
    iconY = -1;
    movingWindow = false;
    sizingWindow = false;
    indicatorsVisible = 0;
    fFrameFunctions = 0;
    fFrameDecors = 0;
    fFrameOptions = 0;
    fFrameIcon = 0;
    fMiniIcon = 0;
    fTransient = 0;
    fNextTransient = 0;
    fOwner = 0;
    fManaged = false;
    fKillMsgBox = 0;
    fWasMinimized = false;

    setStyle(wsOverrideRedirect);

    PRECONDITION(client != 0);
    fClient = client;

    fWinWorkspace = fRoot->activeWorkspace();
    fWinLayer = WinLayerNormal;
    fWinState = 0;

    createPointerWindows();

    fClientContainer = new YClientContainer(this, this);
    fClientContainer->show();

    fTitleBar = new YFrameTitleBar(this, this);
    fTitleBar->show();

    if (!isButton('m')) /// optimize strchr (flags)
        fMaximizeButton = 0;
    else {
        fMaximizeButton = new YFrameButton(fTitleBar, this, actionMaximize, actionMaximizeVert);
        //fMaximizeButton->setWinGravity(NorthEastGravity);
        fMaximizeButton->show();
        fMaximizeButton->setToolTip("Maximize");
    }

    if (!isButton('i'))
        fMinimizeButton = 0;
    else {
        fMinimizeButton = new YFrameButton(fTitleBar, this, actionMinimize, actionHide);
        //fMinimizeButton->setWinGravity(NorthEastGravity);
        fMinimizeButton->setToolTip("Minimize");
        fMinimizeButton->show();
    }

    if (!isButton('x'))
        fCloseButton = 0;
    else {
        fCloseButton = new YFrameButton(fTitleBar, this, actionClose, actionKill);
        //fCloseButton->setWinGravity(NorthEastGravity);
        fCloseButton->setToolTip("Close");
        if (useXButton)
            fCloseButton->show();
    }

    if (!isButton('h'))
        fHideButton = 0;
    else {
        fHideButton = new YFrameButton(fTitleBar, this, actionHide, actionHide);
        //fHideButton->setWinGravity(NorthEastGravity);
        fHideButton->setToolTip("Hide");
        fHideButton->show();
    }

    if (!isButton('r'))
        fRollupButton = 0;
    else {
        fRollupButton = new YFrameButton(fTitleBar, this, actionRollup, actionRollup);
        //fRollupButton->setWinGravity(NorthEastGravity);
        fRollupButton->setToolTip("Rollup");
        fRollupButton->show();
    }

    if (!isButton('d'))
        fDepthButton = 0;
    else {
        fDepthButton = new YFrameButton(fTitleBar, this, actionDepth, actionDepth);
        //fDepthButton->setWinGravity(NorthEastGravity);
        fDepthButton->setToolTip("Raise/Lower");
        fDepthButton->show();
    }

    if (!isButton('s'))
        fMenuButton = 0;
    else {
        fMenuButton = new YFrameButton(fTitleBar, this, 0);
        fMenuButton->show();
        fMenuButton->setActionListener(this);
    }

    getFrameHints();
    updateIcon();

    manage(client);
    insertFrame();

    getDefaultOptions();
    if (frameOptions() & foAllWorkspaces)
        setSticky(true);

    addAsTransient();
    addTransients();

    if (!(frameOptions() & foFullKeys))
        grabKeys();
    fClientContainer->grabButtons();

#ifndef LITE
    if (minimizeToDesktop)
        fMiniIcon = new MiniIcon(fRoot, this, this);
#endif
#if 0
#ifdef CONFIG_WINLIST
    if (fRoot->getWindowList() && !(frameOptions() & foIgnoreWinList))
        fWinListItem = fRoot->getWindowList()->addWindowListApp(this);
#endif
#endif
    fRoot->restackWindows(this);
    if (getLayer() == WinLayerDock)
        fRoot->updateWorkArea();
#ifdef CONFIG_GUIEVENTS
    wmapp->signalGuiEvent(geWindowOpened);
#endif
    fRoot->updateClientList();
}

YFrameWindow::~YFrameWindow() {
    fManaged = false;
    if (fKillMsgBox) {
        fRoot->unmanageClient(fKillMsgBox->handle());
        fKillMsgBox = 0;
    }
#ifdef CONFIG_GUIEVENTS
    wmapp->signalGuiEvent(geWindowClosed);
#endif
    if (fAutoRaiseTimer && fAutoRaiseTimer->getTimerListener() == this) {
        fAutoRaiseTimer->stopTimer();
        fAutoRaiseTimer->setTimerListener(0);
    }
    if (fDelayFocusTimer && fDelayFocusTimer->getTimerListener() == this) {
        fDelayFocusTimer->stopTimer();
        fDelayFocusTimer->setTimerListener(0);
    }
    if (movingWindow || sizingWindow)
        endMoveSize();
    if (fPopupActive)
        fPopupActive->cancelPopup();
#if 0
#ifdef CONFIG_TASKBAR
    if (fTaskBarApp) {
        if (fRoot->getTaskBar() && fRoot->getTaskBar()->taskPane())
            fRoot->getTaskBar()->taskPane()->removeApp(this);
        else
            delete fTaskBarApp;
        fTaskBarApp = 0;
    }
#endif
#endif
#if 0
#ifdef CONFIG_WINLIST
    if (fWinListItem) {
        if (fRoot->getWindowList())
            fRoot->getWindowList()->removeWindowListApp(fWinListItem);
        delete fWinListItem; fWinListItem = 0;
    }
#endif
#endif
    if (fMiniIcon) {
        delete fMiniIcon;
        fMiniIcon = 0;
    }
    // perhaps should be done another way
#ifndef LITE
    if (fRoot->getSwitchWindow())
        fRoot->getSwitchWindow()->destroyedFrame(this);
#endif
    removeTransients();
    removeAsTransient();
    fRoot->removeClientFrame(this);
    removeFrame();
    if (fClient != 0) {
        if (!fClient->destroyed())
            XRemoveFromSaveSet(app->display(), client()->handle());
        XDeleteContext(app->display(), client()->handle(), frameContext);
    }
    if (getLayer() == WinLayerDock)
        fRoot->updateWorkArea();

    delete fClient; fClient = 0;
    delete fClientContainer; fClientContainer = 0;
    delete fMenuButton; fMenuButton = 0;
    delete fCloseButton; fCloseButton = 0;
    delete fMaximizeButton; fMaximizeButton = 0;
    delete fMinimizeButton; fMinimizeButton = 0;
    delete fHideButton; fHideButton = 0;
    delete fRollupButton; fRollupButton = 0;
    delete fTitleBar; fTitleBar = 0;
    delete fDepthButton; fDepthButton = 0;

    XDestroyWindow(app->display(), topSide);
    XDestroyWindow(app->display(), leftSide);
    XDestroyWindow(app->display(), rightSide);
    XDestroyWindow(app->display(), bottomSide);
    XDestroyWindow(app->display(), topLeftCorner);
    XDestroyWindow(app->display(), topRightCorner);
    XDestroyWindow(app->display(), bottomLeftCorner);
    XDestroyWindow(app->display(), bottomRightCorner);
    fRoot->updateClientList();
}


// create 8 windows that are used to show the proper pointer
// on frame (for resize)
void YFrameWindow::createPointerWindows() {
    XSetWindowAttributes attributes;
    unsigned int klass = InputOnly;

    attributes.event_mask = 0;

    attributes.cursor = sizeTopPointer;
    topSide = XCreateWindow(app->display(), handle(), 0, 0, 1, 1, 0,
                            CopyFromParent, klass, CopyFromParent,
                            CWCursor | CWEventMask, &attributes);

    attributes.cursor = sizeLeftPointer;
    leftSide = XCreateWindow(app->display(), handle(), 0, 0, 1, 1, 0,
                            CopyFromParent, klass, CopyFromParent,
                            CWCursor | CWEventMask, &attributes);

    attributes.cursor = sizeRightPointer;
    rightSide = XCreateWindow(app->display(), handle(), 0, 0, 1, 1, 0,
                            CopyFromParent, klass, CopyFromParent,
                            CWCursor | CWEventMask, &attributes);

    attributes.cursor = sizeBottomPointer;
    bottomSide = XCreateWindow(app->display(), handle(), 0, 0, 1, 1, 0,
                            CopyFromParent, klass, CopyFromParent,
                            CWCursor | CWEventMask, &attributes);

    attributes.cursor = sizeTopLeftPointer;
    topLeftCorner = XCreateWindow(app->display(), handle(), 0, 0, 1, 1, 0,
                                  CopyFromParent, klass, CopyFromParent,
                                  CWCursor | CWEventMask, &attributes);

    attributes.cursor = sizeTopRightPointer;
    topRightCorner = XCreateWindow(app->display(), handle(), 0, 0, 1, 1, 0,
                                   CopyFromParent, klass, CopyFromParent,
                                   CWCursor | CWEventMask, &attributes);

    attributes.cursor = sizeBottomLeftPointer;
    bottomLeftCorner = XCreateWindow(app->display(), handle(), 0, 0, 1, 1, 0,
                                     CopyFromParent, klass, CopyFromParent,
                                     CWCursor | CWEventMask, &attributes);

    attributes.cursor = sizeBottomRightPointer;
    bottomRightCorner = XCreateWindow(app->display(), handle(), 0, 0, 1, 1, 0,
                                      CopyFromParent, klass, CopyFromParent,
                                      CWCursor | CWEventMask, &attributes);

    XMapSubwindows(app->display(), handle());
    indicatorsVisible = 1;
}

void YFrameWindow::grabKeys() {
    if (app->AltMask) {
        GRAB_WMKEY(gKeyWinRaise);
        GRAB_WMKEY(gKeyWinOccupyAll);
        GRAB_WMKEY(gKeyWinLower);
        GRAB_WMKEY(gKeyWinClose);
        GRAB_WMKEY(gKeyWinRestore);
        GRAB_WMKEY(gKeyWinNext);
        GRAB_WMKEY(gKeyWinPrev);
        GRAB_WMKEY(gKeyWinMove);
        GRAB_WMKEY(gKeyWinSize);
        GRAB_WMKEY(gKeyWinMinimize);
        GRAB_WMKEY(gKeyWinMaximize);
        GRAB_WMKEY(gKeyWinMaximizeVert);
        GRAB_WMKEY(gKeyWinHide);
        GRAB_WMKEY(gKeyWinRollup);
        GRAB_WMKEY(gKeyWinMenu);
    }
}

void YFrameWindow::manage(YFrameClient *client) {
    PRECONDITION(client != 0);
    fClient = client;

    XSetWindowBorderWidth(app->display(),
                          client->handle(),
                          0);

    XAddToSaveSet(app->display(), client->handle());

    client->reparent(fClientContainer, 0, 0);

    client->setFrame(this);

    sendConfigure();
}

void YFrameWindow::unmanage() {
    PRECONDITION(fClient != 0);

    if (!fClient->destroyed()) {
        int gx, gy;
        client()->gravityOffsets(gx, gy);

        XSetWindowBorderWidth(app->display(),
                              client()->handle(),
                              client()->getBorder());

        int posX, posY, posWidth, posHeight;

        getNormalGeometry(&posX, &posY, &posWidth, &posHeight);
        // !!! check this (+ cleanup)
        if (gx < 0)
            posX -= borderLeft();
        else if (gx > 0)
            posX += borderRight() - 2 * client()->getBorder();
        if (gy < 0)
            posY -= borderTop() + titleY();
        else if (gy > 0)
            posY += borderBottom() - 2 * client()->getBorder();

        client()->reparent(fRoot, posX, posY);
        client()->setSize(posWidth, posHeight);

        if (phase != phaseRestart)
            client()->setFrameState(WithdrawnState);

        if (!client()->destroyed())
            XRemoveFromSaveSet(app->display(), client()->handle());
    }

    client()->setFrame(0);

    fClient = 0;
}

void YFrameWindow::configureClient(const XConfigureRequestEvent &configureRequest) {
    client()->setBorder((configureRequest.value_mask & CWBorderWidth) ? configureRequest.border_width : client()->getBorder());
    int cx = (configureRequest.value_mask & CWX) ? configureRequest.x : x() + borderLeft();
    int cy = (configureRequest.value_mask & CWY) ? configureRequest.y : y() + titleY() + borderTop();
    int cwidth = (configureRequest.value_mask & CWWidth) ? configureRequest.width : client()->width();
    int cheight = (configureRequest.value_mask & CWHeight) ? configureRequest.height : client()->height();

    configureClient(cx, cy, cwidth, cheight);

    if (configureRequest.value_mask & CWStackMode) {
        YFrameWindow *sibling = 0;
        XWindowChanges xwc;

        if ((configureRequest.value_mask & CWSibling) &&
            XFindContext(app->display(),
                         configureRequest.above,
                         clientContext,
                         (XPointer *)&sibling) == 0)
            xwc.sibling = sibling->handle();
        else
            xwc.sibling = configureRequest.above;

        xwc.stack_mode = configureRequest.detail;

        /* !!! implement the rest, and possibly fix these: */

        if (sibling && xwc.sibling != None) { /* ICCCM suggests sibling==None */
            switch (xwc.stack_mode) {
            case Above:
                setAbove(sibling);
                break;
            case Below:
                setBelow(sibling);
                break;
            default:
                return ;
            }
            XConfigureWindow (app->display(),
                              handle(),
                              configureRequest.value_mask & (CWSibling | CWStackMode),
                              &xwc);
        } else if (xwc.sibling == None && fRoot->top(getLayer()) != 0) {
            switch (xwc.stack_mode) {
            case Above:
                if (canRaise()) {
                    wmRaise();
                }
#if 1
                if (!(frameOptions() & foNoFocusOnAppRaise) &&
                   (clickFocus || !strongPointerFocus))
                    activate();
#endif
                break;
            case Below:
                wmLower();
                break;
            default:
                return ;
            }
        } else
            return ;
    }
}

void YFrameWindow::configureClient(int cx, int cy, int cwidth, int cheight) {
    cx -= borderLeft();
    cy -= borderTop() + titleY();
    cwidth += borderLeft() + borderRight();
    cheight += borderTop() + borderBottom() + titleY();

#if 0
    // !!! should be an option
    if (cx != x() || cy != y() ||
        (unsigned int)cwidth != width() || (unsigned int)cheight != height())
        if (isMaximized()) {
            fWinState &= ~(WinStateMaximizedVert | WinStateMaximizedHoriz);
            if (fMaximizeButton) {
                fMaximizeButton->setActions(actionMaximize, actionMaximizeVert);
                fMaximizeButton->setToolTip("Maximize");
            }
        }
#endif

    MSG(("setting geometry (%d:%d %dx%d)", cx, cy, cwidth, cheight));

    if (isIconic()) {
        cx += borderLeft();
        cy += borderTop();
        cwidth -= borderLeft() + borderRight();
        cheight -= borderTop() + borderBottom() + titleY();

        client()->setGeometry(0, 0, cwidth, cheight);

        int nx = cx;
        int ny = cy;
        int nw = cwidth;
        int nh = cheight;
        XSizeHints *sh = client()->sizeHints();
        bool cxw = true;
        bool cy = true;
        bool ch = true;

        if (isMaximizedHoriz())
            cxw = false;
        if (isMaximizedVert())
            cy = ch = false;
        if (isRollup())
            ch = false;

        if (cxw) {
            normalX = nx;
            normalWidth = sh ? (nw - sh->base_width) / sh->width_inc : nw;
        }
        if (cy)
            normalY = ny;
        if (ch)
            normalHeight = sh ? (nh - sh->base_height) / sh->height_inc : nh;
    } else if (isRollup()) {
        //!!!
    } else {
        setGeometry(cx, cy, cwidth, cheight);
    }
}

void YFrameWindow::handleClick(const XButtonEvent &up, int /*count*/) {
    if (up.button == 3) {
        popupSystemMenu(up.x_root, up.y_root, -1, -1,
                        YPopupWindow::pfCanFlipVertical |
                        YPopupWindow::pfCanFlipHorizontal |
                        YPopupWindow::pfPopupMenu);
    }
}

void YFrameWindow::handleCrossing(const XCrossingEvent &crossing) {
    static int old_x = -1, old_y = -1;

    if (crossing.type == EnterNotify &&
        (crossing.mode == NotifyNormal || (strongPointerFocus && crossing.mode == NotifyUngrab)) &&
        crossing.window == handle() &&
        (strongPointerFocus ||
         old_x != crossing.x_root || old_y != crossing.y_root))
    {
        old_x = crossing.x_root;
        old_y = crossing.y_root;

        if (!clickFocus && visible()) {
            if (!delayPointerFocus)
                focus(false);
            else {
                if (fDelayFocusTimer == 0)
                    fDelayFocusTimer = new YTimer(pointerFocusDelay);
                if (fDelayFocusTimer) {
                    fDelayFocusTimer->setTimerListener(this);
                    fDelayFocusTimer->startTimer();
                }
            }
        }
        if (autoRaise) {
            if (fAutoRaiseTimer == 0) {
                fAutoRaiseTimer = new YTimer(autoRaiseDelay);
            }
            if (fAutoRaiseTimer) {
                fAutoRaiseTimer->setTimerListener(this);
                fAutoRaiseTimer->startTimer();
            }
        }
    } else if (crossing.type == LeaveNotify &&
               fFocused &&
               focusRootWindow &&
               crossing.window == handle())
    {
        if (crossing.detail != NotifyInferior &&
            crossing.mode == NotifyNormal)
        {
            if (fDelayFocusTimer && fDelayFocusTimer->getTimerListener() == this) {
                fDelayFocusTimer->stopTimer();
                fDelayFocusTimer->setTimerListener(0);
            }
#if 0 /// !!! focus root
            if (!clickFocus) {
                deactivate();
            }
#endif
            if (autoRaise) {
                if (fAutoRaiseTimer && fAutoRaiseTimer->getTimerListener() == this) {
                    fAutoRaiseTimer->stopTimer();
                    fAutoRaiseTimer->setTimerListener(0);
                }
            }
        }
    }
}

void YFrameWindow::handleFocus(const XFocusChangeEvent &focus) {
#ifndef LITE
    if (fRoot->getSwitchWindow() && fRoot->getSwitchWindow()->visible())
        return ;
#endif
#if 1
    if (focus.type == FocusIn &&
        focus.mode != NotifyGrab &&
        focus.window == handle() &&
        focus.detail != NotifyInferior &&
        focus.detail != NotifyPointer &&
        focus.detail != NotifyPointerRoot)
    {
        fRoot->switchFocusTo(this);
    }
#endif
#if 0
    else if (focus.type == FocusOut &&
               focus.mode == NotifyNormal &&
               focus.detail != NotifyInferior &&
               focus.detail != NotifyPointer &&
               focus.detail != NotifyPointerRoot &&
               focus.window == handle())
    {
        manager->switchFocusFrom(this);
    }
#endif
}

bool YFrameWindow::handleTimer(YTimer *t) {
    if (t == fAutoRaiseTimer) {
        if (canRaise())
            wmRaise();
    }
    if (t == fDelayFocusTimer)
        focus(false);
    return false;
}

void YFrameWindow::raise() {
    if (this != fRoot->top(getLayer())) {
        YWindow::raise();
        setAbove(fRoot->top(getLayer()));
    }
}

void YFrameWindow::lower() {
    if (this != fRoot->bottom(getLayer())) {
        YWindow::lower();
        setAbove(0);
    }
}

void YFrameWindow::removeFrame() {
#ifdef DEBUG
    if (debug_z) dumpZorder(fRoot, "before removing", this);
#endif
    if (prev())
        prev()->setNext(next());
    else
        fRoot->setTop(getLayer(), next());

    if (next())
        next()->setPrev(prev());
    else
        fRoot->setBottom(getLayer(), prev());

    setPrev(0);
    setNext(0);

#ifdef DEBUG
    if (debug_z) dumpZorder(fRoot, "after removing", this);
#endif
}

void YFrameWindow::insertFrame() {
#ifdef DEBUG
    if (debug_z) dumpZorder(fRoot, "before inserting", this);
#endif
    setNext(fRoot->top(getLayer()));
    setPrev(0);
    if (next())
        next()->setPrev(this);
    else
        fRoot->setBottom(getLayer(), this);
    fRoot->setTop(getLayer(), this);
#ifdef DEBUG
    if (debug_z) dumpZorder(fRoot, "after inserting", this);
#endif
}

void YFrameWindow::setAbove(YFrameWindow *aboveFrame) {
#ifdef DEBUG
    if (debug_z) dumpZorder(fRoot, "before setAbove", this, aboveFrame);
#endif
    if (aboveFrame != next() && aboveFrame != this) {
        if (prev())
            prev()->setNext(next());
        else
            fRoot->setTop(getLayer(), next());

        if (next())
            next()->setPrev(prev());
        else
            fRoot->setBottom(getLayer(), prev());

        setNext(aboveFrame);
        if (next()) {
            setPrev(next()->prev());
            next()->setPrev(this);
        } else {
            setPrev(fRoot->bottom(getLayer()));
            fRoot->setBottom(getLayer(), this);
        }
        if (prev())
            prev()->setNext(this);
        else
            fRoot->setTop(getLayer(), this);
#ifdef DEBUG
        if (debug_z) dumpZorder(fRoot, "after setAbove", this, aboveFrame);
#endif
    }
}

void YFrameWindow::setBelow(YFrameWindow *belowFrame) {
    if (belowFrame != next())
        setAbove(belowFrame->next());
}

YFrameWindow *YFrameWindow::findWindow(int flags) {
    YFrameWindow *p = this;

     if (flags & fwfNext)
         goto next;

     do {
         if ((flags & fwfMinimized) && !p->isMinimized())
             goto next;
         if ((flags & fwfUnminimized) && p->isMinimized())
             goto next;
         if ((flags & fwfVisible) && !p->visible())
             goto next;
         if ((flags & fwfHidden) && !p->isHidden())
             goto next;
         if ((flags & fwfNotHidden) && p->isHidden())
             goto next;
         if ((flags & fwfFocusable) && !p->isFocusable())
             goto next;
         if ((flags & fwfWorkspace) && !p->visibleNow())
             goto next;
         if ((flags & fwfSwitchable) && (p->frameOptions() & foIgnoreQSwitch))
             goto next;
         if (!p->client()->adopted())
             goto next;

         return p;

     next:
         if (flags & fwfBackward)
             p = (flags & fwfLayers) ? p->prevLayer() : p->prev();
         else
             p = (flags & fwfLayers) ? p->nextLayer() : p->next();
         if (p == 0)
             if (!(flags & fwfCycle))
                 return 0;
             else if (flags & fwfBackward)
                 p = (flags & fwfLayers) ? fRoot->bottomLayer() : fRoot->bottom(getLayer());
             else
                 p = (flags & fwfLayers) ? fRoot->topLayer() : fRoot->top(getLayer());
     } while (p != this);

     if (!(flags & fwfSame))
         return 0;
     if ((flags & fwfVisible) && !p->visible())
         return 0;
     if ((flags & fwfFocusable) && !p->isFocusable())
         return 0;
     if ((flags & fwfWorkspace) && !p->visibleNow())
         return 0;
     if (!p->client()->adopted())
         return 0;

     return this;
}

void YFrameWindow::handleConfigure(const XConfigureEvent &/*configure*/) {
}

void YFrameWindow::sendConfigure() {
    XEvent xev;

    memset(&xev, 0, sizeof(xev));
    xev.xconfigure.type = ConfigureNotify;
    xev.xconfigure.display = app->display();
    xev.xconfigure.event = client()->handle();
    xev.xconfigure.window = client()->handle();
    xev.xconfigure.x = x() + borderLeft();
    xev.xconfigure.y = y() + borderTop()
#ifndef TITLEBAR_BOTTOM
        + titleY()
#endif
        ;
    xev.xconfigure.width = client()->width();
    xev.xconfigure.height = client()->height();
    xev.xconfigure.border_width = client()->getBorder();

    xev.xconfigure.above = None;
    xev.xconfigure.override_redirect = False;

#ifdef DEBUG_C
    Status rc =
#endif
        XSendEvent(app->display(),
               client()->handle(),
               False,
               StructureNotifyMask,
               &xev);

#ifdef DEBUG_C
    MSG(("sent %d: x=%d, y=%d, width=%d, height=%d",
         rc,
         x(),
         y(),
         client()->width(),
         client()->height()));
#endif
}

void YFrameWindow::actionPerformed(YAction *action, unsigned int modifiers) {
    if (action == actionRestore) {
        wmRestore();
    } else if (action == actionMinimize) {
        if (canMinimize())
            wmMinimize();
    } else if (action == actionMaximize) {
        if (canMaximize())
            wmMaximize();
    } else if (action == actionMaximizeVert) {
        if (canMaximize())
            wmMaximizeVert();
    } else if (action == actionLower) {
        if (canLower())
            wmLower();
    } else if (action == actionRaise) {
        if (canRaise())
            wmRaise();
    } else if (action == actionDepth) {
        if (Overlaps(true) && canRaise()){
            wmRaise();
            fRoot->setFocus(this, true);
        } else if (Overlaps(false) && canLower())
            wmLower();
    } else if (action == actionRollup) {
        if (canRollup())
            wmRollup();
    } else if (action == actionClose) {
        if (canClose())
            wmClose();
    } else if (action == actionKill) {
        wmConfirmKill();
    } else if (action == actionHide) {
        if (canHide())
            wmHide();
    } else if (action == actionShow) {
        wmShow();
    } else if (action == actionMove) {
        if (canMove())
            wmMove();
    } else if (action == actionSize) {
        if (canSize())
            wmSize();
    } else if (action == actionOccupyAllOrCurrent) {
        wmOccupyAllOrCurrent();
    } else {
        for (int l = 0; l < WinLayerCount; l++) {
            if (action == layerActionSet[l]) {
                wmSetLayer(l);
                return ;
            }
        }
        for (int w = 0; w < fRoot->workspaceCount(); w++) {
            if (action == workspaceActionMoveTo[w]) {
                wmMoveToWorkspace(w);
                return ;
            }
        }
        fRoot->actionPerformed(action, modifiers);
    }
}

void YFrameWindow::wmSetLayer(long layer) {
    setLayer(layer);
}

void YFrameWindow::wmMove() {
    Window root, child;
    int rx, ry, wx, wy;
    unsigned int mask;
    XQueryPointer(app->display(), desktop->handle(),
                  &root, &child, &rx, &ry, &wx, &wy, &mask);
    if (wx > int(x() + width()))
        wx = x() + width();
    if (wy > int(y() + height()))
        wy = y() + height();
    if (wx < x())
        wx = x();
    if (wy < y())
        wy = y();
    startMoveSize(1, 0,
                  0, 0,
                  wx - x(), wy - y());
}

void YFrameWindow::wmSize() {
    startMoveSize(0, 0,
                  0, 0,
                  0, 0);
}

void YFrameWindow::wmRestore() {
#ifdef CONFIG_GUIEVENTS
    wmapp->signalGuiEvent(geWindowRestore);
#endif
    setState(WinStateMaximizedVert | WinStateMaximizedHoriz |
             WinStateMinimized |
             WinStateHidden |
             WinStateRollup, 0);
}

void YFrameWindow::wmMinimize() {
#ifdef DEBUG_S
    MSG(("wmMinimize - Frame: %d", visible()));
    MSG(("wmMinimize - Client: %d", client()->visible()));
#endif
    if (isMinimized()) {
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowRestore);
#endif
        setState(WinStateMinimized, 0);
    } else {
        //if (!canMinimize())
        //    return ;
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowMin);
#endif
        setState(WinStateMinimized, WinStateMinimized);
        wmLower();
    }
    if (clickFocus || !strongPointerFocus)
        fRoot->focusTopWindow();
}

void YFrameWindow::minimizeTransients() {
    YFrameWindow *w = transient();
    while (w) {
        w->fWasMinimized = w->isMinimized();
        if (!w->fWasMinimized && !w->isMinimized())
            w->wmMinimize();
        w = w->nextTransient();
    }
}

void YFrameWindow::restoreTransients() {
    YFrameWindow *w = transient();
    while (w) {
        if (!w->fWasMinimized && w->isMinimized())
            w->wmMinimize();
        w->fWasMinimized = false;
        w = w->nextTransient();
    }
}

void YFrameWindow::DoMaximize(long flags) {
    setState(WinStateRollup, 0);

    if (isMaximized()) {
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowRestore);
#endif
        setState(WinStateMaximizedVert |
                 WinStateMaximizedHoriz |
                 WinStateMinimized, 0);
    } else {
        //if (!canMaximize())
        //    return ;

#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowMax);
#endif
        setState(WinStateMaximizedVert |
                 WinStateMaximizedHoriz |
                 WinStateMinimized, flags);
    }
}

void YFrameWindow::wmMaximize() {
    DoMaximize(WinStateMaximizedVert | WinStateMaximizedHoriz);
}

void YFrameWindow::wmMaximizeVert() {
    if (isMaximizedVert()) {
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowRestore);
#endif
        setState(WinStateMaximizedVert, 0);
    } else {
        //if (!canMaximize())
        //    return ;
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowMax);
#endif
        setState(WinStateMaximizedVert, WinStateMaximizedVert);
    }
}

void YFrameWindow::wmMaximizeHorz() {
    if (isMaximizedHoriz()) {
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowRestore);
#endif
        setState(WinStateMaximizedHoriz, 0);
    } else {
        //if (!canMaximize())
        //    return ;
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowMax);
#endif
        setState(WinStateMaximizedHoriz, WinStateMaximizedHoriz);
    }
}

void YFrameWindow::wmRollup() {
    if (isRollup()) {
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowRestore);
#endif
        setState(WinStateRollup, 0);
    } else {
        //if (!canRollup())
        //    return ;
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowRollup);
#endif
        setState(WinStateRollup, WinStateRollup);
    }
}

void YFrameWindow::wmHide() {
    if (isHidden()) {
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowRestore);
#endif
        setState(WinStateHidden, 0);
    } else {
        //if (!canHide())
        //    return ;

#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowHide);
#endif
        setState(WinStateHidden, WinStateHidden);
    }
    if (clickFocus || !strongPointerFocus)
        fRoot->focusTopWindow();
}

void YFrameWindow::wmLower() {
    if (this != fRoot->bottom(getLayer())) {
        YFrameWindow *w = this;

#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowLower);
#endif
        while (w) {
            w->doLower();
            w = w->owner();
        }
        fRoot->restackWindows(this);

        if (clickFocus || !strongPointerFocus)
            fRoot->focusTopWindow();
    }
}

void YFrameWindow::doLower() {
    setAbove(0);
}

void YFrameWindow::wmRaise() {
    doRaise();
    fRoot->restackWindows(this);
}

void YFrameWindow::doRaise() {
#ifdef DEBUG
    if (debug_z) dumpZorder(fRoot, "wmRaise: ", this);
#endif
    if (this != fRoot->top(getLayer())) {
        setAbove(fRoot->top(getLayer()));
        {
            YFrameWindow *w = transient();
            while (w) {
                w->doRaise();
                w = w->nextTransient();
            }
        }
#ifdef DEBUG
        if (debug_z) dumpZorder(fRoot, "wmRaise after raise: ", this);
#endif
    }
}

void YFrameWindow::wmClose() {
    if (!canClose())
        return ;

    XGrabServer(app->display());
    client()->getProtocols();

    if (client()->protocols() & YFrameClient::wpDeleteWindow)
        client()->sendMessage(_XA_WM_DELETE_WINDOW);
    else {
        wmConfirmKill();
    }
    XUngrabServer(app->display());
}

void YFrameWindow::wmConfirmKill() {
#ifndef LITE
    if (fKillMsgBox == 0) {
        YMsgBox *msgbox = new YMsgBox(YMsgBox::mbOK|YMsgBox::mbCancel);
        char *title = strJoin("Kill Client: ", getTitle(), 0);
        fKillMsgBox = msgbox;

        msgbox->setTitle(title);
        delete title; title = 0;
        msgbox->setText("Warning! Unsaved changes will be lost!\nProceed?");
        msgbox->autoSize();
        msgbox->setMsgBoxListener(this);
        msgbox->showFocused();
    }
#endif
}

void YFrameWindow::wmKill() {
    if (!canClose())
        return ;
#ifdef DEBUG
    if (debug)
        msg("No WM_DELETE_WINDOW protocol");
#endif
    XKillClient(app->display(), client()->handle());
}

void YFrameWindow::wmPrevWindow() {
    if (next() != this) {
        YFrameWindow *f = findWindow(fwfNext | fwfBackward | fwfVisible | fwfCycle | fwfFocusable | fwfWorkspace | fwfSame);
        if (f) {
            f->wmRaise();
            fRoot->setFocus(f, true);
        }
    }
}

void YFrameWindow::wmNextWindow() {
    if (next() != this) {
        wmLower();
        fRoot->setFocus(findWindow(fwfNext | fwfVisible | fwfCycle | fwfFocusable | fwfWorkspace | fwfSame), true);
    }
}

void YFrameWindow::wmLastWindow() {
    if (next() != this) {
        YFrameWindow *f = findWindow(fwfNext | fwfVisible | fwfCycle | fwfFocusable | fwfWorkspace | fwfSame);
        if (f) {
            f->wmRaise();
            fRoot->setFocus(f, true);
        }
    }
}

void YFrameWindow::loseWinFocus() {
    if (fFocused && fManaged) {
        fFocused = false;

        if (true || !clientMouseActions)
            if (focusOnClickClient || raiseOnClickClient)
                if (fClientContainer)
                    fClientContainer->grabButtons();
        if (isIconic())
            fMiniIcon->repaint();
        else {
            repaint();
            titlebar()->deactivate();
        }
#if 0
#ifdef CONFIG_TASKBAR
        updateTaskBar();
#endif
#endif
    }
}

void YFrameWindow::setWinFocus() {
    if (!fFocused) {
        fFocused = true;

        if (isIconic())
            fMiniIcon->repaint();
        else {
            titlebar()->activate();
            repaint();
        }
#if 0
#ifdef CONFIG_TASKBAR
        updateTaskBar();
#endif
#endif

        if (true || !clientMouseActions)
            if (focusOnClickClient &&
                !(raiseOnClickClient && (this != fRoot->top(getLayer()))))
                fClientContainer->releaseButtons();
    }
}

void YFrameWindow::focusOnMap() {
    if (owner() != 0) {
        if (focusOnMapTransient)
            if (owner()->focused() || !focusOnMapTransientActive)
                activate();
    } else {
        if (::focusOnMap)
            activate();
    }
}

void YFrameWindow::wmShow() {
    // recover lost (offscreen) windows !!! (unify with code below)
    // !!! fix to handle workarea
    if (x() >= int(fRoot->width()) ||
        y() >= int(fRoot->height()) ||
        x() <= - int(width()) ||
        y() <= - int(height()))
    {
        int newX = x();
        int newY = y();

        if (x() >= int(fRoot->width()))
            newX = int(fRoot->width() - width() + borderRight());
        if (y() >= int(fRoot->height()))
            newY = int(fRoot->height() - height() + borderBottom());

        if (newX < int(- borderLeft()))
            newX = int(- borderLeft());
        if (newY < int(- borderTop()))
            newY = int(- borderTop());
        setPosition(newX, newY);
    }

    setState(WinStateHidden | WinStateMinimized, 0);
}

void YFrameWindow::focus(bool canWarp) {
    if (!visibleOn(fRoot->activeWorkspace()))
        fRoot->activateWorkspace(getWorkspace());
    // recover lost (offscreen) windows !!!
    if (limitPosition &&
        (x() >= int(fRoot->width()) ||
         y() >= int(fRoot->height()) ||
         x() <= - int(width()) ||
         y() <= - int(height())))
    {
        int newX = x();
        int newY = y();

        if (x() >= int(fRoot->width()))
            newX = int(fRoot->width() - width() + borderRight());
        if (y() >= int(fRoot->height()))
            newY = int(fRoot->height() - height() + borderBottom());

        if (newX < int(- borderLeft()))
            newX = int(- borderLeft());
        if (newY < int(- borderTop()))
            newY = int(- borderTop());
        setPosition(newX, newY);
    }

    if (isFocusable())
        fRoot->setFocus(this, canWarp);
    if (raiseOnFocus && /* clickFocus && */ phase == phaseRunning)
        wmRaise();
}

void YFrameWindow::activate(bool canWarp) {
    if (fWinState & (WinStateHidden | WinStateMinimized))
        setState(WinStateHidden | WinStateMinimized, 0);
    focus(canWarp);
}

void YFrameWindow::paint(Graphics &g, int , int , unsigned int , unsigned int ) {
    YColor *bg;

    if (!(frameDecors() & (fdResize | fdBorder)))
        return ;

    if (focused())
        bg = activeBorderBg;
    else
        bg = inactiveBorderBg;

    g.setColor(bg);
    switch (wmLook) {
#if defined(CONFIG_LOOK_WIN95) || defined(CONFIG_LOOK_WARP4) || defined(CONFIG_LOOK_NICE)
#ifdef CONFIG_LOOK_WIN95
    case lookWin95:
#endif
#ifdef CONFIG_LOOK_WARP4
    case lookWarp4:
#endif
#ifdef CONFIG_LOOK_NICE
    case lookNice:
#endif
        g.fillRect(1, 1, width() - 3, height() - 3);
        g.drawBorderW(0, 0, width() - 1, height() - 1, true);
        break;
#endif
#if defined(CONFIG_LOOK_MOTIF) || defined(CONFIG_LOOK_WARP3)
#ifdef CONFIG_LOOK_MOTIF
    case lookMotif:
#endif
#ifdef CONFIG_LOOK_WARP3
    case lookWarp3:
#endif
        g.draw3DRect(0, 0, width() - 1, height() - 1, true);
        g.draw3DRect(borderLeft() - 1, borderTop() - 1,
                     width() - (borderLeft() + borderRight()) + 1, height() - (borderTop() + borderBottom()) + 1,
                     false);

        g.fillRect(1, 1, width() - 2, borderTop() - 2);
        g.fillRect(1, 1, borderLeft() - 2, height() - 2);
        g.fillRect(1, (height() - 1) - (borderBottom() - 2), width() - 2, borderLeft() - 2);
        g.fillRect((width() - 1) - (borderRight() - 2), 1, borderLeft() - 2, height() - 2);

#ifdef CONFIG_LOOK_MOTIF
        if (wmLook == lookMotif && canSize()) {
            YColor *b = bg->brighter();
            YColor *d = bg->darker();


            g.setColor(d);
            g.drawLine(wsCornerX - 1, 0, wsCornerX - 1, height() - 1);
            g.drawLine(width() - wsCornerX - 1, 0, width() - wsCornerX - 1, height() - 1);
            g.drawLine(0, wsCornerY - 1, width(),wsCornerY - 1);
            g.drawLine(0, height() - wsCornerY - 1, width(), height() - wsCornerY - 1);
            g.setColor(b);
            g.drawLine(wsCornerX, 0, wsCornerX, height() - 1);
            g.drawLine(width() - wsCornerX, 0, width() - wsCornerX, height() - 1);
            g.drawLine(0, wsCornerY, width(), wsCornerY);
            g.drawLine(0, height() - wsCornerY, width(), height() - wsCornerY);
        }
        break;
#endif
#endif
#ifdef CONFIG_LOOK_PIXMAP
    case lookPixmap:
    case lookMetal:
    case lookGtk:
        {
            int n = focused() ? 1 : 0;
            int t = (frameDecors() & fdResize) ? 0 : 1;

            if (frameTL[t][n] &&
                frameT[t][n] &&
                frameTR[t][n] &&
                frameL[t][n] &&
                frameR[t][n] &&
                frameBL[t][n] &&
                frameB[t][n] &&
                frameBR[t][n])
            {
                g.drawPixmap(frameTL[t][n], 0, 0);
                g.repHorz(frameT[t][n], wsCornerX, 0, width() - 2 * wsCornerX);
                g.drawPixmap(frameTR[t][n], width() - wsCornerX, 0);
                g.repVert(frameL[t][n], 0, wsCornerY, height() - 2 * wsCornerY);
                g.repVert(frameR[t][n], width() - borderRight(), wsCornerY, height() - 2 * wsCornerY);
                g.drawPixmap(frameBL[t][n], 0, height() - wsCornerY);
                g.repHorz(frameB[t][n], wsCornerX, height() - borderBottom(), width() - 2 * wsCornerX);
                g.drawPixmap(frameBR[t][n], width() - wsCornerX, height() - wsCornerY);
            } else {
                g.fillRect(1, 1, width() - 3, height() - 3);
                g.drawBorderW(0, 0, width() - 1, height() - 1, true);
            }
        }
        break;
#endif
    default:
        break;
    }
}

void YFrameWindow::handlePopDown(YPopupWindow *popup) {
    MSG(("popdown %ld up %ld", popup, fPopupActive));
    if (fPopupActive == popup)
        fPopupActive = 0;
}

void YFrameWindow::popupSystemMenu() {
    if (fPopupActive == 0) {
        if (fMenuButton && fMenuButton->visible() &&
            fTitleBar && fTitleBar->visible())
            fMenuButton->popupMenu();
        else {
            int ax = x() + container()->x();
            int ay = y() + container()->y();
            if (isIconic()) {
                ax = x();
                ay = y() + height();
            }
            popupSystemMenu(ax, ay,
                            -1, -1, //width(), height(),
                            YPopupWindow::pfCanFlipVertical);
        }
    }
}

void YFrameWindow::popupSystemMenu(int x, int y,
                                   int x_delta, int y_delta,
                                   unsigned int flags,
                                   YWindow *forWindow)
{
    if (fPopupActive == 0) {
        updateMenu();
        if (windowMenu()->popup(forWindow, this,
                                x, y, x_delta, y_delta, flags))
            fPopupActive = windowMenu();
    }
}

void YFrameWindow::updateTitle() {
    titlebar()->repaint();
    updateIconTitle();
#if 0
#ifdef CONFIG_WINLIST
    if (fWinListItem) {
        fRoot->getWindowList()->repaintItem(fWinListItem);
    }
#endif
#ifdef CONFIG_TASKBAR
    if (fTaskBarApp) {
        fTaskBarApp->setToolTip((const char *)client()->windowTitle());
    }
#endif
#endif
}

void YFrameWindow::updateIconTitle() {
#if 0
#ifdef CONFIG_TASKBAR
    if (fTaskBarApp) {
        fTaskBarApp->repaint();
        fTaskBarApp->setToolTip((const char *)client()->windowTitle());
    }
#endif
#endif
    if (isIconic()) {
        fMiniIcon->repaint();
    }
}

void YFrameWindow::wmOccupyAllOrCurrent() {
    if (isSticky()) {
        setWorkspace(fRoot->activeWorkspace());
        setSticky(false);
    } else {
        setSticky(true);
    }
#if 0
#ifdef CONFIG_TASKBAR
    if (fRoot->getTaskBar() && fRoot->getTaskBar()->taskPane())
        fRoot->getTaskBar()->taskPane()->relayout();
#endif
#endif
}

void YFrameWindow::wmOccupyAll() {
    setSticky(!isSticky());
    if (getLayer() == WinLayerDock)
        fRoot->updateWorkArea();
#if 0
#ifdef CONFIG_TASKBAR
    if (fRoot->getTaskBar() && fRoot->getTaskBar()->taskPane())
        fRoot->getTaskBar()->taskPane()->relayout();
#endif
#endif
}

void YFrameWindow::wmOccupyWorkspace(long workspace) {
    PRECONDITION(workspace < gWorkspaceCount);
    setWorkspace(workspace);
}

void YFrameWindow::wmOccupyOnlyWorkspace(long workspace) {
    PRECONDITION(workspace < gWorkspaceCount);
    setWorkspace(workspace);
    setSticky(false);
}

void YFrameWindow::wmMoveToWorkspace(long workspace) {
    wmOccupyOnlyWorkspace(workspace);
}

void YFrameWindow::getFrameHints() {
#ifndef NO_MWM_HINTS
    long decors = client()->mwmDecors();
    long functions = client()->mwmFunctions();
#ifdef GNOME1_HINTS
    long win_hints = client()->winHints();
#endif

    fFrameFunctions = 0;
    fFrameDecors = 0;
    fFrameOptions = 0;

    if (decors & MWM_DECOR_BORDER)      fFrameDecors |= fdBorder;
    if (decors & MWM_DECOR_RESIZEH)     fFrameDecors |= fdResize;
    if (decors & MWM_DECOR_TITLE)       fFrameDecors |= fdTitleBar | fdDepth;
    if (decors & MWM_DECOR_MENU)        fFrameDecors |= fdSysMenu;
    if (decors & MWM_DECOR_MAXIMIZE)    fFrameDecors |= fdMaximize;
    if (decors & MWM_DECOR_MINIMIZE)    fFrameDecors |= fdMinimize | fdHide | fdRollup;

    if (functions & MWM_FUNC_MOVE)      fFrameFunctions |= ffMove;
    if (functions & MWM_FUNC_RESIZE)    fFrameFunctions |= ffResize;
    if (functions & MWM_FUNC_MAXIMIZE)  fFrameFunctions |= ffMaximize;
    if (functions & MWM_FUNC_MINIMIZE)
        fFrameFunctions |= ffMinimize | ffHide | ffRollup;
    if (functions & MWM_FUNC_CLOSE) {
        fFrameFunctions |= ffClose;
        fFrameDecors |= fdClose; /* hack */
    }
#else
    fFrameFunctions =
        ffMove | ffResize | ffClose |
        ffMinimize | ffMaximize | ffHide | ffRollup;
    fFrameDecors =
        fdTitleBar | fdSysMenu | fdBorder | fdResize | fdDepth |
        fdClose | fdMinimize | fdMaximize | fdHide | fdRollup;

#endif

#ifdef GNOME1_HINTS
    if (win_hints & WinHintsSkipFocus)
        fFrameOptions |= foIgnoreQSwitch;
    if (win_hints & WinHintsSkipWindowMenu)
        fFrameOptions |= foIgnoreWinList;
    if (win_hints & WinHintsSkipTaskBar)
        fFrameOptions |= foIgnoreTaskBar;
#endif

#ifndef NO_WINDOW_OPTIONS
    WindowOption wo;

    getWindowOptions(wo, false);

    /*fprintf(stderr, "decor: %lX %lX %lX %lX %lX %lX",
            wo.function_mask, wo.functions,
            wo.decor_mask, wo.decors,
            wo.option_mask, wo.options);*/

    fFrameFunctions &= ~wo.function_mask;
    fFrameFunctions |= wo.functions;
    fFrameDecors &= ~wo.decor_mask;
    fFrameDecors |= wo.decors;
    fFrameOptions &= ~wo.option_mask;
    fFrameOptions |= wo.options;
#endif
}

#ifndef NO_WINDOW_OPTIONS

void YFrameWindow::getWindowOptions(WindowOption &opt, bool remove) {
    memset((void *)&opt, 0, sizeof(opt));
    opt.workspace = WinWorkspaceInvalid;
    opt.layer = WinLayerInvalid;

    if (defOptions)
        getWindowOptions(defOptions, opt, false);
    if (hintOptions)
        getWindowOptions(hintOptions, opt, remove);
}

void YFrameWindow::getWindowOptions(WindowOptions *list, WindowOption &opt, bool remove) {
    XClassHint *h = client()->classHint();
    WindowOption *wo;

    if (!h)
        return;

    if (h->res_name && h->res_class) {
        char *both = new char[strlen(h->res_name) + 1 +
                              strlen(h->res_class) + 1];
        if (both) {
            strcpy(both, h->res_name);
            strcat(both, ".");
            strcat(both, h->res_class);
        }
        wo = both ? list->getWindowOption(both, false, remove) : 0;
        if (wo) WindowOptions::combineOptions(opt, *wo);
        delete both;
    }
    if (h->res_class) {
        wo = list->getWindowOption(h->res_class, false, remove);
        if (wo) WindowOptions::combineOptions(opt, *wo);
    }
    if (h->res_name) {
        wo = list->getWindowOption(h->res_name, false, remove);
        if (wo) WindowOptions::combineOptions(opt, *wo);
    }
    wo = list->getWindowOption(0, false, remove);
    if (wo) WindowOptions::combineOptions(opt, *wo);
}
#endif

void YFrameWindow::getDefaultOptions() {
#ifndef NO_WINDOW_OPTIONS
    WindowOption wo;
    getWindowOptions(wo, true);

    if (wo.icon) {
#ifndef LITE
        if (fFrameIcon)
            delete fFrameIcon;
        fFrameIcon = app->getIcon(wo.icon);
#endif
    }
    if (wo.workspace != (long)WinWorkspaceInvalid && wo.workspace < fRoot->workspaceCount() && wo.workspace >= 0)
        setWorkspace(wo.workspace);
    if (wo.layer != (long)WinLayerInvalid && wo.layer < WinLayerCount)
        setLayer(wo.layer);
#endif
}

#ifndef LITE
YIcon *newClientIcon(int count, int reclen, long *elem) {
    int i;
    YPixmap *small = 0, *large = 0;

    if (reclen < 2)
        return 0;

    for (i = 0; i < count; i++, elem += reclen) {
        Pixmap pixmap, mask;

        pixmap = elem[0];
        mask = elem[1];

        if (pixmap == None)
            continue;

        int x, y;
        Window root;
        unsigned int w = 0, h = 0, border, depth = 0;

        if (reclen >= 6) {
            w = elem[2];
            h = elem[3];
            depth = elem[4];
            root = elem[5];
        } else {
            if (XGetGeometry(app->display(), pixmap,
                             &root, &x, &y, &w, &h, &border, &depth) == BadDrawable)
                continue;
        }

        if (w != h || w == 0 || h == 0)
            continue;

        if (depth == (unsigned)DefaultDepth(app->display(),
                                            DefaultScreen(app->display())))
        {
            //printf("%d %d\n", w, h);
            if (w == ICON_SMALL)
                small = new YPixmap(pixmap, mask, w, h, false);
            else if (w == ICON_LARGE)
                large = new YPixmap(pixmap, mask, w, h, false);
        }
    }
    if (small || large)
        return new YIcon(small, large);
    else
        return 0;
}

void YFrameWindow::updateIcon() {
    int count;
    Pixmap *pixmap;
#ifdef GNOME1_HINTS
    Atom type;
    long *elem;

    if (client()->getWinIcons(&type, &count, &elem)) {
        if (type == _XA_WIN_ICONS)
            fFrameIcon = newClientIcon(elem[0], elem[1], elem + 2);
        else // compatibility
            fFrameIcon = newClientIcon(count, 2, elem);
        XFree(elem);
    } else
#endif
    if (client()->getKwmIcon(&count, &pixmap) && count == 2) {
        XWMHints *h = client()->hints();
        if (h && (h->flags & IconPixmapHint)) {
            long pix[4];
            pix[0] = pixmap[0];
            pix[1] = pixmap[1];
            pix[2] = h->icon_pixmap;
            pix[3] = (h->flags & IconMaskHint) ? h->icon_mask : None;
            fFrameIcon = newClientIcon(2, 2, pix);
        } else {
            long pix[2];
            for (int i = 0; i < count; i++) {
                pix[i] = pixmap[i];
            }
            pix[0] = pixmap[0];
            pix[1] = pixmap[1];
            fFrameIcon = newClientIcon(count / 2, 2, pix);
        }
        XFree(pixmap);
    } else {
        XWMHints *h = client()->hints();
        if (h && (h->flags & IconPixmapHint) && (h->flags & IconMaskHint)) {
            long pix[2];
            pix[0] = h->icon_pixmap;
            pix[1] = (h->flags & IconMaskHint) ? h->icon_mask : None;
            fFrameIcon = newClientIcon(1, 2, pix);
        }
    }
    if (fFrameIcon) {
        if (fFrameIcon->small() == 0 &&
            fFrameIcon->large() == 0)
        {
            delete fFrameIcon; fFrameIcon = 0;
        }
    }
}
#endif

YFrameWindow *YFrameWindow::nextLayer() {
    if (fNextFrame)
        return fNextFrame;
    long l = getLayer();
    while (l-- > 0)
        if (fRoot->top(l))
            return fRoot->top(l);
    return 0;
}

YFrameWindow *YFrameWindow::prevLayer() {
    if (fPrevFrame)
        return fPrevFrame;
    long l = getLayer();
    while (++l < WinLayerCount)
        if (fRoot->bottom(l))
            return fRoot->bottom(l);
    return 0;
}

YMenu *YFrameWindow::windowMenu() {
    //if (frameOptions() & foFullKeys)
    //    return windowMenuNoKeys;
    //else
    return ::windowMenu;
}

void YFrameWindow::addAsTransient() {
    Window fTransientFor = client()->ownerWindow();
    if (fTransientFor) {
        fOwner = fRoot->findFrame(fTransientFor);
        if (fOwner != 0) {
            MSG(("transient for 0x%lX: 0x%lX", fTransientFor, fOwner));
            if (fOwner) {
                fNextTransient = fOwner->transient();
                fOwner->setTransient(this);
            } else {
                fTransientFor = 0; // ?

                fNextTransient = 0;
                fOwner = 0;
            }
        }
    }
}

void YFrameWindow::removeAsTransient() {
    if (fOwner) {
        MSG(("removeAsTransient"));

        YFrameWindow *w = fOwner->transient(), *cp = 0;

        while (w) {
            if (w == this) {
                if (cp)
                    cp->setNextTransient(nextTransient());
                else
                    fOwner->setTransient(nextTransient());
            }
            w = w->nextTransient();
        }
        fOwner = 0;
        fNextTransient = 0;
    }
}

void YFrameWindow::addTransients() {
    YFrameWindow *w = fRoot->bottomLayer();

    while (w) {
        if (w->owner() == 0)
            w->addAsTransient();
        w = w->prevLayer();
    }
}

void YFrameWindow::removeTransients() {
    if (transient()) {
        MSG(("removeTransients"));
        YFrameWindow *w = transient(), *n;

        while (w) {
            n = w->nextTransient();
            w->setNextTransient(0);
            w->setOwner(0);
            w = n;
        }
        fTransient = 0;
    }
}

bool YFrameWindow::isModal() {
    if (!client())
        return false;

    MwmHints *mwmHints = client()->mwmHints();
    if (mwmHints && (mwmHints->flags & MWM_HINTS_INPUT_MODE))
        if (mwmHints->input_mode != MWM_INPUT_MODELESS)
            return true;

    if (hasModal())
        return true;

    return false;
}

bool YFrameWindow::hasModal() {
    YFrameWindow *w = transient();
    while (w) {
        if (w->isModal())
            return true;
        w = w->nextTransient();
    }
    /* !!! missing code for app modal dialogs */
    return false;
}

bool YFrameWindow::isFocusable() {
    if (hasModal())
        return false;

    if (!client())
        return false;

    XWMHints *hints = client()->hints();

    if (!hints)
        return true;
    if (frameOptions() & foIgnoreNoFocusHint)
        return true;
    if (!(hints->flags & InputHint))
        return true;
    if (hints->input)
        return true;
#if 0
    if (client()->protocols() & YFrameClient::wpTakeFocus)
        return true;
#endif
    return false;
}

void YFrameWindow::setWorkspace(long workspace) {
    if (workspace >= fRoot->workspaceCount() || workspace < 0)
        return ;
    if (workspace != fWinWorkspace) {
        fWinWorkspace = workspace;
#ifdef GNOME1_HINTS
        client()->setWinWorkspaceHint(fWinWorkspace);
#endif
        updateState();
        if (clickFocus || !strongPointerFocus)
            fRoot->focusTopWindow();
#if 0
#ifdef CONFIG_TASKBAR
        updateTaskBar();
#endif
#endif
    }
}

void YFrameWindow::setLayer(long layer) {
    if (layer >= WinLayerCount || layer < 0)
        return ;
    if (layer != fWinLayer) {
        long oldLayer = fWinLayer;

        removeFrame();
        fWinLayer = layer;
        insertFrame();
#ifdef GNOME1_HINTS
        client()->setWinLayerHint(fWinLayer);
#endif
        fRoot->restackWindows(this);
        if (getLayer() == WinLayerDock || oldLayer == WinLayerDock)
            fRoot->updateWorkArea();
    }
}

void YFrameWindow::updateState() {
    if (!isManaged())
        return ;

#ifdef GNOME1_HINTS
    client()->setWinStateHint(WIN_STATE_ALL, fWinState);
#endif

    FrameState newState = NormalState;
    bool show_frame = true;
    bool show_client = true;

    // some code is probably against the ICCCM.
    // some applications misbehave either way.
    // (some hide windows on iconize, this is bad when switching workspaces
    // or rolling up the window).

    if (isHidden()) {
        show_frame = false;
        show_client = false;
        newState = IconicState;
    } else if (!visibleNow()) {
        show_frame = false;
        if (isMinimized() || isIconic()) {
            show_client = false;
            newState = IconicState;
        } else {
            show_client = false;
            newState = NormalState; // ?
        }
    } else if (isMinimized()) {
        if (minimizeToDesktop)
            show_frame = true;
        else
            show_frame = false;
        show_client = false;
        newState = IconicState;
    } else if (isRollup()) {
        show_frame = true;
        show_client = false;
        newState = NormalState; // ?
    } else {
        show_frame = true;
        show_client = true;
    }

    MSG(("updateState: winState=%lX, frame=%d, client=%d",
         fWinState, show_frame, show_client));

    if (show_client) {
        client()->show();
        fClientContainer->show();
    }
    if (show_frame)
        show();
    else
        hide();
    if (!show_client) {
        fClientContainer->hide();
        client()->hide();
    }

    client()->setFrameState(newState);
}

void YFrameWindow::getNormalGeometry(int *x, int *y, int *w, int *h) {
    XSizeHints *sh = client()->sizeHints();
    bool cxw = true;
    bool cy = true;
    bool ch = true;

    *x = this->x() + borderLeft();
    *y = this->y() + borderTop() + titleY();
    *w = client()->width();
    *h = client()->height();

    if (isIconic())
        cxw = cy = ch = false;
    else {
        if (isMaximizedHoriz())
            cxw = false;
        if (isMaximizedVert())
            cy = ch = false;
        if (isRollup())
            ch = false;
    }
    if (!cxw) {
        if (x) *x = normalX;
        if (w) *w = sh ? normalWidth * sh->width_inc + sh->base_width : normalWidth;
    }
    if (!cy)
        if (y) *y = normalY + titleY();
    if (!ch)
        if (h) *h = sh ? normalHeight * sh->height_inc + sh->base_height : normalHeight;
}

void YFrameWindow::updateNormalSize() {
    if (isIconic()) {
        iconX = this->x();
        iconY = this->y();
    } else {
        int nx = this->x() + borderLeft();
        int ny = this->y() + borderTop();
        int nw = client()->width();
        int nh = client()->height();
        XSizeHints *sh = client()->sizeHints();
        bool cxw = true;
        bool cy = true;
        bool ch = true;

        if (isMaximizedHoriz())
            cxw = false;
        if (isMaximizedVert())
            cy = ch = false;
        if (isRollup())
            ch = false;

        if (cxw) {
            normalX = nx;
            normalWidth = sh ? (nw - sh->base_width) / sh->width_inc : nw;
        }
        if (cy)
            normalY = ny;
        if (ch)
            normalHeight = sh ? (nh - sh->base_height) / sh->height_inc : nh;
    }

    //fprintf(stderr, "setNormal: (%d:%d %dx%d) icon (%d:%d)\n", normalX, normalY, normalWidth, normalHeight, iconX, iconY);
}

void YFrameWindow::updateLayout() {
    XSizeHints *sh = client()->sizeHints();
    int nx = normalX;
    int ny = normalY;
    int nw = sh ? normalWidth * sh->width_inc + sh->base_width : normalWidth;
    int nh = sh ? normalHeight * sh->height_inc + sh->base_height : normalHeight;

    if (isIconic()) {
        if (iconX == -1 && iconY == -1)
            fRoot->getIconPosition(this, &iconX, &iconY);
        nx = iconX;
        ny = iconY;
        nw = fMiniIcon->width();
        nh = fMiniIcon->height();
    } else {
        if (isMaximizedHoriz()) {
            nw = fRoot->maxWidth(getLayer());

            if (nx + nw > int(fRoot->maxX(getLayer())))
                nx = fRoot->minX(getLayer());
            if (nx < fRoot->minX(getLayer()))
                nx = fRoot->minX(getLayer());
        }
        if (isMaximizedVert()) {
            nh = fRoot->maxHeight(getLayer()) - titleY();

            if (ny + nh + int(wsTitleBar) > int(fRoot->maxY(getLayer())))
                ny = fRoot->minY(getLayer());
            if (ny < fRoot->minY(getLayer()))
                ny = fRoot->minY(getLayer());
        }
        client()->constrainSize(nw, nh, getLayer());

        if (isRollup())
            nh = 0;

        nx -= borderLeft();
        ny -= borderTop();
        nw += borderLeft() + borderRight();
        nh += borderTop() + borderBottom() + titleY();
    }
    setGeometry(nx, ny, nw, nh);
}

void YFrameWindow::setState(long mask, long state) {
    updateNormalSize(); // !!! fix this to move below (or remove totally)

    long fOldState = fWinState;
    long fNewState = (fWinState & ~mask) | (state & mask);

    // !!! this should work
    //if (fNewState == fOldState)
    //    return ;

    // !!! move here

    fWinState = fNewState;

    if ((fOldState ^ fNewState) & WinStateAllWorkspaces) {
        MSG(("WinStateAllWorkspaces: %d", isSticky()));
#if 0
#ifdef CONFIG_TASKBAR
        updateTaskBar();
#endif
#endif
    }
    MSG(("setState: oldState: %lX, newState: %lX, mask: %lX, state: %lX",
         fOldState, fNewState, mask, state));
    //fprintf(stderr, "normal1: (%d:%d %dx%d)\n", normalX, normalY, normalWidth, normalHeight);
    if ((fOldState ^ fNewState) & (WinStateMaximizedVert |
                                   WinStateMaximizedHoriz))
    {
        MSG(("WinStateMaximized: %d", isMaximized()));

        if (fMaximizeButton)
            if (isMaximized()) {
                fMaximizeButton->setActions(actionRestore, actionRestore);
                fMaximizeButton->setToolTip("Restore");
            } else {
                fMaximizeButton->setActions(actionMaximize, actionMaximizeVert);
                fMaximizeButton->setToolTip("Maximize");
            }
    }
    if ((fOldState ^ fNewState) & WinStateMinimized) {
        MSG(("WinStateMinimized: %d", isMaximized()));
        if (fNewState & WinStateMinimized) {
            minimizeTransients();
        } else {
            if (owner())
                if (owner()->isMinimized())
                    owner()->setState(WinStateMinimized, 0);
        }

        if (minimizeToDesktop && fMiniIcon) {
            if (isIconic()) {
                fMiniIcon->raise();
                fMiniIcon->show();
            } else {
                fMiniIcon->hide();
                iconX = x();
                iconY = y();
            }
        }
#if 0
#ifdef CONFIG_TASKBAR
        updateTaskBar();
#endif
#endif
        if (clickFocus || !strongPointerFocus)
            fRoot->focusTopWindow();
    }
    if ((fOldState ^ fNewState) & WinStateRollup) {
        MSG(("WinStateRollup: %d", isRollup()));
        if (fRollupButton) {
            if (isRollup()) {
                fRollupButton->setToolTip("Rolldown");
            } else {
                fRollupButton->setToolTip("Rollup");
            }
            fRollupButton->repaint();
        }
    }
    if ((fOldState ^ fNewState) & WinStateHidden) {
        MSG(("WinStateHidden: %d", isHidden()));
#if 0
#ifdef CONFIG_TASKBAR
        updateTaskBar();
#endif
#endif
    }
#if 0 //!!!
    if ((fOldState ^ fNewState) & WinStateDockHorizontal) {
        if (getLayer() == WinLayerDock)
            manager->updateWorkArea();
    }
#endif

    updateState();
    updateLayout();

#ifdef SHAPE
    if ((fOldState ^ fNewState) & (WinStateRollup | WinStateMinimized))
        setShape();
#endif
    if ((fOldState ^ fNewState) & WinStateMinimized) {
        if (!(fNewState & WinStateMinimized)) {
            restoreTransients();
        }
    }
    if ((clickFocus || !strongPointerFocus) &&
        this == fRoot->getFocus() &&
        ((fOldState ^ fNewState) & WinStateRollup))
    {
        fRoot->setFocus(this);
    }
}

void YFrameWindow::setSticky(bool sticky) {
    setState(WinStateAllWorkspaces, sticky ? WinStateAllWorkspaces : 0);
}

void YFrameWindow::updateMwmHints() {
    int bx = borderLeft();
    int by = borderTop();
    int ty = titleY(), tt;

    getFrameHints();

    int gx, gy;
    client()->gravityOffsets(gx, gy);

#ifdef TITLEBAR_BOTTOM
    if (gy == -1)
#else
    if (gy == 1)
#endif
        tt = titleY() - ty;
    else
        tt = 0;

    if (!isRollup() && !isIconic()) /// !!! check (emacs hates this)
        configureClient(x() + bx - borderLeft() + borderLeft(),
                y() + by - borderTop() + tt + borderTop() + titleY(),
                client()->width(), client()->height());
}

#ifndef LITE
YIcon *YFrameWindow::clientIcon() {
    YFrameWindow *f = this;
    while (f) {
        if (f->getClientIcon())
            return f->getClientIcon();
        f = f->owner();
    }
    return defaultAppIcon;
}
#endif

void YFrameWindow::updateProperties() {
#ifdef GNOME1_HINTS
    client()->setWinWorkspaceHint(fWinWorkspace);
    client()->setWinLayerHint(fWinLayer);
    client()->setWinStateHint(WIN_STATE_ALL, fWinState);
#endif
}

#if 0
#ifdef CONFIG_TASKBAR
void YFrameWindow::updateTaskBar() {
    bool needTaskBarApp = false;

    if (fRoot->getTaskBar() && fManaged && fRoot->getTaskBar()->taskPane()) {
        if (!isHidden() &&
            !(frameOptions() & foIgnoreTaskBar))
            if (taskBarShowAllWindows || visibleOn(fRoot->activeWorkspace()))
                needTaskBarApp = true;

        if (needTaskBarApp && fTaskBarApp == 0)
            fTaskBarApp = fRoot->getTaskBar()->taskPane()->addApp(this);

        if (fTaskBarApp) {
            fTaskBarApp->setShown(needTaskBarApp);
            if (fTaskBarApp->getShown()) ///!!! optimize
                fTaskBarApp->repaint();
        }
        /// !!! optimize

        fRoot->getTaskBar()->taskPane()->relayout();
    }
}
#endif
#endif

void YFrameWindow::handleMsgBox(YMsgBox *msgbox, int operation) {
    //printf("msgbox operation %d\n", operation);
    if (msgbox == fKillMsgBox && fKillMsgBox) {
        if (fKillMsgBox) {
            fRoot->unmanageClient(fKillMsgBox->handle());
            fKillMsgBox = 0;
            if (clickFocus || !strongPointerFocus)
                fRoot->focusTopWindow();
        }
        if (operation == YMsgBox::mbOK)
            wmKill();
    }
}
