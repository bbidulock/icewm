/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "wmclient.h"

#include "wmframe.h"
#include "wmmgr.h"
#include "wmapp.h"
#include "sysdep.h"

YFrameClient::YFrameClient(YWindow *parent, YFrameWindow *frame, Window win): YWindow(parent, win) {
    fFrame = frame;
    fBorder = 0;
    fProtocols = 0;
    fWindowTitle = 0;
    fIconTitle = 0;
    fColormap = None;
    fShaped = false;
    fHints = 0;
    fWinHints = 0;
    //fSavedFrameState =
    fSizeHints = XAllocSizeHints();
    fClassHint = XAllocClassHint();
    fTransientFor = 0;
    fClientLeader = None;
    fWindowRole = 0;
#ifdef CONFIG_MOTIF_HINTS
    fMwmHints = 0;
#endif

    getProtocols();
    getNameHint();
    getIconNameHint();
    getSizeHints();
    getClassHint();
    getTransient();
    getWMHints();
    getWinHintsHint(&fWinHints);
#ifdef CONFIG_MOTIF_HINTS
    getMwmHints();
#endif

#ifdef CONFIG_WM_SESSION
    getPidHint();
#endif

#ifdef CONFIG_SHAPE
    if (shapesSupported) {
        XShapeSelectInput(app->display(), handle(), ShapeNotifyMask);
        queryShape();
    }
#endif

    saveContext();
}

YFrameClient::~YFrameClient() {
    deleteContext();

    delete fWindowTitle;
    delete fIconTitle;
    if (fSizeHints) XFree(fSizeHints);

    if (fClassHint) {
        if (fClassHint->res_name) XFree(fClassHint->res_name);
        if (fClassHint->res_class) XFree(fClassHint->res_class);
        XFree(fClassHint);
    }

    if (fHints) XFree(fHints);
    if (fMwmHints) XFree(fMwmHints);
    if (fWindowRole) XFree(fWindowRole);
}

void YFrameClient::getProtocols() {
    Atom *wmp = 0;
    int count;

    fProtocols = fProtocols & wpDeleteWindow; // always keep WM_DELETE_WINDOW

    if (XGetWMProtocols(app->display(),
                        handle(),
                        &wmp, &count) && wmp)
    {
        for (int i = 0; i < count; i++) {
            if (wmp[i] == atoms.wmDeleteWindow) fProtocols |= wpDeleteWindow;
            if (wmp[i] == atoms.wmTakeFocus) fProtocols |= wpTakeFocus;
        }
        XFree(wmp);
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
        if (newTransientFor == manager->handle() || /* bug in xfm */
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

        if (limitSize) {
            w = min(w, (int)(considerHorizBorder && !getFrame()->doNotCover()
	      ? manager->maxWidth(layer) - 2 * getFrame()->borderX()
	      : manager->maxWidth(layer)));
            h = min(h, (int)(considerVertBorder && !getFrame()->doNotCover()
	      ? manager->maxHeight(layer) - getFrame()->titleY()
	      				  - 2 * getFrame()->borderY()
	      : manager->maxHeight(layer) - getFrame()->titleY()));
        }

#if 0
        w = wBase + (w - wBase + ((flags & csRound) ? wInc / 2 : 0)) / wInc
								     * wInc;
        h = hBase + (h - hBase + ((flags & csRound) ? hInc / 2 : 0)) / hInc
								     * hInc;
#else
	if (flags & csRound) { w+= wInc / 2; h+= hInc / 2; }

	w-= max(0, w - wBase) % wInc;
	h-= max(0, h - hBase) % hInc;
#endif								     

    }

    if (w <= 0) w = 1;
    if (h <= 0) h = 1;
}

struct _gravity_offset
{
  int x, y;
};

void YFrameClient::gravityOffsets(int &xp, int &yp) {
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
    xev.message_type = atoms.wmProtocols;
    xev.format = 32;
    xev.data.l[0] = msg;
    xev.data.l[1] = timeStamp;
    XSendEvent(app->display(), handle(), False, 0L, (XEvent *) &xev);
}

void YFrameClient::setFrame(YFrameWindow *newFrame) {
    if (newFrame != getFrame()) {
        deleteContext();
        fFrame = newFrame;
        saveContext();
    }
}

void YFrameClient::setFrameState(FrameState state) {
    unsigned long arg[2];

    arg[0] = (unsigned long) state;
    arg[1] = (unsigned long) None;

    //msg("setting frame state to %d", arg[0]);

    if (state == WithdrawnState) {
        if (phase != phaseRestart && phase != phaseShutdown) {
            MSG(("deleting window properties id=%lX", handle()));
            XDeleteProperty(app->display(), handle(), atoms.winWorkspace);
            XDeleteProperty(app->display(), handle(), atoms.winLayer);
#ifdef CONFIG_TRAY
            XDeleteProperty(app->display(), handle(), atoms.icewmTrayOpt);
#endif	    
            XDeleteProperty(app->display(), handle(), atoms.winState);
            XDeleteProperty(app->display(), handle(), atoms.wmState);
        }
    } else {
        XChangeProperty(app->display(), handle(),
                        atoms.wmState, atoms.wmState,
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
                           atoms.wmState, 0, 3, False, atoms.wmState,
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

        XEvent ev;
        if (XCheckTypedWindowEvent(app->display(), unmap.window,
				   DestroyNotify, &ev)) {
            manager->destroyedClient(unmap.window);
            return; // gets destroyed
        } else if (XCheckTypedWindowEvent(app->display(), unmap.window,
					  ReparentNotify, &ev)) {
            manager->unmanageClient(unmap.window, true, false);
            return; // gets destroyed
        } else {
            manager->unmanageClient(unmap.window, false);
            return; // gets destroyed
        }
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
	    if (getFrame()) getFrame()->getFrameHints();
	    break;

	case XA_WM_HINTS:
	    getWMHints();
#ifndef LITE
            if (getFrame()) getFrame()->updateIcon();
#endif            
	    break;

	case XA_WM_NORMAL_HINTS:
	    getSizeHints();
	    if (getFrame()) {
		getFrame()->updateMwmHints();
		getFrame()->updateNormalSize();
	    }
	    break;

	case XA_WM_TRANSIENT_FOR:
	    getTransient();
	    break;

	default: // `extern Atom' does not reduce to an integer constant...
	    if (atoms.wmProtocols == property.atom) {
		getProtocols();
#ifndef LITE
	    } else if (atoms.kwmWinIcon == property.atom ||
		       atoms.winIcons == property.atom) {
		if (getFrame()) getFrame()->updateIcon();
#endif

	    } else if (atoms.winHints == property.atom) {
		getWinHintsHint(&fWinHints);

		if (getFrame()) {
                    getFrame()->getFrameHints();
                    manager->updateWorkArea();
#ifdef CONFIG_TASKBAR
                    getFrame()->updateTaskBar();
#endif
		}
#ifdef CONFIG_MOTIF_HINTS
	    } else if (atoms.mwmHints == property.atom) {
		getMwmHints();
		if (getFrame()) getFrame()->updateMwmHints();
		break;
#endif
	    } else
		MSG(("Unknown property changed: %s, window=0x%lX",
		     XGetAtomName(app->display(), property.atom), handle()));

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
    delete[] fWindowTitle; fWindowTitle = newstr(title);
    if (getFrame()) getFrame()->updateTitle();
}

void YFrameClient::setIconTitle(const char *title) {
    delete[] fIconTitle; fIconTitle = newstr(title);
    if (getFrame()) getFrame()->updateIconTitle();
}

#ifdef CONFIG_I18N
void YFrameClient::setWindowTitle(const XTextProperty & title) {
    if (NULL == title.value || title.encoding == XA_STRING)
        setWindowTitle((const char *)title.value);
    else {
        int count;
        char ** strings(NULL);

        if (XmbTextPropertyToTextList(app->display(), &title,
                                      &strings, &count) >= 0 &&
            count > 0 && strings[0])
            setWindowTitle((const char *)strings[0]);
        else
            setWindowTitle((const char *)title.value);

        if (strings) XFreeStringList(strings);
    }
}

void YFrameClient::setIconTitle(const XTextProperty & title) {
    if (NULL == title.value || title.encoding == XA_STRING)
        setIconTitle((const char *)title.value);
    else {
        int count;
        char ** strings(NULL);

        if (XmbTextPropertyToTextList(app->display(), &title,
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

        XShapeQueryExtents(app->display(), handle(),
                           &boundingShaped, &xws, &yws, &wws, &hws,
                           &clipShaped, &xbs, &ybs, &wbs, &hbs);
        fShaped = boundingShaped;
  }
}
#endif

void YFrameClient::handleClientMessage(const XClientMessageEvent &message) {
    if (message.message_type == atoms.wmChangeState) {
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

    } else if (message.message_type == atoms.winWorkspace) {
        if (getFrame())
            getFrame()->setWorkspace(message.data.l[0]);
        else
            setWinWorkspaceHint(message.data.l[0]);
    } else if (message.message_type == atoms.winLayer) {
        if (getFrame())
            getFrame()->setLayer(message.data.l[0]);
        else
            setWinLayerHint(message.data.l[0]);
#ifdef CONFIG_TRAY	    
    } else if (message.message_type == atoms.icewmTrayOpt) {
        if (getFrame())
            getFrame()->setTrayOption(message.data.l[0]);
        else
            setWinTrayHint(message.data.l[0]);
#endif	    
    } else if (message.message_type == atoms.winState) {
        if (getFrame())
            getFrame()->setState(message.data.l[0], message.data.l[1]);
        else
            setWinStateHint(message.data.l[0], message.data.l[1]);
    } else
        YWindow::handleClientMessage(message);
}

void YFrameClient::getNameHint() {
#ifdef CONFIG_I18N
    XTextProperty name;
    if (XGetWMName(app->display(), handle(), &name))
#else    
    char * name;
    if (XFetchName(app->display(), handle(), &name))
#endif
        setWindowTitle(name);
    else
        setWindowTitle(NULL);
}

void YFrameClient::getIconNameHint() {
#ifdef CONFIG_I18N
    XTextProperty name;
    if (XGetWMIconName(app->display(), handle(), &name))
#else    
    char * name;
    if (XGetIconName(app->display(), handle(), &name))
#endif
        setIconTitle(name);
    else
        setIconTitle(NULL);
}

void YFrameClient::getWMHints() {
    if (fHints) XFree(fHints);
    fHints = XGetWMHints(app->display(), handle());
}

#ifdef CONFIG_MOTIF_HINTS
void YFrameClient::getMwmHints() {
    int retFormat;
    Atom retType;
    unsigned long retCount, remain;

    if (fMwmHints) {
        XFree(fMwmHints);
        fMwmHints = 0;
    }
    if (XGetWindowProperty(app->display(), handle(),
                           atoms.mwmHints, 0L, 20L, False, atoms.mwmHints,
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
                    atoms.mwmHints, atoms.mwmHints,
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
#endif /* CONFIG_MOTIF_HINTS */

bool YFrameClient::getKwmIcon(int *count, Pixmap **pixmap) {
    Atom r_type;
    int r_format;
    unsigned long nitems;
    unsigned long bytes_remain;
    unsigned char *prop;

    if (XGetWindowProperty(app->display(), handle(),
                           atoms.kwmWinIcon, 0, 2, False, atoms.kwmWinIcon,
                           &r_type, &r_format, &nitems, &bytes_remain,
                           &prop) == Success && prop)
    {
        if (r_format == 32 &&
            r_type == atoms.kwmWinIcon &&
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

bool YFrameClient::getWinIcons(Atom *type, int *count, long **elem) {
    Atom r_type;
    int r_format;
    unsigned long nitems;
    unsigned long bytes_remain;
    unsigned char *prop;

    if (XGetWindowProperty(app->display(), handle(),
                           atoms.winIcons, 0, 4096, False, AnyPropertyType,
                           &r_type, &r_format, &nitems, &bytes_remain,
                           &prop) == Success && prop)
    {
        if (r_format == 32 && nitems > 0) {

            if (r_type == atoms.winIcons ||
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

void YFrameClient::setWinWorkspaceHint(long wk) {
    XChangeProperty(app->display(),
                    handle(),
                    atoms.winWorkspace,
                    XA_CARDINAL,
                    32, PropModeReplace,
                    (unsigned char *)&wk, 1);
}

bool YFrameClient::getWinWorkspaceHint(long *workspace) {
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop;

    if (XGetWindowProperty(app->display(),
                           handle(),
                           atoms.winWorkspace,
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
    XChangeProperty(app->display(),
                    handle(),
                    atoms.winLayer,
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

    if (XGetWindowProperty(app->display(),
                           handle(),
                           atoms.winLayer,
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

#ifdef CONFIG_TRAY
bool YFrameClient::getWinTrayHint(long *tray_opt) {
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop;
    
    if (XGetWindowProperty(app->display(),
                           handle(),
                           atoms.icewmTrayOpt,
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
    XChangeProperty(app->display(),
                    handle(),
                    atoms.icewmTrayOpt,
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

    if (XGetWindowProperty(app->display(),
                           handle(),
                           atoms.winState,
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

    s[0] = state;
    s[1] = mask;

    MSG(("set state=%lX mask=%lX", state, mask));

    XChangeProperty(app->display(),
                    handle(),
                    atoms.winState,
                    XA_CARDINAL,
                    32, PropModeReplace,
                    (unsigned char *)&s, 2);
}

bool YFrameClient::getWinHintsHint(long *state) {
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop;

    if (XGetWindowProperty(app->display(),
                           handle(),
                           atoms.winHints,
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

    XChangeProperty(app->display(),
                    handle(),
                    atoms.winHints,
                    XA_CARDINAL,
                    32, PropModeReplace,
                    (unsigned char *)&s, 1);
}

#ifdef CONFIG_WM_SESSION
void YFrameClient::getPidHint() {
    Atom r_type; int r_format;
    unsigned long count, bytes_remain;
    unsigned char *prop;

    fPid = PID_MAX;

    if (XGetWindowProperty(app->display(), handle(),
    			   XA_ICEWM_PID, 0, 1, False, XA_CARDINAL,
                           &r_type, &r_format, &count, &bytes_remain,
			   &prop) == Success && prop) {
	if (r_type == XA_CARDINAL && r_format == 32 && count == 1U) 
	    fPid = *((pid_t*)prop);

        XFree(prop);
	return;
    }
    
    warn(_("Window %p has no XA_ICEWM_PID property. "
	   "Export the LD_PRELOAD variable to preload the preice library."),
	   handle());
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
                           atoms.wmClientLeader,
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
                           atoms.smClientId,
                           0, 256, False, XA_STRING,
                           &r_type, &r_format,
                           &count, &bytes_remain, (unsigned char **)&cid) == Success && cid)
    {
        if (r_type == XA_STRING && r_format == 8) {
            //msg("cid=%s", cid);
        } else {
            XFree(cid);
            cid = 0;
        }
    }
    return cid;
}

void YFrameClient::deleteContext() {
    XDeleteContext(app->display(), handle(),
                   getFrame() ? YWindowManager::frameContext
                              : YWindowManager::clientContext);
}

void YFrameClient::saveContext() {
    XSaveContext(app->display(), handle(),
                 getFrame() ? YWindowManager::frameContext
                            : YWindowManager::clientContext,
                 getFrame() ? (XPointer) getFrame()
                            : (XPointer) this);
}
