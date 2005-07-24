/*
 * IceWM
 *
 * Copyright (C) 1997-2003 Marko Macek
 */

#include "config.h"
#include "yfull.h"
#include "wmframe.h"

#include "yprefs.h"
#include "prefs.h"
#include "atasks.h"
#include "atray.h"
#include "aaddressbar.h"
#include "wmaction.h"
#include "wmclient.h"
#include "wmcontainer.h"
#include "wmtitle.h"
#include "wmbutton.h"
#include "wmminiicon.h"
#include "wmswitch.h"
#include "wmtaskbar.h"
#include "wmwinlist.h"
#include "wmmgr.h"
#include "wmapp.h"
#include "ypixbuf.h"
#include "sysdep.h"
#include "yrect.h"
#include "yicon.h"

#include "intl.h"

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

YFrameWindow::YFrameWindow(YWindow *parent): YWindow(parent) {
    if (activeBorderBg == 0)
        activeBorderBg = new YColor(clrActiveBorder);
    if (inactiveBorderBg == 0)
        inactiveBorderBg = new YColor(clrInactiveBorder);

    setDoubleBuffer(false);
    fClient = 0;
    fFocused = false;
    fNextFrame = fPrevFrame = 0;
    fNextCreatedFrame = 0;
    fPrevCreatedFrame = 0;
    fNextFocusFrame = 0;
    fPrevFocusFrame = 0;

    fPopupActive = 0;
    fWmUrgency = false;
    fClientUrgency = false;
    fTypeDesktop = false;
    fTypeDock = false;
    fTypeSplash = false;

    normalX = 0;
    normalY = 0;
    normalW = 1;
    normalH = 1;
    posX = 0;
    posY = 0;
    posW = 1;
    posH = 1;
    iconX = -1;
    iconY = -1;
    movingWindow = 0;
    sizingWindow = 0;
    indicatorsVisible = 0;
    fFrameFunctions = 0;
    fFrameDecors = 0;
    fFrameOptions = 0;
#ifndef LITE
    fFrameIcon = 0;
#endif
#ifdef CONFIG_TASKBAR
    fTaskBarApp = 0;
#endif
#ifdef CONFIG_TRAY
    fTrayApp = 0;
#endif
#ifdef CONFIG_WINLIST
    fWinListItem = 0;
#endif
    fMiniIcon = 0;
    fTransient = 0;
    fNextTransient = 0;
    fOwner = 0;
    fManaged = false;
    fKillMsgBox = 0;
    fStrutLeft = 0;
    fStrutRight = 0;
    fStrutTop = 0;
    fStrutBottom = 0;
    fMouseFocusX = -1;
    fMouseFocusY = -1;

    setStyle(wsOverrideRedirect);
    setPointer(YXApplication::leftPointer);

    fWinWorkspace = manager->activeWorkspace();
    fWinActiveLayer = WinLayerNormal;
    fWinRequestedLayer = WinLayerNormal;
    fOldLayer = fWinActiveLayer;
#ifdef CONFIG_TRAY
    fWinTrayOption = WinTrayIgnore;
#endif
    fWinState = 0;
    fWinOptionMask = ~0;

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
        fMaximizeButton->setToolTip(_("Maximize"));
    }

    if (!isButton('i'))
        fMinimizeButton = 0;
    else {
        fMinimizeButton = new YFrameButton(fTitleBar, this,
#ifndef CONFIG_PDA
                                           actionMinimize, actionHide);
#else
                                           actionMinimize, 0);
#endif
        //fMinimizeButton->setWinGravity(NorthEastGravity);
        fMinimizeButton->setToolTip(_("Minimize"));
        fMinimizeButton->show();
    }

    if (!isButton('x'))
        fCloseButton = 0;
    else {
        fCloseButton = new YFrameButton(fTitleBar, this, actionClose, actionKill);
        //fCloseButton->setWinGravity(NorthEastGravity);
        fCloseButton->setToolTip(_("Close"));
        fCloseButton->show();
    }

#ifndef CONFIG_PDA
    if (!isButton('h'))
#endif
        fHideButton = 0;
#ifndef CONFIG_PDA
    else {
        fHideButton = new YFrameButton(fTitleBar, this, actionHide, actionHide);
        //fHideButton->setWinGravity(NorthEastGravity);
        fHideButton->setToolTip(_("Hide"));
        fHideButton->show();
    }
#endif

    if (!isButton('r'))
        fRollupButton = 0;
    else {
        fRollupButton = new YFrameButton(fTitleBar, this, actionRollup, actionRollup);
        //fRollupButton->setWinGravity(NorthEastGravity);
        fRollupButton->setToolTip(_("Rollup"));
        fRollupButton->show();
    }

    if (!isButton('d'))
        fDepthButton = 0;
    else {
        fDepthButton = new YFrameButton(fTitleBar, this, actionDepth, actionDepth);
        //fDepthButton->setWinGravity(NorthEastGravity);
        fDepthButton->setToolTip(_("Raise/Lower"));
        fDepthButton->show();
    }

    if (!isButton('s'))
        fMenuButton = 0;
    else {
        fMenuButton = new YFrameButton(fTitleBar, this, 0);
        fMenuButton->show();
        fMenuButton->setActionListener(this);
    }
#ifndef LITE
    if (minimizeToDesktop)
        fMiniIcon = new MiniIcon(this, this);
#endif
}

YFrameWindow::~YFrameWindow() {
    fManaged = false;
    if (fKillMsgBox) {
        manager->unmanageClient(fKillMsgBox->handle());
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
#ifdef CONFIG_TASKBAR
    if (fTaskBarApp) {
        if (taskBar && taskBar->taskPane())
            taskBar->taskPane()->removeApp(this);
        else
            delete fTaskBarApp;
        fTaskBarApp = 0;
    }
#endif
#ifdef CONFIG_TRAY
    if (fTrayApp) {
        if (taskBar && taskBar->trayPane())
            taskBar->trayPane()->removeApp(this);
        else
            delete fTrayApp;
        fTrayApp = 0;
    }
#endif
#ifdef CONFIG_WINLIST
    if (fWinListItem) {
        if (windowList)
            windowList->removeWindowListApp(fWinListItem);
        delete fWinListItem; fWinListItem = 0;
    }
#endif
    if (fMiniIcon) {
        delete fMiniIcon;
        fMiniIcon = 0;
    }
#ifndef LITE
    if (fFrameIcon && !fFrameIcon->isCached()) {
        delete fFrameIcon;
        fFrameIcon = 0;
    }
#endif
    // perhaps should be done another way
    removeTransients();
    removeAsTransient();
    removeFocusFrame();
    manager->removeClientFrame(this);
    {
        // !!! consider having an array instead
        if (fNextCreatedFrame)
            fNextCreatedFrame->setPrevCreated(fPrevCreatedFrame);
        else
            manager->setLastFrame(fPrevCreatedFrame);

        if (fPrevCreatedFrame)
            fPrevCreatedFrame->setNextCreated(fNextCreatedFrame);
        else
            manager->setFirstFrame(fNextCreatedFrame);
    }
    removeFrame();
    if (switchWindow)
        switchWindow->destroyedFrame(this);
    if (fClient != 0) {
        if (!fClient->destroyed())
            XRemoveFromSaveSet(xapp->display(), client()->handle());
        XDeleteContext(xapp->display(), client()->handle(), frameContext);
    }
    if (affectsWorkArea())
        manager->updateWorkArea();
    // FIX !!! should actually check if < than current values
    if (fStrutLeft != 0 || fStrutRight != 0 ||
        fStrutTop != 0 || fStrutBottom != 0)
        manager->updateWorkArea();

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

    XDestroyWindow(xapp->display(), topSide);
    XDestroyWindow(xapp->display(), leftSide);
    XDestroyWindow(xapp->display(), rightSide);
    XDestroyWindow(xapp->display(), bottomSide);
    XDestroyWindow(xapp->display(), topLeftCorner);
    XDestroyWindow(xapp->display(), topRightCorner);
    XDestroyWindow(xapp->display(), bottomLeftCorner);
    XDestroyWindow(xapp->display(), bottomRightCorner);
    manager->updateClientList();
}

void YFrameWindow::doManage(YFrameClient *clientw) {
    PRECONDITION(clientw != 0);
    fClient = clientw;

    {
        int x = client()->x();
        int y = client()->y();
        int w = client()->width();
        int h = client()->height();

        XSizeHints *sh = client()->sizeHints();
        normalX = x;
        normalY = y;
        normalW = sh ? (w - sh->base_width) / sh->width_inc : w;
        normalH = sh ? (h - sh->base_height) / sh->height_inc : h ;
    }

#ifndef LITE
    updateIcon();
#endif
    manage(fClient);
    getFrameHints();
    insertFrame();
    {
        if (manager->lastFrame())
            manager->lastFrame()->setNextCreated(this);
        else
            manager->setFirstFrame(this);
        setPrevCreated(manager->lastFrame());
        manager->setLastFrame(this);
    }
    insertFocusFrame((manager->wmState() == YWindowManager::wmSTARTUP) ?
                     true : false);

    {
#ifdef WMSPEC_HINTS
        long layer = 0;
        Atom net_wm_window_type;
        if (fClient->getNetWMWindowType(&net_wm_window_type)) {
            if (net_wm_window_type ==
                _XA_NET_WM_WINDOW_TYPE_DOCK)
            {
                setSticky(true);
                setTypeDock(true);
                updateMwmHints();
            } else if (net_wm_window_type ==
                       _XA_NET_WM_WINDOW_TYPE_DESKTOP)
            {
/// TODO #warning "this needs some cleanup"
                setSticky(true);
                setTypeDesktop(true);
                updateMwmHints();
            } else if (net_wm_window_type ==
                       _XA_NET_WM_WINDOW_TYPE_SPLASH)
            {
                setTypeSplash(true);
                updateMwmHints();
            }
            updateLayer(true);
        } else if (fClient->getWinLayerHint(&layer))
            setRequestedLayer(layer);
#endif
    }

    getDefaultOptions();
#ifndef NO_WINDOW_OPTIONS
    if (frameOptions() & foAllWorkspaces)
        setSticky(true);
#endif
#ifndef NO_WINDOW_OPTIONS
    if (frameOptions() & foFullscreen)
        setState(WinStateFullscreen, WinStateFullscreen);
#endif

    addAsTransient();
    addTransients();

    manager->restackWindows(this);
#ifdef WMSPEC_HINTS
    updateNetWMStrut(); /// ? here
#endif
    if (affectsWorkArea())
        manager->updateWorkArea();
    manager->updateClientList();

    long workspace = getWorkspace(), state_mask(0), state(0);
#ifdef CONFIG_TRAY
    long tray(0);
#endif

    MSG(("Map - Frame: %d", visible()));
    MSG(("Map - Client: %d", client()->visible()));

    if (client()->getNetWMStateHint(&state_mask, &state)) {
        setState(state_mask, state);
    }
    if (client()->getWinStateHint(&state_mask, &state)) {
        setState(state_mask, state);
    }
    {
        FrameState st = client()->getFrameState();

        if (st == WithdrawnState) {
            XWMHints *h = client()->hints();
            if (h && (h->flags & StateHint))
                st = h->initial_state;
            else
                st = NormalState;
        }
        MSG(("FRAME state = %d", st));
        switch (st) {
        case IconicState:
            setState(WinStateMinimized, WinStateMinimized);
            break;

        case NormalState:
        case WithdrawnState:
            break;
        }
    }

    if (client()->getNetWMDesktopHint(&workspace)) {
        if (workspace == (long)0xFFFFFFFF)
            setSticky(true);
        else
            setWorkspace(workspace);
    } else if (client()->getWinWorkspaceHint(&workspace))
        setWorkspace(workspace);

#ifdef CONFIG_TRAY
    if (client()->getWinTrayHint(&tray))
        setTrayOption(tray);
#endif

    afterManage();
}

void YFrameWindow::afterManage() {
#ifdef CONFIG_SHAPE
    setShape();
#endif
#ifndef NO_WINDOW_OPTIONS
    if (!(frameOptions() & foFullKeys))
        grabKeys();
#endif
    fClientContainer->grabButtons();
#ifdef CONFIG_WINLIST
#ifndef NO_WINDOW_OPTIONS
    if (windowList && !(frameOptions() & foIgnoreWinList))
        fWinListItem = windowList->addWindowListApp(this);
#endif
#endif
#ifdef CONFIG_GUIEVENTS
    wmapp->signalGuiEvent(geWindowOpened);
#endif
}

// create 8 windows that are used to show the proper pointer
// on frame (for resize)
void YFrameWindow::createPointerWindows() {
    XSetWindowAttributes attributes;
    unsigned int klass = InputOnly;

    attributes.event_mask = 0;

    attributes.cursor = YWMApp::sizeTopPointer.handle();
    topSide = XCreateWindow(xapp->display(), handle(), 0, 0, 1, 1, 0,
                            CopyFromParent, klass, CopyFromParent,
                            CWCursor | CWEventMask, &attributes);

    attributes.cursor = YWMApp::sizeLeftPointer.handle();
    leftSide = XCreateWindow(xapp->display(), handle(), 0, 0, 1, 1, 0,
                            CopyFromParent, klass, CopyFromParent,
                            CWCursor | CWEventMask, &attributes);

    attributes.cursor = YWMApp::sizeRightPointer.handle();
    rightSide = XCreateWindow(xapp->display(), handle(), 0, 0, 1, 1, 0,
                            CopyFromParent, klass, CopyFromParent,
                            CWCursor | CWEventMask, &attributes);

    attributes.cursor = YWMApp::sizeBottomPointer.handle();
    bottomSide = XCreateWindow(xapp->display(), handle(), 0, 0, 1, 1, 0,
                            CopyFromParent, klass, CopyFromParent,
                            CWCursor | CWEventMask, &attributes);

    attributes.cursor = YWMApp::sizeTopLeftPointer.handle();
    topLeftCorner = XCreateWindow(xapp->display(), handle(), 0, 0, 1, 1, 0,
                                  CopyFromParent, klass, CopyFromParent,
                                  CWCursor | CWEventMask, &attributes);

    attributes.cursor = YWMApp::sizeTopRightPointer.handle();
    topRightCorner = XCreateWindow(xapp->display(), handle(), 0, 0, 1, 1, 0,
                                   CopyFromParent, klass, CopyFromParent,
                                   CWCursor | CWEventMask, &attributes);

    attributes.cursor = YWMApp::sizeBottomLeftPointer.handle();
    bottomLeftCorner = XCreateWindow(xapp->display(), handle(), 0, 0, 1, 1, 0,
                                     CopyFromParent, klass, CopyFromParent,
                                     CWCursor | CWEventMask, &attributes);

    attributes.cursor = YWMApp::sizeBottomRightPointer.handle();
    bottomRightCorner = XCreateWindow(xapp->display(), handle(), 0, 0, 1, 1, 0,
                                      CopyFromParent, klass, CopyFromParent,
                                      CWCursor | CWEventMask, &attributes);

    XMapSubwindows(xapp->display(), handle());
    indicatorsVisible = 1;
}

void YFrameWindow::grabKeys() {
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
    GRAB_WMKEY(gKeyWinMaximizeHoriz);
    GRAB_WMKEY(gKeyWinHide);
    GRAB_WMKEY(gKeyWinRollup);
    GRAB_WMKEY(gKeyWinFullscreen);
    GRAB_WMKEY(gKeyWinMenu);
    GRAB_WMKEY(gKeyWinArrangeN);
    GRAB_WMKEY(gKeyWinArrangeNE);
    GRAB_WMKEY(gKeyWinArrangeE);
    GRAB_WMKEY(gKeyWinArrangeSE);
    GRAB_WMKEY(gKeyWinArrangeS);
    GRAB_WMKEY(gKeyWinArrangeSW);
    GRAB_WMKEY(gKeyWinArrangeW);
    GRAB_WMKEY(gKeyWinArrangeNW);
    GRAB_WMKEY(gKeyWinArrangeC);
    GRAB_WMKEY(gKeyWinSnapMoveN);
    GRAB_WMKEY(gKeyWinSnapMoveNE);
    GRAB_WMKEY(gKeyWinSnapMoveE);
    GRAB_WMKEY(gKeyWinSnapMoveSE);
    GRAB_WMKEY(gKeyWinSnapMoveS);
    GRAB_WMKEY(gKeyWinSnapMoveSW);
    GRAB_WMKEY(gKeyWinSnapMoveW);
    GRAB_WMKEY(gKeyWinSnapMoveNW);
    GRAB_WMKEY(gKeyWinSmartPlace);
}

void YFrameWindow::manage(YFrameClient *client) {
    PRECONDITION(client != 0);
    fClient = client;

/// TODO #warning "optimize this, do it only if needed"
    XSetWindowBorderWidth(xapp->display(),
                          client->handle(),
                          0);

    {
        XSetWindowAttributes xswa;
        xswa.win_gravity = NorthWestGravity;

        XChangeWindowAttributes(xapp->display(), client->handle(),
                                CWWinGravity, &xswa);
    }

    XAddToSaveSet(xapp->display(), client->handle());

    client->reparent(fClientContainer, 0, 0);

    client->setFrame(this);

#if 0
    sendConfigure();
#endif
}

void YFrameWindow::unmanage(bool reparent) {
    PRECONDITION(fClient != 0);

    if (!fClient->destroyed()) {
        int gx, gy;
        client()->gravityOffsets(gx, gy);

        XSetWindowBorderWidth(xapp->display(),
                              client()->handle(),
                              client()->getBorder());

        int posX, posY, posWidth, posHeight;

        getNormalGeometryInner(&posX, &posY, &posWidth, &posHeight);
        if (gx < 0)
            posX -= borderXN();
        else if (gx > 0)
            posX += borderXN() - 2 * client()->getBorder();
        if (gy < 0)
            posY -= borderYN();
        else if (gy > 0)
            posY += borderYN() - 2 * client()->getBorder();

        if (reparent)
            client()->reparent(manager, posX, posY);

        client()->setSize(posWidth, posHeight);

        if (manager->wmState() != YWindowManager::wmSHUTDOWN)
            client()->setFrameState(WithdrawnState);

        if (!client()->destroyed())
            XRemoveFromSaveSet(xapp->display(), client()->handle());
    }

    client()->setFrame(0);

    fClient = 0;
}

void YFrameWindow::getNewPos(const XConfigureRequestEvent &cr,
                             int &cx, int &cy, int &cw, int &ch)
{


    cw = (cr.value_mask & CWWidth) ? cr.width : client()->width();
    ch = (cr.value_mask & CWHeight) ? cr.height : client()->height();

    int grav = NorthWestGravity;

    XSizeHints *sh = client()->sizeHints();
    if (sh && sh->flags & PWinGravity)
        grav = sh->win_gravity;

    int cur_x = x() + container()->x();
    int cur_y = y() + container()->y();

    //msg("%d %d %d %d", cr.x, cr.y, cr.width, cr.height);

    if (cr.value_mask & CWX) {
        if (grav == StaticGravity)
            cx = cr.x;
        else {
            cx = cr.x + container()->x();
            if (frameOptions() & foNonICCCMConfigureRequest) {
                warn("nonICCCMpositioning: adjusting x %d by %d", cx, -container()->x());
                cx -= container()->x();
            }
        }
    } else {
        if (grav == NorthGravity ||
            grav == CenterGravity ||
            grav == SouthGravity)
        {
            cx = cur_x + (client()->width() - cw) / 2;
        } else if (grav == NorthEastGravity ||
                   grav == EastGravity ||
                   grav == SouthEastGravity)
        {
            cx = cur_x + (client()->width() - cw);
        } else {
            cx = cur_x;
        }
    }

    if (cr.value_mask & CWY) {
        if (grav == StaticGravity)
            cy = cr.y;
        else {
            cy = cr.y + container()->y();
            if (frameOptions() & foNonICCCMConfigureRequest) {
                warn("nonICCCMpositioning: adjusting y %d by %d", cy, -container()->y());
                cy -= container()->y();
            }
        }
    } else {
        if (grav == WestGravity ||
            grav == CenterGravity ||
            grav == EastGravity)
        {
            cy = cur_y + (client()->height() - ch) / 2;
        } else if (grav == SouthEastGravity ||
                   grav == SouthGravity ||
                   grav == SouthWestGravity)
        {
            cy = cur_y + (client()->height() - ch);
        } else {
            cy = cur_y;
        }
    }
}

void YFrameWindow::configureClient(const XConfigureRequestEvent &configureRequest) {
    client()->setBorder((configureRequest.value_mask & CWBorderWidth) ? configureRequest.border_width : client()->getBorder());

    int cx, cy, cw, ch;
    getNewPos(configureRequest, cx, cy, cw, ch);

    configureClient(cx, cy, cw, ch);

    if (configureRequest.value_mask & CWStackMode) {
        YFrameWindow *sibling = 0;
        XWindowChanges xwc;

        if ((configureRequest.value_mask & CWSibling) &&
            XFindContext(xapp->display(),
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
                return;
            }
            XConfigureWindow(xapp->display(),
                             handle(),
                             configureRequest.value_mask & (CWSibling | CWStackMode),
                             &xwc);
        } else if (xwc.sibling == None /*&& manager->top(getLayer()) != 0*/) {
            switch (xwc.stack_mode) {
            case Above:
                if (!focusOnAppRaise) {
                    if (canRaise()) {
                        setWmUrgency(true);
                    }
                } else {
                    if (canRaise()) {
                        wmRaise();
                    }
#if 1

#if 1
                    if (
#ifndef NO_WINDOW_OPTIONS
                        !(frameOptions() & foNoFocusOnAppRaise) &&
#endif
                        (clickFocus || !strongPointerFocus))
                    {
                        if (focusChangesWorkspace ||
                            visibleOn(manager->activeWorkspace()))
                        {
                            activate();
                        } else {
                            setWmUrgency(true);
                        }
                    }
#endif
#if 1
                    { /* warning, tcl/tk "fix" here */
                        XEvent xev;
/// TODO #warning "looks like sendConfigure but not quite, investigate!"

                        memset(&xev, 0, sizeof(xev));
                        xev.xconfigure.type = ConfigureNotify;
                        xev.xconfigure.display = xapp->display();
                        xev.xconfigure.event = handle();
                        xev.xconfigure.window = handle();
                        xev.xconfigure.x = x();
                        xev.xconfigure.y = y();
                        xev.xconfigure.width = width();
                        xev.xconfigure.height = height();
                        xev.xconfigure.border_width = 0;

                        MSG(("sendConfigureHack %d %d %d %d",
                             xev.xconfigure.x,
                             xev.xconfigure.y,
                             xev.xconfigure.width,
                             xev.xconfigure.height));

                        xev.xconfigure.above = None;
                        xev.xconfigure.override_redirect = False;

                        XSendEvent(xapp->display(),
                                   handle(),
                                   False,
                                   StructureNotifyMask,
                                   &xev);
                    }
#endif
#endif
                }
                break;
            case Below:
                wmLower();
                break;
            }
        }
    }
    sendConfigure();
}

void YFrameWindow::configureClient(int cx, int cy, int cwidth, int cheight) {
    MSG(("setting geometry (%d:%d %dx%d)", cx, cy, cwidth, cheight));
    cy -= titleYN();
/// TODO #warning "alternative configure mechanism would be nice"
    if (isFullscreen())
        return;
    else {
        int posX, posY, posW, posH;
        getNormalGeometryInner(&posX, &posY, &posW, &posH);

        if (isMaximizedVert() || isRollup()) {
            cy = posY;
            cheight = posH;
        }
        if (isMaximizedHoriz()) {
            cx = posX;
            cwidth = posW;
        }
    }

    setNormalGeometryInner(cx, cy, cwidth, cheight);
}

void YFrameWindow::handleClick(const XButtonEvent &up, int /*count*/) {
    if (up.button == 3) {
        popupSystemMenu(this, up.x_root, up.y_root,
                        YPopupWindow::pfCanFlipVertical |
                        YPopupWindow::pfCanFlipHorizontal |
                        YPopupWindow::pfPopupMenu);
    }
}

void YFrameWindow::handleCrossing(const XCrossingEvent &crossing) {
    if (crossing.type == EnterNotify &&
        (crossing.mode == NotifyNormal || (strongPointerFocus && crossing.mode == NotifyUngrab)) &&
        crossing.window == handle()
        &&
        (strongPointerFocus ||
         fMouseFocusX != crossing.x_root ||
         fMouseFocusY != crossing.y_root)
       )
    {
        fMouseFocusX = crossing.x_root;
        fMouseFocusY = crossing.y_root;

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
                fMouseFocusX = -1;
                fMouseFocusY = -1;
            }
            if (autoRaise) {
                if (fAutoRaiseTimer && fAutoRaiseTimer->getTimerListener() == this) {
                    fAutoRaiseTimer->stopTimer();
                    fAutoRaiseTimer->setTimerListener(0);
                    fMouseFocusX = -1;
                    fMouseFocusY = -1;
                }
            }
        }
    }
}

void YFrameWindow::handleFocus(const XFocusChangeEvent &focus) {
    if (switchWindow && switchWindow->visible())
        return ;
#if 1
    if (focus.type == FocusIn &&
        focus.mode != NotifyGrab &&
        focus.window == handle() &&
        focus.detail != NotifyInferior &&
        focus.detail != NotifyPointer &&
        focus.detail != NotifyPointerRoot)
        manager->switchFocusTo(this);
#endif
#if 0
    else if (focus.type == FocusOut &&
               focus.mode == NotifyNormal &&
               focus.detail != NotifyInferior &&
               focus.detail != NotifyPointer &&
               focus.detail != NotifyPointerRoot &&
               focus.window == handle())
        manager->switchFocusFrom(this);
#endif
    layoutShape();
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
    if (this != manager->top(getActiveLayer())) {
        YWindow::raise();
        setAbove(manager->top(getActiveLayer()));
    }
}

void YFrameWindow::lower() {
    if (this != manager->bottom(getActiveLayer())) {
        YWindow::lower();
        setAbove(0);
    }
}

void YFrameWindow::removeFrame() {
#ifdef DEBUG
    if (debug_z) dumpZorder("before removing", this);
#endif
    if (prev())
        prev()->setNext(next());
    else
        manager->setTop(getActiveLayer(), next());

    if (next())
        next()->setPrev(prev());
    else
        manager->setBottom(getActiveLayer(), prev());

    setPrev(0);
    setNext(0);

#ifdef DEBUG
    if (debug_z) dumpZorder("after removing", this);
#endif
}

void YFrameWindow::insertFrame() {
#ifdef DEBUG
    if (debug_z) dumpZorder("before inserting", this);
#endif
    setNext(manager->top(getActiveLayer()));
    setPrev(0);
    if (next())
        next()->setPrev(this);
    else
        manager->setBottom(getActiveLayer(), this);
    manager->setTop(getActiveLayer(), this);
#ifdef DEBUG
    if (debug_z) dumpZorder("after inserting", this);
#endif
}

void YFrameWindow::setAbove(YFrameWindow *aboveFrame) {
#ifdef DEBUG
    if (debug_z) dumpZorder("before setAbove", this, aboveFrame);
#endif
    if (aboveFrame != next() && aboveFrame != this) {
        if (prev())
            prev()->setNext(next());
        else
            manager->setTop(getActiveLayer(), next());

        if (next())
            next()->setPrev(prev());
        else
            manager->setBottom(getActiveLayer(), prev());

        setNext(aboveFrame);
        if (next()) {
            setPrev(next()->prev());
            next()->setPrev(this);
        } else {
            setPrev(manager->bottom(getActiveLayer()));
            manager->setBottom(getActiveLayer(), this);
        }
        if (prev())
            prev()->setNext(this);
        else
            manager->setTop(getActiveLayer(), this);
#ifdef DEBUG
        if (debug_z) dumpZorder("after setAbove", this, aboveFrame);
#endif
    }
    manager->updateFullscreenLayer();
}

void YFrameWindow::setBelow(YFrameWindow *belowFrame) {
    if (belowFrame != next())
        setAbove(belowFrame->next());
}

void YFrameWindow::insertFocusFrame(bool focus) {
    if (focus) {
        if (manager->lastFocusFrame())
            manager->lastFocusFrame()->setNextFocus(this);
        else
            manager->setFirstFocusFrame(this);
        setPrevFocus(manager->lastFocusFrame());
        manager->setLastFocusFrame(this);
    } else {
/// TODO #warning "XXX: insert as next focus, not as last focus"
        if (manager->firstFocusFrame())
            manager->firstFocusFrame()->setPrevFocus(this);
        else
            manager->setLastFocusFrame(this);
        setNextFocus(manager->firstFocusFrame());
        manager->setFirstFocusFrame(this);
    }
}

void YFrameWindow::removeFocusFrame() {
    if (fNextFocusFrame)
        fNextFocusFrame->setPrevFocus(fPrevFocusFrame);
    else
        manager->setLastFocusFrame(fPrevFocusFrame);

    if (fPrevFocusFrame)
        fPrevFocusFrame->setNextFocus(fNextFocusFrame);
    else
        manager->setFirstFocusFrame(fNextFocusFrame);
    fNextFocusFrame = 0;
    fPrevFocusFrame = 0;
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
        if ((flags & fwfFocusable) && !p->isFocusable(true))
            goto next;
        if ((flags & fwfWorkspace) && !p->visibleNow())
            goto next;
#if 0
#ifndef NO_WINDOW_OPTIONS
        if ((flags & fwfSwitchable) && (p->frameOptions() & foIgnoreQSwitch))
            goto next;
#endif
#endif
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
                p = (flags & fwfLayers) ? manager->bottomLayer() : manager->bottom(getActiveLayer());
            else
                p = (flags & fwfLayers) ? manager->topLayer() : manager->top(getActiveLayer());
    } while (p != this);

    if (!(flags & fwfSame))
        return 0;
    if ((flags & fwfVisible) && !p->visible())
        return 0;
    if ((flags & fwfFocusable) && !p->isFocusable(true))
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
    xev.xconfigure.display = xapp->display();
    xev.xconfigure.event = client()->handle();
    xev.xconfigure.window = client()->handle();
    xev.xconfigure.x = x() + borderX();
    xev.xconfigure.y = y() + borderY()
#ifndef TITLEBAR_BOTTOM
        + titleY()
#endif
        ;
    xev.xconfigure.width = client()->width();
    xev.xconfigure.height = client()->height();
    xev.xconfigure.border_width = client()->getBorder();

    xev.xconfigure.above = None;
    xev.xconfigure.override_redirect = False;

    MSG(("sendConfigure %d %d %d %d",
         xev.xconfigure.x,
         xev.xconfigure.y,
         xev.xconfigure.width,
         xev.xconfigure.height));

#ifdef DEBUG_C
    Status rc =
#endif
        XSendEvent(xapp->display(),
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
            manager->setFocus(this, true);
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
#ifndef CONFIG_PDA
    } else if (action == actionHide) {
        if (canHide())
            wmHide();
#endif
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
#if DO_NOT_COVER_OLD
    } else if (action == actionDoNotCover) {
        wmToggleDoNotCover();
#endif
    } else if (action == actionFullscreen) {
        wmToggleFullscreen();
#ifdef CONFIG_TRAY
    } else if (action == actionToggleTray) {
        wmToggleTray();
#endif
    } else {
        for (int l(0); l < WinLayerCount; l++) {
            if (action == layerActionSet[l]) {
                wmSetLayer(l);
                return ;
            }
        }
        for (int w(0); w < workspaceCount; w++) {
            if (action == workspaceActionMoveTo[w]) {
                wmMoveToWorkspace(w);
                return ;
            }
        }
        wmapp->actionPerformed(action, modifiers);
    }
}

void YFrameWindow::wmSetLayer(long layer) {
    setRequestedLayer(layer);
}

#ifdef CONFIG_TRAY
void YFrameWindow::wmSetTrayOption(long option) {
    setTrayOption(option);
}
#endif

#if DO_NOT_COVER_OLD
void YFrameWindow::wmToggleDoNotCover() {
    setDoNotCover(!doNotCover());
}
#endif

void YFrameWindow::wmToggleFullscreen() {
    if (isFullscreen()) {
        setState(WinStateFullscreen, 0);
    } else {
        setState(WinStateFullscreen, WinStateFullscreen);
    }
}

#ifdef CONFIG_TRAY
void YFrameWindow::wmToggleTray() {
    if (getTrayOption() != WinTrayExclusive) {
        setTrayOption(WinTrayExclusive);
    } else {
        setTrayOption(WinTrayIgnore);
    }
}
#endif

void YFrameWindow::wmMove() {
    Window root, child;
    int rx, ry, wx, wy;
    unsigned int mask;
    XQueryPointer(xapp->display(), desktop->handle(),
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
    manager->lockFocus();
    if (isMinimized()) {
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowRestore);
#endif
        setState(WinStateMinimized, 0);
    } else {
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowMin);
#endif
        setState(WinStateMinimized, WinStateMinimized);
        wmLower();
    }
    manager->unlockFocus();
    manager->focusLastWindow();
}

void YFrameWindow::minimizeTransients() {
    for (YFrameWindow *w = transient(); w; w = w->nextTransient()) {
// Since a) YFrameWindow::setState is too heavy but b) we want to save memory
        MSG(("> isMinimized: %d\n", w->isMinimized()));
        if (w->isMinimized())
            w->fWinState|= WinStateWasMinimized;
        else
            w->fWinState&= ~WinStateWasMinimized;
        MSG(("> wasMinimized: %d\n", w->wasMinimized()));
        if (!w->isMinimized()) w->wmMinimize();
    }
}

void YFrameWindow::restoreMinimizedTransients() {
    for (YFrameWindow *w = transient(); w; w = w->nextTransient())
        if (w->isMinimized() && !w->wasMinimized())
            w->setState(WinStateMinimized, 0);
}

void YFrameWindow::hideTransients() {
    for (YFrameWindow *w = transient(); w; w = w->nextTransient()) {
// See YFrameWindow::minimizeTransients() for reason
        MSG(("> isHidden: %d\n", w->isHidden()));
        if (w->isHidden())
            w->fWinState|= WinStateWasHidden;
        else
            w->fWinState&= ~WinStateWasHidden;
        MSG(("> was visible: %d\n", w->wasHidden());
        if (!w->isHidden()) w->wmHide());
    }
}

void YFrameWindow::restoreHiddenTransients() {
    for (YFrameWindow *w = transient(); w; w = w->nextTransient())
        if (w->isHidden() && !w->wasHidden())
            w->setState(WinStateHidden, 0);
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
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowHide);
#endif
        setState(WinStateHidden, WinStateHidden);
    }
    manager->focusLastWindow();
}

void YFrameWindow::wmLower() {
    if (this != manager->bottom(getActiveLayer())) {
        YFrameWindow *w = this;

        manager->lockFocus();
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowLower);
#endif
        while (w) {
            w->doLower();
            w = w->owner();
        }
        manager->restackWindows(this);
        manager->unlockFocus();
        manager->focusTopWindow();
    }
}

void YFrameWindow::doLower() {
    setAbove(0);
}

void YFrameWindow::wmRaise() {
    doRaise();
    manager->restackWindows(this);
}

void YFrameWindow::doRaise() {
#ifdef DEBUG
    if (debug_z) dumpZorder("wmRaise: ", this);
#endif
    if (this != manager->top(getActiveLayer())) {
        setAbove(manager->top(getActiveLayer()));

        for (YFrameWindow * w (transient()); w; w = w->nextTransient())
            w->doRaise();

#ifdef DEBUG
        if (debug_z) dumpZorder("wmRaise after raise: ", this);
#endif
    }
}

void YFrameWindow::wmClose() {
    if (!canClose())
        return ;

#ifdef CONFIG_PDA
    wmHide();
#else
    XGrabServer(xapp->display());
    client()->getProtocols(true);

    if (client()->protocols() & YFrameClient::wpDeleteWindow) {
        client()->sendDelete();
    } else {
        if (frameOptions() & foForcedClose) {
            wmKill();
        } else {
            wmConfirmKill();
        }
    }
    XUngrabServer(xapp->display());
#endif
}

void YFrameWindow::wmConfirmKill() {
#ifndef LITE
    if (fKillMsgBox == 0) {
        YMsgBox *msgbox = new YMsgBox(YMsgBox::mbOK|YMsgBox::mbCancel);
        char *title = strJoin(_("Kill Client: "), getTitle(), 0);
        fKillMsgBox = msgbox;

        msgbox->setTitle(title);
        delete title; title = 0;
        msgbox->setText(_("WARNING! All unsaved changes will be lost when\n"
                          "this client is killed. Do you wish to proceed?"));
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
    XKillClient(xapp->display(), client()->handle());
}

void YFrameWindow::wmPrevWindow() {
    if (next() != this) {
        YFrameWindow *f = findWindow(fwfNext | fwfBackward | fwfVisible | fwfCycle | fwfFocusable | fwfWorkspace | fwfSame);
        if (f) {
            f->wmRaise();
            manager->setFocus(f, true);
        }
    }
}

void YFrameWindow::wmNextWindow() {
    if (next() != this) {
        wmLower();
        manager->setFocus(findWindow(fwfNext | fwfVisible | fwfCycle | fwfFocusable | fwfWorkspace | fwfSame), true);
    }
}

void YFrameWindow::wmLastWindow() {
    if (next() != this) {
        YFrameWindow *f = findWindow(fwfNext | fwfVisible | fwfCycle | fwfFocusable | fwfWorkspace | fwfSame);
        if (f) {
            f->wmRaise();
            manager->setFocus(f, true);
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
#ifdef CONFIG_TASKBAR
        updateTaskBar();
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
#ifdef CONFIG_TASKBAR
        updateTaskBar();
#endif

        if (true || !clientMouseActions)
            if (focusOnClickClient &&
                !(raiseOnClickClient && (this != manager->top(getActiveLayer()))))
                fClientContainer->releaseButtons();
    }
}

void YFrameWindow::focusOnMap() {
    bool onCurrentWorkspace = visibleOn(manager->activeWorkspace());

    if (!onCurrentWorkspace && !focusChangesWorkspace) {
        setWmUrgency(true);
        return ;
    }


    if (!(frameOptions() & foNoFocusOnMap)) {
        if (owner() != 0) {
            if (focusOnMapTransient) {
                if (owner()->focused() || !focusOnMapTransientActive)
                {
                    if (fDelayFocusTimer) {
                        fDelayFocusTimer->stopTimer();
                        fDelayFocusTimer->setTimerListener(0);
                    }
                    if (fAutoRaiseTimer) {
                        fAutoRaiseTimer->stopTimer();
                        fAutoRaiseTimer->setTimerListener(0);
                    }
                    activate();
                }
            }
        } else {
            if (::focusOnMap) {
                if (fDelayFocusTimer) {
                    fDelayFocusTimer->stopTimer();
                    fDelayFocusTimer->setTimerListener(0);
                }
                if (fAutoRaiseTimer) {
                    fAutoRaiseTimer->stopTimer();
                    fAutoRaiseTimer->setTimerListener(0);
                }
                activate();
            } else {
                setWmUrgency(true);
            }
        }
    }
}

void YFrameWindow::wmShow() {
    // recover lost (offscreen) windows !!! (unify with code below)
/// TODO #warning "this is really broken"
    if (x() >= int(manager->width()) ||
        y() >= int(manager->height()) ||
        x() <= - int(width()) ||
        y() <= - int(height()))
    {
        int newX = x();
        int newY = y();

        if (x() >= int(manager->width()))
            newX = int(manager->width() - width() + borderX());
        if (y() >= int(manager->height()))
            newY = int(manager->height() - height() + borderY());

        if (newX < int(- borderX()))
            newX = int(- borderX());
        if (newY < int(- borderY()))
            newY = int(- borderY());
        setCurrentPositionOuter(newX, newY);
    }

    setState(WinStateHidden | WinStateMinimized, 0);
}

void YFrameWindow::focus(bool canWarp) {
/// TODO #warning "move focusChangesWorkspace check out of here, to (some) callers"
    manager->lockFocus();
    if (!visibleOn(manager->activeWorkspace())) {
        manager->activateWorkspace(getWorkspace());
    }
    // recover lost (offscreen) windows !!!
    if (limitPosition &&
        (x() >= int(manager->width()) ||
         y() >= int(manager->height()) ||
         x() <= - int(width()) ||
         y() <= - int(height())))
    {
        int newX = x();
        int newY = y();

        if (x() >= int(manager->width()))
            newX = int(manager->width() - width() + borderX());
        if (y() >= int(manager->height()))
            newY = int(manager->height() - height() + borderY());

        if (newX < int(- borderX()))
            newX = int(- borderX());
        if (newY < int(- borderY()))
            newY = int(- borderY());
        setCurrentPositionOuter(newX, newY);
    }

    //    if (isFocusable())
    manager->unlockFocus();
    manager->setFocus(this, canWarp);
    if (raiseOnFocus && /* clickFocus && */
        manager->wmState() == YWindowManager::wmRUNNING)
        wmRaise();
}

void YFrameWindow::activate(bool canWarp) {
    manager->lockFocus();
    if (fWinState & (WinStateHidden | WinStateMinimized))
        setState(WinStateHidden | WinStateMinimized, 0);

    manager->unlockFocus();
    focus(canWarp);
}

void YFrameWindow::activateWindow(bool raise) {
    if (raise) wmRaise();
    activate(true);
}


void YFrameWindow::paint(Graphics &g, const YRect &/*r*/) {
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
        g.draw3DRect(borderX() - 1, borderY() - 1,
                     width() - 2 * borderX() + 1, height() - 2 * borderY() + 1,
                     false);

        g.fillRect(1, 1, width() - 2, borderY() - 2);
        g.fillRect(1, 1, borderX() - 2, height() - 2);
        g.fillRect(1, (height() - 1) - (borderY() - 2), width() - 2, borderX() - 2);
        g.fillRect((width() - 1) - (borderX() - 2), 1, borderX() - 2, height() - 2);

#ifdef CONFIG_LOOK_MOTIF
        if (wmLook == lookMotif && canSize()) {
            YColor *b(bg->brighter());
            YColor *d(bg->darker());


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

            if ((frameT[t][n] != null || TEST_GRADIENT(rgbFrameT[t][n] != null)) &&
                (frameL[t][n] != null || TEST_GRADIENT(rgbFrameL[t][n] != null)) &&
                (frameR[t][n] != null || TEST_GRADIENT(rgbFrameR[t][n] != null)) &&
                (frameB[t][n] != null || TEST_GRADIENT(rgbFrameB[t][n] != null)) &&
                frameTL[t][n] != null && frameTR[t][n] != null &&
                frameBL[t][n] != null && frameBR[t][n] != null) {
                int const xtl(frameTL[t][n]->width());
                int const ytl(frameTL[t][n]->height());
                int const xtr(frameTR[t][n]->width());
                int const ytr(frameTR[t][n]->height());
                int const xbl(frameBL[t][n]->width());
                int const ybl(frameBL[t][n]->height());
                int const xbr(frameBR[t][n]->width());
                int const ybr(frameBR[t][n]->height());

                int const cx(width()/2);
                int const cy(height()/2);

                int mxtl = min(xtl, cx);
                int mytl = min(ytl, max(titleY() + borderY(), cy));
                int mxtr = min(xtr, cx);
                int mytr = min(ytr, max(titleY() + borderY(), cy));
                int mxbl = min(xbl, cx);
                int mybl = min(ybl, cy);
                int mxbr = min(xbr, cx);
                int mybr = min(ybr, cy);

                g.copyPixmap(frameTL[t][n], 0, 0,
                             mxtl, mytl, 0, 0);
                g.copyPixmap(frameTR[t][n], max(0, (int)xtr - (int)cx), 0,
                             mxtr, mytr,
                             width() - mxtr, 0);
                g.copyPixmap(frameBL[t][n], 0, max(0, (int)ybl - (int)cy),
                             mxbl, mybl,
                             0, height() - mybl);
                g.copyPixmap(frameBR[t][n],
                             max(0, (int)xbr - (int)cx), max(0, (int)ybr - (int)cy),
                             mxbr, mybr,
                             width() - mxbr, height() - mybr);

                if (width() > (mxtl + mxtr))
                    if (frameT[t][n] != null) g.repHorz(frameT[t][n],
                        mxtl, 0, width() - mxtl - mxtr);
#ifdef CONFIG_GRADIENTS
                    else g.drawGradient(rgbFrameT[t][n],
                        mxtl, 0, width() - mxtl - mxtr, borderY());
#endif

                if (height() > (mytl + mybl))
                    if (frameL[t][n] != null) g.repVert(frameL[t][n],
                        0, mytl, height() - mytl - mybl);
#ifdef CONFIG_GRADIENTS
                    else g.drawGradient(rgbFrameL[t][n],
                        0, mytl, borderX(), height() - mytl - mybl);
#endif

                if (height() > (mytr + mybr))
                    if (frameR[t][n] != null) g.repVert(frameR[t][n],
                        width() - borderX(), mytr, height() - mytr - mybr);
#ifdef CONFIG_GRADIENTS
                    else g.drawGradient(rgbFrameR[t][n],
                        width() - borderX(), mytr,
                        borderX(), height() - mytr - mybr);
#endif

                if (width() > (mxbl + mxbr))
                    if (frameB[t][n] != null) g.repHorz(frameB[t][n],
                        mxbl, height() - borderY(), width() - mxbl - mxbr);
#ifdef CONFIG_GRADIENTS
                    else g.drawGradient(rgbFrameB[t][n],
                        mxbl, height() - borderY(),
                        width() - mxbl - mxbr, borderY());
#endif

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

void YFrameWindow::popupSystemMenu(YWindow *owner) {
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
            popupSystemMenu(owner, ax, ay,
                            YPopupWindow::pfCanFlipVertical);
        }
    }
}

void YFrameWindow::popupSystemMenu(YWindow *owner, int x, int y,
                                   unsigned int flags,
                                   YWindow *forWindow)
{
    if (fPopupActive == 0) {
        updateMenu();
        if (windowMenu()->popup(owner, forWindow, this,
                                x, y, flags))
            fPopupActive = windowMenu();
    }
}

void YFrameWindow::updateTitle() {
    titlebar()->repaint();
    layoutShape();
    updateIconTitle();
#ifdef CONFIG_WINLIST
    if (fWinListItem && windowList->visible())
        windowList->repaintItem(fWinListItem);
#endif
#ifdef CONFIG_TASKBAR
    if (fTaskBarApp)
        fTaskBarApp->setToolTip((const char *)client()->windowTitle());
#endif
#ifdef CONFIG_TRAY
    if (fTrayApp)
        fTrayApp->setToolTip((const char *)client()->windowTitle());
#endif
}

void YFrameWindow::updateIconTitle() {
#ifdef CONFIG_TASKBAR
    if (fTaskBarApp) {
        fTaskBarApp->repaint();
        fTaskBarApp->setToolTip((const char *)client()->windowTitle());
    }
#endif
#ifdef CONFIG_TRAY
    if (fTrayApp)
        fTrayApp->setToolTip((const char *)client()->windowTitle());
#endif
    if (isIconic()) {
        fMiniIcon->repaint();
    }
}

void YFrameWindow::wmOccupyAllOrCurrent() {
    if (isSticky()) {
        setWorkspace(manager->activeWorkspace());
        setSticky(false);
    } else {
        setSticky(true);
    }
#ifdef CONFIG_TASKBAR
    if (taskBar && taskBar->taskPane())
        taskBar->taskPane()->relayout();
#endif
#ifdef CONFIG_TRAY
    if (taskBar && taskBar->trayPane())
        taskBar->trayPane()->relayout();
#endif
}

void YFrameWindow::wmOccupyAll() {
    setSticky(!isSticky());
    if (affectsWorkArea())
        manager->updateWorkArea();
#ifdef CONFIG_TASKBAR
    if (taskBar && taskBar->taskPane())
        taskBar->taskPane()->relayout();
#endif
#ifdef CONFIG_TRAY
    if (taskBar && taskBar->trayPane())
        taskBar->trayPane()->relayout();
#endif
}

void YFrameWindow::wmOccupyWorkspace(long workspace) {
    PRECONDITION(workspace < workspaceCount);
    setWorkspace(workspace);
}

void YFrameWindow::wmOccupyOnlyWorkspace(long workspace) {
    PRECONDITION(workspace < workspaceCount);
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
    long win_hints = client()->winHints();
    MwmHints *mwm_hints = client()->mwmHints();
    int functions_only = (mwm_hints &&
                      (mwm_hints->flags & (MWM_HINTS_FUNCTIONS |
                                           MWM_HINTS_DECORATIONS))
                      == MWM_HINTS_FUNCTIONS);

    fFrameFunctions = 0;
    fFrameDecors = 0;
    fFrameOptions = 0;

    if (decors & MWM_DECOR_BORDER)      fFrameDecors |= fdBorder;
    if (decors & MWM_DECOR_RESIZEH)     fFrameDecors |= fdResize;
    if (decors & MWM_DECOR_TITLE)       fFrameDecors |= fdTitleBar | fdDepth;
    if (decors & MWM_DECOR_MENU)        fFrameDecors |= fdSysMenu;
    if (decors & MWM_DECOR_MAXIMIZE)    fFrameDecors |= fdMaximize;
    if (decors & MWM_DECOR_MINIMIZE)    fFrameDecors |= fdMinimize | fdHide | fdRollup;

    if (functions & MWM_FUNC_MOVE) {
        fFrameFunctions |= ffMove;
        if (functions_only)
            fFrameDecors |= fdBorder;
    }
    if (functions & MWM_FUNC_RESIZE)    {
        fFrameFunctions |= ffResize;
        if (functions_only)
            fFrameDecors |= fdResize | fdBorder;
    }
    if (functions & MWM_FUNC_MAXIMIZE) {
        fFrameFunctions |= ffMaximize;
        if (functions_only)
            fFrameDecors |= fdMaximize;
    }
    if (functions & MWM_FUNC_MINIMIZE) {
        fFrameFunctions |= ffMinimize | ffHide | ffRollup;
        if (functions_only)
            fFrameDecors |= fdMinimize | fdHide | fdRollup;
    }
    if (functions & MWM_FUNC_CLOSE) {
        fFrameFunctions |= ffClose;
        fFrameDecors |= fdClose;
    }
#else
    fFrameFunctions =
        ffMove | ffResize | ffClose |
        ffMinimize | ffMaximize | ffHide | ffRollup;
    fFrameDecors =
        fdTitleBar | fdSysMenu | fdBorder | fdResize | fdDepth |
        fdClose | fdMinimize | fdMaximize | fdHide | fdRollup;

#endif

    /// !!! fFrameOptions needs refactoring
    if (win_hints & WinHintsSkipFocus)
        fFrameOptions |= foIgnoreQSwitch;
    if (win_hints & WinHintsSkipWindowMenu)
        fFrameOptions |= foIgnoreWinList;
    if (win_hints & WinHintsSkipTaskBar)
        fFrameOptions |= foIgnoreTaskBar;
    if (win_hints & WinHintsDoNotCover)
        fFrameOptions |= foDoNotCover;

/// TODO #warning "need initial window mapping cleanup"
    if (fTypeDesktop) {
        fFrameDecors = 0;
        fFrameOptions |= foIgnoreTaskBar;
    }
    if (fTypeDock) {
        fFrameDecors = 0;
        fFrameOptions |= foIgnoreTaskBar;
    }
    if (fTypeSplash) {
        fFrameDecors = 0;
    }

#ifndef NO_WINDOW_OPTIONS
    WindowOption wo(0);
    getWindowOptions(wo, false);

    /*msg("decor: %lX %lX %lX %lX %lX %lX",
            wo.function_mask, wo.functions,
            wo.decor_mask, wo.decors,
            wo.option_mask, wo.options);*/

    fFrameFunctions &= ~wo.function_mask;
    fFrameFunctions |= wo.functions;
    fFrameDecors &= ~wo.decor_mask;
    fFrameDecors |= wo.decors;
    fFrameOptions &= ~(wo.option_mask & fWinOptionMask);
    fFrameOptions |= (wo.options & fWinOptionMask);
#endif
}

#ifndef NO_WINDOW_OPTIONS

void YFrameWindow::getWindowOptions(WindowOption &opt, bool remove) {
    memset((void *)&opt, 0, sizeof(opt));

    opt.workspace = WinWorkspaceInvalid;
    opt.layer = WinLayerInvalid;
#ifdef CONFIG_TRAY
    opt.tray = WinTrayInvalid;
#endif

    if (defOptions) getWindowOptions(defOptions, opt, false);
    if (hintOptions) getWindowOptions(hintOptions, opt, remove);
}

void YFrameWindow::getWindowOptions(WindowOptions *list, WindowOption &opt,
                                    bool remove)
{
    XClassHint const *h(client()->classHint());
    const char *role = client()->windowRole();
    WindowOption *wo;

    if (!h) return;

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
        delete[] both;
    }
    if (h->res_name && role) {
        char *both = new char[strlen(h->res_name) + 1 +
                              strlen(role) + 1];
        if (both) {
            strcpy(both, h->res_name);
            strcat(both, ".");
            strcat(both, role);
        }
        wo = both ? list->getWindowOption(both, false, remove) : 0;
        if (wo) WindowOptions::combineOptions(opt, *wo);
        delete[] both;
    }
    if (h->res_class) {
        wo = list->getWindowOption(h->res_class, false, remove);
        if (wo) WindowOptions::combineOptions(opt, *wo);
    }
    if (h->res_name) {
        wo = list->getWindowOption(h->res_name, false, remove);
        if (wo) WindowOptions::combineOptions(opt, *wo);
    }
    if (role) {
        wo = list->getWindowOption(role, false, remove);
        if (wo) WindowOptions::combineOptions(opt, *wo);
    }
    wo = list->getWindowOption(0, false, remove);
    if (wo) WindowOptions::combineOptions(opt, *wo);
}
#endif

void YFrameWindow::getDefaultOptions() {
#ifndef NO_WINDOW_OPTIONS
    WindowOption wo(0);
    getWindowOptions(wo, true);

    if (wo.icon) {
#ifndef LITE
        if (fFrameIcon && !fFrameIcon->isCached()) {
            delete fFrameIcon;
            fFrameIcon = 0;
        }
        fFrameIcon = YIcon::getIcon(wo.icon);
#endif
    }
    if (wo.workspace != (long)WinWorkspaceInvalid && wo.workspace < workspaceCount)
        setWorkspace(wo.workspace);
    if (wo.layer != (long)WinLayerInvalid && wo.layer < WinLayerCount)
        setRequestedLayer(wo.layer);
#ifdef CONFIG_TRAY
    if (wo.tray != (long)WinTrayInvalid && wo.tray < WinTrayOptionCount)
        setTrayOption(wo.tray);
#endif
#endif
}

#ifndef LITE
YIcon *newClientIcon(int count, int reclen, long * elem) {
    ref<YIconImage> small = null;
    ref<YIconImage> large = null;
    ref<YIconImage> huge = null;

    if (reclen < 2)
        return 0;
    for (int i = 0; i < count; i++, elem += reclen) {
        Pixmap pixmap(elem[0]), mask(elem[1]);

        if (pixmap == None) {
            warn("pixmap == None for subicon #%d", i);
            continue;
        }

        Window root;
        int x, y;
        int w = 0, h = 0;
        unsigned border = 0, depth = 0;

        if (reclen >= 6) {
            w = elem[2];
            h = elem[3];
            depth = elem[4];
            root = elem[5];
        } else {
            unsigned w1, h1;
            if (BadDrawable == XGetGeometry(xapp->display(), pixmap,
                                            &root, &x, &y, &w1, &h1,
                                            &border, &depth)) {
                warn("BadDrawable for subicon #%d", i);
                continue;
            }
            w = w1;
            h = h1;
        }

        if (w == 0 || h == 0) {
            MSG(("Invalid pixmap size for subicon #%d: %dx%d", i, w, h));
            continue;
        }
        MSG(("client icon: %ld %d %d %d %d", pixmap, w, h, depth, xapp->depth()));
        if (depth == 1) {
            ref<YPixmap> img(new YPixmap(w, h));
            Graphics g(img, 0, 0);

            g.setColor(YColor::white);
            g.fillRect(0, 0, w, h);
            g.setColor(YColor::black);
            g.setClipMask(pixmap);
            g.fillRect(0, 0, w, h);

#ifdef CONFIG_ANTIALIASING
            ref<YIconImage> img2(new YIconImage(img->pixmap(), mask, img->width(), img->height(), w, h));

            if (w <= YIcon::smallSize())
                small = img2;
            else if (w <= YIcon::largeSize())
                large = img2;
            else
                huge = img2;
            img = null;
#else
            if (w <= YIcon::smallSize())
                small = img;
            else if (w <= YIcon::largeSize())
                large = img;
            else
                huge = img;
#endif

        }

        if (depth == xapp->depth()) {
            if (w <= YIcon::smallSize()) {
#if defined(CONFIG_XPM) && !defined(CONFIG_ANTIALIASING)
                small.init(new YIconImage(pixmap, mask, w, h));
#else
                small.init(new YIconImage(pixmap, mask, w, h, YIcon::smallSize(), YIcon::smallSize()));
#endif
            } else if (w <= YIcon::largeSize()) {
#if defined(CONFIG_XPM) && !defined(CONFIG_ANTIALIASING)
                large.init(new YIconImage(pixmap, mask, w, h));
#else
                large.init(new YIconImage(pixmap, mask, w, h, YIcon::largeSize(), YIcon::largeSize()));
#endif
            } else if (w <= YIcon::hugeSize()) {
#if defined(CONFIG_XPM) && !defined(CONFIG_ANTIALIASING)
                huge.init(new YIconImage(pixmap, mask, w, h));
#else
                huge.init(new YIconImage(pixmap, mask, w, h, YIcon::hugeSize(), YIcon::hugeSize()));
#endif
            }
        }
    }

    return (small != null || large != null || huge != null ? new YIcon(small, large, huge) : 0);
}

void YFrameWindow::updateIcon() {
    int count;
    long *elem;
    Pixmap *pixmap;
    Atom type;

/// TODO #warning "think about winoptions specified icon here"

    YIcon *oldFrameIcon(fFrameIcon);

    if (client()->getWinIcons(&type, &count, &elem)) {
        if (type == _XA_WIN_ICONS)
            fFrameIcon = newClientIcon(elem[0], elem[1], elem + 2);
        else // compatibility
            fFrameIcon = newClientIcon(count/2, 2, elem);
        XFree(elem);
    } else if (client()->getKwmIcon(&count, &pixmap) && count == 2) {
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
            pix[0] = pixmap[0];
            pix[1] = pixmap[1];
            fFrameIcon = newClientIcon(count / 2, 2, pix);
        }
        XFree(pixmap);
    } else {
        XWMHints *h = client()->hints();
        if (h && (h->flags & IconPixmapHint)) {
            long pix[2];
            pix[0] = h->icon_pixmap;
            pix[1] = (h->flags & IconMaskHint) ? h->icon_mask : None;
            fFrameIcon = newClientIcon(1, 2, pix);
        }
    }

    if (fFrameIcon && !(fFrameIcon->small() != null || fFrameIcon->large() != null)) {
        if (!fFrameIcon->isCached()) {
            delete fFrameIcon;
            fFrameIcon = 0;
        }
    }

    if (NULL == fFrameIcon) {
        fFrameIcon = oldFrameIcon;
    } else if (oldFrameIcon != fFrameIcon) {
        if (oldFrameIcon && !oldFrameIcon->isCached()) {
            delete oldFrameIcon;
            oldFrameIcon = 0;
        }
    }

// !!! BAH, we need an internal signaling framework
    if (menuButton()) menuButton()->repaint();
    if (getMiniIcon()) getMiniIcon()->repaint();
#ifdef CONFIG_TRAY
    if (fTrayApp) fTrayApp->repaint();
#endif
#ifdef CONFIG_TASKBAR
    if (fTaskBarApp) fTaskBarApp->repaint();
#endif
    if (windowList && fWinListItem)
        windowList->getList()->repaintItem(fWinListItem);
}
#endif

YFrameWindow *YFrameWindow::nextLayer() {
    if (fNextFrame) return fNextFrame;

    for (long l(getActiveLayer() - 1); l > -1; --l)
        if (manager->top(l)) return manager->top(l);

    return 0;
}

YFrameWindow *YFrameWindow::prevLayer() {
    if (fPrevFrame) return fPrevFrame;

    for (long l(getActiveLayer() + 1); l < WinLayerCount; ++l)
        if (manager->bottom(l)) return manager->bottom(l);

    return 0;
}

YMenu *YFrameWindow::windowMenu() {
    //if (frameOptions() & foFullKeys)
    //    return windowMenuNoKeys;
    //else
    return ::windowMenu;
}

void YFrameWindow::addAsTransient() {
    Window groupLeader(client()->ownerWindow());

    if (groupLeader) {
        fOwner = manager->findFrame(groupLeader);

        if (fOwner) {
            MSG(("transient for 0x%lX: 0x%lX", groupLeader, fOwner));
            PRECONDITION(fOwner->transient() != this);

            fNextTransient = fOwner->transient();
            fOwner->setTransient(this);

            if (getActiveLayer() < fOwner->getActiveLayer()) {
                setRequestedLayer(fOwner->getActiveLayer());
            }
        }
    }
}

void YFrameWindow::removeAsTransient() {
    if (fOwner) {
        MSG(("removeAsTransient"));

        for (YFrameWindow *curr = fOwner->transient(), *prev = NULL;
             curr; prev = curr, curr = curr->nextTransient()) {
            if (curr == this) {
                if (prev) prev->setNextTransient(nextTransient());
                else fOwner->setTransient(nextTransient());
                break;
            }
        }

        fOwner = NULL;
        fNextTransient = NULL;
    }
}

void YFrameWindow::addTransients() {
    for (YFrameWindow * w(manager->bottomLayer()); w; w = w->prevLayer())
        if (w->owner() == 0) w->addAsTransient();
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

    if (getState() & WinStateModal)
        return true;

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

bool YFrameWindow::isFocusable(bool takeFocus) {
    if (hasModal())
        return false;

    if (!client())
        return false;

#if 0
    if (frameOptions() & foDoNotFocus)
        return false;

    XWMHints *hints = client()->hints();

    if (!hints)
        return true;
#ifndef NO_WINDOW_OPTIONS
    if (frameOptions() & foIgnoreNoFocusHint)
        return true;
#endif
    if (!(hints->flags & InputHint))
        return true;
    if (hints->input)
        return true;
#endif
    if (getInputFocusHint())
        return true;
    if (takeFocus) {
        if (client()->protocols() & YFrameClient::wpTakeFocus)
            return true;
    }
    return false;
}

bool YFrameWindow::getInputFocusHint() {
//    if (fClient == 0) return true;
    XWMHints *hints = fClient->hints();
    bool input = true;

    if (!(frameOptions() & YFrameWindow::foIgnoreNoFocusHint)) {
        if (hints && (hints->flags & InputHint) && !hints->input)
            input = false;
    }
    if (frameOptions() & foDoNotFocus) {
        input = false;
    }
    return input;
}


void YFrameWindow::setWorkspace(long workspace) {
    if (workspace >= workspaceCount || workspace < 0)
        return ;
    if (workspace != fWinWorkspace) {
        fWinWorkspace = workspace;
        client()->setWinWorkspaceHint(fWinWorkspace);
        updateState();
        manager->focusLastWindow();
#ifdef CONFIG_TASKBAR
        updateTaskBar();
#endif
#ifdef CONFIG_WINLIST
        windowList->updateWindowListApp(fWinListItem);
#endif
    }
}

void YFrameWindow::setRequestedLayer(long layer) {
    if (layer >= WinLayerCount || layer < 0)
        return ;

    if (layer != fWinRequestedLayer) {
        fWinRequestedLayer = layer;
        updateLayer();
    }
}

void YFrameWindow::updateLayer(bool restack) {
    long oldLayer = fWinActiveLayer;
    long newLayer;

    newLayer = fWinRequestedLayer;

    if (fTypeDesktop)
        newLayer = WinLayerDesktop;
    if (fTypeDock)
        newLayer = WinLayerDock;
    if (getState() & WinStateBelow)
        newLayer = WinLayerBelow;
    if (getState() & WinStateAbove)
        newLayer = WinLayerOnTop;
    if (fOwner) {
        if (newLayer < fOwner->getActiveLayer())
            newLayer = fOwner->getActiveLayer();
    }
    {
        YFrameWindow *focus = manager->getFocus();
        while (focus) {
            if (focus == this && isFullscreen() && manager->fullscreenEnabled() && !canRaise()) {
                newLayer = WinLayerFullscreen;
                break;
            }
            focus = focus->owner();
        }
    }

    if (newLayer != fWinActiveLayer) {
        removeFrame();
        fWinActiveLayer = newLayer;
        insertFrame();

        if (client())
            client()->setWinLayerHint(fWinRequestedLayer);

        if (limitByDockLayer &&
           (getActiveLayer() == WinLayerDock || oldLayer == WinLayerDock))
            manager->updateWorkArea();

        YFrameWindow *w = transient();
        while (w) {
            w->updateLayer(false);
            w = w->nextTransient();
        }

        if (restack)
            manager->restackWindows(this);
    }
}

#ifdef CONFIG_TRAY
void YFrameWindow::setTrayOption(long option) {
    if (option >= WinTrayOptionCount || option < 0)
        return ;
    if (option != fWinTrayOption) {
        fWinTrayOption = option;
        client()->setWinTrayHint(fWinTrayOption);
#ifdef CONFIG_TASKBAR
        updateTaskBar();
#endif
    }
}
#endif

void YFrameWindow::updateState() {
    if (!isManaged())
        return ;

    client()->setWinStateHint(WIN_STATE_ALL, fWinState);

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
        show_client = false;
        newState = isMinimized() || isIconic() ? IconicState : NormalState;
    } else if (isMinimized()) {
        show_frame = minimizeToDesktop;
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

    client()->setFrameState(newState);

    if (show_client) {
        client()->show();
        fClientContainer->show();
    }

    if (show_frame) show();
    else hide();

    if (!show_client) {
        fClientContainer->hide();
        client()->hide();
    }
    if (!show_frame) {
        if (fDelayFocusTimer && fDelayFocusTimer->getTimerListener() == this) {
            fDelayFocusTimer->stopTimer();
            fDelayFocusTimer->setTimerListener(0);
        }
        if (fAutoRaiseTimer && fAutoRaiseTimer->getTimerListener() == this) {
            fAutoRaiseTimer->stopTimer();
            fAutoRaiseTimer->setTimerListener(0);
        }
        fMouseFocusX = -1;
        fMouseFocusY = -1;
    }
}

bool YFrameWindow::affectsWorkArea() const {
    if (doNotCover())
        return true;
    if (getActiveLayer() == WinLayerDock)
        return true;
    if (fStrutLeft != 0 ||
        fStrutRight != 0 ||
        fStrutTop != 0 ||
        fStrutBottom != 0)
        return true;
    return false;
}

bool YFrameWindow::inWorkArea() const {
    if (doNotCover())
        return false;
    if (isFullscreen())
        return false;
    if (getActiveLayer() >= WinLayerDock)
        return false;
    if (fStrutLeft != 0 ||
        fStrutRight != 0 ||
        fStrutTop != 0 ||
        fStrutBottom != 0)
        return false;
    return true;
}

void YFrameWindow::getNormalGeometryInner(int *x, int *y, int *w, int *h) {
    XSizeHints *sh = client()->sizeHints();
    *x = normalX;
    *y = normalY;
    *w = sh ? normalW * sh->width_inc + sh->base_width : normalW;
    *h = sh ? normalH * sh->height_inc + sh->base_height : normalH;
}

void YFrameWindow::setNormalGeometryOuter(int x, int y, int w, int h) {
    x += borderXN();
    y += borderYN();
    w -= 2 * borderXN();
    h -= 2 * borderYN() + titleYN();
    setNormalGeometryInner(x, y, w, h);
}

void YFrameWindow::setNormalPositionOuter(int x, int y) {
    XSizeHints *sh = client()->sizeHints();
    x += borderXN();
    y += borderYN();
    int w = sh ? normalW * sh->width_inc + sh->base_width : normalW;
    int h = sh ? normalH * sh->height_inc + sh->base_height : normalH;
    setNormalGeometryInner(x, y, w, h);
}

void YFrameWindow::setNormalGeometryInner(int x, int y, int w, int h) {
    XSizeHints *sh = client()->sizeHints();
    normalX = x;
    normalY = y;
    normalW = sh ? (w - sh->base_width) / sh->width_inc : w;
    normalH = sh ? (h - sh->base_height) / sh->height_inc : h ;

    updateDerivedSize((isMaximizedVert() ? WinStateMaximizedVert : 0) | (isMaximizedHoriz() ? WinStateMaximizedHoriz : 0));
    updateLayout();
}

void YFrameWindow::updateDerivedSize(long flagmask) {
    XSizeHints *sh = client()->sizeHints();

    int nx = normalX;
    int ny = normalY;
    int nw = sh ? normalW * sh->width_inc + sh->base_width : normalW;
    int nh = sh ? normalH * sh->height_inc + sh->base_height : normalH;

    int xiscreen = manager->getScreenForRect(nx,
                                             ny,
                                             nw,
                                             nh);
    int mx, my, Mx, My;
    manager->getWorkArea(this, &mx, &my, &Mx, &My, xiscreen);
    int Mw = Mx - mx;
    int Mh = My - my;

    Mh -= titleYN();

    if (1) { // aspect of maximization
        int aMw, aMh;
        aMw = Mw;
        aMh = Mh;

        client()->constrainSize(aMw, aMh, YFrameClient::csKeepX);
        if (aMh < Mh) {
            Mh = aMh;
        } else {
            client()->constrainSize(aMw, aMh, YFrameClient::csKeepY);
            if (aMw < Mw) {
                Mw = aMw;
            }
        }
    }

    bool horiz = false;
    bool vert = false;

    if (isMaximizedHoriz()) {
        nw = Mw;
        if (considerHorizBorder) {
            nw -= 2 * borderXN();
        }
        horiz = true;
    }

    if (isMaximizedVert()) {
        nh = Mh;
        if (considerVertBorder) {
            nh -= 2 * borderYN();
        }
        vert = true;
    }

    if (horiz || vert) {
        client()->constrainSize(nw, nh, ///getLayer(),
                                (nw >= Mw) ? YFrameClient::csKeepY
                                : YFrameClient::csKeepX);
    }
    nw += 2 * borderXN();
    nh += 2 * borderYN();

    if (horiz) {
        int cx = mx;

        if (centerMaximizedWindows && !(sh && (sh->flags & PMaxSize)))
            cx = mx + (Mw - nw) / 2;
        else if (!considerHorizBorder)
            cx -= borderXN();
        if (flagmask & WinStateMaximizedVert)
            nx = cx;
        else
            nx = x();
    } else {
        nx -= borderXN();
    }
    if (vert) {
        int cy = my;

        if (centerMaximizedWindows && !(sh && (sh->flags & PMaxSize)))
            cy = my + (Mh - nh) / 2;
        else if (!considerVertBorder)
            cy -= borderYN();

        if (flagmask & WinStateMaximizedVert)
            ny = cy;
        else
            ny = y();
    } else {
        ny -= borderYN();
    }

    bool cx = true;
    bool cy = true;
    bool cw = true;
    bool ch = true;

    if (isMaximizedVert() && !vert)
        cy = ch = false;
    if (isMaximizedHoriz() && !horiz)
        cx = cw = false;

    if (cx)
        posX = nx;
    if (cy)
        posY = ny;
    if (cw)
        posW = nw;
    if (ch)
        posH = nh + titleYN();
}

void YFrameWindow::updateNormalSize() {
    MSG(("updateNormalSize: %d %d %d %d", normalX, normalY, normalW, normalH));
    XSizeHints *sh = client()->sizeHints();

    bool cx = true;
    bool cy = true;
    bool cw = true;
    bool ch = true;

    if (isFullscreen() || isIconic())
        cy = ch = cx = cw = false;
    if (isMaximizedHoriz())
        cx = cw = false;
    if (isMaximizedVert())
        cy = ch = false;
    if (isRollup())
        ch = false;

    if (cx)
        normalX = posX + borderXN();
    if (cy)
        normalY = posY + borderYN();
    if (cw) {
        normalW = posW - 2 * borderXN();
        normalW = sh ? (normalW - sh->base_width) / sh->width_inc : normalW;
    }
    if (ch) {
        normalH = posH - (2 * borderYN() + titleYN());
        normalH = sh ? (normalH - sh->base_height) / sh->height_inc : normalH;
    }
    MSG(("updateNormalSize> %d %d %d %d", normalX, normalY, normalW, normalH));
}

void YFrameWindow::setCurrentGeometryOuter(YRect newSize) {
    MSG(("setCurrentGeometryOuter: %d %d %d %d",
         newSize.x(), newSize.y(), newSize.width(), newSize.height()));
    setWindowGeometry(newSize);

    bool cx = true;
    bool cy = true;
    bool cw = true;
    bool ch = true;

    if (isFullscreen() || isIconic())
        cy = ch = cx = cw = false;
    if (isRollup())
        ch = false;

    if (cx)
        posX = x();
    if (cy)
        posY = y();
    if (cw)
        posW = width();
    if (ch)
        posH = height();

    if (isIconic()) {
        iconX = x();
        iconY = y();
    }

    updateNormalSize();
    MSG(("setCurrentGeometryOuter> %d %d %d %d",
         posX, posY, posW, posH));
}

void YFrameWindow::setCurrentPositionOuter(int x, int y) {
    setCurrentGeometryOuter(YRect(x, y, width(), height()));
}

void YFrameWindow::updateLayout() {
    if (isIconic()) {
        if (iconX == -1 && iconY == -1)
            manager->getIconPosition(this, &iconX, &iconY);

        setWindowGeometry(YRect(iconX, iconY, fMiniIcon->width(), fMiniIcon->height()));
    } else {
        int xiscreen = manager->getScreenForRect(posX,
                                                 posY,
                                                 posW,
                                                 posH);

        int dx, dy, dw, dh;
        manager->getScreenGeometry(&dx, &dy, &dw, &dh, xiscreen);

        if (isFullscreen()) {
            setWindowGeometry(YRect(dx, dy, dw, dh));
        } else {

            if (isRollup()) {
                setWindowGeometry(YRect(posX, posY, posW, 2 * borderY() + titleY()));
            } else {
                MSG(("updateLayout %d %d %d %d",
                     posX, posY, posW, posH));
                setWindowGeometry(YRect(posX, posY, posW, posH));
            }
        }
    }
}

void YFrameWindow::setState(long mask, long state) {
    long fOldState = fWinState;
    long fNewState = (fWinState & ~mask) | (state & mask);

    // !!! this should work
    //if (fNewState == fOldState)
    //    return ;

    // !!! move here

    fWinState = fNewState;

    if ((fOldState ^ fNewState) & WinStateAllWorkspaces) {
        MSG(("WinStateAllWorkspaces: %d", isSticky()));
#ifdef CONFIG_TASKBAR
        updateTaskBar();
#endif
    }
    MSG(("setState: oldState: %lX, newState: %lX, mask: %lX, state: %lX",
         fOldState, fNewState, mask, state));
    //msg("normal1: (%d:%d %dx%d)", normalX, normalY, normalWidth, normalHeight);
    if ((fOldState ^ fNewState) & (WinStateMaximizedVert |
                                   WinStateMaximizedHoriz))
    {
        MSG(("WinStateMaximized: %d", isMaximized()));

        if (fMaximizeButton) {
            if (isMaximized()) {
                fMaximizeButton->setActions(actionRestore, actionRestore);
                fMaximizeButton->setToolTip(_("Restore"));
            } else {
                fMaximizeButton->setActions(actionMaximize, actionMaximizeVert);
                fMaximizeButton->setToolTip(_("Maximize"));
            }
        }
    }
    if ((fOldState ^ fNewState) & WinStateMinimized) {
        MSG(("WinStateMinimized: %d", isMaximized()));
        if (fNewState & WinStateMinimized)
            minimizeTransients();
        else if (owner() && owner()->isMinimized())
            owner()->setState(WinStateMinimized, 0);

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
#ifdef CONFIG_TASKBAR
        updateTaskBar();
#endif
        layoutResizeIndicators();
        manager->focusLastWindow();
    }
    if ((fOldState ^ fNewState) & WinStateRollup) {
        MSG(("WinStateRollup: %d", isRollup()));
        if (fRollupButton) {
            if (isRollup()) {
                fRollupButton->setToolTip(_("Rolldown"));
            } else {
                fRollupButton->setToolTip(_("Rollup"));
            }
            fRollupButton->repaint();
        }
        layoutResizeIndicators();
    }
    if ((fOldState ^ fNewState) & WinStateHidden) {
        MSG(("WinStateHidden: %d", isHidden()));

        if (fNewState & WinStateHidden)
            hideTransients();
        else if (owner() && owner()->isHidden())
            owner()->setState(WinStateHidden, 0);

#ifdef CONFIG_TASKBAR
        updateTaskBar();
#endif
    }
    updateState();
    updateLayer();
    manager->updateFullscreenLayer();
    updateDerivedSize(fOldState ^ fNewState);
    updateLayout();

#ifdef CONFIG_SHAPE
    if ((fOldState ^ fNewState) & (WinStateRollup | WinStateMinimized))
        setShape();
#endif
    if ((fOldState ^ fNewState) & WinStateMinimized) {
        if (!(fNewState & WinStateMinimized))
            restoreMinimizedTransients();
    }
    if ((fOldState ^ fNewState) & WinStateHidden) {
        if (!(fNewState & WinStateHidden))
            restoreHiddenTransients();
    }
    if ((clickFocus || !strongPointerFocus) &&
        this == manager->getFocus() &&
        ((fOldState ^ fNewState) & WinStateRollup)) {
        manager->setFocus(this);
    }
}

void YFrameWindow::setSticky(bool sticky) {
    setState(WinStateAllWorkspaces, sticky ? WinStateAllWorkspaces : 0);

#ifdef CONFIG_WINLIST
    windowList->updateWindowListApp(fWinListItem);
#endif
    if (affectsWorkArea())
        manager->updateWorkArea();
}

#if DO_NOT_COVER_OLD
void YFrameWindow::setDoNotCover(bool doNotCover) {
    fWinOptionMask &= ~foDoNotCover;

    if (doNotCover) {
        fFrameOptions |= foDoNotCover;
    } else {
        fFrameOptions &= ~foDoNotCover;
    }
    manager->updateWorkArea();
}
#endif

void YFrameWindow::updateMwmHints() {
    int bx = borderX();
    int by = borderY();
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
        configureClient(x() + bx + bx - borderX(),
                        y() + by + by - borderY() + titleY(),
                        client()->width(), client()->height());
}

#ifndef LITE
YIcon *YFrameWindow::clientIcon() const {
    for(YFrameWindow const *f(this); f != NULL; f = f->owner())
        if (f->getClientIcon())
            return f->getClientIcon();

    return defaultAppIcon;
}
#endif

void YFrameWindow::updateProperties() {
    client()->setWinWorkspaceHint(fWinWorkspace);
    client()->setWinLayerHint(fWinRequestedLayer);
#ifdef CONFIG_TRAY
    client()->setWinTrayHint(fWinTrayOption);
#endif
    client()->setWinStateHint(WIN_STATE_ALL, fWinState);
}

#ifdef CONFIG_TASKBAR
void YFrameWindow::updateTaskBar() {
#ifdef CONFIG_TRAY
    bool needTrayApp(false);
    int dw(0);

    if (taskBar && fManaged && taskBar->trayPane()) {
        if (!isHidden() &&
            !(frameOptions() & foIgnoreTaskBar) &&
            (getTrayOption() != WinTrayIgnore))
            if (trayShowAllWindows || visibleOn(manager->activeWorkspace()))
                needTrayApp = true;

        if (needTrayApp && fTrayApp == 0)
            fTrayApp = taskBar->trayPane()->addApp(this);

        if (fTrayApp) {
            fTrayApp->setShown(needTrayApp);
            if (fTrayApp->getShown()) ///!!! optimize
                fTrayApp->repaint();
        }
#if 0
        /// !!! optimize
        TrayPane *tp = taskBar->trayPane();
        int const nw(tp->getRequiredWidth());

        if ((dw = nw - tp->width()))
            taskBar->trayPane()->setGeometry(
                YRect(tp->x() - dw, tp->y(), nw, tp->height()));

#endif
        taskBar->trayPane()->relayout();
    }
#endif

    bool needTaskBarApp(false);

    if (taskBar && fManaged && taskBar->taskPane()) {
#ifndef CONFIG_TRAY
        if (!(isHidden() || (frameOptions() & foIgnoreTaskBar)))
#else
        if (!(isHidden() || (frameOptions() & foIgnoreTaskBar)) &&
            (getTrayOption() == WinTrayIgnore ||
            (getTrayOption() == WinTrayMinimized && !isMinimized())))
#endif
            if (taskBarShowAllWindows || visibleOn(manager->activeWorkspace()))
                needTaskBarApp = true;

        if (needTaskBarApp && fTaskBarApp == 0)
            fTaskBarApp = taskBar->taskPane()->addApp(this);

        if (fTaskBarApp) {
            fTaskBarApp->setShown(needTaskBarApp);
            if (fTaskBarApp->getShown()) ///!!! optimize
                fTaskBarApp->repaint();
        }
        /// !!! optimize

#ifdef CONFIG_TRAY
        if (dw) taskBar->taskPane()->setSize
            (taskBar->taskPane()->width() - dw, taskBar->taskPane()->height());
#endif
        taskBar->taskPane()->relayout();
    }

#ifndef LITE
    if (dw && NULL == taskBar->taskPane() && NULL != taskBar->addressBar())
        taskBar->addressBar()->setSize
            (taskBar->addressBar()->width() - dw,
             taskBar->addressBar()->height());
#endif
}
#endif

void YFrameWindow::handleMsgBox(YMsgBox *msgbox, int operation) {
    //msg("msgbox operation %d", operation);
    if (msgbox == fKillMsgBox && fKillMsgBox) {
        if (fKillMsgBox) {
            manager->unmanageClient(fKillMsgBox->handle());
            fKillMsgBox = 0;
            manager->focusTopWindow();
        }
        if (operation == YMsgBox::mbOK)
            wmKill();
    }
}

#ifdef WMSPEC_HINTS
void YFrameWindow::updateNetWMStrut() {
    int l, r, t, b;
    client()->getNetWMStrut(&l, &r, &t, &b);
    if (l != fStrutLeft ||
        r != fStrutRight ||
        t != fStrutTop ||
        b != fStrutBottom)
    {
        fStrutLeft = l;
        fStrutRight = r;
        fStrutTop = t;
        fStrutBottom = b;
        MSG(("strut: %d %d %d %d", l, r, t, b));
        manager->updateWorkArea();
    }
}
#endif

/* Work around for X11R5 and earlier */
#ifndef XUrgencyHint
#define XUrgencyHint (1 << 8)
#endif

void YFrameWindow::updateUrgency() {
    fClientUrgency = false;
    XWMHints *h = client()->hints();
    if (h && (h->flags & XUrgencyHint))
        fClientUrgency = true;

#ifdef CONFIG_TASKBAR
    if (fTaskBarApp) {
        bool shown = fTaskBarApp->getShown();
        fTaskBarApp->setFlash(fWmUrgency || fClientUrgency);
        if (shown != fTaskBarApp->getShown())
            if (taskBar && taskBar->taskPane())
                taskBar->taskPane()->relayout();
    }
#endif
    /// something else when no taskbar (flash titlebar, flash icon)
}

void YFrameWindow::setWmUrgency(bool wmUrgency) {
    fWmUrgency = wmUrgency;
    updateUrgency();
}

int YFrameWindow::getScreen() {
    int nx, ny, nw, nh;
    getNormalGeometryInner(&nx, &ny, &nw, &nh);
    return manager->getScreenForRect(nx, ny, nw, nh);
}

void YFrameWindow::wmArrange(int tcb, int lcr) {
    int mx, my, Mx, My, newX = 0, newY = 0;

    int xiscreen = manager->getScreenForRect(x(), y(), width(), height());

    manager->getWorkArea(this, &mx, &my, &Mx, &My, xiscreen);

    switch (tcb) {
    case waTop:
        newY = my - (considerVertBorder ? 0 : borderY());
        break;
    case waCenter:
        newY = my + ((My - my) >> 1) - (height() >> 1) - (considerVertBorder ? 0 : borderY());
        break;
    case waBottom:
        newY = My - height() + (considerVertBorder ? 0 : borderY());
        break;
    }

    switch (lcr) {
    case waLeft:
        newX = mx - (considerHorizBorder ? 0 : borderX());
        break;
    case waCenter:
        newX = mx + ((Mx - mx) >> 1) - ((width()) >> 1) - (considerHorizBorder ? 0 : borderX());
        break;
    case waRight:
        newX = Mx - width() + (considerHorizBorder ? 0 : borderX());
        break;
    }

    MSG(("wmArrange: setPosition(x = %d, y = %d)", newX, newY));

    setCurrentPositionOuter(newX, newY);
}

void YFrameWindow::wmSnapMove(int tcb, int lcr) {
   int mx, my, Mx, My, newX = 0, newY = 0;

    int xiscreen = manager->getScreenForRect(x(), y(), width(), height());

    YFrameWindow **w = 0;
    int count = 0;

    manager->getWindowsToArrange(&w, &count);

    manager->getWorkArea(this, &mx, &my, &Mx, &My, xiscreen);

    MSG(("WorkArea: mx = %d, my = %d, Mx = %d, My = %d", mx, my, Mx, My));
    MSG(("thisFrame: x = %d, y = %d, w = %d, h = %d, bx = %d, by = %d, ty = %d",
         x(), y(), width(), height(), borderX(), borderY(), titleY()));
    MSG(("thisClient: w = %d, h = %d", client()->width(), client()->height()));

    switch (tcb) {
       case waTop:
           newY = getTopCoord(my, w, count);
           if (!considerVertBorder && newY == my)
               newY -= borderY();
           break;
       case waCenter:
           newY = y();
           break;
       case waBottom:
           newY = getBottomCoord(My, w, count) - height();
           if (!considerVertBorder && (newY + height()) == My)
               newY += borderY();
           break;
    }

    switch (lcr) {
       case waLeft:
           newX = getLeftCoord(mx, w, count);
           if (!considerHorizBorder && newX == mx)
               newX -= borderX();
           break;
       case waCenter:
           newX = x();
           break;
       case waRight:
           newX = getRightCoord(Mx, w, count) - width();
           if (!considerHorizBorder && (newX + width()) == Mx)
               newX += borderX();
           break;
    }

    MSG(("NewPosition: x = %d, y = %d", newX, newY));

    setCurrentPositionOuter(newX, newY);

    delete [] w;
}

int YFrameWindow::getTopCoord(int my, YFrameWindow **w, int count)
{
    int i, n;

    if (y() < my)
        return y();

    for (i = y() - 2; i > my; i--) {
        for (n = 0; n < count; n++) {
            if (    (this != w[n])
                 && (i == (w[n]->y() + w[n]->height()))
                 && ( x()            < (w[n]->x() + w[n]->width()))
                 && ((x() + width()) > (w[n]->x())) ) {
                return i;
            }
        }
    }

    return my;
}

int YFrameWindow::getBottomCoord(int My, YFrameWindow **w, int count)
{
    int i, n;

    if ((y() + height()) > My)
        return y() + height();

    for (i = y() + height() + 2; i < My; i++) {
        for (n = 0; n < count; n++) {
            if (    (this != w[n])
                 && (i == w[n]->y())
                 && ( x()            < (w[n]->x() + w[n]->width()))
                 && ((x() + width()) > (w[n]->x())) ) {
                return i;
            }
        }
    }

    return My;
}

int YFrameWindow::getLeftCoord(int mx, YFrameWindow **w, int count)
{
    int i, n;

    if (x() < mx)
        return x();

    for (i = x() - 2; i > mx; i--) {
        for (n = 0; n < count; n++) {
            if (    (this != w[n])
                 && (i == (w[n]->x() + w[n]->width()))
                 && ( y()             < (w[n]->y() + w[n]->height()))
                 && ((y() + height()) > (w[n]->y())) ) {
                return i;
            }
        }
    }

    return mx;
}

int YFrameWindow::getRightCoord(int Mx, YFrameWindow **w, int count)
{
    int i, n;

    if ((x() + width()) > Mx)
        return x() + width();

    for (i = x() + width() + 2; i < Mx; i++) {
        for (n = 0; n < count; n++) {
            if (    (this != w[n])
                 && (i == w[n]->x())
                 && ( y()             < (w[n]->y() + w[n]->height()))
                 && ((y() + height()) > (w[n]->y())) ) {
                return i;
            }
        }
    }

    return Mx;
}
