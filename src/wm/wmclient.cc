/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "wmclient.h"

#include "wmframe.h"
#include "wmmgr.h"
#include "wmapp.h"
#include "sysdep.h"
#include "ycstring.h"

YBoolPrefProperty YFrameClient::gLimitSize("icewm", "LimitSize", true); // remove this from this class

extern XContext frameContext;
extern XContext clientContext;

YFrameClient::YFrameClient(YWindow *parent, YFrameWindow *frame, Window win): YWindow(parent, win) {
    fFrame = frame;
    fBorder = 0;
    fProtocols = 0;
    fWindowTitle = 0;
    fIconTitle = 0;
    fColormap = None;
    fShaped = 0;
    fHints = 0;
    fWinHints = 0;
    //fSavedFrameState =
    fSizeHints = XAllocSizeHints();
    fClassHint = XAllocClassHint();
    fTransientFor = 0;
    fClientLeader = None;
    fWindowRole = 0;
#ifndef NO_MWM_HINTS
    fMwmHints = 0;
#endif

    getProtocols();
    getNameHint();
    getIconNameHint();
    getSizeHints();
    getClassHint();
    getTransient();
    getWMHints();
#ifdef GNOME1_HINTS
    getWinHintsHint(&fWinHints);
#endif
#ifndef NO_MWM_HINTS
    getMwmHints();
#endif

#ifdef SHAPE
    if (shapesSupported) {
        XShapeSelectInput (app->display(), handle(), ShapeNotifyMask);
        queryShape();
    }
#endif
    XSaveContext(app->display(), handle(),
                 getFrame() ? frameContext : clientContext,
                 getFrame() ? (XPointer)getFrame() : (XPointer)this);
}

YFrameClient::~YFrameClient() {
    XDeleteContext(app->display(), handle(), getFrame() ? frameContext : clientContext);
    delete fWindowTitle; fWindowTitle = 0;
    delete fIconTitle; fIconTitle = 0;
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
    if (fWindowRole) { XFree(fWindowRole); fWindowRole = 0; }
}

void YFrameClient::getProtocols() {
    Atom *wmp = 0;
    int count;

    fProtocols = fProtocols & wpDeleteWindow; // always keep WM_DELETE_WINDOW

    if (XGetWMProtocols (app->display(),
                         handle(),
                         &wmp, &count) && wmp)
    {
        for (int i = 0; i < count; i++) {
            if (wmp[i] == _XA_WM_DELETE_WINDOW) fProtocols |= wpDeleteWindow;
            if (wmp[i] == _XA_WM_TAKE_FOCUS) fProtocols |= wpTakeFocus;
        }
        XFree (wmp);
    }
}

void YFrameClient::getSizeHints() {
    if (fSizeHints) {
        long supplied;

        if (!XGetWMNormalHints(app->display(),
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
    if (fClassHint) {
        if (fClassHint->res_name) {
            XFree(fClassHint->res_name);
            fClassHint->res_name = 0;
        }
        if (fClassHint->res_class) {
            XFree(fClassHint->res_class);
            fClassHint->res_class = 0;
        }
        XGetClassHint(app->display(), handle(), fClassHint);
    }
}

void YFrameClient::getTransient() {
    Window newTransientFor;

    if (XGetTransientForHint(app->display(),
                             handle(),
                             &newTransientFor))
    {
        // !!! getFrame?
        if ((getFrame() && newTransientFor == getFrame()->getRoot()->handle()) || /* bug in xfm */
            newTransientFor == desktop->handle() ||
            newTransientFor == handle()             /* bug in fdesign */
            /* !!! TODO: check for recursion */
           )
            newTransientFor = 0;

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
}

void YFrameClient::constrainSize(int &w, int &h, long layer, int flags) {
    if (fSizeHints) {
        int wm = fSizeHints->min_width;
        int hm = fSizeHints->min_height;
        int wM = fSizeHints->max_width;
        int hM = fSizeHints->max_height;
        int wb = fSizeHints->base_width;
        int hb = fSizeHints->base_height;
        int wf = fSizeHints->width_inc;
        int hf = fSizeHints->height_inc;

        if (fSizeHints->flags & PAspect) { // aspect ratios
            int xm = fSizeHints->min_aspect.x;
            int ym = fSizeHints->min_aspect.y;
            int xM = fSizeHints->max_aspect.x;
            int yM = fSizeHints->max_aspect.y;

            // !!! fix handling of KeepX and KeepY together
            if (xm * h > ym * w) { // min aspect
                if (flags & csKeepX) {
                    if (h < hm) h = hm;
                    if (h > hM) h = hM;
                    h = w * ym / xm;
                    if (h < hm) h = hm;
                    if (h > hM) h = hM;
                    w = h * xm / ym;
                } else {
                    if (w > wM) w = wM; // maximum size
                    if (w < wm) w = wm; // minimum size
                    w = h * xm / ym;
                    if (w < wm) w = wm; // minimum size
                    if (w > wM) w = wM;
                    h = w * ym / xm;
                }
            }
            if (xM * h < yM * w) { // max aspect
                if (flags & csKeepX) {
                    if (h < hm) h = hm;
                    if (h > hM) h = hM;
                    h = w * yM / xM;
                    if (h > hM) h = hM;
                    if (h < hm) h = hm;
                    w = h * xM / yM;
                } else {
                    if (w > wM) w = wM; // maximum size
                    if (w < wm) w = wm; // minimum size
                    w = h * xM / yM;
                    if (w < wm) w = wm; // minimum size
                    if (w > wM) w = wM;
                    h = w * yM / xM;
                }
            }
        }

        if (w < wm) w = wm; // minimum size
        if (h < hm) h = hm;

        if (w > wM) w = wM; // maximum size
        if (h > hM) h = hM;

        if (gLimitSize.getBool()) {
            if (w >= int(getFrame()->getRoot()->maxWidth(layer))) w = getFrame()->getRoot()->maxWidth(layer);
            if (h >= int(getFrame()->getRoot()->maxHeight(layer))) h = getFrame()->getRoot()->maxHeight(layer);
        }

        w = wb + (w - wb + ((flags & csRound) ? wf / 2 : 0)) / wf * wf;
        h = hb + (h - hb + ((flags & csRound) ? hf / 2 : 0)) / hf * hf;
    }
    if (w <= 0) w = 1;
    if (h <= 0) h = 1;
}

struct _gravity_offset
{
  int x, y;
};

void YFrameClient::gravityOffsets (int &xp, int &yp) {
    xp = 0;
    yp = 0;

    if (fSizeHints == 0)
        return ;

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
    XSendEvent (app->display(), handle(), False, 0L, (XEvent *) &xev);
}

void YFrameClient::setFrame(YFrameWindow *newFrame) {
    if (newFrame != getFrame()) {
        XDeleteContext(app->display(), handle(),
                       getFrame() ? frameContext : clientContext);
        fFrame = newFrame;
        XSaveContext(app->display(), handle(),
                     getFrame() ? frameContext : clientContext,
                     getFrame() ? (XPointer)getFrame() : (XPointer)this);
    }
}

void YFrameClient::setFrameState(FrameState state) {
    unsigned long arg[2];

    arg[0] = (unsigned long) state;
    arg[1] = (unsigned long) None;

    //printf("setting frame state to %d\n", arg[0]);

    if (state == WithdrawnState) {
        if (phase != phaseRestart && phase != phaseShutdown) {
            MSG(("deleting window properties id=%lX", handle()));
#ifdef GNOME1_HINTS
            XDeleteProperty(app->display(), handle(), _XA_WIN_WORKSPACE);
            XDeleteProperty(app->display(), handle(), _XA_WIN_LAYER);
            XDeleteProperty(app->display(), handle(), _XA_WIN_STATE);
#endif
            XDeleteProperty(app->display(), handle(), _XA_WM_STATE);
        }
    } else {
        XChangeProperty(app->display(), handle(),
                        _XA_WM_STATE, _XA_WM_STATE,
                        32, PropModeReplace,
                        (unsigned char *)arg, 2);
    }
}

FrameState YFrameClient::getFrameState() {
    FrameState st = WithdrawnState;
    Atom type;
    int format;
    unsigned long nitems, lbytes;
    unsigned char *propdata;

    if (XGetWindowProperty(app->display(), handle(),
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
    MSG(("Unmap: unmapped %d visible %d", unmapped(), visible()));
    if (!unmapped()) {
        MSG(("UnmapWindow"));
#ifdef NEED_GRAB2
        XGrabServer(app->display());
        XSync(app->display(), False);
#endif
        XEvent ev;
        if (XCheckTypedWindowEvent(app->display(), unmap.window, DestroyNotify, &ev)) {
            getFrame()->getRoot()->destroyedClient(unmap.window);
            XUngrabServer(app->display());
            return ; // gets destroyed
        } else {
            getFrame()->getRoot()->unmanageClient(unmap.window, false);
            return ; // gets destroyed
        }
#ifdef NEED_GRAB2
        XUngrabServer(app->display());
#endif
    }
    YWindow::handleUnmap(unmap);
}

void YFrameClient::handleProperty(const XPropertyEvent &property) {
    switch (property.atom) {
    case XA_WM_NAME:
        getNameHint();
        break;
    case XA_WM_ICON_NAME:
        getIconNameHint();
        break;
    case XA_WM_CLASS:
        getClassHint();
        if (getFrame())
            getFrame()->getFrameHints();
        break;
    case XA_WM_HINTS:
        getWMHints();
        break;
    case XA_WM_NORMAL_HINTS:
        getSizeHints();
        if (getFrame())
            getFrame()->updateMwmHints();
        break;
    case XA_WM_TRANSIENT_FOR:
        getTransient();
        break;
    default:
        if (property.atom == _XA_WM_PROTOCOLS) {
            getProtocols();
#ifndef LITE
        } else if (property.atom == _XA_KWM_WIN_ICON) {
            if (getFrame())
                getFrame()->updateIcon();
#ifdef GNOME1_HINTS
        } else if (property.atom == _XA_WIN_ICONS) {
            if (getFrame())
                getFrame()->updateIcon();
#endif
#endif
#ifdef WMSPEC_HINTS
        } else if (property.atom == _XA_NET_WM_STRUT) {
            if (getFrame())
                getFrame()->updateNetWMStrut();
#endif
        }
#ifdef GNOME1_HINTS
        else if (property.atom == _XA_WIN_HINTS) {
            getWinHintsHint(&fWinHints);
            if (getFrame()) {
                getFrame()->getFrameHints();
#ifdef CONFIG_TASKBAR
                getFrame()->updateTaskBar();
#endif
            }
        }
#endif
#ifndef NO_MWM_HINTS
        else if (property.atom == _XATOM_MWM_HINTS) {
            getMwmHints();
            if (getFrame())
                getFrame()->updateMwmHints();
        }
#endif
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
        getFrame()->getRoot()->destroyedClient(destroyWindow.window);
}

#ifdef SHAPE
void YFrameClient::handleShapeNotify(const XShapeEvent &shape) {
    if (shapesSupported) {
        MSG(("shape event: %d %d %d:%d=%dx%d time=%ld",
             shape.shaped, shape.kind,
             shape.x, shape.y, shape.width, shape.height, shape.time));
        if (shape.kind == ShapeBounding) {
            int newShaped = shape.shaped ? 1 : 0;
            if (newShaped)
                fShaped = newShaped;
            if (getFrame())
                getFrame()->setShape();
            fShaped = newShaped;
        }
    }
}
#endif

void YFrameClient::setWindowTitle(const char *aWindowTitle) {
    delete fWindowTitle;
    fWindowTitle = CStr::newstr(aWindowTitle);
    if (getFrame())
        getFrame()->updateTitle();
}

#ifdef I18N
void YFrameClient::setWindowTitle(XTextProperty  *prop) {
    Status status;
    char **cl;
    int n;
    if (!prop->value || prop->encoding == XA_STRING) {
        setWindowTitle((const char *)prop->value);
        return;
    }
    status = XmbTextPropertyToTextList (app->display(), prop, &cl, &n);
    if (status >= Success && n > 0 && cl[0]) {
        setWindowTitle((const char *)cl[0]);
        XFreeStringList(cl);
    } else {
        setWindowTitle((const char *)prop->value);
    }
}
#endif

void YFrameClient::setIconTitle(const char *aIconTitle) {
    delete fIconTitle;
    fIconTitle = CStr::newstr(aIconTitle);
    if (getFrame())
        getFrame()->updateIconTitle();
}

#ifdef I18N
void YFrameClient::setIconTitle(XTextProperty  *prop) {
    Status status;
    char **cl;
    int n;
    if (!prop->value || prop->encoding == XA_STRING) {
        setIconTitle((const char *)prop->value);
        return;
    }
    status = XmbTextPropertyToTextList (app->display(), prop, &cl, &n);
    if (status >= Success && n > 0 && cl[0]) {
        setIconTitle((const char *)cl[0]);
        XFreeStringList(cl);
    } else {
        setIconTitle((const char *)prop->value);
    }
}
#endif

void YFrameClient::setColormap(Colormap cmap) {
    fColormap = cmap;
    if (getFrame() && getFrame()->getRoot()->colormapWindow() == getFrame())
        getFrame()->getRoot()->installColormap(cmap);
}

#ifdef SHAPE
void YFrameClient::queryShape() {
    fShaped = 0;

    if (shapesSupported) {
        int xws, yws, xbs, ybs;
        unsigned wws, hws, wbs, hbs;
        Bool boundingShaped, clipShaped;

        XShapeQueryExtents(app->display(), handle(),
                           &boundingShaped, &xws, &yws, &wws, &hws,
                           &clipShaped, &xbs, &ybs, &wbs, &hbs);
        fShaped = boundingShaped ? 1 : 0;
  }
}
#endif

void YFrameClient::handleClientMessage(const XClientMessageEvent &message) {
    if (message.message_type == _XA_NET_ACTIVE_WINDOW) {
        if (getFrame())
            getFrame()->activateWindow(true);
    } else if (message.message_type == _XA_NET_CLOSE_WINDOW) {
        if (getFrame())
            getFrame()->wmClose();
    } else if (message.message_type == _XA_WM_CHANGE_STATE) {
        YFrameWindow *frame = getFrame()->getRoot()->findFrame(message.window);

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
    } else if (message.message_type == _XA_WIN_WORKSPACE ||
               message.message_type == _XA_NET_WM_DESKTOP)
    {
        if (getFrame())
            getFrame()->setWorkspace(message.data.l[0]);
        else
            setWinWorkspaceHint(message.data.l[0]);
    } else if (message.message_type == _XA_WIN_LAYER) {
        if (getFrame())
            getFrame()->setLayer(message.data.l[0]);
        else
            setWinLayerHint(message.data.l[0]);
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
    XTextProperty prop;

    if (XGetWMName(app->display(), handle(), &prop)) {
#ifdef I18N
        if (true /*multiByte!!!*/) {
            setWindowTitle(&prop);
        } else
#endif
        {
            setWindowTitle((char *)prop.value);
        }
        if (prop.value) XFree(prop.value);
    } else {
        setWindowTitle((const char*)0);
    }
}

void YFrameClient::getIconNameHint() {
    XTextProperty prop;

    if (XGetWMIconName(app->display(), handle(), &prop)) {
#ifdef I18N
        if (true /*multiByte!!!*/) {
            setIconTitle(&prop);
        } else
#endif
        {
            setIconTitle((char *)prop.value);
        }
        if (prop.value) XFree(prop.value);
    } else {
        setIconTitle((const char *)0);
    }
}

void YFrameClient::getWMHints() {
    if (fHints)
        XFree(fHints);
    fHints = XGetWMHints(app->display(), handle());
}

#ifndef NO_MWM_HINTS
void YFrameClient::getMwmHints() {
    int retFormat;
    Atom retType;
    unsigned long retCount, remain;

    if (fMwmHints) {
        XFree(fMwmHints);
        fMwmHints = 0;
    }
    if (XGetWindowProperty(app->display(), handle(),
                           _XATOM_MWM_HINTS, 0L, 20L, False, _XATOM_MWM_HINTS,
                           &retType, &retFormat, &retCount,
                           &remain,(unsigned char **)&fMwmHints) == Success && fMwmHints)
        if (retCount >= PROP_MWM_HINTS_ELEMENTS)
            return;
        else
            XFree(fMwmHints);
    fMwmHints = 0;
}

void YFrameClient::setMwmHints(const MwmHints &mwm) {
    if (fMwmHints) {
        XFree(fMwmHints);
        fMwmHints = 0;
    }
    XChangeProperty(app->display(), handle(),
                    _XATOM_MWM_HINTS, _XATOM_MWM_HINTS,
                    32, PropModeReplace,
                    (const unsigned char *)&mwm, sizeof(mwm)/sizeof(long)); ///!!! ???
    fMwmHints = (MwmHints *)malloc(sizeof(MwmHints));
    if (fMwmHints)
        *fMwmHints = mwm;
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
        if (fMwmHints->decorations & MWM_FUNC_ALL)
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
    Atom r_type;
    int r_format;
    unsigned long nitems;
    unsigned long bytes_remain;
    unsigned char *prop;

    if (XGetWindowProperty(app->display(), handle(),
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
            if ((fHints = XGetWMHints(app->display(), handle())) != 0) {
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
    Atom r_type;
    int r_format;
    unsigned long nitems;
    unsigned long bytes_remain;
    unsigned char *prop;

    if (XGetWindowProperty(app->display(), handle(),
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

#ifdef GNOME1_HINTS
void YFrameClient::setWinWorkspaceHint(long wk) {
    XChangeProperty(app->display(),
                    handle(),
                    _XA_WIN_WORKSPACE,
                    XA_CARDINAL,
                    32, PropModeReplace,
                    (unsigned char *)&wk, 1);
#ifdef WMSPEC_HINTS
    XChangeProperty(app->display(),
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
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop;

    if (XGetWindowProperty(app->display(),
                           handle(),
                           _XA_WIN_WORKSPACE,
                           0, 1, False, XA_CARDINAL,
                           &r_type, &r_format,
                           &count, &bytes_remain, &prop) == Success && prop)
    {
        if (r_type == XA_CARDINAL && r_format == 32 && count == 1U) {
            long ws = *(long *)prop;
            if (ws >= 0 && ws < getFrame()->getRoot()->workspaceCount()) {
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

#ifdef GNOME1_HINTS
void YFrameClient::setWinLayerHint(long layer) {
    XChangeProperty(app->display(),
                    handle(),
                    _XA_WIN_LAYER,
                    XA_CARDINAL,
                    32, PropModeReplace,
                    (unsigned char *)&layer, 1);
}
#endif

#ifdef GNOME1_HINTS
bool YFrameClient::getWinLayerHint(long *layer) {
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop;

    if (XGetWindowProperty(app->display(),
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

#ifdef GNOME1_HINTS
bool YFrameClient::getWinStateHint(long *mask, long *state) {
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop;

    if (XGetWindowProperty(app->display(),
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
#endif

#ifdef GNOME1_HINTS
void YFrameClient::setWinStateHint(long mask, long state) {
    long s[2];

    s[0] = state;
    s[1] = mask;

    MSG(("set state=%lX mask=%lX", state, mask));

    XChangeProperty(app->display(),
                    handle(),
                    _XA_WIN_STATE,
                    XA_CARDINAL,
                    32, PropModeReplace,
                    (unsigned char *)&s, 2);
}
#endif

#ifdef GNOME1_HINTS
bool YFrameClient::getWinHintsHint(long *state) {
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop;

    if (XGetWindowProperty(app->display(),
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

    XChangeProperty(app->display(),
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
    if (XGetWindowProperty(app->display(),
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
}

char *YFrameClient::getClientId(Window leader) { /// !!! fix
    char *cid = 0;
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;

    if (XGetWindowProperty(app->display(),
                           leader,
                           _XA_SM_CLIENT_ID,
                           0, 256, False, XA_STRING,
                           &r_type, &r_format,
                           &count, &bytes_remain, (unsigned char **)&cid) == Success && cid)
    {
        if (r_type == XA_STRING && r_format == 8) {
            //printf("cid=%s\n", cid);
        } else {
            XFree(cid);
            cid = 0;
        }
    }
    return cid;
}

bool YFrameClient::getNetWMStrut(int *left, int *right, int *top, int *bottom) {
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop;

    *left = 0;
    *right = 0;
    *top = 0;
    *bottom = 0;


    if (XGetWindowProperty(app->display(),
                           handle(),
                           _XA_NET_WM_STRUT,
                           0, 4, False, XA_CARDINAL,
                           &r_type, &r_format,
                           &count, &bytes_remain, &prop) == Success && prop)
    {
        if (r_type == XA_CARDINAL && r_format == 32 && count == 4U) {
            long *strut = (long *)prop;

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
