/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "wmclient.h"
#include "prefs.h"

#include "yrect.h"
#include "wmframe.h"
#include "wmmgr.h"
#include "wmapp.h"
#include "sysdep.h"
#include "yxcontext.h"
#include "workspaces.h"

bool operator==(const XSizeHints& a, const XSizeHints& b) {
    long mask = PMinSize | PMaxSize | PResizeInc |
                PAspect | PBaseSize | PWinGravity;
    return (a.flags & mask) == (b.flags & mask) &&
        (notbit(a.flags, PMinSize) ||
               (a.min_width == b.min_width && a.min_height == b.min_height)) &&
        (notbit(a.flags, PMaxSize) ||
               (a.max_width == b.max_width && a.max_height == b.max_height)) &&
        (notbit(a.flags, PResizeInc) ||
               (a.width_inc == b.width_inc && a.height_inc == b.height_inc)) &&
        (notbit(a.flags, PAspect) ||
               (a.min_aspect.x == b.min_aspect.x &&
                a.min_aspect.y == b.min_aspect.y &&
                a.max_aspect.x == b.max_aspect.x &&
                a.max_aspect.y == b.max_aspect.y)) &&
        (notbit(a.flags, PBaseSize) ||
               (a.base_width == b.base_width &&
                a.base_height == b.base_height)) &&
        (notbit(a.flags, PWinGravity) || a.win_gravity == b.win_gravity) ;
}
bool operator!=(const XSizeHints& a, const XSizeHints& b) {
    return !(a == b);
}

YFrameClient::YFrameClient(YWindow *parent, YFrameWindow *frame, Window win,
                           int depth, Visual *visual, Colormap colormap):
    YWindow(parent, win, depth, visual, colormap),
    fWindowTitle(),
    fIconTitle(),
    fWindowRole()
{
    fFrame = frame;
    fBorder = 0;
    fProtocols = 0;
    fColormap = colormap;
    fShaped = false;
    fPinging = false;
    fPingTime = 0;
    fHints = nullptr;
    fWinHints = 0;
    fSavedFrameState = InvalidFrameState;
    fSavedWinState[0] = None;
    fSavedWinState[1] = None;
    fSizeHints = XAllocSizeHints();
    fTransientFor = None;
    fClientLeader = None;
    fMwmHints = nullptr;
    fPid = 0;
    prop = {};

    if (win == None) {
        getSizeHints();
    } else {
        getPropertiesList();
        getProtocols(false);
        getNameHint();
        getIconNameHint();
        getNetWmName();
        getNetWmIconName();
        getSizeHints();
        getClassHint();
        getTransient();
        getClientLeader();
        getWMHints();
        getWindowRole();
        getWinHintsHint(&fWinHints);
        getMwmHints();

#ifdef CONFIG_SHAPE
        if (shapes.supported) {
            XShapeSelectInput(xapp->display(), handle(), ShapeNotifyMask);
            queryShape();
        }
#endif
    }

    if (getFrame()) {
        frameContext.save(handle(), getFrame());
    }
    else {
        clientContext.save(handle(), this);
    }
}

YFrameClient::~YFrameClient() {
    if (getFrame()) {
        frameContext.remove(handle());
    }
    else {
        clientContext.remove(handle());
    }

    if (fSizeHints) { XFree(fSizeHints); fSizeHints = nullptr; }
    if (fHints) { XFree(fHints); fHints = nullptr; }
    if (fMwmHints) { XFree(fMwmHints); fMwmHints = nullptr; }
}

void YFrameClient::getProtocols(bool force) {
    if (!prop.wm_protocols && !force)
        return;

    Atom *wmp = nullptr;
    int count = 0;

    fProtocols &= wpDeleteWindow; // always keep WM_DELETE_WINDOW

    if (XGetWMProtocols(xapp->display(), handle(), &wmp, &count) && wmp) {
        prop.wm_protocols = true;
        for (int i = 0; i < count; i++) {
            fProtocols |=
                (wmp[i] == _XA_WM_DELETE_WINDOW) ? wpDeleteWindow :
                (wmp[i] == _XA_WM_TAKE_FOCUS) ? wpTakeFocus :
                (wmp[i] == _XA_NET_WM_PING) ? wpPing :
                0;
        }
        XFree(wmp);
    }
}

void YFrameClient::getSizeHints() {
    if (fSizeHints) {
        long supplied;

        if (!prop.wm_normal_hints ||
            !XGetWMNormalHints(xapp->display(),
                               handle(),
                               fSizeHints, &supplied))
            fSizeHints->flags = 0;

        if (fSizeHints->flags & PResizeInc) {
            if (fSizeHints->width_inc == 0) fSizeHints->width_inc = 1;
            if (fSizeHints->height_inc == 0) fSizeHints->height_inc = 1;
        } else
            fSizeHints->width_inc = fSizeHints->height_inc = 1;


        if (!(fSizeHints->flags & PBaseSize)) {
            if (fSizeHints->flags & PMinSize) {
                fSizeHints->base_width = fSizeHints->min_width;
                fSizeHints->base_height = fSizeHints->min_height;
            } else
                fSizeHints->base_width = fSizeHints->base_height = 0;
        }
        if (!(fSizeHints->flags & PMinSize)) {
            fSizeHints->min_width = fSizeHints->base_width;
            fSizeHints->min_height = fSizeHints->base_height;
        }
        if (!(fSizeHints->flags & PMaxSize)) {
            fSizeHints->max_width = 32767;
            fSizeHints->max_height = 32767;
        }
        if (fSizeHints->max_width < fSizeHints->min_width)
            fSizeHints->max_width = 32767;
        if (fSizeHints->max_height < fSizeHints->min_height)
            fSizeHints->max_height = 32767;

        if (fSizeHints->min_height <= 0)
            fSizeHints->min_height = 1;
        if (fSizeHints->min_width <= 0)
            fSizeHints->min_width = 1;

        if (!(fSizeHints->flags & PWinGravity)) {
            fSizeHints->win_gravity = NorthWestGravity;
            fSizeHints->flags |= PWinGravity;
        }
    }
}

void YFrameClient::getClassHint() {
    if (!prop.wm_class)
        return;

    fClassHint.reset();
    XGetClassHint(xapp->display(), handle(), &fClassHint);
}

void YFrameClient::getTransient() {
    if (!prop.wm_transient_for)
        return;

    Window newTransientFor = None;

    if (XGetTransientForHint(xapp->display(),
                             handle(),
                             &newTransientFor))
    {
        if (newTransientFor == handle())    /* bug in fdesign */
            newTransientFor = None;
    }

    if (newTransientFor != fTransientFor) {
        if (fTransientFor)
            if (getFrame())
                getFrame()->removeAsTransient();
        fTransientFor = newTransientFor;
        if (fTransientFor)
            if (getFrame())
                getFrame()->addAsTransient();
    }
}

void YFrameClient::constrainSize(int &w, int &h, int flags)
{
    if (fSizeHints && (considerSizeHintsMaximized || !getFrame()->isMaximized())) {
        int const wMin(fSizeHints->min_width);
        int const hMin(fSizeHints->min_height);
        int const wMax(fSizeHints->max_width);
        int const hMax(fSizeHints->max_height);
        int const wBase(fSizeHints->base_width);
        int const hBase(fSizeHints->base_height);
        int const wInc(fSizeHints->width_inc);
        int const hInc(fSizeHints->height_inc);

        if (fSizeHints->flags & PAspect) { // aspect ratios
            int const xMin(fSizeHints->min_aspect.x);
            int const yMin(fSizeHints->min_aspect.y);
            int const xMax(fSizeHints->max_aspect.x);
            int const yMax(fSizeHints->max_aspect.y);

            MSG(("aspect"));
            if (flags & csKeepX) {
                MSG(("keepX"));
            }
            if (flags & csKeepY) {
                MSG(("keepY"));
            }

            // !!! fix handling of KeepX and KeepY together
            if (xMin * h > yMin * w) { // min aspect
                if (flags & csKeepX) {
                    w = clamp(w, wMin, wMax);
                    h = w * yMin / non_zero(xMin);
                    h = clamp(h, hMin, hMax);
                    w = h * xMin / non_zero(yMin);
                } else {
                    h = clamp(h, hMin, hMax);
                    w = h * xMin / non_zero(yMin);
                    w = clamp(w, wMin, wMax);
                    h = w * yMin / non_zero(xMin);
                }
            }
            if (xMax * h < yMax * w) { // max aspect
                if (flags & csKeepX) {
                    w = clamp(w, wMin, wMax);
                    h = w * yMax / non_zero(xMax);
                    h = clamp(h, hMin, hMax);
                    w = h * xMax / non_zero(yMax);
                } else {
                    h = clamp(h, hMin, hMax);
                    w = h * xMax / non_zero(yMax);
                    w = clamp(w, wMin, wMax);
                    h = w * yMax / non_zero(xMax);
                }
            }
        }

        h = clamp(h, hMin, hMax);
        w = clamp(w, wMin, wMax);

        if (flags & csRound) {
            w += wInc / 2;
            h += hInc / 2;
        }

        w -= max(0, w - wBase) % non_zero(wInc);
        h -= max(0, h - hBase) % non_zero(hInc);
    }

    if (w <= 0) w = 1;
    if (h <= 0) h = 1;
}

void YFrameClient::gravityOffsets(int &xp, int &yp) {
    xp = 0;
    yp = 0;

    if (fSizeHints == nullptr || notbit(fSizeHints->flags, PWinGravity))
        return;

    static struct {
        int x, y;
    } gravOfsXY[11] = {
        {  0,  0 },  /* ForgetGravity */
        { -1, -1 },  /* NorthWestGravity */
        {  0, -1 },  /* NorthGravity */
        {  1, -1 },  /* NorthEastGravity */
        { -1,  0 },  /* WestGravity */
        {  0,  0 },  /* CenterGravity */
        {  1,  0 },  /* EastGravity */
        { -1,  1 },  /* SouthWestGravity */
        {  0,  1 },  /* SouthGravity */
        {  1,  1 },  /* SouthEastGravity */
        {  0,  0 },  /* StaticGravity */
    };

    int g = fSizeHints->win_gravity;

    if (!(g < ForgetGravity || g > StaticGravity)) {
        xp = (int)gravOfsXY[g].x;
        yp = (int)gravOfsXY[g].y;
    }
}

void YFrameClient::sendMessage(Atom msg, Time timeStamp) {
    XClientMessageEvent xev;

    memset(&xev, 0, sizeof(xev));
    xev.type = ClientMessage;
    xev.window = handle();
    xev.message_type = _XA_WM_PROTOCOLS;
    xev.format = 32;
    xev.data.l[0] = msg;
    xev.data.l[1] = timeStamp;
    XSendEvent(xapp->display(), handle(), False, 0L, (XEvent *) &xev);
}

///extern Time lastEventTime;

bool YFrameClient::sendTakeFocus() {
    if (protocols() & wpTakeFocus) {
        sendMessage(_XA_WM_TAKE_FOCUS, xapp->getEventTime("sendTakeFocus"));
        return true;
    }
    return false;
}

bool YFrameClient::sendDelete() {
    if (protocols() & wpDeleteWindow) {
        sendMessage(_XA_WM_DELETE_WINDOW, xapp->getEventTime("sendDelete"));
        return true;
    }
    return false;
}

bool YFrameClient::sendPing() {
    bool sent = false;
    if (hasbit(protocols(), (unsigned) wpPing) && fPinging == false) {
        XClientMessageEvent xev = {};
        xev.type = ClientMessage;
        xev.window = handle();
        xev.message_type = _XA_WM_PROTOCOLS;
        xev.format = 32;
        xev.data.l[0] = (long) _XA_NET_WM_PING;
        xev.data.l[1] = xapp->getEventTime("sendPing");
        xev.data.l[2] = (long) handle();
        xev.data.l[3] = (long) this;
        xev.data.l[4] = (long) fFrame;
        xapp->send(xev, handle(), NoEventMask);
        fPinging = true;
        fPingTime = xev.data.l[1];
        fPingTimer->setTimer(3000L, this, true);
        sent = true;
    }
    return sent;
}

bool YFrameClient::handleTimer(YTimer* timer) {
    if (timer == nullptr || timer != fPingTimer) {
        return false;
    }

    fPingTimer = null;
    fPinging = false;

    if (destroyed() || getFrame() == nullptr) {
        return false;
    }

    if (killPid() == false && getFrame()->owner()) {
        getFrame()->owner()->client()->killPid();
    }

    return false;
}

bool YFrameClient::killPid() {
    long pid = 0;
    return getNetWMPid(&pid) && 0 < pid && 0 == kill(pid, SIGTERM);
}

bool YFrameClient::getNetWMPid(long *pid) {
    *pid = 0;

    if (!prop.net_wm_pid)
        return false;

    if (fPid > 0) {
        *pid = fPid;
        return true;
    }

    YProperty prop(this, _XA_NET_WM_PID, F32, 1, XA_CARDINAL);
    if (prop) {
        YTextProperty text(nullptr);
        if (XGetWMClientMachine(xapp->display(), handle(), &text)) {
            char myhost[HOST_NAME_MAX + 1] = {};
            gethostname(myhost, HOST_NAME_MAX);
            int len = (int) strnlen(myhost, HOST_NAME_MAX);
            char* theirs = (char *) text.value;
            if (strncmp(myhost, theirs, len) == 0 &&
                (theirs[len] == 0 || theirs[len] == '.'))
            {
                *pid = fPid = *prop;
            }
        }
    }

    return fPid > 0 && fPid == *pid;
}

void YFrameClient::recvPing(const XClientMessageEvent &message) {
    const long* l = message.data.l;
    if (fPinging &&
        l[0] == (long) _XA_NET_WM_PING &&
        l[1] == fPingTime &&
        l[2] == (long) handle() &&
        // l[3] == (long) this &&
        // l[4] == (long) fFrame &&
        l[0] && l[1] && l[2])
    {
        fPinging = false;
        fPingTime = xapp->getEventTime("recvPing");
        fPingTimer = null;
    }
}

void YFrameClient::setFrame(YFrameWindow *newFrame) {
    if (newFrame != getFrame()) {
        if (getFrame()) {
            frameContext.remove(handle());
        }
        else {
            clientContext.remove(handle());
        }

        fFrame = newFrame;
        if (getFrame()) {
            frameContext.save(handle(), getFrame());
        }
        else {
            clientContext.save(handle(), this);
        }
    }
}

void YFrameClient::setFrameState(FrameState state) {
    if (state == WithdrawnState) {
        if (manager->wmState() != YWindowManager::wmSHUTDOWN) {
            MSG(("deleting window properties id=%lX", handle()));
            XDeleteProperty(xapp->display(), handle(), _XA_NET_FRAME_EXTENTS);
            XDeleteProperty(xapp->display(), handle(), _XA_NET_WM_VISIBLE_NAME);
            XDeleteProperty(xapp->display(), handle(), _XA_NET_WM_VISIBLE_ICON_NAME);
            XDeleteProperty(xapp->display(), handle(), _XA_NET_WM_DESKTOP);
            XDeleteProperty(xapp->display(), handle(), _XA_NET_WM_STATE);
            XDeleteProperty(xapp->display(), handle(), _XA_NET_WM_ALLOWED_ACTIONS);
            XDeleteProperty(xapp->display(), handle(), _XA_WIN_WORKSPACE);
            XDeleteProperty(xapp->display(), handle(), _XA_WIN_LAYER);
            XDeleteProperty(xapp->display(), handle(), _XA_WIN_TRAY);
            XDeleteProperty(xapp->display(), handle(), _XA_WIN_STATE);
            XDeleteProperty(xapp->display(), handle(), _XA_WM_STATE);
            fSavedFrameState = InvalidFrameState;
            fSavedWinState[0] = fSavedWinState[1] = 0;
        }
    }
    else if (state != fSavedFrameState) {
        Atom arg[2] = { Atom(state), None };
        setProperty(_XA_WM_STATE, _XA_WM_STATE, arg, 2);
        fSavedFrameState = state;
    }
}

FrameState YFrameClient::getFrameState() {
    if (!prop.wm_state)
        return WithdrawnState;

    fSavedFrameState = InvalidFrameState;

    FrameState st = WithdrawnState;
    YProperty prop(this, _XA_WM_STATE, F32, 2, _XA_WM_STATE);
    if (prop) {
        fSavedFrameState = st = *prop;
    }
    return st;
}

void YFrameClient::handleUnmap(const XUnmapEvent &unmap) {
    YWindow::handleUnmap(unmap);

    MSG(("UnmapWindow"));

    bool unmanage = true;
    bool destroy = false;
    do {
        XEvent ev;
        if (XCheckTypedWindowEvent(xapp->display(), unmap.window,
                                   DestroyNotify, &ev)) {
            YWindow::handleDestroyWindow(ev.xdestroywindow);
            manager->destroyedClient(unmap.window);
            unmanage = false;
        }
        else {
            destroy = (adopted() && destroyed() == false && testDestroyed());
        }
    } while (unmanage && destroy);
    if (unmanage) {
        manager->unmanageClient(this);
    }
}

void YFrameClient::handleProperty(const XPropertyEvent &property) {
    bool new_prop = (property.state != PropertyDelete);

    switch (property.atom) {
    case XA_WM_NAME:
        if (new_prop) prop.wm_name = true;
        getNameHint();
        prop.wm_name = new_prop;
        break;

    case XA_WM_ICON_NAME:
        if (new_prop) prop.wm_icon_name = true;
        getIconNameHint();
        prop.wm_icon_name = false;
        break;

    case XA_WM_CLASS:
        prop.wm_class = new_prop;
        if (prop.wm_class) {
            ClassHint old(fClassHint);
            getClassHint();
            if (fClassHint.nonempty() && fClassHint != old && getFrame()) {
                getFrame()->getFrameHints();
            }
        }
        break;

    case XA_WM_HINTS:
        if (new_prop) prop.wm_hints = true;
        getWMHints();
        if (getFrame()) {
            getFrame()->updateIcon();
            getFrame()->updateUrgency();
        }
        prop.wm_hints = new_prop;
        break;

    case XA_WM_NORMAL_HINTS:
        if (new_prop) prop.wm_normal_hints = true;
        if (fSizeHints) {
            XSizeHints old(*fSizeHints);
            getSizeHints();
            if (old != *fSizeHints) {
                if (getFrame())
                    getFrame()->updateMwmHints();
            }
        }
        prop.wm_normal_hints = new_prop;
        break;

    case XA_WM_TRANSIENT_FOR:
        if (new_prop) prop.wm_transient_for = true;
        getTransient();
        prop.wm_transient_for = new_prop;
        break;
    default:
        if (property.atom == _XA_WM_PROTOCOLS) {
            if (new_prop) prop.wm_protocols = true;
            getProtocols(false);
            prop.wm_protocols = new_prop;
        } else if (property.atom == _XA_WM_STATE) {
            prop.wm_state = new_prop;
        } else if (property.atom == _XA_KWM_WIN_ICON) {
            if (new_prop) prop.kwm_win_icon = true;
            if (getFrame())
                getFrame()->updateIcon();
            prop.kwm_win_icon = new_prop;
        } else if (property.atom == _XA_WIN_ICONS) {
            if (new_prop) prop.win_icons = true;
            if (getFrame())
                getFrame()->updateIcon();
            prop.win_icons = new_prop;
        } else if (property.atom == _XA_NET_WM_NAME) {
            if (new_prop) prop.net_wm_name = true;
            getNetWmName();
            prop.net_wm_name = new_prop;
        } else if (property.atom == _XA_NET_WM_ICON_NAME) {
            if (new_prop) prop.net_wm_icon_name = true;
            getNetWmIconName();
            prop.net_wm_icon_name = new_prop;
        } else if (property.atom == _XA_NET_WM_STRUT) {
            MSG(("change: net wm strut"));
            if (new_prop) prop.net_wm_strut = true;
            if (getFrame())
                getFrame()->updateNetWMStrut();
            prop.net_wm_strut = new_prop;
        } else if (property.atom == _XA_NET_WM_STRUT_PARTIAL) {
            MSG(("change: net wm strut partial"));
            if (new_prop) prop.net_wm_strut_partial = true;
            if (getFrame())
                getFrame()->updateNetWMStrutPartial();
            prop.net_wm_strut_partial = new_prop;
        } else if (property.atom == _XA_NET_WM_USER_TIME) {
            MSG(("change: net wm user time"));
            if (new_prop) prop.net_wm_user_time = true;
            if (getFrame())
                getFrame()->updateNetWMUserTime();
            prop.net_wm_user_time = new_prop;
        } else if (property.atom == _XA_NET_WM_USER_TIME_WINDOW) {
            MSG(("change: net wm user time window"));
            if (new_prop) prop.net_wm_user_time_window = true;
            if (getFrame())
                getFrame()->updateNetWMUserTimeWindow();
            prop.net_wm_user_time_window = new_prop;
        } else if (property.atom == _XA_NET_WM_WINDOW_OPACITY) {
            MSG(("change: net wm window opacity"));
            if (new_prop) prop.net_wm_window_opacity = true;
            if (getFrame())
                getFrame()->updateNetWMWindowOpacity();
        } else if (property.atom == _XA_NET_WM_FULLSCREEN_MONITORS) {
            // ignore - we triggered this event
            // (do i need to set a property here?)
        } else if (property.atom == _XA_NET_WM_ICON) {
            MSG(("change: net wm icon"));
            if (new_prop) prop.net_wm_icon = true;
            if (getFrame())
                getFrame()->updateIcon();
            prop.net_wm_icon = new_prop;
        } else if (property.atom == _XA_WIN_HINTS) {
            long old = fWinHints;
            if (new_prop) prop.win_hints = true;
            getWinHintsHint(&fWinHints);
            if (getFrame()) {
                if (hasbit(old ^ fWinHints,
                            WinHintsSkipFocus |
                            WinHintsSkipWindowMenu |
                            WinHintsSkipTaskBar))
                    getFrame()->getFrameHints();
                if (hasbit(fWinHints, WinHintsDoNotCover))
                    manager->updateWorkArea();
                if (hasbit(old ^ fWinHints, WinHintsSkipTaskBar))
                    getFrame()->updateTaskBar();
            }
            prop.win_hints = new_prop;
        } else if (property.atom == _XA_WIN_WORKSPACE) {
            prop.win_workspace = new_prop;
        } else if (property.atom == _XA_WIN_LAYER) {
            prop.win_layer = new_prop;
        } else if (property.atom == _XA_WIN_STATE) {
            prop.win_state = new_prop;
        } else if (property.atom == _XATOM_MWM_HINTS) {
            if (new_prop) prop.mwm_hints = true;
            getMwmHints();
            if (getFrame())
                getFrame()->updateMwmHints();
            prop.mwm_hints = new_prop;
        } else if (property.atom == _XA_WM_CLIENT_LEADER) { // !!! check these
            if (new_prop) prop.wm_client_leader = true;
            getClientLeader();
            prop.wm_client_leader = new_prop;
        } else if (property.atom == _XA_SM_CLIENT_ID) {
            prop.sm_client_id = new_prop;
        } else if (property.atom == _XA_NET_WM_DESKTOP) {
            prop.net_wm_desktop = new_prop;
        } else if (property.atom == _XA_NET_WM_STATE) {
            prop.net_wm_state = new_prop;
        } else if (property.atom == _XA_NET_WM_WINDOW_TYPE) {
            // !!! do we do dynamic update? (discuss on wm-spec)
            prop.net_wm_window_type = new_prop;
        } else if (property.atom == _XA_NET_WM_PID) {
            prop.net_wm_pid = new_prop;
        } else if (property.atom == _XA_NET_WM_VISIBLE_NAME) {
        } else if (property.atom == _XA_NET_WM_VISIBLE_ICON_NAME) {
        } else if (property.atom == _XA_NET_WM_ALLOWED_ACTIONS) {
        } else if (property.atom == _XA_NET_FRAME_EXTENTS) {
        } else if (property.atom == _XA_WIN_TRAY) {
        } else {
            MSG(("Unknown property changed: %s, window=0x%lX",
                 XGetAtomName(xapp->display(), property.atom), handle()));
        }
        break;
    }
}

void YFrameClient::handleColormap(const XColormapEvent &colormap) {
    if (colormap.colormap == None ||
        colormap.c_new == True ||
        colormap.state == ColormapInstalled)
    {
        setColormap(colormap.colormap);
    }
    // else if (colormap.state == ColormapUninstalled) {}
}


void YFrameClient::handleDestroyWindow(const XDestroyWindowEvent &destroyWindow) {
    //msg("DESTROY: %lX", destroyWindow.window);
    YWindow::handleDestroyWindow(destroyWindow);

    if (destroyed())
        manager->destroyedClient(destroyWindow.window);
}

#ifdef CONFIG_SHAPE
void YFrameClient::handleShapeNotify(const XShapeEvent &shape) {
    if (shapes.supported) {
        MSG(("shape event: %d %d %d:%d=%dx%d time=%ld",
             shape.shaped, shape.kind,
             shape.x, shape.y, shape.width, shape.height, shape.time));
        if (shape.kind == ShapeBounding) {
            bool const newShaped(shape.shaped);
            if (newShaped)
                fShaped = newShaped;
            if (getFrame())
                getFrame()->setShape();
            fShaped = newShaped;
        }
    }
}
#endif

void YFrameClient::setWindowTitle(const char *title) {
    if (fWindowTitle != title) {
        fWindowTitle = title;
        if (title) {
            XChangeProperty(xapp->display(), handle(),
                    _XA_NET_WM_VISIBLE_NAME, _XA_UTF8_STRING,
                    8, PropModeReplace,
                    (const unsigned char *) title,
                    strlen(title));
        } else {
            XDeleteProperty(xapp->display(), handle(),
                    _XA_NET_WM_VISIBLE_NAME);
        }
        if (getFrame())
            getFrame()->updateTitle();
    }
}

void YFrameClient::setIconTitle(const char *title) {
    if (fIconTitle != title) {
        fIconTitle = title;
        if (title) {
            XChangeProperty(xapp->display(), handle(),
                    _XA_NET_WM_VISIBLE_ICON_NAME, _XA_UTF8_STRING,
                    8, PropModeReplace,
                    (const unsigned char *) title,
                    strlen(title));
        } else {
            XDeleteProperty(xapp->display(), handle(),
                    _XA_NET_WM_VISIBLE_ICON_NAME);
        }
        if (getFrame())
            getFrame()->updateIconTitle();
    }
}

void YFrameClient::setColormap(Colormap cmap) {
    fColormap = cmap;
    if (getFrame() && manager->colormapWindow() == getFrame())
        manager->installColormap(cmap);
}

void YFrameClient::queryShape() {
#ifdef CONFIG_SHAPE
    fShaped = false;

    if (shapes.supported) {
        int xws, yws, xbs, ybs;
        unsigned wws, hws, wbs, hbs;
        Bool boundingShaped = False, clipShaped;

        if (XShapeQueryExtents(xapp->display(), handle(),
                               &boundingShaped, &xws, &yws, &wws, &hws,
                               &clipShaped, &xbs, &ybs, &wbs, &hbs))
        {
            fShaped = boundingShaped;
        }
    }
#endif
}

static long getMask(Atom a) {
    return a == _XA_NET_WM_STATE_ABOVE ? WinStateAbove :
           a == _XA_NET_WM_STATE_BELOW ? WinStateBelow :
           a == _XA_NET_WM_STATE_DEMANDS_ATTENTION ? WinStateUrgent :
           a == _XA_NET_WM_STATE_FOCUSED ? WinStateFocused :
           a == _XA_NET_WM_STATE_FULLSCREEN ? WinStateFullscreen :
           a == _XA_NET_WM_STATE_HIDDEN ? WinStateHidden :
           a == _XA_NET_WM_STATE_MAXIMIZED_HORZ ? WinStateMaximizedHoriz :
           a == _XA_NET_WM_STATE_MAXIMIZED_VERT ? WinStateMaximizedVert :
           a == _XA_NET_WM_STATE_MODAL ? WinStateModal :
           a == _XA_NET_WM_STATE_SHADED ? WinStateRollup :
           a == _XA_NET_WM_STATE_SKIP_PAGER ? WinStateSkipPager :
           a == _XA_NET_WM_STATE_SKIP_TASKBAR ? WinStateSkipTaskBar :
           a == _XA_NET_WM_STATE_STICKY ? WinStateSticky :
           None;
}

void YFrameClient::setNetWMFullscreenMonitors(int top, int bottom, int left, int right) {
    long data[4] = { top, bottom, left, right };
    setProperty(_XA_NET_WM_FULLSCREEN_MONITORS, XA_CARDINAL,
                (const Atom *) data, 4);
}

void YFrameClient::setNetFrameExtents(int left, int right, int top, int bottom) {
    long data[4] = { left, right, top, bottom };
    setProperty(_XA_NET_FRAME_EXTENTS, XA_CARDINAL, (const Atom *) data, 4);
}

void YFrameClient::setNetWMAllowedActions(Atom *actions, int count) {
    setProperty(_XA_NET_WM_ALLOWED_ACTIONS, XA_ATOM, actions, count);
}

void YFrameClient::handleClientMessage(const XClientMessageEvent &message) {
    if (message.message_type == _XA_WM_CHANGE_STATE) {
        const long state = message.data.l[0];
        YFrameWindow* frame = getFrame();
        if (state == IconicState && frame &&
            !frame->hasState(WinStateMinimized | WinStateRollup))
        {
            frame->actionPerformed(actionMinimize, None);
        }
        else if (state == NormalState && frame &&
            frame->hasState(WinStateMinimized | WinStateRollup | WinStateHidden))
        {
            frame->actionPerformed(actionRestore, None);
        }
        else if (state == WithdrawnState && frame &&
            !frame->hasState(WinStateHidden))
        {
            frame->actionPerformed(actionHide, None);
        }
    } else if (message.message_type == _XA_NET_RESTACK_WINDOW) {
        if (getFrame()) {
            XConfigureRequestEvent cre;
            cre.type = ConfigureRequest;
            cre.value_mask = CWStackMode;
            cre.above = message.data.l[1];
            if (cre.above != None)
                cre.value_mask |= CWSibling;
            cre.detail = message.data.l[2];
            getFrame()->configureClient(cre);
        }
    } else if (message.message_type == _XA_NET_ACTIVE_WINDOW) {
        //printf("active window w=0x%lX\n", message.window);
        YFrameWindow* f = getFrame();
        if (f && !f->frameOption(YFrameWindow::foIgnoreActivationMessages)) {
            f->activate();
            f->wmRaise();
        }
    } else if (message.message_type == _XA_NET_CLOSE_WINDOW) {
        //printf("close window w=0x%lX\n", message.window);
        if (getFrame())
            getFrame()->wmClose();
    } else if (message.message_type == _XA_NET_WM_MOVERESIZE &&
               message.format == 32)
    {
        ///YFrameWindow *frame(findFrame(message.window));
        if (getFrame())
            getFrame()->startMoveSize(message.data.l[0], message.data.l[1],
                                      message.data.l[2]);
    } else if (message.message_type == _XA_NET_MOVERESIZE_WINDOW) {
        if (getFrame()) {
            long flag = message.data.l[0];
            long grav = Elvis(int(flag & 0xFF), sizeHints()->win_gravity);
            long mask = ((flag >> 8) & 0x0F);
            long xpos = (mask & 1) ? message.data.l[1] : x();
            long ypos = (mask & 2) ? message.data.l[2] : y();
            long hori = (mask & 4) ? message.data.l[3] : width();
            long vert = (mask & 8) ? message.data.l[4] : height();
            XConfigureRequestEvent conf = { ConfigureRequest, None, };
            conf.value_mask = mask;
            conf.x = xpos;
            conf.y = ypos;
            conf.width = hori;
            conf.height = vert;
            int wing = sizeHints()->win_gravity;
            sizeHints()->win_gravity = grav;
            getFrame()->configureClient(conf);
            sizeHints()->win_gravity = wing;
        }
    } else if (message.message_type == _XA_NET_WM_FULLSCREEN_MONITORS) {
        if (getFrame()) {
            const long* l = message.data.l;
            getFrame()->updateNetWMFullscreenMonitors(l[0], l[1], l[2], l[3]);
        }
    } else if (message.message_type == _XA_NET_WM_STATE) {
        long mask = (getMask(message.data.l[1]) | getMask(message.data.l[2]))
                  & ~(WinStateFocused | WinStateHidden);
        if (mask && getFrame() && inrange(message.data.l[0], 0L, 2L)) {
            long have = (getFrame()->getState() & mask);
            long want = (message.data.l[0] == _NET_WM_STATE_ADD) ? mask :
                        (message.data.l[0] == _NET_WM_STATE_REMOVE) ? 0 :
                        (have ^ mask);
            if (have != want) {
                getFrame()->setState(mask, want);
            }
        }
    } else if (message.message_type == _XA_WM_PROTOCOLS &&
               message.data.l[0] == long(_XA_NET_WM_PING)) {
        recvPing(message);
    } else if (message.message_type == _XA_WIN_WORKSPACE) {
        if (getFrame())
            getFrame()->setWorkspace(message.data.l[0]);
        else
            setWinWorkspaceHint(message.data.l[0]);
    } else if (message.message_type == _XA_NET_WM_DESKTOP) {
        if (getFrame())
            getFrame()->setWorkspace(message.data.l[0]);
        else
            setWinWorkspaceHint(message.data.l[0]);
    } else if (message.message_type == _XA_WIN_LAYER) {
        if (getFrame())
            getFrame()->setRequestedLayer(message.data.l[0]);
        else
            setWinLayerHint(message.data.l[0]);
    } else if (message.message_type == _XA_WIN_TRAY) {
        if (getFrame())
            getFrame()->setTrayOption(message.data.l[0]);
        else
            setWinTrayHint(message.data.l[0]);
    } else if (message.message_type == _XA_WIN_STATE) {
        if (getFrame())
            getFrame()->setState(message.data.l[0], message.data.l[1]);
        else
            setWinStateHint(message.data.l[0], message.data.l[1]);
    } else
        YWindow::handleClientMessage(message);
}

void YFrameClient::getNameHint() {
    if (!prop.wm_name)
        return;
    if (prop.net_wm_name)
        return;

    XTextProperty text = { nullptr, None, 0, 0 };
    XGetWMName(xapp->display(), handle(), &text);
    setWindowTitle((char *)text.value);
    XFree(text.value);
}

void YFrameClient::getNetWmName() {
    if (!prop.net_wm_name)
        return;

    XTextProperty text = { nullptr, None, 0, 0 };
    XGetTextProperty(xapp->display(), handle(), &text, _XA_NET_WM_NAME);
    setWindowTitle((char *)text.value);
    XFree(text.value);
}

void YFrameClient::getIconNameHint() {
    if (!prop.wm_icon_name)
        return;
    if (prop.net_wm_icon_name)
        return;

    XTextProperty text = { nullptr, None, 0, 0 };
    XGetWMIconName(xapp->display(), handle(), &text);
    setIconTitle((char *)text.value);
    XFree(text.value);
}

void YFrameClient::getNetWmIconName() {
    if (!prop.net_wm_icon_name)
        return;

    XTextProperty text = { nullptr, None, 0, 0 };
    XGetTextProperty(xapp->display(), handle(), &text, _XA_NET_WM_ICON_NAME);
    setIconTitle((char *)text.value);
    XFree(text.value);
}

void YFrameClient::getWMHints() {
    if (!prop.wm_hints)
        return;

    if (fHints)
        XFree(fHints);
    fHints = XGetWMHints(xapp->display(), handle());
}

void YFrameClient::getMwmHints() {
    if (!prop.mwm_hints)
        return;

    if (fMwmHints) {
        XFree(fMwmHints);
        fMwmHints = nullptr;
    }
    YProperty prop(this, _XATOM_MWM_HINTS, F32,
                   PROP_MWM_HINTS_ELEMENTS, _XATOM_MWM_HINTS);
    if (prop && prop.size() == PROP_MWM_HINTS_ELEMENTS)
        fMwmHints = prop.retrieve<MwmHints>();
}

void YFrameClient::setMwmHints(const MwmHints &mwm) {
    setProperty(_XATOM_MWM_HINTS, _XATOM_MWM_HINTS,
                (const Atom *)&mwm, PROP_MWM_HINTS_ELEMENTS);
    if (fMwmHints == nullptr)
        fMwmHints = (MwmHints *)malloc(sizeof(MwmHints));
    if (fMwmHints)
        *fMwmHints = mwm;
}

void YFrameClient::saveSizeHints()
{
    memcpy(&savedSizeHints, fSizeHints, sizeof(XSizeHints));
}
void YFrameClient::restoreSizeHints() {
    memcpy(fSizeHints, &savedSizeHints, sizeof(XSizeHints));
}


long YFrameClient::mwmFunctions() {
    long functions = ~0U;

    if (fMwmHints && (fMwmHints->flags & MWM_HINTS_FUNCTIONS)) {
        if (fMwmHints->functions & MWM_FUNC_ALL)
            functions = ~fMwmHints->functions;
        else
            functions = fMwmHints->functions;
    } else {
        XSizeHints *sh = sizeHints();

        if (sh) {
            bool minmax = false;
            if (sh->min_width == sh->max_width &&
                sh->min_height == sh->max_height)
            {
                functions &= ~MWM_FUNC_RESIZE;
                minmax = true;
            }
            if ((minmax && !(sh->flags & PResizeInc)) ||
                (sh->width_inc == 0 && sh->height_inc == 0))
                functions &= ~MWM_FUNC_MAXIMIZE;
        }
    }
    functions &= (MWM_FUNC_RESIZE | MWM_FUNC_MOVE |
                  MWM_FUNC_MINIMIZE | MWM_FUNC_MAXIMIZE |
                  MWM_FUNC_CLOSE);
    return functions;
}

long YFrameClient::mwmDecors() {
    long decors = ~0U;
    long func = mwmFunctions();

    if (fMwmHints && (fMwmHints->flags & MWM_HINTS_DECORATIONS)) {
        if (fMwmHints->decorations & MWM_DECOR_ALL)
            decors = ~fMwmHints->decorations;
        else
            decors = fMwmHints->decorations;
    } else {
        XSizeHints *sh = sizeHints();

        if (sh) {
            bool minmax = false;
            if (sh->min_width == sh->max_width &&
                sh->min_height == sh->max_height)
            {
                decors &= ~MWM_DECOR_RESIZEH;
                minmax = true;
            }
            if ((minmax && !(sh->flags & PResizeInc)) ||
                (sh->width_inc == 0 && sh->height_inc == 0))
                decors &= ~MWM_DECOR_MAXIMIZE;
        }
    }
    decors &= (MWM_DECOR_BORDER | MWM_DECOR_RESIZEH |
               MWM_DECOR_TITLE | MWM_DECOR_MENU |
               MWM_DECOR_MINIMIZE | MWM_DECOR_MAXIMIZE);

    /// !!! add disabled buttons
    decors &=
        ~(/*((func & MWM_FUNC_RESIZE) ? 0 : MWM_DECOR_RESIZEH) |*/
          ((func & MWM_FUNC_MINIMIZE) ? 0 : MWM_DECOR_MINIMIZE) |
          ((func & MWM_FUNC_MAXIMIZE) ? 0 : MWM_DECOR_MAXIMIZE));

    return decors;
}

bool YFrameClient::getKwmIcon(long* count, Pixmap** pixmap) {
    *count = 0;
    *pixmap = None;

    if (!prop.kwm_win_icon)
        return false;

    YProperty prop(this, _XA_KWM_WIN_ICON, F32, 2, _XA_KWM_WIN_ICON);
    if (prop && prop.size() == 2) {
        *count = prop.size();
        *pixmap = prop.retrieve<Pixmap>();
        getWMHints();
        return true;
    }
    return false;
}

bool YFrameClient::getWinIcons(Atom* type, long* count, long** elem) {
    *type = None;
    *count = 0;
    *elem = nullptr;

    if (!prop.win_icons)
        return false;

    YProperty prop(this, _XA_WIN_ICONS, F32, 4096);
    if (prop.typed(_XA_WIN_ICONS) || prop.typed(XA_PIXMAP)) {
        *type = prop.type();
        *count = prop.size();
        *elem = prop.retrieve<long>();
        return true;
    }
    return false;
}

bool YFrameClient::getNetWMIcon(long* count, long** elems) {
    *count = 0;
    *elems = nullptr;
    if (prop.net_wm_icon) {
        YProperty prop(this, _XA_NET_WM_ICON, F32, 1L << 22);
        if (prop) {
            if (prop.typed(XA_CARDINAL)) {
                *count = prop.size();
                *elems = prop.retrieve<long>();
            }
            else if (testOnce("_NET_WM_ICON", int(handle()))) {
                TLOG(("Bad _NET_WM_ICON for window 0x%lx: N=%ld, F=%d, T=%s",
                     handle(), prop.size(), F32,
                     XGetAtomName(xapp->display(), prop.type())));
            }
        }
    }
    return (*elems != nullptr);
}

void YFrameClient::setWinWorkspaceHint(long wk) {
    setProperty(_XA_WIN_WORKSPACE, XA_CARDINAL, wk);
    setProperty(_XA_NET_WM_DESKTOP, XA_CARDINAL, wk);
}

bool YFrameClient::getWinWorkspaceHint(long *workspace) {
    *workspace = 0;

    if (!prop.win_workspace)
        return false;

    YProperty prop(this, _XA_WIN_WORKSPACE, F32, 1, XA_CARDINAL);
    if (prop && inrange<int>(int(*prop) + 1, 0, workspaceCount)) {
        *workspace = int(*prop);
        return true;
    }
    return false;
}

void YFrameClient::setWinLayerHint(long layer) {
    setProperty(_XA_WIN_LAYER, XA_CARDINAL, layer);
}

bool YFrameClient::getWinLayerHint(long *layer) {
    YProperty prop(this, _XA_WIN_LAYER, F32, 1, XA_CARDINAL);
    if (prop && inrange(*prop, 0L, WinLayerCount - 1L)) {
        *layer = *prop;
        return true;
    }
    return false;
}

bool YFrameClient::getWinTrayHint(long *tray_opt) {
    YProperty prop(this, _XA_WIN_TRAY, F32, 1, XA_CARDINAL);
    if (prop && *prop < WinTrayOptionCount) {
        *tray_opt = *prop;
        return true;
    }
    return false;
}

void YFrameClient::setWinTrayHint(long tray_opt) {
    setProperty(_XA_WIN_TRAY, XA_CARDINAL, tray_opt);
}

bool YFrameClient::getWinStateHint(long *mask, long *state) {
    YProperty prop(this, _XA_WIN_STATE, F32, 2, XA_CARDINAL);
    if (prop) {
        MSG(("got state"));
        *mask = (prop.size() == 2) ? prop[1] : WIN_STATE_ALL;
        *state = (prop[0] & *mask);
        return true;
    }
    return false;
}

void YFrameClient::setWinStateHint(long mask, long state) {
    MSG(("set state=%8lX mask=%3lX, saved %8lX, %3lX, %p",
          state, mask, fSavedWinState[0], fSavedWinState[1], this));

    if (destroyed())
        return;

    if (hasbit(mask, state ^ fSavedWinState[0]) || mask != fSavedWinState[1]) {
        long prop[2] = { state & mask, mask };
        setProperty(_XA_WIN_STATE, XA_CARDINAL, (const Atom *) prop, 2);
    }
    else if (state == fSavedWinState[0]) {
        return;
    }

/// TODO #warning "hack"
    // !!! hack
    Atom a[15];
    int i = 0;

    /* the next one is kinda messy */
    if ((state & WinStateMinimized) || (state & WinStateHidden))
        a[i++] = _XA_NET_WM_STATE_HIDDEN;
    else if ((state & WinStateFocused) && !(state & WinStateRollup))
        a[i++] = _XA_NET_WM_STATE_FOCUSED;
    if (state & WinStateSkipPager)
        a[i++] = _XA_NET_WM_STATE_SKIP_PAGER;
    if (state & WinStateSkipTaskBar)
        a[i++] = _XA_NET_WM_STATE_SKIP_TASKBAR;
    if (state & WinStateSticky)
        a[i++] = _XA_NET_WM_STATE_STICKY;

    if (state & WinStateRollup)
        a[i++] = _XA_NET_WM_STATE_SHADED;
    if (state & WinStateAbove)
        a[i++] = _XA_NET_WM_STATE_ABOVE;
    if (state & WinStateBelow)
        a[i++] = _XA_NET_WM_STATE_BELOW;
    if (state & WinStateModal)
        a[i++] = _XA_NET_WM_STATE_MODAL;
    if (state & WinStateFullscreen)
        a[i++] = _XA_NET_WM_STATE_FULLSCREEN;
    if (state & WinStateMaximizedVert)
        a[i++] = _XA_NET_WM_STATE_MAXIMIZED_VERT;
    if (state & WinStateMaximizedHoriz)
        a[i++] = _XA_NET_WM_STATE_MAXIMIZED_HORZ;
    if (state & WinStateUrgent)
        a[i++] = _XA_NET_WM_STATE_DEMANDS_ATTENTION;

    setProperty(_XA_NET_WM_STATE, XA_ATOM, a, i);

    fSavedWinState[0] = state;
    fSavedWinState[1] = mask;
}

bool YFrameClient::getNetWMStateHint(long *mask, long *state) {
    long flags = None;
    YProperty prop(this, _XA_NET_WM_STATE, F32, 32, XA_ATOM);
    for (int i = 0; i < int(prop.size()); ++i) {
        flags |= getMask(prop[i]);
    }
    if (hasbit(flags, WinStateHidden)) {
        flags = (flags & ~WinStateHidden) | WinStateMinimized;
    }
    if (manager->wmState() != YWindowManager::wmSTARTUP) {
        flags &= ~WinStateFocused;
    }
    *mask = flags;
    *state = flags;
    return prop.typed(XA_ATOM);
}

bool YFrameClient::getWinHintsHint(long *state) {
    *state = 0;
    if (!prop.win_hints)
        return false;

    YProperty prop(this, _XA_WIN_HINTS, F32, 1, XA_CARDINAL);
    if (prop) {
        MSG(("got hints"));
        *state = *prop;
        return true;
    }
    return false;
}

void YFrameClient::setWinHintsHint(long hints) {
    fWinHints = hints;
    setProperty(_XA_WIN_HINTS, XA_CARDINAL, hints);
}

void YFrameClient::getClientLeader() {
    Window leader = None;
    if (prop.wm_client_leader) {
        YProperty prop(this, _XA_WM_CLIENT_LEADER, F32, 1, XA_WINDOW);
        if (prop)
            leader = *prop;
    }
    fClientLeader = leader;
}

void YFrameClient::getWindowRole() {
    if (!prop.wm_window_role && !prop.window_role)
        return;

    Atom atom = prop.wm_window_role ? _XA_WM_WINDOW_ROLE : _XA_WINDOW_ROLE;
    fWindowRole = YProperty(this, atom, F8, 256, XA_STRING).data<char>();
}

mstring YFrameClient::getClientId(Window leader) { /// !!! fix

    if (!prop.sm_client_id)
        return null;

    return YProperty(leader, _XA_SM_CLIENT_ID, F8, 256, XA_STRING).data<char>();
}

bool YFrameClient::getNetWMStrut(int *left, int *right, int *top, int *bottom) {

    if (prop.net_wm_strut_partial)
        return false;

    *left = 0;
    *right = 0;
    *top = 0;
    *bottom = 0;

    if (!prop.net_wm_strut)
        return false;

    YProperty prop(this, _XA_NET_WM_STRUT, F32, 4, XA_CARDINAL);
    if (prop && prop.size() == 4) {
        *left = prop[0];
        *right = prop[1];
        *top = prop[2];
        *bottom = prop[3];
        MSG(("got strut %d, %d, %d, %d", *left, *right, *top, *bottom));
        return true;
    }
    return false;
}

bool YFrameClient::getNetWMStrutPartial(int *left, int *right, int *top, int *bottom)
{
    *left   = 0;
    *right  = 0;
    *top    = 0;
    *bottom = 0;

    if (!prop.net_wm_strut_partial)
        return false;

    YProperty prop(this, _XA_NET_WM_STRUT_PARTIAL, F32, 12, XA_CARDINAL);
    if (prop && prop.size() == 12) {
        *left = prop[0];
        *right = prop[1];
        *top = prop[2];
        *bottom = prop[3];
        MSG(("strut partial %d, %d, %d, %d", *left, *right, *top, *bottom));
        return true;
    }
    return false;
}

bool YFrameClient::getNetStartupId(unsigned long &time) {
    if (!prop.net_startup_id)
        return false;

    YTextProperty id(nullptr);
    if (XGetTextProperty(xapp->display(), handle(), &id, _XA_NET_STARTUP_ID)) {
        char* str = strstr((char *)id.value, "_TIME");
        if (str) {
            time = atol(str + 5) & 0xffffffff;
            if (time == -1UL)
                time = -2UL;
            return true;
        }
    }
    return false;
}

bool YFrameClient::getNetWMUserTime(Window window, unsigned long &time) {
    if (!prop.net_wm_user_time && !prop.net_wm_user_time_window)
        return false;

    YProperty prop(this, _XA_NET_WM_USER_TIME, F32, 1, XA_CARDINAL);
    if (prop) {
        MSG(("got user time"));
        time = *prop & 0xffffffff;
        if (time == -1UL)
            time = -2UL;
        return true;
    }
    return false;
}


bool YFrameClient::getNetWMUserTimeWindow(Window &window) {
    if (!prop.net_wm_user_time_window)
        return false;

    YProperty prop(this, _XA_NET_WM_USER_TIME_WINDOW, F32, 1, XA_WINDOW);
    if (prop) {
        MSG(("got user time window"));
        window = *prop;
        return true;
    }
    return false;
}

bool YFrameClient::getNetWMWindowOpacity(long &opacity) {
    if (!prop.net_wm_window_opacity)
        return false;

    YProperty prop(this, _XA_NET_WM_WINDOW_OPACITY, F32, 1, XA_CARDINAL);
    if (prop) {
        MSG(("got window opacity"));
        opacity = *prop;
        return true;
    }
    return false;
}

bool YFrameClient::getNetWMWindowType(Atom *window_type) { // !!! for now, map to layers
    *window_type = None;

    if (!prop.net_wm_window_type)
        return false;

    YProperty prop(this, _XA_NET_WM_WINDOW_TYPE, F32, 16);
    if (prop) {
        Atom types[] = {
            _XA_NET_WM_WINDOW_TYPE_COMBO,
            _XA_NET_WM_WINDOW_TYPE_DESKTOP,
            _XA_NET_WM_WINDOW_TYPE_DIALOG,
            _XA_NET_WM_WINDOW_TYPE_DND,
            _XA_NET_WM_WINDOW_TYPE_DOCK,
            _XA_NET_WM_WINDOW_TYPE_DROPDOWN_MENU,
            _XA_NET_WM_WINDOW_TYPE_MENU,
            _XA_NET_WM_WINDOW_TYPE_NORMAL,
            _XA_NET_WM_WINDOW_TYPE_NOTIFICATION,
            _XA_NET_WM_WINDOW_TYPE_POPUP_MENU,
            _XA_NET_WM_WINDOW_TYPE_SPLASH,
            _XA_NET_WM_WINDOW_TYPE_TOOLBAR,
            _XA_NET_WM_WINDOW_TYPE_TOOLTIP,
            _XA_NET_WM_WINDOW_TYPE_UTILITY,
        };
        for (int p = 0; p < int(prop.size()); ++p) {
            for (int t = 0; t < int ACOUNT(types); ++t) {
                if (Atom(prop[p]) == types[t]) {
                    *window_type = types[t];
                    break;
                }
            }
        }
    }
    return *window_type;
}

bool YFrameClient::getNetWMDesktopHint(long *workspace) {
    *workspace = 0;

    if (!prop.net_wm_desktop)
        return false;

    YProperty prop(this, _XA_NET_WM_DESKTOP, F32, 1, XA_CARDINAL);
    if (prop) {
        if (inrange<int>(int(*prop) + 1, 0, workspaceCount)) {
            *workspace = *prop;
            return true;
        }
    }
    return false;
}

void YFrameClient::getPropertiesList() {
    int count;
    Atom *p;

    memset(&prop, 0, sizeof(prop));

    p = XListProperties(xapp->display(), handle(), &count);

#define HAS(x)   ((x) = true)

    if (p) {
        for (int i = 0; i < count; i++) {
            Atom a = p[i];

            if      (a == XA_WM_HINTS) HAS(prop.wm_hints);
            else if (a == XA_WM_NORMAL_HINTS) HAS(prop.wm_normal_hints);
            else if (a == XA_WM_TRANSIENT_FOR) HAS(prop.wm_transient_for);
            else if (a == XA_WM_NAME) HAS(prop.wm_name);
            else if (a == _XA_NET_WM_NAME) HAS(prop.net_wm_name);
            else if (a == XA_WM_ICON_NAME) HAS(prop.wm_icon_name);
            else if (a == _XA_NET_WM_ICON_NAME) HAS(prop.net_wm_icon_name);
            else if (a == _XA_NET_WM_ICON) HAS(prop.net_wm_icon);
            else if (a == XA_WM_CLASS) HAS(prop.wm_class);
            else if (a == _XA_WM_PROTOCOLS) HAS(prop.wm_protocols);
            else if (a == _XA_WM_CLIENT_LEADER) HAS(prop.wm_client_leader);
            else if (a == _XA_WM_WINDOW_ROLE) HAS(prop.wm_window_role);
            else if (a == _XA_WINDOW_ROLE) HAS(prop.window_role);
            else if (a == _XA_SM_CLIENT_ID) HAS(prop.sm_client_id);
            else if (a == _XATOM_MWM_HINTS) HAS(prop.mwm_hints);
            else if (a == _XA_KWM_WIN_ICON) HAS(prop.kwm_win_icon);
            else if (a == _XA_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR) HAS(prop.kde_net_wm_system_tray_window_for);
            else if (a == _XA_NET_WM_STRUT) HAS(prop.net_wm_strut);
            else if (a == _XA_NET_WM_STRUT_PARTIAL) HAS(prop.net_wm_strut_partial);
            else if (a == _XA_NET_WM_DESKTOP) HAS(prop.net_wm_desktop);
            else if (a == _XA_NET_WM_PID) HAS(prop.net_wm_pid);
            else if (a == _XA_NET_WM_STATE) HAS(prop.net_wm_state);
            else if (a == _XA_NET_WM_WINDOW_TYPE) HAS(prop.net_wm_window_type);
            else if (a == _XA_NET_STARTUP_ID) HAS(prop.net_startup_id);
            else if (a == _XA_NET_WM_USER_TIME) HAS(prop.net_wm_user_time);
            else if (a == _XA_NET_WM_USER_TIME_WINDOW) HAS(prop.net_wm_user_time_window);
            else if (a == _XA_NET_WM_WINDOW_OPACITY) HAS(prop.net_wm_window_opacity);
            else if (a == _XA_WIN_HINTS) HAS(prop.win_hints);
            else if (a == _XA_WIN_WORKSPACE) HAS(prop.win_workspace);
            else if (a == _XA_WIN_STATE) HAS(prop.win_state);
            else if (a == _XA_WIN_LAYER) HAS(prop.win_layer);
            else if (a == _XA_WIN_ICONS) HAS(prop.win_icons);
            else if (a == _XA_XEMBED_INFO) HAS(prop.xembed_info);
#ifdef DEBUG
            else {
                MSG(("unknown atom: %s", XGetAtomName(xapp->display(), a)));
            }
#endif
#undef HAS
        }
        XFree(p);
    }
}

void YFrameClient::handleGravityNotify(const XGravityEvent &gravity) {
    int ox = x(), oy = y();
    YWindow::handleGravityNotify(gravity);
    if ((gravity.x < 0 || gravity.y < 0) && ox >= 0 && oy >= 0) {
        int nx = max(0, x()), ny = max(0, y());
        MSG(("gravity notify %+d%+d -> %+d%+d -> %+d%+d",
                    ox, oy, gravity.x, gravity.y, nx, ny));
        XMoveWindow(xapp->display(), handle(), nx, ny);
    }
}

bool ClassHint::match(const char* resource) const {
    if (isEmpty(resource))
        return false;
    if (*resource != '.') {
        if (isEmpty(res_name))
            return false;
        size_t len(strlen(res_name));
        if (strncmp(res_name, resource, len))
            return false;
        if (resource[len] == 0)
            return true;
        if (resource[len] != '.')
            return false;
        resource += len;
    }
    return 0 == strcmp(1 + resource, res_class ? res_class : "");
}

char* ClassHint::resource() const {
    mstring str(res_name, ".", res_class);
    return str == "." ? nullptr : strdup(str);
}

// vim: set sw=4 ts=4 et:
