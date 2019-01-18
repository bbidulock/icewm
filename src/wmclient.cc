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

YFrameClient::YFrameClient(YWindow *parent, YFrameWindow *frame, Window win):
    YWindow(parent, win),
    fWindowTitle(),
    fIconTitle(),
    fWMWindowRole(),
    fWindowRole()
{
    fFrame = frame;
    fBorder = 0;
    fProtocols = 0;
    fColormap = None;
    fShaped = false;
    fPinging = false;
    fPingTime = 0;
    fHints = 0;
    fWinHints = 0;
    fSavedFrameState = InvalidFrameState;
    fSavedWinState[0] = 0;
    fSavedWinState[1] = 0;
    fSizeHints = XAllocSizeHints();
    fTransientFor = 0;
    fClientLeader = None;
    fMwmHints = 0;
    fPid = 0;

    getPropertiesList();

    getProtocols(false);
    getNameHint();
    getIconNameHint();
    getNetWmName();
    getNetWmIconName();
    getSizeHints();
    getClassHint();
    getTransient();
    getWMHints();
    getWMWindowRole();
    getWindowRole();
    getWinHintsHint(&fWinHints);
    getMwmHints();

#ifdef CONFIG_SHAPE
    if (shapesSupported) {
        XShapeSelectInput(xapp->display(), handle(), ShapeNotifyMask);
        queryShape();
    }
#endif
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

    if (fSizeHints) { XFree(fSizeHints); fSizeHints = 0; }
    if (fHints) { XFree(fHints); fHints = 0; }
    if (fMwmHints) { XFree(fMwmHints); fMwmHints = 0; }
}

void YFrameClient::getProtocols(bool force) {
    if (!prop.wm_protocols && !force)
        return;

    Atom *wmp = 0;
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

    Window newTransientFor = 0;

    if (XGetTransientForHint(xapp->display(),
                             handle(),
                             &newTransientFor))
    {
        if (//newTransientFor == manager->handle() || /* bug in xfm */
            //newTransientFor == desktop->handle() ||
            newTransientFor == handle()             /* bug in fdesign */
            /* !!! TODO: check for recursion */
           )
            newTransientFor = 0;
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

    if (fSizeHints == 0)
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
    if (timer == 0 || timer != fPingTimer) {
        return false;
    }

    fPingTimer = null;
    fPinging = false;

    if (destroyed() || getFrame() == 0) {
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

    Atom type = 0;
    int format = 0;
    unsigned long nitems = 0;
    unsigned long after = 0;
    unsigned char* prop = 0;
    if (XGetWindowProperty(xapp->display(), handle(),
                           _XA_NET_WM_PID, 0, 1, False, XA_CARDINAL,
                           &type, &format, &nitems, &after,
                           &prop) == Success && prop)
    {
        if (type == XA_CARDINAL && format == 32 && nitems == 1) {
            XTextProperty text = {};
            if (XGetWMClientMachine(xapp->display(), handle(), &text)) {
                char myhost[HOST_NAME_MAX + 1] = {};
                gethostname(myhost, HOST_NAME_MAX);
                int len = (int) strnlen(myhost, HOST_NAME_MAX);
                char* theirs = (char *) text.value;
                if (strncmp(myhost, theirs, len) == 0 &&
                    (theirs[len] == 0 || theirs[len] == '.'))
                {
                    *pid = fPid = *(long *)prop;
                }
                XFree(text.value);
            }
        }
        XFree(prop);
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
        long arg[2] = { state, None };
        XChangeProperty(xapp->display(), handle(),
                        _XA_WM_STATE, _XA_WM_STATE,
                        32, PropModeReplace,
                        (unsigned char *)arg, 2);
        fSavedFrameState = state;
    }
}

FrameState YFrameClient::getFrameState() {
    if (!prop.wm_state)
        return WithdrawnState;

    fSavedFrameState = InvalidFrameState;

    FrameState st = WithdrawnState;
    Atom type;
    int format;
    unsigned long nitems, lbytes;
    unsigned char *propdata(0);

    if (XGetWindowProperty(xapp->display(), handle(),
                           _XA_WM_STATE, 0, 2, False, _XA_WM_STATE,
                           &type, &format, &nitems, &lbytes,
                           &propdata) == Success && propdata)
    {
        if (format == 32 && nitems >= 1)
            fSavedFrameState = st = *(long *)propdata;
        XFree(propdata);
    }
    return st;
}

void YFrameClient::handleUnmap(const XUnmapEvent &unmap) {
    YWindow::handleUnmap(unmap);

    MSG(("UnmapWindow"));

    XEvent ev;
    if (XCheckTypedWindowEvent(xapp->display(), unmap.window,
                               DestroyNotify, &ev)) {
        YWindow::handleDestroyWindow(ev.xdestroywindow);
        manager->destroyedClient(unmap.window);
    } else {
        if (adopted()) {
            // When destroyed set wfDestroyed flag.
            XWindowAttributes attr;
            getWindowAttributes(&attr);
        }
        manager->unmanageClient(unmap.window, false);
    }
}

void YFrameClient::handleProperty(const XPropertyEvent &property) {
    bool new_prop = (property.state == PropertyDelete) ? false : true;
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
        if (new_prop) prop.wm_class = true;
        getClassHint();
        if (getFrame())
            getFrame()->getFrameHints();
        prop.wm_class = new_prop;
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
        getSizeHints();
        if (getFrame())
            getFrame()->updateMwmHints();
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
            if (new_prop) prop.win_hints = true;
            getWinHintsHint(&fWinHints);

            if (getFrame()) {
                getFrame()->getFrameHints();
                    manager->updateWorkArea();
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
    if (shapesSupported) {
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
    if (title == 0 || fWindowTitle == null || !fWindowTitle.equals(title)) {
        fWindowTitle = ustring::newstr(title);
        if (title != 0) {
            cstring cs(fWindowTitle);
            XChangeProperty(xapp->display(), handle(),
                    _XA_NET_WM_VISIBLE_NAME, _XA_UTF8_STRING,
                    8, PropModeReplace,
                    (const unsigned char *)cs.c_str(),
                    cs.c_str_len());
        } else {
            XDeleteProperty(xapp->display(), handle(),
                    _XA_NET_WM_VISIBLE_NAME);
        }
        if (getFrame()) getFrame()->updateTitle();
    }
}

void YFrameClient::setIconTitle(const char *title) {
    if (title == 0 || fIconTitle == null || !fIconTitle.equals(title)) {
        fIconTitle = ustring::newstr(title);
        if (title != 0) {
            cstring cs(fIconTitle);
            XChangeProperty(xapp->display(), handle(),
                    _XA_NET_WM_VISIBLE_ICON_NAME, _XA_UTF8_STRING,
                    8, PropModeReplace,
                    (const unsigned char *)cs.c_str(),
                    cs.c_str_len());
        } else {
            XDeleteProperty(xapp->display(), handle(),
                    _XA_NET_WM_VISIBLE_ICON_NAME);
        }
        if (getFrame()) getFrame()->updateIconTitle();
    }
}

#ifdef CONFIG_I18N
void YFrameClient::setWindowTitle(const XTextProperty & title) {
    if (NULL == title.value /*|| title.encoding == XA_STRING*/)
        setWindowTitle((const char *)title.value);
    else {
        int count;
        char ** strings(NULL);

        if (XmbTextPropertyToTextList(xapp->display(), const_cast<XTextProperty *>(&title),
                                      &strings, &count) >= 0 &&
            count > 0 && strings[0])
            setWindowTitle((const char *)strings[0]);
        else
            setWindowTitle((const char *)title.value);

        if (strings) XFreeStringList(strings);
    }
}

void YFrameClient::setIconTitle(const XTextProperty & title) {
    if (NULL == title.value /*|| title.encoding == XA_STRING*/)
        setIconTitle((const char *)title.value);
    else {
        int count;
        char ** strings(NULL);

        if (XmbTextPropertyToTextList(xapp->display(), const_cast<XTextProperty *>(&title),
                                      &strings, &count) >= 0 &&
            count > 0 && strings[0])
            setIconTitle((const char *)strings[0]);
        else
            setIconTitle((const char *)title.value);

        if (strings) XFreeStringList(strings);
    }
}
#endif

void YFrameClient::setColormap(Colormap cmap) {
    fColormap = cmap;
    if (getFrame() && manager->colormapWindow() == getFrame())
        manager->installColormap(cmap);
}

#ifdef CONFIG_SHAPE
void YFrameClient::queryShape() {
    fShaped = 0;

    if (shapesSupported) {
        int xws, yws, xbs, ybs;
        unsigned wws, hws, wbs, hbs;
        Bool boundingShaped, clipShaped;

        XShapeQueryExtents(xapp->display(), handle(),
                           &boundingShaped, &xws, &yws, &wws, &hws,
                           &clipShaped, &xbs, &ybs, &wbs, &hbs);
        fShaped = boundingShaped;
    }
}
#endif

long getMask(Atom a) {
    long mask = 0;

    if (a == _XA_NET_WM_STATE_MAXIMIZED_VERT)
        mask |= WinStateMaximizedVert;
    if (a == _XA_NET_WM_STATE_MAXIMIZED_HORZ)
        mask |= WinStateMaximizedHoriz;
    if (a == _XA_NET_WM_STATE_SHADED)
        mask |= WinStateRollup;
    if (a == _XA_NET_WM_STATE_ABOVE)
        mask |= WinStateAbove;
    if (a == _XA_NET_WM_STATE_MODAL)
        mask |= WinStateModal;
    if (a == _XA_NET_WM_STATE_BELOW)
        mask |= WinStateBelow;
    if (a == _XA_NET_WM_STATE_FULLSCREEN)
        mask |= WinStateFullscreen;
    if (a == _XA_NET_WM_STATE_SKIP_PAGER)
        mask |= WinStateSkipPager;
    if (a == _XA_NET_WM_STATE_SKIP_TASKBAR)
        mask |= WinStateSkipTaskBar;
    if (a == _XA_NET_WM_STATE_STICKY)
        mask |= WinStateSticky;
#if 0
    /* controlled by WM only */
    if (a == _XA_NET_WM_STATE_FOCUSED)
        mask |= WinStateFocused;
    if (a == _XA_NET_WM_STATE_HIDDEN)
        mask |= WinStateHidden;
#endif
    if (a == _XA_NET_WM_STATE_DEMANDS_ATTENTION)
        mask |= WinStateUrgent;
    return mask;
}

void YFrameClient::setNetWMFullscreenMonitors(int top, int bottom, int left, int right) {
    long data[4] = { top, bottom, left, right };
    XChangeProperty (xapp->display(), handle(),
                        _XA_NET_WM_FULLSCREEN_MONITORS, XA_CARDINAL,
                        32, PropModeReplace,
                        (unsigned char *) data, 4);
}

void YFrameClient::setNetFrameExtents(int left, int right, int top, int bottom) {
    long data[4] = { left, right, top, bottom };
    XChangeProperty(xapp->display(), handle(),
                        _XA_NET_FRAME_EXTENTS, XA_CARDINAL,
                        32, PropModeReplace,
                        (unsigned  char *) data, 4);
}

void YFrameClient::setNetWMAllowedActions(Atom *actions, int count) {
    XChangeProperty (xapp->display(), handle(),
                        _XA_NET_WM_ALLOWED_ACTIONS, XA_ATOM,
                        32, PropModeReplace,
                        (unsigned char *) actions, count);
}

void YFrameClient::handleClientMessage(const XClientMessageEvent &message) {
    if (message.message_type == _XA_WM_CHANGE_STATE) {
        YFrameWindow *frame = manager->findFrame(message.window);

        if (message.data.l[0] == IconicState) {
            if (frame && !(frame->isMinimized() || frame->isRollup()))
                frame->wmMinimize();
        } else if (message.data.l[0] == NormalState) {
            if (frame)
                frame->setState(WinStateHidden |
                                WinStateRollup |
                                WinStateMinimized, 0);
        } // !!! handle WithdrawnState if needed

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
        if (getFrame()) {
            getFrame()->activate();
            getFrame()->wmRaise();
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
            int grav = (message.data.l[0] & 0x00FF);
            if (grav) getFrame()->setWinGravity(grav == 0 ? sizeHints()->win_gravity : grav);
            getFrame()->setCurrentGeometryOuter(YRect(message.data.l[1], message.data.l[2],
                                                message.data.l[3], message.data.l[4]));
        }
    } else if (message.message_type == _XA_NET_WM_FULLSCREEN_MONITORS) {
        if (getFrame()) {
            getFrame()->updateNetWMFullscreenMonitors(message.data.l[0], message.data.l[1],
                                                      message.data.l[2], message.data.l[3]);
        }
    } else if (message.message_type == _XA_NET_WM_STATE) {
        long mask =
            getMask(message.data.l[1]) |
            getMask(message.data.l[2]);

        //printf("new state, mask = %ld\n", mask);
        if (message.data.l[0] == _NET_WM_STATE_ADD) {
            //puts("add");
            if (getFrame())
                getFrame()->setState(mask, mask);
        } else if (message.data.l[0] == _NET_WM_STATE_REMOVE) {
            //puts("remove");
            if (getFrame())
                getFrame()->setState(mask, 0);
        } else if (message.data.l[0] == _NET_WM_STATE_TOGGLE) {
            //puts("toggle");
            if (getFrame())
                getFrame()->setState(mask,
                                     getFrame()->getState() ^ mask);
        } else {
            warn("_NET_WM_STATE unknown command: %ld", message.data.l[0]);
        }
    } else if (message.message_type == _XA_WM_PROTOCOLS &&
               message.data.l[0] == (long) _XA_NET_WM_PING) {
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

#ifdef CONFIG_I18N
    XTextProperty name;
    if (XGetWMName(xapp->display(), handle(), &name))
#else
    char *name;
    if (XFetchName(xapp->display(), handle(), &name))
#endif
    {
        setWindowTitle(name);
#ifdef CONFIG_I18N
        XFree(name.value);
#else
        XFree(name);
#endif
    }
    else
        setWindowTitle(NULL);
}

void YFrameClient::getNetWmName() {
    if (!prop.net_wm_name)
        return;

    XTextProperty name;
    if (XGetTextProperty(xapp->display(), handle(), &name,
                _XA_NET_WM_NAME))
    {
        setWindowTitle((char *)name.value);
        XFree(name.value);
    }
    else
        setWindowTitle(NULL);
}

void YFrameClient::getIconNameHint() {
    if (!prop.wm_icon_name)
        return;
    if (prop.net_wm_icon_name)
        return;

#ifdef CONFIG_I18N
    XTextProperty name;
    if (XGetWMIconName(xapp->display(), handle(), &name))
#else
    char *name;
    if (XGetIconName(xapp->display(), handle(), &name))
#endif
    {
        setIconTitle(name);
#ifdef CONFIG_I18N
        XFree(name.value);
#else
        XFree(name);
#endif
    }
    else
        setIconTitle(NULL);
}

void YFrameClient::getNetWmIconName() {
    if (!prop.net_wm_icon_name)
        return;

    XTextProperty name;
    if (XGetTextProperty(xapp->display(), handle(), &name,
                _XA_NET_WM_ICON_NAME))
    {
        setIconTitle((char *)name.value);
        XFree(name.value);
    }
    else
        setIconTitle(NULL);
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

    int retFormat;
    Atom retType;
    unsigned long retCount, remain;

    if (fMwmHints) {
        XFree(fMwmHints);
        fMwmHints = 0;
    }
    union {
        MwmHints *ptr;
        unsigned char *xptr;
    } mwmHints = { 0 };

    if (XGetWindowProperty(xapp->display(), handle(),
                           _XATOM_MWM_HINTS, 0L, 20L, False, _XATOM_MWM_HINTS,
                           &retType, &retFormat, &retCount,
                           &remain, &(mwmHints.xptr)) == Success && mwmHints.ptr)
    {
        if (retCount >= PROP_MWM_HINTS_ELEMENTS) {
            fMwmHints = mwmHints.ptr;
            return;
        } else
            XFree(mwmHints.xptr);
    }
    fMwmHints = 0;
}

void YFrameClient::setMwmHints(const MwmHints &mwm) {
    XChangeProperty(xapp->display(), handle(),
                    _XATOM_MWM_HINTS, _XATOM_MWM_HINTS,
                    32, PropModeReplace,
                    (const unsigned char *)&mwm, PROP_MWM_HINTS_ELEMENTS);
    if (fMwmHints == 0)
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

bool YFrameClient::getKwmIcon(int *count, Pixmap **pixmap) {
    *count = 0;
    *pixmap = None;

    if (!prop.kwm_win_icon)
        return false;

    Atom r_type;
    int r_format;
    unsigned long nitems;
    unsigned long bytes_remain;
    unsigned char *prop(0);

    if (XGetWindowProperty(xapp->display(), handle(),
                           _XA_KWM_WIN_ICON, 0, 2, False, _XA_KWM_WIN_ICON,
                           &r_type, &r_format, &nitems, &bytes_remain,
                           &prop) == Success && prop)
    {
        if (r_format == 32 &&
            r_type == _XA_KWM_WIN_ICON &&
            nitems == 2)
        {
            *count = nitems;
            *pixmap = (Pixmap *)prop;

            if (fHints)
                XFree(fHints);
            if ((fHints = XGetWMHints(xapp->display(), handle())) != 0) {
            }
            return true;
        } else {
            XFree(prop);
        }
    }
    return false;
}

bool YFrameClient::getWinIcons(Atom *type, int *count, long **elem) {
    *type = None;
    *count = 0;
    *elem = 0;

    if (!prop.win_icons)
        return false;

    Atom r_type;
    int r_format;
    unsigned long nitems;
    unsigned long bytes_remain;
    unsigned char *prop(0);

    if (XGetWindowProperty(xapp->display(), handle(),
                           _XA_WIN_ICONS, 0, 4096, False, AnyPropertyType,
                           &r_type, &r_format, &nitems, &bytes_remain,
                           &prop) == Success && prop)
    {
        if (r_format == 32 && nitems > 0) {

            if (r_type == _XA_WIN_ICONS ||
                r_type == XA_PIXMAP) // for compatibility, obsolete, (will be removed?)
            {
                *type = r_type;
                *count = nitems;
                *elem = (long *)prop;
                return true;
            }
        }
        XFree(prop);
    }
    return false;
}

static void *GetFullWindowProperty(Display *display, Window handle, Atom propAtom, int &itemCount, int itemSize1)
{
    void *data = NULL;
    itemCount = 0;
    int itemSize = itemSize1;
    if (itemSize1 == 32)
        itemSize = sizeof(long) * 8;

    {
        Atom r_type;
        int r_format;
        unsigned long nitems;
        unsigned long bytes_remain;
        unsigned char *prop(0);

        while (XGetWindowProperty(display, handle,
                               propAtom, (itemCount * itemSize1) / 32, 1024*32, False, AnyPropertyType,
                               &r_type, &r_format, &nitems, &bytes_remain,
                               &prop) == Success && prop)
        {
            if (r_format == itemSize1 && nitems > 0) {
                data = realloc(data, (itemCount + nitems) * itemSize / 8);

                memcpy((char *)data + itemCount * itemSize / 8, prop, nitems * itemSize / 8);
                itemCount += nitems;
                XFree(prop);
                if (bytes_remain == 0)
                    break;
                continue;
            }
            XFree(prop);
            free(data);
            itemCount = 0;
            return NULL;
        }
    }
    return data;
}

bool YFrameClient::getNetWMIcon(int *count, long **elem) {
    *count = 0;
    *elem = 0;

    MSG(("get_net_wm_icon 1"));
    //if (!prop.net_wm_icon)
//        return false;

    *elem = (long *)GetFullWindowProperty(xapp->display(), handle(),
                                          _XA_NET_WM_ICON, *count, 32);

    if (elem && count && *count>0)
        return true;

#if 0
    msg("get_net_wm_icon 2");
    Atom r_type;
    int r_format;
    unsigned long nitems;
    unsigned long bytes_remain;
    unsigned char *prop(0);

    if (XGetWindowProperty(xapp->display(), handle(),
                           _XA_NET_WM_ICON, 0, 1024*256, False, AnyPropertyType,
                           &r_type, &r_format, &nitems, &bytes_remain,
                           &prop) == Success && prop)
    {
        msg("get_net_wm_icon 3");
        if (r_format == 32 && nitems > 0 && bytes_remain == 0) {

            msg("get_net_wm_icon 4, %ld %ld", (long)_XA_NET_WM_ICON, (long)r_type);

            *count = nitems;
            *elem = (long *)prop;
            return true;
        }
        XFree(prop);
    }
#endif
    return false;
}

void YFrameClient::setWinWorkspaceHint(long wk) {
    XChangeProperty(xapp->display(),
                    handle(),
                    _XA_WIN_WORKSPACE,
                    XA_CARDINAL,
                    32, PropModeReplace,
                    (unsigned char *)&wk, 1);

    XChangeProperty(xapp->display(),
                    handle(),
                    _XA_NET_WM_DESKTOP,
                    XA_CARDINAL,
                    32, PropModeReplace,
                    (unsigned char *)&wk, 1);
}

bool YFrameClient::getWinWorkspaceHint(long *workspace) {
    *workspace = 0;

    if (!prop.win_workspace)
        return false;

    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop(0);

    if (XGetWindowProperty(xapp->display(),
                           handle(),
                           _XA_WIN_WORKSPACE,
                           0, 1, False, XA_CARDINAL,
                           &r_type, &r_format,
                           &count, &bytes_remain, &prop) == Success && prop)
    {
        if (r_type == XA_CARDINAL && r_format == 32 && count == 1U) {
            long ws = *(long *)prop;
            if (ws < workspaceCount) {
                *workspace = ws;
                XFree(prop);
                return true;
            }
        }
        XFree(prop);
    }
    return false;
}

void YFrameClient::setWinLayerHint(long layer) {
    XChangeProperty(xapp->display(),
                    handle(),
                    _XA_WIN_LAYER,
                    XA_CARDINAL,
                    32, PropModeReplace,
                    (unsigned char *)&layer, 1);
}

bool YFrameClient::getWinLayerHint(long *layer) {
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop(0);

    if (XGetWindowProperty(xapp->display(),
                           handle(),
                           _XA_WIN_LAYER,
                           0, 1, False, XA_CARDINAL,
                           &r_type, &r_format,
                           &count, &bytes_remain, &prop) == Success && prop)
    {
        if (r_type == XA_CARDINAL && r_format == 32 && count == 1U) {
            long l = *(long *)prop;
            if (inrange(l, 0L, WinLayerCount - 1L)) {
                *layer = l;
                XFree(prop);
                return true;
            }
        }
        XFree(prop);
    }
    return false;
}

bool YFrameClient::getWinTrayHint(long *tray_opt) {
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop(0);

    if (XGetWindowProperty(xapp->display(),
                           handle(),
                           _XA_WIN_TRAY,
                           0, 1, False, XA_CARDINAL,
                           &r_type, &r_format,
                           &count, &bytes_remain, &prop) == Success && prop)
    {
        if (r_type == XA_CARDINAL && r_format == 32 && count == 1U) {
            long o = *(long *)prop;
            if (o < WinTrayOptionCount) {
                *tray_opt = o;
                XFree(prop);
                return true;
            }
        }
        XFree(prop);
    }
    return false;
}

void YFrameClient::setWinTrayHint(long tray_opt) {
    XChangeProperty(xapp->display(),
                    handle(),
                    _XA_WIN_TRAY,
                    XA_CARDINAL,
                    32, PropModeReplace,
                    (unsigned char *)&tray_opt, 1);
}

bool YFrameClient::getWinStateHint(long *mask, long *state) {
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop(0);

    if (XGetWindowProperty(xapp->display(),
                           handle(),
                           _XA_WIN_STATE,
                           0, 2, False, XA_CARDINAL,
                           &r_type, &r_format,
                           &count, &bytes_remain, &prop) == Success && prop)
    {
        MSG(("got state"));
        if (r_type == XA_CARDINAL && r_format == 32 && count >= 1U) {
            long s = ((long *)prop)[0];
            long m = WIN_STATE_ALL;

            if (count >= 2U)
                m = ((long *)prop)[1];

            fSavedWinState[0] = *state = s;
            fSavedWinState[1] = *mask = m;
            XFree(prop);
            return true;
        }
        MSG(("bad state"));
        XFree(prop);
    }
    fSavedWinState[0] = 0;
    fSavedWinState[1] = 0;
    return false;
}

void YFrameClient::setWinStateHint(long mask, long state) {
    MSG(("set state=%8lX mask=%3lX, saved %8lX, %3lX, %p",
          state, mask, fSavedWinState[0], fSavedWinState[1], this));

    if (destroyed())
        return;

    if (hasbit(mask, state ^ fSavedWinState[0]) || mask != fSavedWinState[1]) {
        long prop[2] = { state & mask, mask };

        XChangeProperty(xapp->display(),
                        handle(),
                        _XA_WIN_STATE,
                        XA_CARDINAL,
                        32, PropModeReplace,
                        (unsigned char *) prop, 2);

        fSavedWinState[0] = state;
        fSavedWinState[1] = mask;
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
    if (state & WinStateFocused)
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

    XChangeProperty(xapp->display(), handle(),
                    _XA_NET_WM_STATE, XA_ATOM,
                    32, PropModeReplace,
                    (unsigned char *)a, i);

    fSavedWinState[0] = state;
    fSavedWinState[1] = mask;
}

bool YFrameClient::getNetWMStateHint(long *mask, long *state) {
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop(0);

    *mask = 0;
    *state = 0;
    if (XGetWindowProperty(xapp->display(),
                           handle(),
                           _XA_NET_WM_STATE,
                           0, 64, False, XA_ATOM,
                           &r_type, &r_format,
                           &count, &bytes_remain, &prop) == Success && prop)
    {
        MSG(("got state"));
        if (r_type == XA_ATOM && r_format == 32 && count >= 1U) {
            Atom *s = ((Atom *)prop);

            for (unsigned long i = 0; i < count; i++) {
                // can start hidden
                if (s[i] == _XA_NET_WM_STATE_HIDDEN) {
                    (*state) |= WinStateMinimized;
                    (*mask) |= WinStateMinimized;
                } else
                if (s[i] == _XA_NET_WM_STATE_FOCUSED) {
                    if (manager->wmState() == YWindowManager::wmSTARTUP) {
                        (*state) |= WinStateFocused;
                        (*mask) |= WinStateFocused;
                    }
                } else
                if (s[i] == _XA_NET_WM_STATE_FULLSCREEN) {
                    (*state) |= WinStateFullscreen;
                    (*mask) |= WinStateFullscreen;
                } else
                if (s[i] == _XA_NET_WM_STATE_ABOVE) {
                    (*state) |= WinStateAbove;
                    (*mask) |= WinStateAbove;
                } else
                if (s[i] == _XA_NET_WM_STATE_BELOW) {
                    (*state) |= WinStateBelow;
                    (*mask) |= WinStateBelow;
                } else
                if (s[i] == _XA_NET_WM_STATE_SHADED) {
                    (*state) |= WinStateRollup;
                    (*mask) |= WinStateRollup;
                } else
                if (s[i] == _XA_NET_WM_STATE_MODAL) {
                    (*state) |= WinStateModal;
                    (*mask) |= WinStateModal;
                } else
                if (s[i] == _XA_NET_WM_STATE_MAXIMIZED_VERT) {
                    (*state) |= WinStateMaximizedVert;
                    (*mask) |= WinStateMaximizedVert;
                } else
                if (s[i] == _XA_NET_WM_STATE_MAXIMIZED_HORZ) {
                    (*state) |= WinStateMaximizedHoriz;
                    (*mask) |= WinStateMaximizedHoriz;
                } else
                if (s[i] == _XA_NET_WM_STATE_SKIP_TASKBAR) {
                    (*state) |= WinStateSkipTaskBar;
                    (*mask) |= WinStateSkipTaskBar;
                } else
                if (s[i] == _XA_NET_WM_STATE_STICKY) {
                    (*state) |= WinStateSticky;
                    (*mask) |= WinStateSticky;
                } else
                if (s[i] == _XA_NET_WM_STATE_SKIP_PAGER) {
                    (*state) |= WinStateSkipPager;
                    (*mask) |= WinStateSkipPager;
                } else
                if (s[i] == _XA_NET_WM_STATE_DEMANDS_ATTENTION) {
                    (*state) |= WinStateUrgent;
                    (*mask) |= WinStateUrgent;
                }
            }
            XFree(prop);
            return true;
        }
        MSG(("bad state"));
        XFree(prop);
        return true;
    }
    return false;
}

bool YFrameClient::getWinHintsHint(long *state) {
    *state = 0;
    if (!prop.win_hints)
        return false;

    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop(0);

    if (XGetWindowProperty(xapp->display(),
                           handle(),
                           _XA_WIN_HINTS,
                           0, 1, False, XA_CARDINAL,
                           &r_type, &r_format,
                           &count, &bytes_remain, &prop) == Success && prop)
    {
        MSG(("got state"));
        if (r_type == XA_CARDINAL && r_format == 32 && count == 1U) {
            long s = ((long *)prop)[0];

            *state = s;
            XFree(prop);
            return true;
        }
        MSG(("bad state"));
        XFree(prop);
    }
    return false;
}

void YFrameClient::setWinHintsHint(long hints) {
    long s[1];

    s[0] = hints;
    fWinHints = hints;

    XChangeProperty(xapp->display(),
                    handle(),
                    _XA_WIN_HINTS,
                    XA_CARDINAL,
                    32, PropModeReplace,
                    (unsigned char *)&s, 1);
}

void YFrameClient::getClientLeader() {
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop(0);

    fClientLeader = None;
    if (XGetWindowProperty(xapp->display(),
                           handle(),
                           _XA_WM_CLIENT_LEADER,
                           0, 1, False, XA_WINDOW,
                           &r_type, &r_format,
                           &count, &bytes_remain, &prop) == Success && prop)
    {
        if (r_type == XA_WINDOW && r_format == 32 && count == 1U) {
            long s = ((long *)prop)[0];

            fClientLeader = s;
        }
        XFree(prop);
    }
}

void YFrameClient::getWindowRole() {
    if (!prop.window_role)
        return;

    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    union {
        char *ptr;
        unsigned char *xptr;
    } role = { 0 };

    if (XGetWindowProperty(xapp->display(),
                           handle(),
                           _XA_WINDOW_ROLE,
                           0, 256, False, XA_STRING,
                           &r_type, &r_format,
                           &count, &bytes_remain,
                           &(role.xptr)) == Success && role.ptr)
    {
        if (r_type == XA_STRING && r_format == 8) {
            MSG(("window_role=%s", role.ptr));
        } else {
            XFree(role.xptr);
            role.xptr = 0;
        }
    }

    fWindowRole = role.ptr;
}

void YFrameClient::getWMWindowRole() {
    if (!prop.wm_window_role)
        return;

    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    union {
        char *ptr;
        unsigned char *xptr;
    } role = { 0 };

    if (XGetWindowProperty(xapp->display(),
                           handle(),
                           _XA_WM_WINDOW_ROLE,
                           0, 256, False, XA_STRING,
                           &r_type, &r_format,
                           &count, &bytes_remain,
                           &(role.xptr)) == Success && role.ptr)
    {
        if (r_type == XA_STRING && r_format == 8) {
            MSG(("wm_window_role=%s", role.ptr));
        } else {
            XFree(role.xptr);
            role.xptr = 0;
        }
    }

    fWMWindowRole = role.ptr;
    if (role.xptr)
        XFree(role.xptr);
}

ustring YFrameClient::getClientId(Window leader) { /// !!! fix

    if (!prop.sm_client_id)
        return null;

    union {
        char *ptr;
        unsigned char *xptr;
    } cid = { 0 };
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;

    if (XGetWindowProperty(xapp->display(),
                           leader,
                           _XA_SM_CLIENT_ID,
                           0, 256, False, XA_STRING,
                           &r_type, &r_format,
                           &count, &bytes_remain, &(cid.xptr)) == Success && cid.ptr)
    {
        if (r_type == XA_STRING && r_format == 8) {
            //msg("cid=%s", cid);
        } else {
            XFree(cid.xptr);
            cid.xptr = 0;
        }
    }
    return cid.ptr;
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

    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop(0);

    if (XGetWindowProperty(xapp->display(),
                           handle(),
                           _XA_NET_WM_STRUT,
                           0, 4, False, XA_CARDINAL,
                           &r_type, &r_format,
                           &count, &bytes_remain, &prop) == Success && prop)
    {
        if (r_type == XA_CARDINAL && r_format == 32 && count == 4U) {
            long *strut = (long *)prop;

            MSG(("got strut"));
            *left = strut[0];
            *right = strut[1];
            *top = strut[2];
            *bottom = strut[3];

            XFree(prop);
            return true;
        }
        XFree(prop);
    }
    return false;
}

bool YFrameClient::getNetWMStrutPartial(int *left, int *right, int *top, int *bottom,
        int *left_start_y, int *left_end_y, int *right_start_y, int* right_end_y,
        int *top_start_x, int *top_end_x, int *bottom_start_x, int *bottom_end_x) {
    if (left_start_y   != 0) *left_start_y   = 0;
    if (left_end_y     != 0) *left_end_y     = 0;
    if (right_start_y  != 0) *right_start_y  = 0;
    if (right_end_y    != 0) *right_end_y    = 0;
    if (top_start_x    != 0) *top_start_x    = 0;
    if (top_end_x      != 0) *top_end_x      = 0;
    if (bottom_start_x != 0) *bottom_start_x = 0;
    if (bottom_end_x   != 0) *bottom_end_x   = 0;

    if (prop.net_wm_strut)
        return false;

    *left   = 0;
    *right  = 0;
    *top    = 0;
    *bottom = 0;

    if (!prop.net_wm_strut_partial)
        return false;

    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop(0);

    if (XGetWindowProperty(xapp->display(),
                           handle(),
                           _XA_NET_WM_STRUT_PARTIAL,
                           0, 12, False, XA_CARDINAL,
                           &r_type, &r_format,
                           &count, &bytes_remain, &prop) == Success && prop)
    {
        if (r_type == XA_CARDINAL && r_format == 32 && count == 12U) {
            long *strut = (long *)prop;

            MSG(("got strut partial"));
            *left = strut[0];
            *right = strut[1];
            *top = strut[2];
            *bottom = strut[3];
            if (left_start_y != 0) *left_start_y = strut[4];
            if (left_end_y != 0) *left_end_y = strut[5];
            if (right_start_y != 0) *right_start_y = strut[6];
            if (right_end_y != 0) *right_end_y = strut[7];
            if (top_start_x != 0) *top_start_x = strut[8];
            if (top_end_x != 0) *top_end_x = strut[9];
            if (bottom_start_x != 0) *bottom_start_x = strut[10];
            if (bottom_end_x != 0) *bottom_end_x = strut[11];

            XFree(prop);
            return true;
        }
        XFree(prop);
    }
    return false;
}

bool YFrameClient::getNetStartupId(unsigned long &time) {
    if (!prop.net_startup_id)
        return false;

    XTextProperty id;
    if (XGetTextProperty(xapp->display(), handle(), &id,
                _XA_NET_STARTUP_ID))
    {
        if (strstr((char *)id.value, "_TIME") != NULL) {
            time = atol(strstr((char *)id.value, "_TIME") + 5) & 0xffffffff;
            if (time == -1UL)
                time = -2UL;
            XFree(id.value);
            return true;
        }
        XFree(id.value);
    }
    return false;
}

bool YFrameClient::getNetWMUserTime(Window window, unsigned long &time) {
    if (!prop.net_wm_user_time && !prop.net_wm_user_time_window)
        return false;

    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop(0);

    if (XGetWindowProperty(xapp->display(), window,
                _XA_NET_WM_USER_TIME, 0, 1, False, XA_CARDINAL,
                &r_type, &r_format, &count, &bytes_remain, &prop) == Success && prop)
    {
        if (r_type == XA_CARDINAL && r_format == 32 && count == 1U) {
            long *utime = (long *) prop;

            MSG(("got user time"));
            time = utime[0] & 0xffffffff;
            if (time == (unsigned long) -1)
                    time = (unsigned long) -2;

            XFree(prop);
            return true;
        }
        XFree(prop);
    }
    return false;
}


bool YFrameClient::getNetWMUserTimeWindow(Window &window) {
    if (!prop.net_wm_user_time_window)
        return false;

    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop(0);

    if (XGetWindowProperty(xapp->display(), handle(),
                _XA_NET_WM_USER_TIME_WINDOW, 0, 1, False, XA_WINDOW,
                &r_type, &r_format, &count, &bytes_remain, &prop) == Success && prop)
    {
        if (r_type == XA_WINDOW && r_format == 32 && count == 1U) {
            long *uwin = (long *) prop;

            MSG(("got user time window"));
            window = uwin[0];

            XFree(prop);
            return true;
        }
        XFree(prop);
    }
    return false;
}

bool YFrameClient::getNetWMWindowOpacity(long &opacity) {
    if (!prop.net_wm_window_opacity)
        return false;

    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop(0);

    if (XGetWindowProperty(xapp->display(), handle(),
                _XA_NET_WM_WINDOW_OPACITY, 0, 1, False, XA_CARDINAL,
                &r_type, &r_format, &count, &bytes_remain, &prop) == Success && prop)
    {
        if (r_type == XA_CARDINAL && r_format == 32 && count == 1U) {
            long *data = (long *) prop;

            MSG(("got window opacity"));
            opacity = data[0];

            XFree(prop);
            return true;
        }
        XFree(prop);
    }
    return false;
}

bool YFrameClient::getNetWMWindowType(Atom *window_type) { // !!! for now, map to layers
    *window_type = None;

    if (!prop.net_wm_window_type)
        return false;

    Atom r_type;
    int r_format;
    unsigned long nitems;
    unsigned long bytes_remain;
    unsigned char *prop(0);

    if (XGetWindowProperty(xapp->display(), handle(),
                           _XA_NET_WM_WINDOW_TYPE, 0, 64, False, AnyPropertyType,
                           &r_type, &r_format, &nitems, &bytes_remain,
                           &prop) == Success && prop)
    {
        if (r_format == 32 && nitems > 0) {
            Atom *x = (Atom *)prop;

            for (unsigned int i = 0; i < nitems; i++) {
                if (x[i] == _XA_NET_WM_WINDOW_TYPE_COMBO) {
                    *window_type = x[i];
                    break;
                }
                if (x[i] == _XA_NET_WM_WINDOW_TYPE_DESKTOP) {
                    *window_type = x[i];
                    break;
                }
                if (x[i] == _XA_NET_WM_WINDOW_TYPE_DIALOG) {
                    *window_type = x[i];
                    break;
                }
                if (x[i] == _XA_NET_WM_WINDOW_TYPE_DND) {
                    *window_type = x[i];
                    break;
                }
                if (x[i] == _XA_NET_WM_WINDOW_TYPE_DOCK) {
                    *window_type = x[i];
                    break;
                }
                if (x[i] == _XA_NET_WM_WINDOW_TYPE_DROPDOWN_MENU) {
                    *window_type = x[i];
                    break;
                }
                if (x[i] == _XA_NET_WM_WINDOW_TYPE_MENU) {
                    *window_type = x[i];
                    break;
                }
                if (x[i] == _XA_NET_WM_WINDOW_TYPE_NORMAL) {
                    *window_type = x[i];
                    break;
                }
                if (x[i] == _XA_NET_WM_WINDOW_TYPE_NOTIFICATION) {
                    *window_type = x[i];
                    break;
                }
                if (x[i] == _XA_NET_WM_WINDOW_TYPE_POPUP_MENU) {
                    *window_type = x[i];
                    break;
                }
                if (x[i] == _XA_NET_WM_WINDOW_TYPE_SPLASH) {
                    *window_type = x[i];
                    break;
                }
                if (x[i] == _XA_NET_WM_WINDOW_TYPE_TOOLBAR) {
                    *window_type = x[i];
                    break;
                }
                if (x[i] == _XA_NET_WM_WINDOW_TYPE_TOOLTIP) {
                    *window_type = x[i];
                    break;
                }
                if (x[i] == _XA_NET_WM_WINDOW_TYPE_UTILITY) {
                    *window_type = x[i];
                    break;
                }
            }
            XFree(prop);
            return true;
        }
        XFree(prop);
    }
    return false;
}
bool YFrameClient::getNetWMDesktopHint(long *workspace) {
    *workspace = 0;

    if (!prop.net_wm_desktop)
        return false;

    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop(0);

    if (XGetWindowProperty(xapp->display(),
                           handle(),
                           _XA_NET_WM_DESKTOP,
                           0, 1, False, XA_CARDINAL,
                           &r_type, &r_format,
                           &count, &bytes_remain, &prop) == Success && prop)
    {
        if (r_type == XA_CARDINAL && r_format == 32 && count == 1U) {
            long ws = *(long *)prop;
            if (ws < workspaceCount) {
                *workspace = ws;
                XFree(prop);
                return true;
            }
        }
        XFree(prop);
    }
    return false;
}

void YFrameClient::getPropertiesList() {
    int count;
    Atom *p;

    memset(&prop, 0, sizeof(prop));

    p = XListProperties(xapp->display(), handle(), &count);

//    #define HAS(x) do { puts(#x); x = true; } while (0)
#define HAS(x) do { x = true; } while (0)

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
        }
        XFree(p);
    }
}

void YFrameClient::configure(const YRect &r) {
    (void)r;
    MSG(("client geometry %+d%+d %dx%d", r.x(), r.y(), r.width(), r.height()));
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
    return str == "." ? 0 : strdup(cstring(str));
}

// vim: set sw=4 ts=4 et:
