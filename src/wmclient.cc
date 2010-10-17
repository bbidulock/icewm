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

extern XContext frameContext;
extern XContext clientContext;

YFrameClient::YFrameClient(YWindow *parent, YFrameWindow *frame, Window win): YWindow(parent, win), fWindowTitle(null), fIconTitle(null), fWMWindowRole(null), fWindowRole(null) {
    fFrame = frame;
    fBorder = 0;
    fProtocols = 0;
    fWindowTitle = null;
    fIconTitle = null;
    fColormap = None;
    fShaped = false;
    fHints = 0;
    fWinHints = 0;
    //fSavedFrameState =
    fSizeHints = XAllocSizeHints();
    fClassHint = XAllocClassHint();
    fTransientFor = 0;
    fClientLeader = None;
    fWindowRole = null;
    fWMWindowRole = null;
#ifndef NO_MWM_HINTS
    fMwmHints = 0;
#endif

    getPropertiesList();

    getProtocols(false);
    getNameHint();
    getIconNameHint();
    getSizeHints();
    getClassHint();
    getTransient();
    getWMHints();
    getWMWindowRole();
    getWindowRole();
#ifdef GNOME1_HINTS
    getWinHintsHint(&fWinHints);
#endif
#ifndef NO_MWM_HINTS
    getMwmHints();
#endif

#ifdef CONFIG_SHAPE
    if (shapesSupported) {
        XShapeSelectInput(xapp->display(), handle(), ShapeNotifyMask);
        queryShape();
    }
#endif
    XSaveContext(xapp->display(), handle(),
                 getFrame() ? frameContext : clientContext,
                 getFrame() ? (XPointer)getFrame() : (XPointer)this);
}

YFrameClient::~YFrameClient() {
    XDeleteContext(xapp->display(), handle(), getFrame() ? frameContext : clientContext);
    if (fSizeHints) { XFree(fSizeHints); fSizeHints = 0; }
    if (fClassHint) {
        if (fClassHint->res_name) {
            XFree(fClassHint->res_name);
            fClassHint->res_name = 0;
        }
        if (fClassHint->res_class) {
            XFree(fClassHint->res_class);
            fClassHint->res_class = 0;
        }
        XFree(fClassHint);
        fClassHint = 0;
    }
    if (fHints) { XFree(fHints); fHints = 0; }
    if (fMwmHints) { XFree(fMwmHints); fMwmHints = 0; }
    fWMWindowRole = null;
    fWindowRole = null;
}

void YFrameClient::getProtocols(bool force) {
    if (!prop.wm_protocols && !force)
        return;

    Atom *wmp = 0;
    int count;

    fProtocols = fProtocols & wpDeleteWindow; // always keep WM_DELETE_WINDOW

    if (XGetWMProtocols(xapp->display(),
                        handle(),
                        &wmp, &count) && wmp)
    {
        prop.wm_protocols = true;
        for (int i = 0; i < count; i++) {
            if (wmp[i] == _XA_WM_DELETE_WINDOW) fProtocols |= wpDeleteWindow;
            if (wmp[i] == _XA_WM_TAKE_FOCUS) fProtocols |= wpTakeFocus;
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

    if (fClassHint) {
        if (fClassHint->res_name) {
            XFree(fClassHint->res_name);
            fClassHint->res_name = 0;
        }
        if (fClassHint->res_class) {
            XFree(fClassHint->res_class);
            fClassHint->res_class = 0;
        }
        XGetClassHint(xapp->display(), handle(), fClassHint);
    }
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
    if (fSizeHints) {
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
                    h = w * yMin / xMin;
                    h = clamp(h, hMin, hMax);
                    w = h * xMin / yMin;
                } else {
                    h = clamp(h, hMin, hMax);
                    w = h * xMin / yMin;
                    w = clamp(w, wMin, wMax);
                    h = w * yMin / xMin;
                }
            }
            if (xMax * h < yMax * w) { // max aspect
                if (flags & csKeepX) {
                    w = clamp(w, wMin, wMax);
                    h = w * yMax / xMax;
                    h = clamp(h, hMin, hMax);
                    w = h * xMax / yMax;
                } else {
                    h = clamp(h, hMin, hMax);
                    w = h * xMax / yMax;
                    w = clamp(w, wMin, wMax);
                    h = w * yMax / xMax;
                }
            }
        }

        h = clamp(h, hMin, hMax);
        w = clamp(w, wMin, wMax);

        if (flags & csRound) {
            w += wInc / 2; h += hInc / 2;
        }

        w-= max(0, w - wBase) % wInc;
        h-= max(0, h - hBase) % hInc;
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

void YFrameClient::setFrame(YFrameWindow *newFrame) {
    if (newFrame != getFrame()) {
        XDeleteContext(xapp->display(), handle(),
                       getFrame() ? frameContext : clientContext);
        fFrame = newFrame;
        XSaveContext(xapp->display(), handle(),
                     getFrame() ? frameContext : clientContext,
                     getFrame() ? (XPointer)getFrame() : (XPointer)this);
    }
}

void YFrameClient::setFrameState(FrameState state) {
    unsigned long arg[2];

    arg[0] = (unsigned long) state;
    arg[1] = (unsigned long) None;

    //msg("setting frame state to %d", arg[0]);

    if (state == WithdrawnState) {
        if (manager->wmState() != YWindowManager::wmSHUTDOWN) {
            MSG(("deleting window properties id=%lX", handle()));
            XDeleteProperty(xapp->display(), handle(), _XA_NET_WM_DESKTOP);
            XDeleteProperty(xapp->display(), handle(), _XA_NET_WM_STATE);
            XDeleteProperty(xapp->display(), handle(), _XA_WIN_WORKSPACE);
            XDeleteProperty(xapp->display(), handle(), _XA_WIN_LAYER);
#ifdef CONFIG_TRAY
            XDeleteProperty(xapp->display(), handle(), _XA_WIN_TRAY);
#endif
            XDeleteProperty(xapp->display(), handle(), _XA_WIN_STATE);
            XDeleteProperty(xapp->display(), handle(), _XA_WM_STATE);
        }
    } else {
        XChangeProperty(xapp->display(), handle(),
                        _XA_WM_STATE, _XA_WM_STATE,
                        32, PropModeReplace,
                        (unsigned char *)arg, 2);
    }
}

FrameState YFrameClient::getFrameState() {
    if (!prop.wm_state)
        return WithdrawnState;

    FrameState st = WithdrawnState;
    Atom type;
    int format;
    unsigned long nitems, lbytes;
    unsigned char *propdata;

    if (XGetWindowProperty(xapp->display(), handle(),
                           _XA_WM_STATE, 0, 3, False, _XA_WM_STATE,
                           &type, &format, &nitems, &lbytes,
                           &propdata) == Success && propdata)
    {
        st = *(long *)propdata;
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
        manager->destroyedClient(unmap.window);
    } else {
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
#ifndef LITE
            getFrame()->updateIcon();
#endif
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
#ifndef LITE
        } else if (property.atom == _XA_KWM_WIN_ICON) {
            if (new_prop) prop.kwm_win_icon = true;
            if (getFrame())
                getFrame()->updateIcon();
            prop.kwm_win_icon = new_prop;
#ifdef GNOME1_HINTS
        } else if (property.atom == _XA_WIN_ICONS) {
            if (new_prop) prop.win_icons = true;
            if (getFrame())
                getFrame()->updateIcon();
            prop.win_icons = new_prop;
#endif
#endif
#ifdef WMSPEC_HINTS
        } else if (property.atom == _XA_NET_WM_STRUT) {
            MSG(("change: net wm strut"));
            if (new_prop) prop.net_wm_strut = true;
            if (getFrame())
                getFrame()->updateNetWMStrut();
            prop.net_wm_strut = new_prop;
#endif
#ifdef WMSPEC_HINTS
        } else if (property.atom == _XA_NET_WM_ICON) {
            msg("change: net wm icon");
            if (new_prop) prop.net_wm_icon = true;
#ifndef LITE
            if (getFrame())
                getFrame()->updateIcon();
#endif
            prop.net_wm_icon = new_prop;
#endif
#ifdef GNOME1_HINTS
        } else if (property.atom == _XA_WIN_HINTS) {
            if (new_prop) prop.win_hints = true;
            getWinHintsHint(&fWinHints);

            if (getFrame()) {
                getFrame()->getFrameHints();
                    manager->updateWorkArea();
#ifdef CONFIG_TASKBAR
                getFrame()->updateTaskBar();
#endif
            }
            prop.win_hints = new_prop;
        } else if (property.atom == _XA_WIN_WORKSPACE) {
            prop.win_workspace = new_prop;
        } else if (property.atom == _XA_WIN_LAYER) {
            prop.win_layer = new_prop;
        } else if (property.atom == _XA_WIN_STATE) {
            prop.win_state = new_prop;
#endif
#ifndef NO_MWM_HINTS
        } else if (property.atom == _XATOM_MWM_HINTS) {
            if (new_prop) prop.mwm_hints = true;
            getMwmHints();
            if (getFrame())
                getFrame()->updateMwmHints();
            prop.mwm_hints = new_prop;
#endif
        } else if (property.atom == _XA_WM_CLIENT_LEADER) { // !!! check these
            prop.wm_client_leader = new_prop;
        } else if (property.atom == _XA_SM_CLIENT_ID) {
            prop.sm_client_id = new_prop;
#ifdef WMSPEC_HINTS
        } else if (property.atom == _XA_NET_WM_DESKTOP) {
            prop.net_wm_desktop = new_prop;
        } else if (property.atom == _XA_NET_WM_STATE) {
            prop.net_wm_state = new_prop;
        } else if (property.atom == _XA_NET_WM_WINDOW_TYPE) {
            // !!! do we do dynamic update? (discuss on wm-spec)
            prop.net_wm_window_type = new_prop;
#endif
        } else {
            MSG(("Unknown property changed: %s, window=0x%lX",
                 XGetAtomName(xapp->display(), property.atom), handle()));
        }
        break;
    }
}

void YFrameClient::handleColormap(const XColormapEvent &colormap) {
    setColormap(colormap.colormap); //(colormap.state == ColormapInstalled && colormap.c_new == True)
    //                ? colormap.colormap
    //                : None);
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
        if (getFrame()) getFrame()->updateTitle();
    }
}

void YFrameClient::setIconTitle(const char *title) {
    if (title == 0 || fIconTitle == null || !fIconTitle.equals(title)) {
        fIconTitle = ustring::newstr(title);
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

#ifdef WMSPEC_HINTS
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
#if 0
    if (a == _XA_NET_WM_STATE_MODAL)
        mask |= WinStateModal;
#endif
    if (a == _XA_NET_WM_STATE_BELOW)
        mask |= WinStateBelow;
    if (a == _XA_NET_WM_STATE_FULLSCREEN)
        mask |= WinStateFullscreen;
    if (a == _XA_NET_WM_STATE_SKIP_TASKBAR)
        mask |= WinStateSkipTaskBar;
    return mask;
}
#endif

void YFrameClient::handleClientMessage(const XClientMessageEvent &message) {
#ifdef WMSPEC_HINTS
    if (message.message_type == _XA_NET_ACTIVE_WINDOW) {
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
    }
    if (message.message_type == _XA_NET_WM_STATE) {
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
            warn("_NET_WM_STATE unknown command: %d", message.data.l[0]);
        }
#endif
    } else if (message.message_type == _XA_WM_CHANGE_STATE) {
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

#ifdef GNOME1_HINTS
    } else if (message.message_type == _XA_WIN_WORKSPACE) {
        if (getFrame())
            getFrame()->setWorkspace(message.data.l[0]);
        else
            setWinWorkspaceHint(message.data.l[0]);
    } else if (message.message_type == _XA_NET_WM_DESKTOP) {
        if (getFrame()) {
            if ((unsigned long)message.data.l[0] == 0xFFFFFFFFUL)
                getFrame()->setSticky(true);
            else
                getFrame()->setWorkspace(message.data.l[0]);
        } else
            setWinWorkspaceHint(message.data.l[0]);
    } else if (message.message_type == _XA_WIN_LAYER) {
        if (getFrame())
            getFrame()->setRequestedLayer(message.data.l[0]);
        else
            setWinLayerHint(message.data.l[0]);
#ifdef CONFIG_TRAY
    } else if (message.message_type == _XA_WIN_TRAY) {
        if (getFrame())
            getFrame()->setTrayOption(message.data.l[0]);
        else
            setWinTrayHint(message.data.l[0]);
#endif
    } else if (message.message_type == _XA_WIN_STATE) {
        if (getFrame())
            getFrame()->setState(message.data.l[0], message.data.l[1]);
        else
            setWinStateHint(message.data.l[0], message.data.l[1]);
#endif
    } else
        YWindow::handleClientMessage(message);
}

void YFrameClient::getNameHint() {
    if (!prop.wm_name)
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
    } else
        setWindowTitle(NULL);
}

void YFrameClient::getIconNameHint() {
    if (!prop.wm_icon_name)
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

void YFrameClient::getWMHints() {
    if (!prop.wm_hints)
        return;

    if (fHints)
        XFree(fHints);
    fHints = XGetWMHints(xapp->display(), handle());
}

#ifndef NO_MWM_HINTS
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
    if (fMwmHints) {
        XFree(fMwmHints);
        fMwmHints = 0;
    }
    XChangeProperty(xapp->display(), handle(),
                    _XATOM_MWM_HINTS, _XATOM_MWM_HINTS,
                    32, PropModeReplace,
                    (const unsigned char *)&mwm, sizeof(mwm)/sizeof(long)); ///!!!
    fMwmHints = (MwmHints *)malloc(sizeof(MwmHints));
    if (fMwmHints)
        *fMwmHints = mwm;
}

void YFrameClient::saveSizeHints()
{
    memcpy(&savedSizeHints, fSizeHints, sizeof(XSizeHints));
};
void YFrameClient::restoreSizeHints() {
    memcpy(fSizeHints, &savedSizeHints, sizeof(XSizeHints));
};


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
#endif

bool YFrameClient::getKwmIcon(int *count, Pixmap **pixmap) {
    *count = 0;
    *pixmap = None;

    if (!prop.kwm_win_icon)
        return false;

    Atom r_type;
    int r_format;
    unsigned long nitems;
    unsigned long bytes_remain;
    unsigned char *prop;

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

#ifdef GNOME1_HINTS
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
    unsigned char *prop;

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
#endif

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
        unsigned char *prop;

        while (XGetWindowProperty(display, handle,
                               propAtom, (itemCount * itemSize) / 32, 1024*32, False, AnyPropertyType,
                               &r_type, &r_format, &nitems, &bytes_remain,
                               &prop) == Success && prop && bytes_remain == 0)
        {
            if (r_format == itemSize1 && nitems > 0) {
                data = realloc(data, (itemCount + nitems) * itemSize / 8);

                // access to memory beyound 256MiB causes crashes! But anyhow, size
                // >>2MiB looks suspicious. Detect this case ASAP. However, if
                // the usable icon is somewhere in the beginning, it's okay to
                // return truncated data.
                if (itemCount * itemSize / 8 >= 2097152) {
                     XFree(prop);
                     break;
                }

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
    unsigned char *prop;

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

#if defined(GNOME1_HINTS) || defined(WMSPEC_HINTS)
void YFrameClient::setWinWorkspaceHint(long wk) {
#ifdef GNOME1_HINTS
    XChangeProperty(xapp->display(),
                    handle(),
                    _XA_WIN_WORKSPACE,
                    XA_CARDINAL,
                    32, PropModeReplace,
                    (unsigned char *)&wk, 1);
#endif

#ifdef WMSPEC_HINTS
    XChangeProperty(xapp->display(),
                    handle(),
                    _XA_NET_WM_DESKTOP,
                    XA_CARDINAL,
                    32, PropModeReplace,
                    (unsigned char *)&wk, 1);
#endif
}
#endif

#ifdef GNOME1_HINTS
bool YFrameClient::getWinWorkspaceHint(long *workspace) {
    *workspace = 0;

    if (!prop.win_workspace)
        return false;

    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop;

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
    unsigned char *prop;

    if (XGetWindowProperty(xapp->display(),
                           handle(),
                           _XA_WIN_LAYER,
                           0, 1, False, XA_CARDINAL,
                           &r_type, &r_format,
                           &count, &bytes_remain, &prop) == Success && prop)
    {
        if (r_type == XA_CARDINAL && r_format == 32 && count == 1U) {
            long l = *(long *)prop;
            if (l < WinLayerCount) {
                *layer = l;
                XFree(prop);
                return true;
            }
        }
        XFree(prop);
    }
    return false;
}
#endif

#ifdef CONFIG_TRAY
bool YFrameClient::getWinTrayHint(long *tray_opt) {
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop;

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
#endif /* CONFIG_TRAY */

bool YFrameClient::getWinStateHint(long *mask, long *state) {
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop;

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

            *state = s;
            *mask = m;
            XFree(prop);
            return true;
        }
        MSG(("bad state"));
        XFree(prop);
    }
    return false;
}

void YFrameClient::setWinStateHint(long mask, long state) {
    long s[2];

    s[0] = state & mask;
    s[1] = mask;

    MSG(("set state=%lX mask=%lX", state, mask));

#ifdef GNOME1_HINTS
    XChangeProperty(xapp->display(),
                    handle(),
                    _XA_WIN_STATE,
                    XA_CARDINAL,
                    32, PropModeReplace,
                    (unsigned char *)&s, 2);

#endif
#ifdef WMSPEC_HINTS
/// TODO #warning "hack"
    // !!! hack
    Atom a[15];
    int i = 0;

    /* the next one is kinda messy */
    if ((state & WinStateMinimized) || (state & WinStateHidden))
        a[i++] = _XA_NET_WM_STATE_HIDDEN;
    if (state & WinStateSkipTaskBar)
        a[i++] = _XA_NET_WM_STATE_SKIP_TASKBAR;

    if (state & WinStateRollup)
        a[i++] = _XA_NET_WM_STATE_SHADED;
    if (state & WinStateAbove)
        a[i++] = _XA_NET_WM_STATE_ABOVE;
    if (state & WinStateBelow)
        a[i++] = _XA_NET_WM_STATE_BELOW;
#if 0
    if (state & WinStateModal)
        a[i++] = _XA_NET_WM_STATE_MODAL;
#endif
    if (state & WinStateFullscreen)
        a[i++] = _XA_NET_WM_STATE_FULLSCREEN;
    if (state & WinStateMaximizedVert)
        a[i++] = _XA_NET_WM_STATE_MAXIMIZED_VERT;
    if (state & WinStateMaximizedHoriz)
        a[i++] = _XA_NET_WM_STATE_MAXIMIZED_HORZ;

    XChangeProperty(xapp->display(), handle(),
                    _XA_NET_WM_STATE, XA_ATOM,
                    32, PropModeReplace,
                    (unsigned char *)&a, i);
#endif

}

bool YFrameClient::getNetWMStateHint(long *mask, long *state) {
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop;

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
                if (s[i] == _XA_NET_WM_STATE_FULLSCREEN) {
                    (*state) |= WinStateFullscreen;
                    (*mask) |= WinStateFullscreen;
                }
                if (s[i] == _XA_NET_WM_STATE_ABOVE) {
                    (*state) |= WinStateAbove;
                    (*mask) |= WinStateAbove;
                }
                if (s[i] == _XA_NET_WM_STATE_BELOW) {
                    (*state) |= WinStateBelow;
                    (*mask) |= WinStateBelow;
                }
                if (s[i] == _XA_NET_WM_STATE_SHADED) {
                    (*state) |= WinStateRollup;
                    (*mask) |= WinStateRollup;
                }
#if 0
                if (s[i] == _XA_NET_WM_STATE_MODAL) {
                    (*state) |= WinStateModal;
                    (*mask) |= WinStateModal;
                }
#endif
                if (s[i] == _XA_NET_WM_STATE_MAXIMIZED_VERT) {
                    (*state) |= WinStateMaximizedVert;
                    (*mask) |= WinStateMaximizedVert;
                }
                if (s[i] == _XA_NET_WM_STATE_MAXIMIZED_HORZ) {
                    (*state) |= WinStateMaximizedHoriz;
                    (*mask) |= WinStateMaximizedHoriz;
                }
                if (s[i] == _XA_NET_WM_STATE_SKIP_TASKBAR) {
                    (*state) |= WinStateSkipTaskBar;
                    (*mask) |= WinStateSkipTaskBar;
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

#ifdef GNOME1_HINTS
bool YFrameClient::getWinHintsHint(long *state) {
    *state = 0;
    if (!prop.win_hints)
        return false;

    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop;

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
#endif

#ifdef GNOME1_HINTS
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
#endif

void YFrameClient::getClientLeader() {
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop;

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

#ifdef WMSPEC_HINTS
bool YFrameClient::getNetWMStrut(int *left, int *right, int *top, int *bottom) {
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
    unsigned char *prop;

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


bool YFrameClient::getNetWMWindowType(Atom *window_type) { // !!! for now, map to layers
    *window_type = None;

    if (!prop.net_wm_window_type)
        return false;

    Atom r_type;
    int r_format;
    unsigned long nitems;
    unsigned long bytes_remain;
    unsigned char *prop;

    if (XGetWindowProperty(xapp->display(), handle(),
                           _XA_NET_WM_WINDOW_TYPE, 0, 64, False, AnyPropertyType,
                           &r_type, &r_format, &nitems, &bytes_remain,
                           &prop) == Success && prop)
    {
        if (r_format == 32 && nitems > 0) {
            Atom *x = (Atom *)prop;

            for (unsigned int i = 0; i < nitems; i++) {
                if (x[i] == _XA_NET_WM_WINDOW_TYPE_DOCK) {
                    *window_type = x[i];
                    break;
                }
                if (x[i] == _XA_NET_WM_WINDOW_TYPE_DESKTOP) {
                    *window_type = x[i];
                    break;
                }
                if (x[i] == _XA_NET_WM_WINDOW_TYPE_SPLASH) {
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
    unsigned char *prop;

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
#endif

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
            else if (a == XA_WM_ICON_NAME) HAS(prop.wm_icon_name);
            else if (a == XA_WM_CLASS) HAS(prop.wm_class);
            else if (a == _XA_WM_PROTOCOLS) HAS(prop.wm_protocols);
            else if (a == _XA_WM_CLIENT_LEADER) HAS(prop.wm_client_leader);
            else if (a == _XA_WM_WINDOW_ROLE) HAS(prop.wm_window_role);
            else if (a == _XA_WINDOW_ROLE) HAS(prop.window_role);
            else if (a == _XA_SM_CLIENT_ID) HAS(prop.sm_client_id);
            else if (a == _XATOM_MWM_HINTS) HAS(prop.mwm_hints);
            else if (a == _XA_KWM_WIN_ICON) HAS(prop.kwm_win_icon);
            else if (a == _XA_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR) HAS(prop.kde_net_wm_system_tray_window_for);
#ifdef WMSPEC_HINTS
            else if (a == _XA_NET_WM_STRUT) HAS(prop.net_wm_strut);
            else if (a == _XA_NET_WM_DESKTOP) HAS(prop.net_wm_desktop);
            else if (a == _XA_NET_WM_STATE) HAS(prop.net_wm_state);
            else if (a == _XA_NET_WM_WINDOW_TYPE) HAS(prop.net_wm_window_type);
            else if (a == _XA_NET_WM_USER_TIME) HAS(prop.net_wm_user_time);
#endif
#ifdef GNOME1_HINTS
            else if (a == _XA_WIN_HINTS) HAS(prop.win_hints);
            else if (a == _XA_WIN_WORKSPACE) HAS(prop.win_workspace);
            else if (a == _XA_WIN_STATE) HAS(prop.win_state);
            else if (a == _XA_WIN_LAYER) HAS(prop.win_layer);
            else if (a == _XA_WIN_ICONS) HAS(prop.win_icons);
#endif
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
    MSG(("client geometry %d:%d-%dx%d",
         r.x(),
         r.y(),
         r.width(),
         r.height()));
}

bool YFrameClient::getWmUserTime(long *userTime) {
    *userTime = -1;

    if (!prop.net_wm_user_time)
        return false;

    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop;

    if (XGetWindowProperty(xapp->display(),
                           handle(),
                           _XA_NET_WM_USER_TIME,
                           0, 4, False, XA_CARDINAL,
                           &r_type, &r_format,
                           &count, &bytes_remain, &prop) == Success && prop)
    {
        if (r_type == XA_CARDINAL && r_format == 32 && count == 4U) {
            long *value = (long *)prop;

            *userTime = *value;

            XFree(prop);
            return true;
        }
        XFree(prop);
    }
    return false;
}

