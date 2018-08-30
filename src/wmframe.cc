/*
 * IceWM
 *
 * Copyright (C) 1997-2003 Marko Macek
 */

#include "config.h"
#include "wmframe.h"

#include "yprefs.h"
#include "prefs.h"
#include "atasks.h"
#include "atray.h"
#include "wmcontainer.h"
#include "wmtitle.h"
#include "wmbutton.h"
#include "wmminiicon.h"
#include "wmswitch.h"
#include "wmtaskbar.h"
#include "wmwinlist.h"
#include "wmapp.h"
#include "yrect.h"
#include "wpixmaps.h"
#include "aworkspaces.h"

#include "intl.h"

       YColorName activeBorderBg(&clrActiveBorder);
static YColorName inactiveBorderBg(&clrInactiveBorder);

lazy<YTimer> YFrameWindow::fAutoRaiseTimer;
lazy<YTimer> YFrameWindow::fDelayFocusTimer;

extern XContext windowContext;
extern XContext frameContext;
extern XContext clientContext;

YFrameWindow::YFrameWindow(
    YActionListener *wmActionListener,
    YWindow *parent, int depth, Visual *visual)
    : YWindow(parent, 0, depth, visual)
{
    this->wmActionListener = wmActionListener;

    fShapeWidth = -1;
    fShapeHeight = -1;
    fShapeTitleY = -1;
    fShapeBorderX = -1;
    fShapeBorderY = -1;

    setDoubleBuffer(false);
    fClient = 0;
    fFocused = false;

    topSide = None;
    leftSide = None;
    rightSide = None;
    bottomSide = None;
    topLeft = None;
    topRight = None;
    bottomLeft = None;
    bottomRight = None;
    topLeftSide = None;
    topRightSide = None;
    indicatorsCreated = false;
    indicatorsVisible = false;

    fPopupActive = 0;
    fWmUrgency = false;
    fClientUrgency = false;
    fWindowType = wtNormal;

    normalX = 0;
    normalY = 0;
    normalW = 1;
    normalH = 1;
    posX = 0;
    posY = 0;
    posW = 1;
    posH = 1;
    extentLeft = -1;
    extentRight = -1;
    extentTop = -1;
    extentBottom = -1;
    iconX = -1;
    iconY = -1;
    movingWindow = false;
    sizingWindow = false;
    fFrameFunctions = 0;
    fFrameDecors = 0;
    fFrameOptions = 0;
    fFrameIcon = null;
    fTaskBarApp = 0;
    fTrayApp = 0;
    fWinListItem = 0;
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
    fTitleBar = 0;

    fUserTimeWindow = None;

    fFullscreenMonitorsTop = -1;
    fFullscreenMonitorsBottom = -1;
    fFullscreenMonitorsLeft = -1;
    fFullscreenMonitorsRight = -1;

    setStyle(wsOverrideRedirect);
    setPointer(YXApplication::leftPointer);

    fWinWorkspace = manager->activeWorkspace();
    fWinActiveLayer = WinLayerNormal;
    fWinRequestedLayer = WinLayerNormal;
    fOldLayer = fWinActiveLayer;
    fWinTrayOption = WinTrayIgnore;
    fWinState = 0;
    fWinOptionMask = ~0;
    fTrayOrder = 0;

    fClientContainer = new YClientContainer(this, this);
    fClientContainer->show();
    fClientContainer->setTitle("Container");
    setTitle("Frame");
}

YFrameWindow::~YFrameWindow() {
    fManaged = false;
    if (fKillMsgBox) {
        manager->unmanageClient(fKillMsgBox->handle());
        fKillMsgBox = 0;
    }
    if (fWindowType == wtDialog)
        wmapp->signalGuiEvent(geDialogClosed);
    else
        wmapp->signalGuiEvent(geWindowClosed);
    if (fDelayFocusTimer)
        fDelayFocusTimer->disableTimerListener(this);
    if (fAutoRaiseTimer)
        fAutoRaiseTimer->disableTimerListener(this);
    if (movingWindow || sizingWindow)
        endMoveSize();
    if (fPopupActive)
        fPopupActive->cancelPopup();
    if (fTaskBarApp) {
        if (taskBar)
            taskBar->removeTasksApp(this);
        fTaskBarApp = 0;
    }
    if (fTrayApp) {
        if (taskBar)
            taskBar->removeTrayApp(this);
        fTrayApp = 0;
    }
    if (fWinListItem) {
        if (windowList)
            windowList->removeWindowListApp(fWinListItem);
        delete fWinListItem; fWinListItem = 0;
    }
    if (fMiniIcon) {
        delete fMiniIcon;
        fMiniIcon = 0;
    }
    fFrameIcon = null;
#if 1
    fWinState &= ~WinStateFullscreen;
    updateLayer(false);
#endif
    // perhaps should be done another way
    removeTransients();
    removeAsTransient();
    manager->removeFocusFrame(this);
    manager->removeClientFrame(this);
    manager->removeCreatedFrame(this);
    removeFrame();
    if (wmapp->hasSwitchWindow())
        wmapp->getSwitchWindow()->destroyedFrame(this);
    if (fClient != 0) {
        if (!fClient->destroyed() && fClient->adopted())
            XRemoveFromSaveSet(xapp->display(), client()->handle());
        XDeleteContext(xapp->display(), client()->handle(), frameContext);
    }
    if (fUserTimeWindow != None) {
        XDeleteContext(xapp->display(), fUserTimeWindow, windowContext);
    }

    if (affectsWorkArea())
        manager->updateWorkArea();
    // FIX !!! should actually check if < than current values
    if (fStrutLeft != 0 || fStrutRight != 0 ||
        fStrutTop != 0 || fStrutBottom != 0)
        manager->updateWorkArea();

    delete fClient; fClient = 0;
    delete fClientContainer; fClientContainer = 0;

    /* if (indicatorsCreated) {
        // do we really need to explicitly destroy these?
        XDestroyWindow(xapp->display(), topSide);
        XDestroyWindow(xapp->display(), leftSide);
        XDestroyWindow(xapp->display(), rightSide);
        XDestroyWindow(xapp->display(), bottomSide);
        XDestroyWindow(xapp->display(), topLeft);
        XDestroyWindow(xapp->display(), topRight);
        XDestroyWindow(xapp->display(), bottomLeft);
        XDestroyWindow(xapp->display(), bottomRight);
    } */

    delete fTitleBar; fTitleBar = 0;

    manager->updateClientList();

    // update pager when unfocused windows are killed, because this
    // does not call YWindowManager::updateFullscreenLayer()
    if (!focused() && taskBar && taskBar->workspacesPane()) {
        taskBar->workspacesPane()->repaint();
    }
}

YFrameTitleBar* YFrameWindow::titlebar() {
    if (fTitleBar == 0 && titleY() > 0) {
        fTitleBar = new YFrameTitleBar(this, this);
        fTitleBar->show();
    }
    return fTitleBar;
}

void YFrameWindow::doManage(YFrameClient *clientw, bool &doActivate, bool &requestFocus) {
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
        normalH = sh ? (h - sh->base_height) / sh->height_inc : h;


        if (sh && (sh->flags & PWinGravity) &&
            sh->win_gravity == StaticGravity)
        {
            normalX += borderXN();
            normalY += borderYN() + titleYN();
        } else {
            int gx, gy;
            client()->gravityOffsets(gx, gy);

            if (gx > 0)
                normalX += 2 * borderXN() - 1 - client()->getBorder();
            if (gy > 0)
                normalY += 2 * borderYN() + titleYN() - 1 - client()->getBorder();

        }

        getNormalGeometryInner(&posX, &posY, &posW, &posH);
    }

    updateIcon();
    manage(fClient);
    manager->appendCreatedFrame(this);
    bool isRunning = manager->wmState() == YWindowManager::wmRUNNING;
    insertFrame(!isRunning);
    manager->insertFocusFrame(this, !isRunning);

    getFrameHints();

    {
        long layer = 0;
        Atom net_wm_window_type;
        if (fClient->getNetWMWindowType(&net_wm_window_type)) {
            if (net_wm_window_type == _XA_NET_WM_WINDOW_TYPE_COMBO)
            {
                setWindowType(wtCombo);
            } else
            if (net_wm_window_type == _XA_NET_WM_WINDOW_TYPE_DESKTOP)
            {
/// TODO #warning "this needs some cleanup"
                setWindowType(wtDesktop);
                setAllWorkspaces();
            } else
            if (net_wm_window_type == _XA_NET_WM_WINDOW_TYPE_DIALOG)
            {
                setWindowType(wtDialog);
            } else
            if (net_wm_window_type == _XA_NET_WM_WINDOW_TYPE_DND)
            {
                setWindowType(wtDND);
            } else
            if (net_wm_window_type == _XA_NET_WM_WINDOW_TYPE_DOCK)
            {
                setWindowType(wtDock);
                setAllWorkspaces();
            } else
            if (net_wm_window_type == _XA_NET_WM_WINDOW_TYPE_DROPDOWN_MENU)
            {
                setWindowType(wtDropdownMenu);
            } else
            if (net_wm_window_type == _XA_NET_WM_WINDOW_TYPE_MENU)
            {
                setWindowType(wtMenu);
            } else
            if (net_wm_window_type == _XA_NET_WM_WINDOW_TYPE_NORMAL)
            {
                setWindowType(wtNormal);
            } else
            if (net_wm_window_type == _XA_NET_WM_WINDOW_TYPE_NOTIFICATION)
            {
                setWindowType(wtNotification);
            } else
            if (net_wm_window_type == _XA_NET_WM_WINDOW_TYPE_POPUP_MENU)
            {
                setWindowType(wtPopupMenu);
            } else
            if (net_wm_window_type == _XA_NET_WM_WINDOW_TYPE_SPLASH)
            {
                setWindowType(wtSplash);
            } else
            if (net_wm_window_type == _XA_NET_WM_WINDOW_TYPE_TOOLBAR)
            {
                setWindowType(wtToolbar);
            } else
            if (net_wm_window_type == _XA_NET_WM_WINDOW_TYPE_TOOLTIP)
            {
                setWindowType(wtTooltip);
            } else
            if (net_wm_window_type == _XA_NET_WM_WINDOW_TYPE_UTILITY)
            {
                setWindowType(wtUtility);
            } else
            {
                setWindowType(wtNormal);
            }
            updateMwmHints();
            updateLayer(true);
        }
        else if (fClient->getWinLayerHint(&layer)) {
            setRequestedLayer(layer);
        }
    }

    getDefaultOptions(requestFocus);
    updateNetWMStrut(); /// ? here
    updateNetWMStrutPartial();
    updateNetStartupId();
    updateNetWMUserTime();
    updateNetWMUserTimeWindow();

    long workspace = getWorkspace(), state_mask(0), state(0);
    long tray(0);

    MSG(("Map - Frame: %d", visible()));
    MSG(("Map - Client: %d", client()->visible()));

    if (client()->getNetWMStateHint(&state_mask, &state)) {
        setState(state_mask, state);
    } else
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
            fFrameOptions |= foMinimized;
            if (manager->wmState() != YWindowManager::wmSTARTUP)
                setState(WinStateMinimized, WinStateMinimized);
            break;

        case NormalState:
        case WithdrawnState:
            break;
        }
    }

    if (client()->getNetWMDesktopHint(&workspace))
        setWorkspace(workspace);
    else
    if (client()->getWinWorkspaceHint(&workspace))
        setWorkspace(workspace);

    if (client()->getWinTrayHint(&tray))
        setTrayOption(tray);
    addAsTransient();
    if (owner())
        setWorkspace(mainOwner()->getWorkspace());

    if (isHidden() || isMinimized() || isIconic()) {
        doActivate = false;
        requestFocus = false;
    }

    updateFocusOnMap(doActivate);
    addTransients();
    manager->restackWindows(this);

    afterManage();
}

void YFrameWindow::afterManage() {
    if (affectsWorkArea())
        manager->updateWorkArea();
    manager->updateClientList();
#ifdef CONFIG_SHAPE
    setShape();
#endif
    if (!(frameOptions() & foFullKeys))
        grabKeys();
    fClientContainer->grabButtons();
    if (windowList && !(frameOptions() & foIgnoreWinList))
        fWinListItem = windowList->addWindowListApp(this);
    if (fWindowType == wtDialog)
        wmapp->signalGuiEvent(geDialogOpened);
    else
        wmapp->signalGuiEvent(geWindowOpened);
}

// create a window to show a resize pointer on the frame border
Window YFrameWindow::createPointerWindow(Cursor cursor, Window parent) {
    unsigned long valuemask = CWEventMask | CWCursor;
    XSetWindowAttributes attributes;
    attributes.event_mask = 0;
    attributes.cursor = cursor;
    return XCreateWindow(xapp->display(), parent, 0, 0, 1, 1, 0,
                         0, InputOnly, CopyFromParent,
                         valuemask, &attributes);
}

// create 8 resize pointer indicator windows
void YFrameWindow::createPointerWindows() {

    // There is a competition for mouse input between
    // the resize handles, the titlebar and its buttons.
    // The following solution positions the corner resize
    // handles between the titlebar and the titlebar buttons.
    const Window frameWin = handle();
    const Window titleWin = titlebar() ? titlebar()->handle() : frameWin;

    topSide = createPointerWindow(YWMApp::sizeTopPointer, frameWin);
    leftSide = createPointerWindow(YWMApp::sizeLeftPointer, frameWin);
    rightSide = createPointerWindow(YWMApp::sizeRightPointer, frameWin);
    bottomSide = createPointerWindow(YWMApp::sizeBottomPointer, frameWin);

    topLeft = createPointerWindow(YWMApp::sizeTopLeftPointer, titleWin);
    topRight = createPointerWindow(YWMApp::sizeTopRightPointer, titleWin);
    bottomLeft = createPointerWindow(YWMApp::sizeBottomLeftPointer, frameWin);
    bottomRight = createPointerWindow(YWMApp::sizeBottomRightPointer, frameWin);

    topLeftSide = createPointerWindow(YWMApp::sizeTopLeftPointer, frameWin);
    topRightSide = createPointerWindow(YWMApp::sizeTopRightPointer, frameWin);

    indicatorsCreated = true;

    if (titlebar()) {
        titlebar()->raiseButtons();
    }
    XRaiseWindow(xapp->display(), topSide);
    XStoreName(xapp->display(), topSide, "topSide");
    container()->raise();
}

void YFrameWindow::grabKeys() {
    XUngrabKey(xapp->display(), AnyKey, AnyModifier, handle());

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

    container()->regrabMouse();
}

void YFrameWindow::manage(YFrameClient *client) {
    PRECONDITION(client != 0);
    fClient = client;

/// TODO #warning "optimize this, do it only if needed"
    XSetWindowBorderWidth(xapp->display(),
                          client->handle(),
                          0);

#if 0
    {
        XSetWindowAttributes xswa;
        xswa.backing_store = Always;
        xswa.win_gravity = NorthWestGravity;

        XChangeWindowAttributes(xapp->display(), client->handle(),
                                CWBackingStore | CWWinGravity, &xswa);
    }
#endif

    if (client->adopted())
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
            posY += borderYN() + titleYN() - 2 * client()->getBorder();

        if (gx == 0 && gy == 0) {
            const XSizeHints* sh = client()->sizeHints();
            if (sh && (sh->flags & PWinGravity) &&
                sh->win_gravity == StaticGravity)
            {
                posY += titleYN();
            }
        }

        if (reparent)
            client()->reparent(manager, posX, posY);

        client()->setSize(posWidth, posHeight);

        if (manager->wmState() != YWindowManager::wmSHUTDOWN) {
            client()->setFrameState(WithdrawnState);
            extentLeft = extentRight = extentTop = extentBottom = -1;
        }

        if (!client()->destroyed() && client()->adopted())
            XRemoveFromSaveSet(xapp->display(), client()->handle());
    }
    else
        fClient->unmanageWindow();

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

    // update pager when windows move/resize themselves (like xmms, gmplayer, ...),
    // because this does not call YFrameWindow::endMoveSize()
    if (taskBar && taskBar->workspacesPane()) {
        taskBar->workspacesPane()->repaint();
    }
}

void YFrameWindow::configureClient(const XConfigureRequestEvent &configureRequest) {

    if (hasbit(configureRequest.value_mask, CWBorderWidth))
        client()->setBorder(configureRequest.border_width);

    int cx, cy, cw, ch;
    getNewPos(configureRequest, cx, cy, cw, ch);

    configureClient(cx, cy, cw, ch);

    if (configureRequest.value_mask & CWStackMode) {
        union {
            YFrameWindow *ptr;
            XPointer xptr;
        } sibling = { 0 };
        XWindowChanges xwc;

        if ((configureRequest.value_mask & CWSibling) &&
            XFindContext(xapp->display(),
                         configureRequest.above,
                         clientContext,
                         &(sibling.xptr)) == 0)
            xwc.sibling = sibling.ptr->handle();
        else
            xwc.sibling = configureRequest.above;

        xwc.stack_mode = configureRequest.detail;

        /* !!! implement the rest, and possibly fix these: */

        if (sibling.ptr && xwc.sibling != None) { /* ICCCM suggests sibling==None */
            switch (xwc.stack_mode) {
            case Above:
                setAbove(sibling.ptr);
                break;
            case Below:
                setBelow(sibling.ptr);
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
                    if (requestFocusOnAppRaise) {
                        if (canRaise()) {
                            setWmUrgency(true);
                        }
                    }
                } else {
                    if (canRaise()) {
                        wmRaise();
                    }
#if 1

#if 1
                    if ( !(frameOptions() & foNoFocusOnAppRaise) &&
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
        (crossing.mode == NotifyNormal ||
         (strongPointerFocus && crossing.mode == NotifyUngrab)) &&
        crossing.window == handle() &&
        (strongPointerFocus ||
         (crossing.serial != YWindow::getLastEnterNotifySerial() &&
          crossing.serial != YWindow::getLastEnterNotifySerial() + 1))
#if false
        &&
        (strongPointerFocus ||
         fMouseFocusX != crossing.x_root ||
         fMouseFocusY != crossing.y_root)
#endif
       )
    {
        //msg("xf: %d %d", fMouseFocusX, crossing.x_root, fMouseFocusY, crossing.y_root);

//        fMouseFocusX = crossing.x_root;
//        fMouseFocusY = crossing.y_root;

        if (!clickFocus && visible() && canFocusByMouse()) {
            if (!delayPointerFocus)
                focus(false);
            else {
                fDelayFocusTimer->setTimer(pointerFocusDelay, this, true);
            }
        } else {
            if (fDelayFocusTimer) {
                fDelayFocusTimer->stopTimer();
            }
        }
        if (autoRaise) {
            fAutoRaiseTimer->setTimer(autoRaiseDelay, this, true);
        }
    } else if (crossing.type == LeaveNotify &&
               fFocused &&
               focusRootWindow &&
               crossing.window == handle())
    {
//        fMouseFocusX = crossing.x_root;
//        fMouseFocusY = crossing.y_root;

        if (crossing.detail != NotifyInferior &&
            crossing.mode == NotifyNormal)
        {
            if (fDelayFocusTimer)
                fDelayFocusTimer->disableTimerListener(this);
            if (autoRaise && fAutoRaiseTimer) {
                fAutoRaiseTimer->disableTimerListener(this);
            }
        }
    }
}

void YFrameWindow::handleFocus(const XFocusChangeEvent &focus) {
    if (wmapp->hasSwitchWindow()) {
        if (wmapp->getSwitchWindow()->visible()) {
            return ;
        }
    }
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
    manager->removeLayeredFrame(this);
#ifdef DEBUG
    if (debug_z) dumpZorder("after removing", this);
#endif
}

void YFrameWindow::insertFrame(bool top) {
#ifdef DEBUG
    if (debug_z) dumpZorder("before inserting", this);
#endif
    if (top) {
        manager->setTop(getActiveLayer(), this);
    } else {
        manager->setBottom(getActiveLayer(), this);
    }
#ifdef DEBUG
    if (debug_z) dumpZorder("after inserting", this);
#endif
}

void YFrameWindow::setAbove(YFrameWindow *aboveFrame) {
    manager->setAbove(this, aboveFrame);
}

void YFrameWindow::setBelow(YFrameWindow *belowFrame) {
    manager->setBelow(this, belowFrame);
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
        if ((flags & fwfFocusable) && !p->canFocus())
            goto next;
        if ((flags & fwfWorkspace) && !p->visibleNow())
            goto next;
#if 0
        if ((flags & fwfSwitchable) && (p->frameOptions() & foIgnoreQSwitch))
            goto next;
#endif
        if (!p->client()->adopted() || p->client()->destroyed())
            goto next;

        return p;

    next:
        if (flags & fwfBackward)
            p = (flags & fwfLayers) ? p->prevLayer() : p->prev();
        else
            p = (flags & fwfLayers) ? p->nextLayer() : p->next();
        if (p == 0) {
            if (!(flags & fwfCycle))
                return 0;
            else if (flags & fwfBackward)
                p = (flags & fwfLayers) ? manager->bottomLayer() : manager->bottom(getActiveLayer());
            else
                p = (flags & fwfLayers) ? manager->topLayer() : manager->top(getActiveLayer());
        }
    } while (p != this);

    if (!(flags & fwfSame))
        return 0;
    if ((flags & fwfVisible) && !p->visible())
        return 0;
    if ((flags & fwfWorkspace) && !p->visibleNow())
        return 0;
    if (!p->client()->adopted() || p->client()->destroyed())
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
    xev.xconfigure.y = y() + borderY() + titleY();
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

void YFrameWindow::actionPerformed(YAction action, unsigned int modifiers) {
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
        if (overlaps(bool(Below)) && canRaise()){
            wmRaise();
            manager->setFocus(this, true);
        } else if (overlaps(bool(Above)) && canLower())
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
#if DO_NOT_COVER_OLD
    } else if (action == actionDoNotCover) {
        wmToggleDoNotCover();
#endif
    } else if (action == actionFullscreen) {
        wmToggleFullscreen();
    } else if (action == actionToggleTray) {
        wmToggleTray();
    } else {
        for (int l(0); l < WinLayerCount; l++) {
            if (action == layerActionSet[l]) {
                bool isFull = isFullscreen() && manager->fullscreenEnabled();
                if (isFull)
                    manager->setFullscreenEnabled(false);
                wmSetLayer(l);
                if (isFull)
                    manager->setFullscreenEnabled(true);
                return ;
            }
        }
        for (int w(0); w < workspaceCount; w++) {
            if (action == workspaceActionMoveTo[w]) {
                wmMoveToWorkspace(w);
                return ;
            }
        }
        wmActionListener->actionPerformed(action, modifiers);
    }
}

void YFrameWindow::wmSetLayer(long layer) {
    setRequestedLayer(layer);
}

void YFrameWindow::wmSetTrayOption(long option) {
    setTrayOption(option);
}

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

void YFrameWindow::wmToggleTray() {
    if (getTrayOption() != WinTrayExclusive) {
        setTrayOption(WinTrayExclusive);
    } else {
        setTrayOption(WinTrayIgnore);
    }
}

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
    startMoveSize(true, false,
                  0, 0,
                  wx - x(), wy - y());
}

void YFrameWindow::wmSize() {
    startMoveSize(false, false,
                  0, 0,
                  0, 0);
}

void YFrameWindow::wmRestore() {
    wmapp->signalGuiEvent(geWindowRestore);
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
        wmapp->signalGuiEvent(geWindowRestore);
        setState(WinStateMinimized, 0);
    } else {
        wmapp->signalGuiEvent(geWindowMin);
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
        wmapp->signalGuiEvent(geWindowRestore);
        setState(WinStateMaximizedVert |
                 WinStateMaximizedHoriz |
                 WinStateMinimized, 0);
    } else {
        wmapp->signalGuiEvent(geWindowMax);
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
        wmapp->signalGuiEvent(geWindowRestore);
        setState(WinStateMaximizedVert, 0);
    } else {
        wmapp->signalGuiEvent(geWindowMax);
        setState(WinStateMaximizedVert, WinStateMaximizedVert);
    }
}

void YFrameWindow::wmMaximizeHorz() {
    if (isMaximizedHoriz()) {
        wmapp->signalGuiEvent(geWindowRestore);
        setState(WinStateMaximizedHoriz, 0);
    } else {
        wmapp->signalGuiEvent(geWindowMax);
        setState(WinStateMaximizedHoriz, WinStateMaximizedHoriz);
    }
}

void YFrameWindow::wmRollup() {
    if (isRollup()) {
        wmapp->signalGuiEvent(geWindowRestore);
        setState(WinStateRollup, 0);
    } else {
        //if (!canRollup())
        //    return ;
        wmapp->signalGuiEvent(geWindowRollup);
        setState(WinStateRollup, WinStateRollup);
    }
}

void YFrameWindow::wmHide() {
    if (isHidden()) {
        wmapp->signalGuiEvent(geWindowRestore);
        setState(WinStateHidden, 0);
    } else {
        wmapp->signalGuiEvent(geWindowHide);
        setState(WinStateHidden, WinStateHidden);
    }
    manager->focusLastWindow();
}

void YFrameWindow::wmLower() {
    if (this != manager->bottom(getActiveLayer())) {
        manager->lockFocus();
        if (getState() ^ WinStateMinimized)
            wmapp->signalGuiEvent(geWindowLower);
        for (YFrameWindow *w = this; w; w = w->owner()) {
            w->doLower();
        }
        manager->restackWindows(this);
        manager->unlockFocus();
        manager->focusTopWindow();
    }
}

void YFrameWindow::doLower() {
    setAbove(0);
    manager->lowerFocusFrame(this);
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

        {
            for (YFrameWindow * w (transient()); w; w = w->nextTransient())
                w->doRaise();
        }

        if (client() && client()->clientLeader() != 0) {
            YFrameWindow *o = manager->findFrame(client()->clientLeader());

            if (o != 0) {
                for (YFrameWindow * w (o->transient()); w; w = w->nextTransient())
                    w->doRaise();
            }

            if (client()->ownerWindow() != manager->handle()) {
                for (YFrameWindow * w = manager->bottomLayer(); w; w = w->prevLayer())
                {
                    if (w->client() && w->client()->clientLeader() == client()->clientLeader() && w->client()->ownerWindow() == manager->handle())
                        w->doRaise();
                }
            }
        }

#ifdef DEBUG
        if (debug_z) dumpZorder("wmRaise after raise: ", this);
#endif
    }
}

void YFrameWindow::wmClose() {
    if (!canClose())
        return ;

    XGrabServer(xapp->display());
    client()->getProtocols(true);

    client()->sendPing();
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
}

void YFrameWindow::wmConfirmKill() {
    if (fKillMsgBox == 0)
        fKillMsgBox = wmConfirmKill(_("Kill Client: ") + getTitle(), this);
}

YMsgBox* YFrameWindow::wmConfirmKill(const ustring& title,
        YMsgBoxListener *recvr) {
    YMsgBox *msgbox = new YMsgBox(YMsgBox::mbOK | YMsgBox::mbCancel);
    msgbox->setTitle(title);
    msgbox->setText(_("WARNING! All unsaved changes will be lost when\n"
            "this client is killed. Do you wish to proceed?"));
    msgbox->autoSize();
    msgbox->setMsgBoxListener(recvr);
    msgbox->showFocused();
    return msgbox;
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
    int flags = fwfNext | fwfVisible | fwfCycle |
                fwfFocusable | fwfWorkspace | fwfSame;
    YFrameWindow *f = findWindow(flags | fwfBackward);
    if (f && f != this) {
        f->wmRaise();
        manager->setFocus(f, true);
    }
}

void YFrameWindow::wmNextWindow() {
    int flags = fwfNext | fwfVisible | fwfCycle |
                fwfFocusable | fwfWorkspace | fwfSame;
    YFrameWindow *f = findWindow(flags);
    if (f && f != this) {
        wmLower();
        f->wmRaise();
        manager->setFocus(f, true);
    }
}

void YFrameWindow::loseWinFocus() {
    if (fFocused && fManaged) {
        fFocused = false;

        setState(WinStateFocused, 0);
        if (true || !clientMouseActions)
            if (focusOnClickClient || raiseOnClickClient)
                if (fClientContainer)
                    fClientContainer->grabButtons();
        if (isIconic())
            getMiniIcon()->repaint();
        else {
            repaint();
            if (titlebar())
                titlebar()->deactivate();
        }
        updateTaskBar();
    }
}

void YFrameWindow::setWinFocus() {
    if (!fFocused) {
        fFocused = true;

        setState(WinStateFocused, WinStateFocused);
        if (isIconic())
            getMiniIcon()->repaint();
        else {
            if (titlebar())
                titlebar()->activate();
            repaint();
        }
        updateTaskBar();

        if (true || !clientMouseActions)
            if (focusOnClickClient &&
                !(raiseOnClickClient && (this != manager->top(getActiveLayer()))))
                fClientContainer->releaseButtons();
    }
}

void YFrameWindow::updateFocusOnMap(bool& doActivate) {
    bool onCurrentWorkspace = visibleOn(manager->activeWorkspace());

    if (fDelayFocusTimer) {
        fDelayFocusTimer->stopTimer();
    }
    if (fAutoRaiseTimer) {
        fAutoRaiseTimer->stopTimer();
    }

    if (avoidFocus())
        doActivate = false;

    if (frameOptions() & foNoFocusOnMap)
        doActivate = false;

    if (!onCurrentWorkspace && !focusChangesWorkspace)
        doActivate = false;

    if (owner() != 0) {
        if (owner()->focused() ||
           (nextTransient() && nextTransient()->focused()))
        {
            if (!focusOnMapTransientActive)
                doActivate = false;
        } else {
            if (!focusOnMapTransient)
                doActivate = false;
        }
    } else {
        if (!focusOnMap)
            doActivate = false;
    }

    manager->updateUserTime(fUserTime);
    if (doActivate && fUserTime.good())
        doActivate = (fUserTime.time() && fUserTime == manager->lastUserTime());
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

    manager->unlockFocus();
    manager->setFocus(this, canWarp);
#if 1
        if (raiseOnFocus && /* clickFocus && */
            manager->wmState() == YWindowManager::wmRUNNING)
            wmRaise();
#endif
}

void YFrameWindow::activate(bool canWarp) {
    manager->lockFocus();
    if (fWinState & (WinStateHidden | WinStateMinimized))
        setState(WinStateHidden | WinStateMinimized, 0);
    if (!visibleOn(manager->activeWorkspace()))
        manager->activateWorkspace(getWorkspace());

    manager->unlockFocus();
    focus(canWarp);
}

void YFrameWindow::activateWindow(bool raise) {
    if (raise)
        wmRaise();
    activate(true);
}

MiniIcon *YFrameWindow::getMiniIcon() {
    if (minimizeToDesktop && fMiniIcon == 0)
        fMiniIcon = new MiniIcon(this, this);
    return fMiniIcon;
}

void YFrameWindow::paint(Graphics &g, const YRect &/*r*/) {
    if (g.rdepth() != depth()) {
        tlog("YFrameWindow::%s: attempt to use gc of depth %d on window 0x%lx of depth %d\n",
                __func__, g.rdepth(), handle(), depth());
        return;
    }
    if (!(frameDecors() & (fdResize | fdBorder)))
        return ;

    YColor bg = focused() ? activeBorderBg : inactiveBorderBg;
    g.setColor(bg);

    switch (wmLook) {
    case lookWin95:
    case lookWarp4:
    case lookNice:
        g.fillRect(1, 1, width() - 3, height() - 3);
        g.drawBorderW(0, 0, width() - 1, height() - 1, true);
        break;
    case lookMotif:
    case lookWarp3:
        g.draw3DRect(0, 0, width() - 1, height() - 1, true);
        g.draw3DRect(borderX() - 1, borderY() - 1,
                     width() - 2 * borderX() + 1, height() - 2 * borderY() + 1,
                     false);

        g.fillRect(1, 1, width() - 2, borderY() - 2);
        g.fillRect(1, 1, borderX() - 2, height() - 2);
        g.fillRect(1, (height() - 1) - (borderY() - 2), width() - 2, borderX() - 2);
        g.fillRect((width() - 1) - (borderX() - 2), 1, borderX() - 2, height() - 2);

        if (wmLook == lookMotif && canSize()) {
            g.setColor(bg.darker());
            g.drawLine(wsCornerX - 1, 0, wsCornerX - 1, height() - 1);
            g.drawLine(width() - wsCornerX - 1, 0, width() - wsCornerX - 1, height() - 1);
            g.drawLine(0, wsCornerY - 1, width(),wsCornerY - 1);
            g.drawLine(0, height() - wsCornerY - 1, width(), height() - wsCornerY - 1);
            g.setColor(bg.brighter());
            g.drawLine(wsCornerX, 0, wsCornerX, height() - 1);
            g.drawLine(width() - wsCornerX, 0, width() - wsCornerX, height() - 1);
            g.drawLine(0, wsCornerY, width(), wsCornerY);
            g.drawLine(0, height() - wsCornerY, width(), height() - wsCornerY);
        }
        break;
    case lookPixmap:
    case lookMetal:
    case lookFlat:
    case lookGtk:
        {
            int n = focused() ? 1 : 0;
            int t = (frameDecors() & fdResize) ? 0 : 1;

            if ((frameT[t][n] != null || rgbFrameT[t][n] != null) &&
                (frameL[t][n] != null || rgbFrameL[t][n] != null) &&
                (frameR[t][n] != null || rgbFrameR[t][n] != null) &&
                (frameB[t][n] != null || rgbFrameB[t][n] != null) &&
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

                if ((int) width() > (mxtl + mxtr)) {
                    if (frameT[t][n] != null)
                        g.repHorz(frameT[t][n],
                                  mxtl, 0, width() - mxtl - mxtr);
                    else g.drawGradient(rgbFrameT[t][n],
                        mxtl, 0, width() - mxtl - mxtr, borderY());
                }

                if ((int) height() > (mytl + mybl)) {
                    if (frameL[t][n] != null) g.repVert(frameL[t][n],
                        0, mytl, height() - mytl - mybl);
                    else g.drawGradient(rgbFrameL[t][n],
                        0, mytl, borderX(), height() - mytl - mybl);
                }

                if ((int) height() > (mytr + mybr)) {
                    if (frameR[t][n] != null) g.repVert(frameR[t][n],
                        width() - borderX(), mytr, height() - mytr - mybr);
                    else g.drawGradient(rgbFrameR[t][n],
                        width() - borderX(), mytr,
                        borderX(), height() - mytr - mybr);
                }

                if ((int) width() > (mxbl + mxbr)) {
                    if (frameB[t][n] != null) g.repHorz(frameB[t][n],
                        mxbl, height() - borderY(), width() - mxbl - mxbr);
                    else g.drawGradient(rgbFrameB[t][n],
                        mxbl, height() - borderY(),
                        width() - mxbl - mxbr, borderY());
                }

            } else {
                g.fillRect(1, 1, width() - 3, height() - 3);
                g.drawBorderW(0, 0, width() - 1, height() - 1, true);
            }
        }
        break;
    }
}

void YFrameWindow::handlePopDown(YPopupWindow *popup) {
    MSG(("popdown %p up %p", popup, fPopupActive));
    if (fPopupActive == popup)
        fPopupActive = 0;
}

void YFrameWindow::popupSystemMenu(YWindow *owner) {
    if (fPopupActive == 0) {
        if (titlebar() &&
            titlebar()->visible() &&
            titlebar()->menuButton() &&
            titlebar()->menuButton()->visible())
        {
            titlebar()->menuButton()->popupMenu();
        }
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
        if (windowMenu()->itemCount() == 0)
            return;
        if (windowMenu()->popup(owner, forWindow, this,
                                x, y, flags))
            fPopupActive = windowMenu();
    }
}

void YFrameWindow::updateTitle() {
    if (titlebar())
        titlebar()->repaint();
    layoutShape();
    updateIconTitle();
    if (fWinListItem && windowList->visible())
        windowList->repaintItem(fWinListItem);
    if (fTaskBarApp)
        fTaskBarApp->setToolTip(client()->windowTitle());
    if (fTrayApp)
        fTrayApp->setToolTip(client()->windowTitle());
}

void YFrameWindow::updateIconTitle() {
    if (fTaskBarApp) {
        fTaskBarApp->repaint();
        fTaskBarApp->setToolTip(client()->windowTitle());
    }
    if (fTrayApp)
        fTrayApp->setToolTip(client()->windowTitle());
    if (isIconic()) {
        getMiniIcon()->repaint();
    }
}

void YFrameWindow::wmOccupyAllOrCurrent() {
    if (isAllWorkspaces()) {
        mainOwner()->setWorkspace(manager->activeWorkspace());
    } else {
        setAllWorkspaces();
    }
    if (taskBar)
        taskBar->relayoutTasks();
    if (taskBar)
        taskBar->relayoutTray();
}

void YFrameWindow::wmOccupyAll() {
    if (!isAllWorkspaces())
        setAllWorkspaces();
    if (affectsWorkArea())
        manager->updateWorkArea();
    if (taskBar)
        taskBar->relayoutTasks();
    if (taskBar)
        taskBar->relayoutTray();
}

void YFrameWindow::wmOccupyWorkspace(int workspace) {
    PRECONDITION(workspace < workspaceCount);
    mainOwner()->setWorkspace(workspace);
}

void YFrameWindow::wmOccupyOnlyWorkspace(int workspace) {
    PRECONDITION(workspace < workspaceCount);
    mainOwner()->setWorkspace(workspace);
}

void YFrameWindow::wmMoveToWorkspace(int workspace) {
    wmOccupyOnlyWorkspace(workspace);
}

void YFrameWindow::updateAllowed() {
    Atom atoms[12];
    int i = 0;
    if ((fFrameFunctions & ffMove) || (fFrameDecors & fdTitleBar))
        atoms[i++] = _XA_NET_WM_ACTION_MOVE;
    if ((fFrameFunctions & ffResize) || (fFrameDecors & fdResize))
        atoms[i++] = _XA_NET_WM_ACTION_RESIZE;
    if ((fFrameFunctions & ffClose) || (fFrameDecors & fdClose))
        atoms[i++] = _XA_NET_WM_ACTION_CLOSE;
    if ((fFrameFunctions & ffMinimize) || (fFrameDecors & fdMinimize))
        atoms[i++] = _XA_NET_WM_ACTION_MINIMIZE;
    if ((fFrameFunctions & ffMaximize) || (fFrameDecors & fdMaximize)) {
        atoms[i++] = _XA_NET_WM_ACTION_MAXIMIZE_HORZ;
        atoms[i++] = _XA_NET_WM_ACTION_MAXIMIZE_VERT;
    }
//  if ((fFrameFunctions & ffHide) || (fFrameDecors & fdHide))
//      atoms[i++] = _XA_NET_WM_ACTION_HIDE;
    if ((fFrameFunctions & ffRollup) || (fFrameDecors & fdRollup))
        atoms[i++] = _XA_NET_WM_ACTION_SHADE;
    if ((1) || (fFrameDecors & fdDepth)) {
        atoms[i++] = _XA_NET_WM_ACTION_ABOVE;
        atoms[i++] = _XA_NET_WM_ACTION_BELOW;
    }
    atoms[i++] = _XA_NET_WM_ACTION_STICK;
    atoms[i++] = _XA_NET_WM_ACTION_CHANGE_DESKTOP;
    client()->setNetWMAllowedActions(atoms,i);
}

void YFrameWindow::getFrameHints() {
    long decors = client()->mwmDecors();
    long functions = client()->mwmFunctions();
    long win_hints = client()->winHints();
    MwmHints *mwm_hints = client()->mwmHints();
    int functions_only = (mwm_hints &&
                      (mwm_hints->flags & (MWM_HINTS_FUNCTIONS |
                                           MWM_HINTS_DECORATIONS))
                      == MWM_HINTS_FUNCTIONS);

    unsigned long old_functions = fFrameFunctions;
    unsigned long old_decors = fFrameDecors;
    unsigned long old_options = fFrameOptions;

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
    switch (fWindowType) {
    case wtCombo:
        fFrameFunctions = 0;
        fFrameDecors = 0;
        fFrameOptions |= foIgnoreTaskBar | foIgnoreWinList | foIgnoreQSwitch;
        break;
    case wtDesktop:
        fFrameFunctions = 0;
        fFrameDecors = 0;
        fFrameOptions |= foIgnoreTaskBar | foIgnoreWinList | foIgnoreQSwitch |
                         foNoFocusOnMap;
        break;
    case wtDialog:
        break;
    case wtDND:
        fFrameFunctions = ffMove;
        fFrameDecors = 0;
        fFrameOptions |= foIgnoreTaskBar | foIgnoreWinList | foIgnoreQSwitch |
                         foNoFocusOnMap | foDoNotFocus;
        break;
    case wtDock:
        fFrameFunctions = 0;
        fFrameDecors = 0;
        fFrameOptions |= foIgnoreTaskBar | foIgnoreWinList | foIgnoreQSwitch |
                         foNoFocusOnMap;
        break;
    case wtDropdownMenu:
        fFrameFunctions = 0;
        fFrameDecors = 0;
        fFrameOptions |= foIgnoreTaskBar | foIgnoreWinList | foIgnoreQSwitch;
        break;
    case wtMenu:
        fFrameFunctions &= ~(ffResize | ffMinimize | ffMaximize);
        fFrameDecors = fdTitleBar | fdSysMenu | fdClose | fdRollup;
        fFrameOptions |= foIgnoreTaskBar | foIgnoreWinList | foIgnoreQSwitch;
        break;
    case wtNormal:
        break;
    case wtNotification:
        fFrameFunctions = 0;
        fFrameDecors = 0;
        fFrameOptions |= foIgnoreTaskBar | foIgnoreWinList | foIgnoreQSwitch |
                         foNoFocusOnMap;
        break;
    case wtPopupMenu:
        fFrameFunctions = 0;
        fFrameDecors = 0;
        fFrameOptions |= foIgnoreTaskBar | foIgnoreWinList | foIgnoreQSwitch;
        break;
    case wtSplash:
        fFrameFunctions = 0;
        fFrameDecors = 0;
        fFrameOptions |= foIgnoreTaskBar | foIgnoreWinList | foIgnoreQSwitch |
                         foNoFocusOnMap;
        break;
    case wtToolbar:
        break;
    case wtTooltip:
        fFrameFunctions = 0;
        fFrameDecors = 0;
        fFrameOptions |= foIgnoreTaskBar | foIgnoreWinList | foIgnoreQSwitch |
                         foNoFocusOnMap | foDoNotFocus;
        break;
    case wtUtility:
        break;
    }

    WindowOption wo(null);
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

    if (fFrameFunctions != old_functions ||
        fFrameDecors != old_decors ||
        fFrameOptions != old_options)
    {
        updateAllowed();
    }
}

void YFrameWindow::getWindowOptions(WindowOption &opt, bool remove) {
    if (defOptions) getWindowOptions(defOptions, opt, false);
    if (hintOptions) getWindowOptions(hintOptions, opt, remove);
}

void YFrameWindow::getWindowOptions(WindowOptions *list, WindowOption &opt,
                                    bool remove)
{
    XClassHint const *h(client()->classHint());
    ustring klass = h ? h->res_class : 0;
    ustring name = h ? h->res_name : 0;
    ustring role = client()->windowRole();

    if (klass != null) {
        if (name != null) {
            ustring klass_instance(h->res_class, ".", h->res_name);
            list->mergeWindowOption(opt, klass_instance, remove);

            ustring name_klass(h->res_name, ".", h->res_class);
            list->mergeWindowOption(opt, name_klass, remove);
        }
        list->mergeWindowOption(opt, klass, remove);
    }
    if (name != null) {
        if (role != null) {
            ustring name_role = name.append(".").append(role);
            list->mergeWindowOption(opt, name_role, remove);
        }
        list->mergeWindowOption(opt, name, remove);
    }
    if (role != null)
        list->mergeWindowOption(opt, role, remove);
    list->mergeWindowOption(opt, null, remove);
}

void YFrameWindow::getDefaultOptions(bool &requestFocus) {
    WindowOption wo(null);
    getWindowOptions(wo, true);

    if (wo.icon && wo.icon[0]) {
        fFrameIcon = YIcon::getIcon(wo.icon);
    }
    if (wo.workspace != WinWorkspaceInvalid && wo.workspace < workspaceCount) {
        setWorkspace(wo.workspace);
        if (wo.workspace != manager->activeWorkspace())
            requestFocus = false;
    }
    if (wo.layer != (long)WinLayerInvalid && wo.layer < WinLayerCount)
        setRequestedLayer(wo.layer);
    if (wo.tray != (long)WinTrayInvalid && wo.tray < WinTrayOptionCount)
        setTrayOption(wo.tray);
    fTrayOrder = wo.order;
}

ref<YIcon> newClientIcon(int count, int reclen, long * elem) {
    ref<YImage> small = null;
    ref<YImage> large = null;
    ref<YImage> huge = null;

    if (reclen < 2)
        return null;
    for (int i = 0; i < count; i++, elem += reclen) {
        Pixmap pixmap(elem[0]), mask(elem[1]);

        if (pixmap == None) {
            warn("pixmap == None for subicon #%d", i);
            continue;
        }

        Window root;
        int x, y;
        unsigned w = 0, h = 0;
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
            ref<YPixmap> img = YPixmap::create(w, h, xapp->depth());
            Graphics g(img, 0, 0);

            g.setColorPixel(0xffffff);
            g.fillRect(0, 0, w, h);
            g.setColorPixel(0);
            g.setClipMask(pixmap);
            g.fillRect(0, 0, w, h);

            ref<YImage> img2 =
                YImage::createFromPixmapAndMaskScaled(img->pixmap(), mask,
                                                          img->width(), img->height(),
                                                          w, h);

            if (w <= YIcon::smallSize())
                small = img2;
            else if (w <= YIcon::largeSize())
                large = img2;
            else
                huge = img2;
            img = null;
        }

        if (depth == xapp->depth()) {
            MSG(("client icon color: %ld %d %d %d %d", pixmap, w, h, depth, xapp->depth()));
            if (w <= YIcon::smallSize()) {
                small = YImage::createFromPixmapAndMaskScaled(
                    pixmap, mask, w, h, YIcon::smallSize(), YIcon::smallSize());
            } else if (w <= YIcon::largeSize()) {
                large = YImage::createFromPixmapAndMaskScaled(
                    pixmap, mask, w, h, YIcon::largeSize(), YIcon::largeSize());
            } else if (w <= YIcon::hugeSize() || huge == null || huge->width() < w || huge->height() < h) {
                huge = YImage::createFromPixmapAndMaskScaled(
                    pixmap, mask, w, h, YIcon::hugeSize(), YIcon::hugeSize());
            }
        }
    }

    ref<YIcon> icon;
    if (small != null || large != null || huge != null)
        icon.init(new YIcon(small, large, huge));
    return icon;
}

void YFrameWindow::updateIcon() {
    int count;
    long *elem;
    Pixmap *pixmap;
    Atom type;

/// TODO #warning "think about winoptions specified icon here"

    ref<YIcon> oldFrameIcon = fFrameIcon;

    if (client()->getNetWMIcon(&count, &elem)) {
        ref<YImage> icons[3], largestIcon;
        unsigned sizes[] = { YIcon::smallSize(), YIcon::largeSize(), YIcon::hugeSize()};
        long *largestIconOffset = elem;
        unsigned largestIconSize = 0;

        // Find icons that match Small-/Large-/HugeIconSize and search
        // for the largest icon from NET_WM_ICON set.
        for (long *e = elem;
             e < elem + count && e[0] > 0 && e[1] > 0;
             e += 2 + e[0] * e[1]) {

            if (e + 2 + e[0] * e[1] <= elem + count) {

                if (e[0] > largestIconSize && e[0] == e[1]) {
                    largestIconOffset = e;
                    largestIconSize = e[0];
                }

                // It's possible when huge=large=small, so we must go
                // through all sizes[]
                for (int i = 0; i < 3; i++) {

                    if (e[0] == sizes[i] && e[0] == e[1] && icons[i] == null)
                        icons[i] = YImage::createFromIconProperty(e + 2, e[0], e[1]);
                }
            }
        }

        // create the largest icon
        if (largestIconSize > 0) {
            largestIcon =
                YImage::createFromIconProperty(largestIconOffset + 2,
                                               largestIconSize,
                                               largestIconSize);
        }

        // create the missing icons by downscaling the largest icon
        // Q: Do we need to upscale the largest icon up to missing icon size?
        if (largestIcon != null) {
            for (int i = 0; i < 3; i++) {
                if (icons[i] == null && sizes[i] < largestIconSize)
                    icons[i] = largestIcon->scale(sizes[i], sizes[i]);
            }
        }
        fFrameIcon.init(new YIcon(icons[0], icons[1], icons[2]));
        XFree(elem);
    } else
       if (client()->getWinIcons(&type, &count, &elem)) {
        if (type == _XA_WIN_ICONS)
            fFrameIcon = newClientIcon(elem[0], elem[1], elem + 2);
        else // compatibility
            fFrameIcon = newClientIcon(count/2, 2, elem);
        XFree(elem);
    } else
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

    if (fFrameIcon != null && !(fFrameIcon->small() != null || fFrameIcon->large() != null)) {
        fFrameIcon = null;
    }

    if (fFrameIcon == null) {
        fFrameIcon = oldFrameIcon;
    }

// !!! BAH, we need an internal signaling framework
    if (titlebar() && titlebar()->menuButton())
        titlebar()->menuButton()->repaint();
    if (getMiniIcon()) getMiniIcon()->repaint();
    if (fTrayApp) fTrayApp->repaint();
    if (fTaskBarApp) fTaskBarApp->repaint();
    if (windowList && fWinListItem)
        windowList->getList()->repaintItem(fWinListItem);
}

YFrameWindow *YFrameWindow::nextLayer() {
    if (next()) return next();

    for (long l(getActiveLayer() - 1); l > -1; --l)
        if (manager->top(l)) return manager->top(l);

    return 0;
}

YFrameWindow *YFrameWindow::prevLayer() {
    if (prev()) return prev();

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
            MSG(("transient for 0x%lX: 0x%p", groupLeader, fOwner));
            PRECONDITION(fOwner->transient() != this);

            fNextTransient = fOwner->transient();
            fOwner->setTransient(this);

            if (getActiveLayer() < fOwner->getActiveLayer()) {
                setRequestedLayer(fOwner->getActiveLayer());
            }
            if (fNextTransient != 0)
                setAbove(fNextTransient);
            else
                setAbove(owner());
            setWorkspace(owner()->getWorkspace());
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

bool YFrameWindow::canFocus() {
    if (hasModal())
        return false;

    if (!client())
        return false;

    return true;
}

bool YFrameWindow::canFocusByMouse() {
    return canFocus() && !avoidFocus();
}

bool YFrameWindow::avoidFocus() {
    if (frameOptions() & foDoNotFocus)
        return true;

    if (getInputFocusHint())
        return false;

    if (frameOptions() & foIgnoreNoFocusHint)
        return false;

    if ((client()->protocols() & YFrameClient::wpTakeFocus) ||
        (frameOptions() & foAppTakesFocus))
        return false;

    return true;
}

bool YFrameWindow::getInputFocusHint() {
    XWMHints *hints = fClient->hints();
    bool input = true;

    if (!(frameOptions() & YFrameWindow::foIgnoreNoFocusHint) &&
        (hints && (hints->flags & InputHint) && !hints->input)) {
        input = false;
    }
    if (frameOptions() & foDoNotFocus) {
        input = false;
    }
    return input;
}


void YFrameWindow::setWorkspace(int workspace) {
    if (workspace >= workspaceCount || workspace < -1)
        return ;
    if (workspace != fWinWorkspace) {
        fWinWorkspace = workspace;
        client()->setWinWorkspaceHint(fWinWorkspace);
        updateState();
        manager->focusLastWindow();
        updateTaskBar();
        windowList->updateWindowListApp(fWinListItem);
        YFrameWindow *t = transient();

        while (t != 0) {
            t->setWorkspace(getWorkspace());
            t = t->nextTransient();
        }
    }
}

YFrameWindow *YFrameWindow::mainOwner() {
    YFrameWindow *f = this;
    while (f->owner() != 0)
        f = f->owner();
    return f;
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

    switch (fWindowType) {
    case wtCombo:
        newLayer = WinLayerMenu;
        break;
    case wtDesktop:
        newLayer = WinLayerDesktop;
        break;
    case wtDialog:
        break;
    case wtDND:
        newLayer = WinLayerAboveAll;
        break;
    case wtDock:
        newLayer = WinLayerDock;
        break;
    case wtDropdownMenu:
        newLayer = WinLayerMenu;
        break;
    case wtMenu:
        newLayer = WinLayerMenu;
        break;
    case wtNormal:
        break;
    case wtNotification:
        newLayer = WinLayerAboveDock;
        break;
    case wtPopupMenu:
        newLayer = WinLayerMenu;
        break;
    case wtSplash:
        break;
    case wtToolbar:
        break;
    case wtTooltip:
        newLayer = WinLayerAboveAll;
        break;
    case wtUtility:
        break;
    }
    if (getState() & WinStateBelow)
        newLayer = WinLayerBelow;
    if (getState() & WinStateAbove)
        newLayer = WinLayerOnTop;
    if (fOwner) {
        if (newLayer < fOwner->getActiveLayer())
            newLayer = fOwner->getActiveLayer();
    }
    if (isFullscreen() && manager->fullscreenEnabled() && !canRaise()) {
        YFrameWindow *focus = manager->getFocus();
        while (focus) {
            if (focus == this) {
                newLayer = WinLayerFullscreen;
                break;
            }
            focus = focus->owner();
        }
    }

    if (newLayer != fWinActiveLayer) {
        removeFrame();
        fWinActiveLayer = newLayer;
        insertFrame(true);

        if (client())
            client()->setWinLayerHint(fWinActiveLayer);

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

void YFrameWindow::setTrayOption(long option) {
    if (option >= WinTrayOptionCount || option < 0)
        return ;
    if (option != fWinTrayOption) {
        fWinTrayOption = option;
        client()->setWinTrayHint(fWinTrayOption);
        updateTaskBar();
    }
}

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
        if (fDelayFocusTimer)
            fDelayFocusTimer->disableTimerListener(this);
        if (fAutoRaiseTimer)
            fAutoRaiseTimer->disableTimerListener(this);
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

    if (isFullscreen() || isIconic() || (flagmask & (WinStateFullscreen | WinStateMinimized)))
        horiz = vert = false;

    if (horiz) {
        int cx = mx;

        if (centerMaximizedWindows && !(sh && (sh->flags & PMaxSize)))
            cx = mx + (Mw - nw) / 2;
        else if (!considerHorizBorder)
            cx -= borderXN();
        if (flagmask & WinStateMaximizedHoriz)
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

    if (isFullscreen() || isIconic()) {
        cy = ch = false;
        cx = cw = false;
    }

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

        setWindowGeometry(YRect(iconX, iconY,
                          getMiniIcon()->width(), getMiniIcon()->height()));
    } else {
        if (isFullscreen()) {
            // for _NET_WM_FULLSCREEN_MONITORS
            if (fFullscreenMonitorsTop >= 0 && fFullscreenMonitorsBottom >= 0 &&
                fFullscreenMonitorsLeft >= 0 && fFullscreenMonitorsRight >= 0) {
                int x, y;
                unsigned w, h;
                int monitor[4] = { fFullscreenMonitorsTop, fFullscreenMonitorsBottom,
                                   fFullscreenMonitorsLeft, fFullscreenMonitorsRight };
                manager->getScreenGeometry(&x, &y, &w, &h, monitor[0]);
                YRect r(x, y, w, h);
                for (int i = 1; i < 4; i++) {
                    manager->getScreenGeometry(&x, &y, &w, &h, monitor[i]);
                    r.unionRect(x, y, w, h);
                }
                setWindowGeometry(r);
            } else if (fullscreenUseAllMonitors) {
                setWindowGeometry(YRect(0, 0, manager->width(), manager->height()));
            } else {
                int xiscreen = manager->getScreenForRect(posX,
                                                         posY,
                                                         posW,
                                                         posH);
                int dx, dy;
                unsigned dw, dh;
                manager->getScreenGeometry(&dx, &dy, &dw, &dh, xiscreen);
                setWindowGeometry(YRect(dx, dy, dw, dh));
            }
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
    if (affectsWorkArea())
        manager->updateWorkArea();
    updateExtents();
}

void YFrameWindow::updateExtents() {
    int bX = borderX(), bY = borderY(), tY = titleY();
    int left = bX, right = bX, top = bY + tY, bottom = bY;

    if (extentLeft != left ||
        extentRight != right ||
        extentTop != top ||
        extentBottom != bottom)
    {
        extentLeft = left;
        extentRight = right;
        extentTop = top;
        extentBottom = bottom;
        client()->setNetFrameExtents(left, right, top, bottom);
    }
}

void YFrameWindow::setState(long mask, long state) {
    long fOldState = fWinState;
    long fNewState = (fWinState & ~mask) | (state & mask);

    // !!! this should work
    //if (fNewState == fOldState)
    //    return ;

    if ((fOldState ^ fNewState) & WinStateFullscreen) {
        if ((fNewState & WinStateFullscreen)) {
            // going fullscreen
            client()->saveSizeHints();
        }
        else {
            // going back
            client()->restoreSizeHints();
        }
    }

    // !!! move here

    fWinState = fNewState;

    MSG(("setState: oldState: %lX, newState: %lX, mask: %lX, state: %lX",
         fOldState, fNewState, mask, state));
    //msg("normal1: (%d:%d %dx%d)", normalX, normalY, normalWidth, normalHeight);
    if ((fOldState ^ fNewState) & (WinStateMaximizedVert |
                                   WinStateMaximizedHoriz))
    {
        MSG(("WinStateMaximized: %d", isMaximized()));

        YFrameButton* maximize = titlebar() ? titlebar()->maximizeButton() : 0;
        if (maximize) {
            if (isMaximized()) {
                maximize->setActions(actionRestore, actionRestore);
                maximize->setToolTip(_("Restore"));
            } else {
                maximize->setActions(actionMaximize, actionMaximizeVert);
                maximize->setToolTip(_("Maximize"));
            }
        }
    }
    if ((fOldState ^ fNewState) & WinStateMinimized) {
        MSG(("WinStateMinimized: %d", isMaximized()));
        if (fNewState & WinStateMinimized)
            minimizeTransients();
        else if (owner() && owner()->isMinimized())
            owner()->setState(WinStateMinimized, 0);

        if (minimizeToDesktop && getMiniIcon()) {
            if (isIconic()) {
                getMiniIcon()->raise();
                getMiniIcon()->show();
            } else {
                getMiniIcon()->hide();
                iconX = x();
                iconY = y();
            }
        }
        updateTaskBar();
        layoutResizeIndicators();
        manager->focusLastWindow();
    }
    if ((fOldState ^ fNewState) & WinStateRollup) {
        MSG(("WinStateRollup: %d", isRollup()));
        YFrameButton* rollup = titlebar() ? titlebar()->rollupButton() : 0;
        if (rollup) {
            if (isRollup()) {
                rollup->setToolTip(_("Rolldown"));
            } else {
                rollup->setToolTip(_("Rollup"));
            }
            rollup->repaint();
        }
        layoutResizeIndicators();
    }
    if ((fOldState ^ fNewState) & WinStateHidden) {
        MSG(("WinStateHidden: %d", isHidden()));

        if (fNewState & WinStateHidden)
            hideTransients();
        else if (owner() && owner()->isHidden())
            owner()->setState(WinStateHidden, 0);

        updateTaskBar();
    }
    if ((fOldState ^ fNewState) & WinStateSkipTaskBar) {
        MSG(("WinStateSkipTaskBar: %d", isSkipTaskBar()));
        updateTaskBar();
    }
    if ((fOldState ^ fNewState) & WinStateUrgent) {
        if (fNewState & WinStateUrgent)
            setWmUrgency(true);
        else
            setWmUrgency(false);
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
    if ((fOldState ^ fNewState) & WinStateFullscreen) {
        if ((fNewState & WinStateFullscreen)) {
            activate();
        }
    }
    if ((fOldState ^ fNewState) & WinStateFocused) {
        if ((fNewState & WinStateFocused) &&
             this != manager->getFocus())
            manager->setFocus(this);
    }
}

void YFrameWindow::setAllWorkspaces() {
    setWorkspace(-1);

    windowList->updateWindowListApp(fWinListItem);
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

    getFrameHints();

    int gx, gy;
    client()->gravityOffsets(gx, gy);

    if (!isRollup() && !isIconic()) /// !!! check (emacs hates this)
        configureClient(x() + bx + bx - borderX(),
                        y() + by + by - borderY() + titleY(),
                        client()->width(), client()->height());
}

ref<YIcon> YFrameWindow::clientIcon() const {
    for (YFrameWindow const *f(this); f != NULL; f = f->owner())
        if (f->getClientIcon() != null)
            return f->getClientIcon();

    return YWMApp::getDefaultAppIcon();
}

void YFrameWindow::updateProperties() {
    client()->setWinWorkspaceHint(fWinWorkspace);
    client()->setWinLayerHint(fWinActiveLayer);
    client()->setWinTrayHint(fWinTrayOption);
    client()->setWinStateHint(WIN_STATE_ALL, fWinState);
}

void YFrameWindow::updateTaskBar() {
    bool needTrayApp(false);

    if (taskBar && fManaged) {
        if (!isHidden() &&
            !(frameOptions() & foIgnoreTaskBar) &&
            (getTrayOption() != WinTrayIgnore))
            if (trayShowAllWindows || visibleOn(manager->activeWorkspace()))
                needTrayApp = true;

        if (needTrayApp && fTrayApp == 0)
            fTrayApp = taskBar->addTrayApp(this);

        if (fTrayApp) {
            fTrayApp->setShown(needTrayApp);
            if (fTrayApp->getShown()) ///!!! optimize
                fTrayApp->repaint();
        }
        taskBar->relayoutTray();
    }

    bool needTaskBarApp = true;

    if (taskBar && fManaged) {
        if (isSkipTaskBar())
            needTaskBarApp = false;
        if (isHidden())
            needTaskBarApp = false;
        if (getTrayOption() == WinTrayExclusive)
            needTaskBarApp = false;
        if (getTrayOption() == WinTrayMinimized && isMinimized())
            needTaskBarApp = false;
        if (owner() != 0 && !taskBarShowTransientWindows)
            needTaskBarApp = false;
        if (!visibleOn(manager->activeWorkspace()) && !taskBarShowAllWindows)
            needTaskBarApp = false;
        if (isUrgent())
            needTaskBarApp = true;

        if (frameOptions() & foIgnoreTaskBar)
            needTaskBarApp = false;
        if (frameOptions() & foNoIgnoreTaskBar)
            needTaskBarApp = true;

        if (needTaskBarApp && fTaskBarApp == 0)
            fTaskBarApp = taskBar->addTasksApp(this);

        if (fTaskBarApp) {
            fTaskBarApp->setFlash(isUrgent());
            fTaskBarApp->setShown(needTaskBarApp);
            if (fTaskBarApp->getShown()) ///!!! optimize
                fTaskBarApp->repaint();
        }
#if false
        /// !!! optimize
        if (fTaskBarApp) {
            bool shown = fTaskBarApp->getShown();
            if (shown != fTaskBarApp->getShown())
                if (taskBar && taskBar->taskPane())
                    taskBar->taskPane()->relayout();
        }
#endif
       if (taskBar)
           taskBar->relayoutTasks();
    }
}

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

void YFrameWindow::updateNetWMStrut() {
    int l = fStrutLeft;
    int r = fStrutRight;
    int t = fStrutTop;
    int b = fStrutBottom;
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

void YFrameWindow::updateNetWMStrutPartial() {
    int l = fStrutLeft;
    int r = fStrutRight;
    int t = fStrutTop;
    int b = fStrutBottom;
    client()->getNetWMStrutPartial(&l, &r, &t, &b);
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

void YFrameWindow::updateNetStartupId() {
    unsigned long time = (unsigned long) -1;
    if (client()->getNetStartupId(time)) {
        if (fUserTime.update(time))
            manager->updateUserTime(fUserTime);
    }
}

void YFrameWindow::updateNetWMUserTime() {
    unsigned long time = (unsigned long) -1;
    Window window = fUserTimeWindow ? fUserTimeWindow : client()->handle();
    if (client()->getNetWMUserTime(window, time)) {
        if (fUserTime.update(time))
            manager->updateUserTime(fUserTime);
    }
}

void YFrameWindow::updateNetWMUserTimeWindow() {
    Window window = fUserTimeWindow;
    if (client()->getNetWMUserTimeWindow(window) && window != fUserTimeWindow) {
        if (fUserTimeWindow != None) {
            XDeleteContext(xapp->display(), fUserTimeWindow,
                    windowContext);
        }
        fUserTimeWindow = window;
        if (window != None) {
            XSaveContext(xapp->display(), window,
                    windowContext, (XPointer)client());
            XWindowAttributes wa;
            if (XGetWindowAttributes(xapp->display(), window, &wa))
                XSelectInput(xapp->display(), window,
                             wa.your_event_mask | PropertyChangeMask);
        }
        updateNetWMUserTime();
    }
}

void YFrameWindow::updateNetWMWindowOpacity() {
    long data[1] = { 0, };
    if (client()->getNetWMWindowOpacity(data[0]))
        XChangeProperty(xapp->display(), handle(),
                _XA_NET_WM_WINDOW_OPACITY, XA_CARDINAL,
                32, PropModeReplace,
                (unsigned char *) data, 1);
    else
        XDeleteProperty(xapp->display(), handle(), _XA_NET_WM_WINDOW_OPACITY);
}

void YFrameWindow::updateNetWMFullscreenMonitors(int t, int b, int l, int r) {
    if (t != fFullscreenMonitorsTop ||
        b != fFullscreenMonitorsBottom ||
        l != fFullscreenMonitorsLeft ||
        r != fFullscreenMonitorsRight)
    {
        fFullscreenMonitorsTop = t;
        fFullscreenMonitorsBottom = b;
        fFullscreenMonitorsLeft = l;
        fFullscreenMonitorsRight = r;
        MSG(("fullscreen monitors: %d %d %d %d", t, b, l, r));
        client()->setNetWMFullscreenMonitors(t, b, l, r);
        updateLayout();
    }
}

/* Work around for X11R5 and earlier */
#ifndef XUrgencyHint
#define XUrgencyHint (1 << 8)
#endif

void YFrameWindow::updateUrgency() {
    fClientUrgency = false;
    XWMHints *h = client()->hints();
    if ( !(frameOptions() & foIgnoreUrgent) &&
            h && (h->flags & XUrgencyHint))
        fClientUrgency = true;

    if (isUrgent())
        setState(WinStateUrgent,WinStateUrgent);
    else
        setState(WinStateUrgent,0);

    updateTaskBar();
    /// something else when no taskbar (flash titlebar, flash icon)
}

void YFrameWindow::setWmUrgency(bool wmUrgency) {

    if (!(frameOptions() & foIgnoreUrgent))
    {
        bool change = (wmUrgency != isUrgent());
        fWmUrgency = wmUrgency;
        if (change)
            updateUrgency();
    }
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
           if (!considerVertBorder && (newY + (int) height()) == My)
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
           if (!considerHorizBorder && (newX + (int) width()) == Mx)
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
                 && (i == (w[n]->y() + (int) w[n]->height()))
                 && ( x()            < (w[n]->x() + (int) w[n]->width()))
                 && ((x() + (int) width()) > (w[n]->x())) ) {
                return i;
            }
        }
    }

    return my;
}

int YFrameWindow::getBottomCoord(int My, YFrameWindow **w, int count)
{
    int i, n;

    if ((y() + (int) height()) > My)
        return y() + height();

    for (i = y() + height() + 2; i < My; i++) {
        for (n = 0; n < count; n++) {
            if (    (this != w[n])
                 && (i == w[n]->y())
                 && ( x()            < (w[n]->x() + (int) w[n]->width()))
                 && ((x() + (int) width()) > (w[n]->x())) ) {
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
                 && (i == (w[n]->x() + (int) w[n]->width()))
                 && ( y()             < (w[n]->y() + (int) w[n]->height()))
                 && ((y() + (int) height()) > (w[n]->y())) ) {
                return i;
            }
        }
    }

    return mx;
}

int YFrameWindow::getRightCoord(int Mx, YFrameWindow **w, int count)
{
    int i, n;

    if ((x() + (int) width()) > Mx)
        return x() + width();

    for (i = x() + width() + 2; i < Mx; i++) {
        for (n = 0; n < count; n++) {
            if (    (this != w[n])
                 && (i == w[n]->x())
                 && ( y()             < (w[n]->y() + (int) w[n]->height()))
                 && ((y() + (int) height()) > (w[n]->y())) ) {
                return i;
            }
        }
    }

    return Mx;
}

// vim: set sw=4 ts=4 et:
