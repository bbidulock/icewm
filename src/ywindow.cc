/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "ywindow.h"
#include "ykey.h"

#include "ytooltip.h"
#include "yxapp.h"
#include "sysdep.h"
#include "yprefs.h"
#include "yrect.h"
#include "ascii.h"

#include "ytimer.h"
#include "ypopup.h"
#include <typeinfo>

/******************************************************************************/
/******************************************************************************/

void YWindow::addIgnoreUnmap(Window /*w*/) {
    unmapCount++;
}

bool YWindow::ignoreUnmap(Window /*w*/) {
    if (unmapCount == 0)
        return false;
    unmapCount--;
    return true;
}

void YWindow::removeAllIgnoreUnmap(Window /*w*/) {
    unmapCount = 0;
}

class YAutoScroll: public YTimerListener {
public:
    YAutoScroll();
    virtual ~YAutoScroll();

    virtual bool handleTimer(YTimer *timer);

    void autoScroll(YWindow *w, bool autoScroll, const XMotionEvent *motion);

    YWindow *getWindow() const { return fWindow; }
    bool isScrolling() const { return fScrolling; }
private:
    YTimer fAutoScrollTimer;
    XMotionEvent fMotion;
    YWindow *fWindow;
    bool fScrolling;
};


YAutoScroll::YAutoScroll() :
    fAutoScrollTimer(autoScrollDelay, this, false)
{
    fWindow = 0;
    fScrolling = false;
}

YAutoScroll::~YAutoScroll() {
}

bool YAutoScroll::handleTimer(YTimer *timer) {
    if (fWindow) {
        return fWindow->handleAutoScroll(fMotion);
    }
    return false;
}

void YAutoScroll::autoScroll(YWindow *w, bool autoScroll, const XMotionEvent *motion) {
    if (motion)
        fMotion = *motion;
    else
        w = 0;
    fWindow = w;
    if (w == 0)
        autoScroll = false;
    fScrolling = autoScroll;
    if (autoScroll) {
        fAutoScrollTimer.startTimer();
    } else {
        fAutoScrollTimer.stopTimer();
    }
}

/******************************************************************************/
/******************************************************************************/

extern XContext windowContext;
YAutoScroll *YWindow::fAutoScroll = 0;
YWindow *YWindow::fClickWindow = 0;
Time YWindow::fClickTime = 0;
int YWindow::fClickCount = 0;
XButtonEvent YWindow::fClickEvent;
int YWindow::fClickDrag = 0;
unsigned int YWindow::fClickButton = 0;
unsigned int YWindow::fClickButtonDown = 0;

unsigned long YWindow::lastEnterNotifySerial; // credits to ahwm
unsigned long YWindow::getLastEnterNotifySerial() {
    return lastEnterNotifySerial;
}
void YWindow::updateEnterNotifySerial(const XEvent &event) {
    lastEnterNotifySerial = event.xany.serial;
    xapp->sync();
}

/******************************************************************************/
/******************************************************************************/

YWindow::YWindow(YWindow *parent, Window win, int depth, Visual *visual):
    fDepth(depth), fVisual(visual), fAllocColormap(None),
    fParentWindow(parent),
    fFocusedWindow(0),

    fHandle(win), flags(0), fStyle(0), fX(0), fY(0), fWidth(1), fHeight(1),
    fPointer(), unmapCount(0),
    fGraphics(0),
    fEventMask(KeyPressMask|KeyReleaseMask|FocusChangeMask|
               LeaveWindowMask|EnterWindowMask),
    fWinGravity(NorthWestGravity), fBitGravity(ForgetGravity),
    fEnabled(true), fToplevel(false),
    fDoubleBuffer(doubleBuffer),
    accel(0),
    fToolTip(0),
    fDND(false), XdndDragSource(None), XdndDropTarget(None)
{
    if (fHandle != None) {
        MSG(("adopting window %lX", fHandle));
        flags |= wfAdopted;
        create();
    } else if (fParentWindow == 0) {
        PRECONDITION(desktop != 0);
        fParentWindow = desktop;
    }
    insertWindow();
}

YWindow::~YWindow() {
    if (fAutoScroll &&
        fAutoScroll->isScrolling() &&
        fAutoScroll->getWindow() == this)
        fAutoScroll->autoScroll(0, false, 0);
    fFocusedWindow = 0;
    removeWindow();
    while (nextWindow() != 0)
           nextWindow()->removeWindow();
    while (accel) {
        YAccelerator *next = accel->next;
        delete accel;
        accel = next;
    }
    if (fToolTip) {
        fToolTip->hide();
        if (fToolTipTimer)
            fToolTipTimer->disableTimerListener(fToolTip);
        delete fToolTip; fToolTip = 0;
    }
    if (fClickWindow == this)
        fClickWindow = 0;
    if (fGraphics) {
        delete fGraphics; fGraphics = 0;
    }
    if (flags & wfCreated)
        destroy();
    if (fAllocColormap) {
        XFreeColormap(xapp->display(), fAllocColormap);
        fAllocColormap = None;
    }
}

void YWindow::setWindowFocus() {
    XSetInputFocus(xapp->display(), handle(), RevertToNone, CurrentTime);
}

void YWindow::setTitle(char const * title) {
    XStoreName(xapp->display(), handle(), title);
}

void YWindow::setClassHint(char const * rName, char const * rClass) {
    XClassHint wmclass;
    wmclass.res_name = (char *) rName;
    wmclass.res_class = (char *) rClass;

    XSetClassHint(xapp->display(), handle(), &wmclass);
}

void YWindow::setStyle(unsigned aStyle) {
    if (fStyle != aStyle) {
        fStyle = aStyle;

        if (flags & wfCreated) {
            if (fStyle & wsToolTip)
                fEventMask = ExposureMask;

            if (fStyle & wsPointerMotion)
                fEventMask |= PointerMotionMask;


            if (hasbit(fStyle, wsDesktopAware | wsManager) ||
                (fHandle != xapp->root()))
                fEventMask |=
                    StructureNotifyMask |
                    ColormapChangeMask |
                    PropertyChangeMask;

            if (fStyle & wsManager)
                fEventMask |=
                    FocusChangeMask |
                    SubstructureRedirectMask | SubstructureNotifyMask;

            fEventMask |= ButtonPressMask | ButtonReleaseMask | ButtonMotionMask;
            if (fHandle == xapp->root())
                if (!(fStyle & wsManager) || !grabRootWindow)
                    fEventMask &= ~(ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);

            XSelectInput(xapp->display(), fHandle, fEventMask);
        }
    }
}

void YWindow::addEventMask(long mask) {
    if (hasbits(fEventMask, mask) == false) {
        fEventMask |= mask;
        if (flags & wfCreated)
            XSelectInput(xapp->display(), fHandle, fEventMask);
    }
}

Graphics &YWindow::getGraphics() {
    return *(NULL == fGraphics ? fGraphics = new Graphics(*this) : fGraphics);
}

void YWindow::repaint() {
    XClearArea(xapp->display(), handle(), 0, 0, 0, 0, True);
}

void YWindow::repaintSync() { // useful when server grabbed
    if ((flags & (wfCreated | wfVisible)) == (wfCreated | wfVisible)) {
        Graphics &g = getGraphics();
        YRect r1(0, 0, width(), height());
        ref<YPixmap> pixmap = beginPaint(r1);
        Graphics g1(pixmap, 0, 0);
        paint(g1, r1);
        endPaint(g, pixmap, r1);
    }
}

void YWindow::repaintFocus() {
    repaint();
///    if ((flags & (wfCreated | wfVisible)) == (wfCreated | wfVisible)) {
///        paintFocus(getGraphics(), YRect(0, 0, width(), height()));
///    }
}

bool YWindow::getWindowAttributes(XWindowAttributes* attr) {
    if (fHandle == None)
        return false;

    if (XGetWindowAttributes(xapp->display(), fHandle, attr))
        return true;

    flags |= wfDestroyed;
    return false;
}

void YWindow::readAttributes() {
    XWindowAttributes attributes;

    if (!getWindowAttributes(&attributes))
        return;

    fX = attributes.x;
    fY = attributes.y;
    fWidth = attributes.width;
    fHeight = attributes.height;

    //MSG(("window initial geometry (%d:%d %dx%d)",
    //     fX, fY, fWidth, fHeight));

    if (attributes.map_state != IsUnmapped)
        flags |= wfVisible;
    else
        flags &= ~wfVisible;
}

Window YWindow::create() {
    if (flags & wfCreated) return fHandle;

    if (fHandle == None) {
        XSetWindowAttributes attributes;
        unsigned int attrmask = CWEventMask;

        fEventMask |=
            ExposureMask |
            ButtonPressMask | ButtonReleaseMask | ButtonMotionMask;

        if (fStyle & wsToolTip)
            fEventMask = ExposureMask;

        if (fStyle & wsPointerMotion)
            fEventMask |= PointerMotionMask;

        if (fParentWindow == desktop && !(fStyle & wsOverrideRedirect))
            fEventMask |= StructureNotifyMask | SubstructureRedirectMask;
        if (fStyle & wsManager)
            fEventMask |= SubstructureRedirectMask | SubstructureNotifyMask;

        if (fStyle & wsSaveUnder) {
            attributes.save_under = True;
            attrmask |= CWSaveUnder;
        }
        if (fStyle & wsOverrideRedirect) {
            attributes.override_redirect = True;
            attrmask |= CWOverrideRedirect;
        }
        if (fPointer.handle() != None) {
            attrmask |= CWCursor;
            attributes.cursor = fPointer.handle();
        }
        if (fBitGravity != ForgetGravity) {
            attributes.bit_gravity = fBitGravity;
            attrmask |= CWBitGravity;
        }
        if (fWinGravity != NorthWestGravity) {
            attributes.win_gravity = fWinGravity;
            attrmask |= CWWinGravity;
        }
        if (fVisual != CopyFromParent) {
            fAllocColormap = XCreateColormap(xapp->display(), desktop->handle(), fVisual, AllocNone);
            attributes.colormap = fAllocColormap;
            attrmask |= CWColormap;
        }
        if (fDepth != CopyFromParent) {
            attributes.background_pixel = xapp->black();
            attrmask |= CWBackPixel;
            attributes.border_pixel = xapp->black();
            attrmask |= CWBorderPixel;
        }

        attributes.event_mask = fEventMask;
        int zw = width();
        int zh = height();
        if (zw == 0 || zh == 0) {
            zw = 1;
            zh = 1;
            flags |= wfNullSize;
        }
        fHandle = XCreateWindow(xapp->display(),
                                parent()->handle(),
                                x(), y(), zw, zh,
                                0,
                                fDepth,
                                (fStyle & wsInputOnly) ? InputOnly : InputOutput,
                                fVisual,
                                attrmask,
                                &attributes);

        XWindowAttributes wa;
        XGetWindowAttributes(xapp->display(), fHandle, &wa);
        fDepth = wa.depth;
        fVisual = wa.visual;
        if (parent() == desktop &&
            !(flags & (wsManager | wsOverrideRedirect)))
            XSetWMProtocols(xapp->display(), fHandle, &_XA_WM_DELETE_WINDOW, 1);

        if ((flags & wfVisible) && !(flags & wfNullSize))
            XMapWindow(xapp->display(), fHandle);
    } else {
        readAttributes();

        fEventMask = 0;

        if ((fStyle & wsDesktopAware) || (fStyle & wsManager) ||
            (fHandle != xapp->root()))
            fEventMask |=
                StructureNotifyMask |
                ColormapChangeMask |
                PropertyChangeMask;

        if (fStyle & wsManager) {
            fEventMask |=
                FocusChangeMask |
                SubstructureRedirectMask | SubstructureNotifyMask |
                ButtonPressMask | ButtonReleaseMask | ButtonMotionMask;


            if (!grabRootWindow &&
                fHandle == xapp->root())
                fEventMask &= ~(ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);
        }

        XSelectInput(xapp->display(), fHandle, fEventMask);
    }
    XSaveContext(xapp->display(), fHandle, windowContext, (XPointer)this);
    flags |= wfCreated;
    return fHandle;
}

void YWindow::destroy() {
    if (flags & wfCreated) {
        if (!(flags & wfDestroyed)) {
            if (!(flags & wfAdopted)) {
                MSG(("----------------------destroy %lX", fHandle));
                XDestroyWindow(xapp->display(), fHandle);
                removeAllIgnoreUnmap(fHandle);
            } else {
                XSelectInput(xapp->display(), fHandle, NoEventMask);
            }
            flags |= wfDestroyed;
        }
        XDeleteContext(xapp->display(), fHandle, windowContext);
        fHandle = None;
        flags &= ~wfCreated;
    }
}
void YWindow::removeWindow() {
    if (fParentWindow) {
        fParentWindow->remove(this);
        fParentWindow = 0;
    }
}

void YWindow::insertWindow() {
    if (fParentWindow) {
        fParentWindow->prepend(this);
    }
}

void YWindow::reparent(YWindow *parent, int x, int y) {
    // ensure window was created before reparenting
    (void) handle();
    if (flags & wfVisible) {
        addIgnoreUnmap(handle());
    }

    removeWindow();
    fParentWindow = parent;
    insertWindow();

    MSG(("-----------  reparent %lX to %lX", handle(), parent->handle()));
    XReparentWindow(xapp->display(),
                    handle(),
                    parent->handle(),
                    x, y);
    fX = x;
    fY = y;
}

void YWindow::show() {
    if (!(flags & (wfVisible | wfDestroyed))) {
        flags |= wfVisible;
        if (!(flags & wfNullSize))
            XMapWindow(xapp->display(), handle());
    }
}

void YWindow::hide() {
    if (flags & wfVisible) {
        flags &= ~wfVisible;
        if (!(flags & (wfNullSize | wfDestroyed))) {
            addIgnoreUnmap(handle());
            XUnmapWindow(xapp->display(), handle());
        }
    }
}

void YWindow::setVisible(bool enable) {
    return enable ? show() : hide();
}

void YWindow::setWinGravity(int gravity) {
    if (flags & wfCreated) {
        unsigned long eventmask = CWWinGravity;
        XSetWindowAttributes attributes;
        attributes.win_gravity = gravity;
        fWinGravity = gravity;

        XChangeWindowAttributes(xapp->display(), handle(), eventmask, &attributes);
    } else {
        fWinGravity = gravity;
    }
}

void YWindow::setBitGravity(int gravity) {
    if (flags & wfCreated) {
        unsigned long eventmask = CWBitGravity;
        XSetWindowAttributes attributes;
        attributes.bit_gravity = gravity;
        fBitGravity = gravity;

        XChangeWindowAttributes(xapp->display(), handle(), eventmask, &attributes);
    } else {
        fBitGravity = gravity;
    }
}

void YWindow::raise() {
    XRaiseWindow(xapp->display(), handle());
}

void YWindow::lower() {
    XLowerWindow(xapp->display(), handle());
}

void YWindow::handleEvent(const XEvent &event) {
    switch (event.type) {
    case KeyPress:
    case KeyRelease:
        {
            for (YWindow *w = this; // !!! hack, fix
                 w && w->handleKey(event.xkey) == false;
                 w = w->parent()) {}
        }
        break;

    case ButtonPress:
        captureEvents();
        handleButton(event.xbutton);
        break;

    case ButtonRelease:
        releaseEvents();
        handleButton(event.xbutton);
        break;

    case MotionNotify:
         {
             XEvent new_event, old_event;

             old_event = event;
             while (/*XPending(app->display()) > 0 &&*/
                    XCheckMaskEvent(xapp->display(),
                                 KeyPressMask |
                                 KeyReleaseMask |
                                 ButtonPressMask |
                                 ButtonReleaseMask |
                                 ButtonMotionMask,
                                 &new_event) == True)
             {
                 if (event.type != new_event.type ||
                     event.xmotion.window != new_event.xmotion.window)
                 {
                     XPutBackEvent(xapp->display(), &new_event);
                     break;
                 } else {
                     XSync(xapp->display(), False);
                     old_event = new_event;
                 }
             }
             handleMotion(old_event.xmotion);
         }
         break;

    case EnterNotify:
    case LeaveNotify:
        handleCrossing(event.xcrossing);
        break;

    case FocusIn:
    case FocusOut:
        handleFocus(event.xfocus);
        break;

    case PropertyNotify:
        handleProperty(event.xproperty);
        break;

    case ColormapNotify:
        handleColormap(event.xcolormap);
        break;

    case MapRequest:
        handleMapRequest(event.xmaprequest);
        break;

    case ReparentNotify:
        handleReparentNotify(event.xreparent);
        break;

    case ConfigureNotify:
        updateEnterNotifySerial(event);
#if 1
         {
             XEvent new_event, old_event;

             old_event = event;
             while (/*XPending(app->display()) > 0 &&*/
                    XCheckTypedWindowEvent(xapp->display(), handle(), ConfigureNotify,
                                 &new_event) == True)
             {
                 if (event.type != new_event.type ||
                     event.xconfigure.window != new_event.xconfigure.window)
                 {
                     XPutBackEvent(xapp->display(), &new_event);
                     break;
                 } else {
                     XFlush(xapp->display());
                     old_event = new_event;
                 }
             }
             handleConfigure(old_event.xconfigure);
         }
#else
         handleConfigure(event.xconfigure);
#endif
        break;

    case ConfigureRequest:
        handleConfigureRequest(event.xconfigurerequest);
        break;

    case DestroyNotify:
        handleDestroyWindow(event.xdestroywindow);
        break;

    case Expose:
        handleExpose(event.xexpose);
        break;

    case GraphicsExpose:
        handleGraphicsExpose(event.xgraphicsexpose);
        break;

    case MapNotify:
        updateEnterNotifySerial(event);
        handleMapNotify(event.xmap);
        break;

    case UnmapNotify:
        updateEnterNotifySerial(event);
        handleUnmapNotify(event.xunmap);
        break;

    case ClientMessage:
        handleClientMessage(event.xclient);
        break;

    case SelectionClear:
        handleSelectionClear(event.xselectionclear);
        break;

    case SelectionRequest:
        handleSelectionRequest(event.xselectionrequest);
        break;

    case SelectionNotify:
        handleSelection(event.xselection);
        break;

    case GravityNotify:
        updateEnterNotifySerial(event);
        handleGravityNotify(event.xgravity);
        break;

    case CirculateNotify:
        updateEnterNotifySerial(event);
        break;

    case VisibilityNotify:
        handleVisibility(event.xvisibility);
        break;

    default:
#ifdef CONFIG_SHAPE
        if (shapesSupported && event.type == (shapeEventBase + ShapeNotify)) {
            handleShapeNotify(*(const XShapeEvent *)&event);
            break;
        }
#endif
#ifdef CONFIG_XRANDR
        //msg("event.type=%d %d %d", event.type, xrandrEventBase, xrandrSupported);
        if (xrandrSupported && event.type == (xrandrEventBase + 0)) // XRRScreenChangeNotify
            handleRRScreenChangeNotify(*(const XRRScreenChangeNotifyEvent *)&event);
#endif
        break;
    }
}

ref<YPixmap> YWindow::beginPaint(YRect &r) {
    //    return new YPixmap(width(), height());
    ref<YPixmap> pix = YPixmap::create(r.width(), r.height(), depth());
    return pix;
}

void YWindow::endPaint(Graphics &g, ref<YPixmap> pixmap, YRect &r) {
    if (pixmap != null) {
        g.copyPixmap(pixmap,
                     0, 0, /*r.x(), r.y(),*/ r.width(), r.height(),
                     r.x(), r.y());
    }
}

void YWindow::setDoubleBuffer(bool doubleBuffer) {
    fDoubleBuffer = doubleBuffer;
}

/// TODO #warning "implement expose compression"
void YWindow::paintExpose(int ex, int ey, int ew, int eh) {
    Graphics &g = getGraphics();
    XRectangle r;

    r.x = ex;
    r.y = ey;
    r.width = ew;
    r.height = eh;

    g.setClipRectangles(&r, 1);

    const int ee = 0;

    if (ex < ee) {
        ew += ex;
        ex = 0;
    } else {
        ex -= ee;
        ew += ee;
    }
    if (ey < ee) {
        eh += ey;
        ey = 0;
    } else {
        ey -= ee;
        eh += ee;
    }
    if (ex + ew < (int) width()) {
        ew += ee;
    } else {
        ew = width() - ex;
    }
    if (ey + eh + ee < (int) height()) {
        eh += ee;
    } else {
        eh = int(height()) - ey;
    }

    if (ew > 0 && eh > 0) {
        YRect r1(ex, ey, ew, eh);
        if (fDoubleBuffer) {
            ref<YPixmap> pixmap = beginPaint(r1);
            Graphics g1(pixmap, ex, ey);
            //MSG(("paint %d %d %d %d", ex, ey, ew, eh));
            paint(g1, r1);
            endPaint(g, pixmap, r1);
        } else {
            paint(g, r1);
        }
    }
    g.resetClip();

    //XSetClipMask(xapp->display(), g.handle(), None);
    ///XFlush(app->display());
}

void YWindow::handleExpose(const XExposeEvent &expose) {
    paintExpose(expose.x, expose.y, expose.width, expose.height);
}

void YWindow::handleGraphicsExpose(const XGraphicsExposeEvent &graphicsExpose) {
    paintExpose(graphicsExpose.x,
                graphicsExpose.y,
                graphicsExpose.width,
                graphicsExpose.height);
}

void YWindow::handleConfigure(const XConfigureEvent &configure) {
    if (configure.window == handle()) {
        if (configure.x != fX ||
            configure.y != fY ||
            (unsigned) configure.width != fWidth ||
            (unsigned) configure.height != fHeight)
        {
            fX = configure.x;
            fY = configure.y;
            fWidth = configure.width;
            fHeight = configure.height;

            this->configure(geometry());
        }
    }
}

void YWindow::handleGravityNotify(const XGravityEvent& gravity) {
    if (gravity.window == handle()) {
        if (gravity.x != fX || gravity.y != fY) {
        }
    }
}

bool YWindow::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        KeySym k = keyCodeToKeySym(key.keycode);
        unsigned int m = KEY_MODMASK(key.state);

        if (accel) {
            YAccelerator *a;

            for (a = accel; a; a = a->next) {
                //msg("%c %d - %c %d %d", k, k, a->key, a->key, a->mod);
                if (m == a->mod && k == a->key)
                    if (a->win->handleKey(key) == true)
                        return true;
            }
            if (ASCII::isLower((char)k)) {
                k = ASCII::toUpper((char)k);
                for (a = accel; a; a = a->next)
                    if (m == a->mod && k == a->key)
                        if (a->win->handleKey(key) == true)
                            return true;
            }
        }
        if (k == XK_Tab) {
            if (m == 0) {
                nextFocus();
                return true;
            } else if (m == ShiftMask) {
                prevFocus();
                return true;
            }
        }
    }
    return false;
}

void YWindow::handleButton(const XButtonEvent &button) {
    if (fToolTip) {
        fToolTip->hide();
        if (fToolTipTimer)
            fToolTipTimer->disableTimerListener(fToolTip);
    }

    int const dx(abs(button.x_root - fClickEvent.x_root));
    int const dy(abs(button.y_root - fClickEvent.y_root));
    int const motionDelta(max(dx, dy));

    if (button.type == ButtonPress) {
        fClickDrag = 0;

        if (fClickWindow != this) {
            fClickWindow = this;
            fClickCount = 1;
        } else {
            if ((button.time - fClickTime < (unsigned) MultiClickTime) &&
                fClickButton == button.button &&
                motionDelta <= ClickMotionDistance &&
                button.x >= 0 && button.y >= 0 &&
                button.x < int(width()) && button.y < int(height()))
            {
                fClickCount++;
                handleClickDown(button, fClickCount);
            } else
                fClickCount = 1;
        }
        fClickEvent = button;
        fClickButton = fClickButtonDown = button.button;
        fClickTime = button.time;
    } else if (button.type == ButtonRelease) {
        if ((fClickWindow == this) &&
            !fClickDrag &&
            fClickCount > 0 &&
            fClickButtonDown == button.button &&
            motionDelta <= ClickMotionDistance &&
            button.x >= 0 && button.y >= 0 &&
            button.x < int(width()) && button.y < int(height()))
        {
            fClickButtonDown = 0;
            handleClick(button, fClickCount);
        } else {
            fClickWindow = 0;
            fClickCount = 1;
            fClickButtonDown = 0;
            fClickButton = 0;
            if (fClickDrag)
                handleEndDrag(fClickEvent, button);
        }
    }
}

void YWindow::handleMotion(const XMotionEvent &motion) {
    if (fClickButtonDown) {
        if (fClickDrag) {
            handleDrag(fClickEvent, motion);
        } else {
            int const dx(abs(motion.x_root - fClickEvent.x_root));
            int const dy(abs(motion.y_root - fClickEvent.y_root));
            int const motionDelta(max(dx, dy));
            int curButtons = 0;

            curButtons =
                ((motion.state & Button1Mask) ? (1 << 1) : 0) |
                ((motion.state & Button2Mask) ? (1 << 2) : 0) |
                ((motion.state & Button3Mask) ? (1 << 3) : 0) |
                ((motion.state & Button4Mask) ? (1 << 4) : 0) |
                ((motion.state & Button5Mask) ? (1 << 5) : 0);

            if (((motion.time - fClickTime > (unsigned) ClickMotionDelay) ||
                (motionDelta >= ClickMotionDistance)) &&
                ((1 << fClickButton) == curButtons)
               )
            {
                //msg("start drag %d %d %d", curButtons, fClickButton, motion.state);
                fClickDrag = 1;
                handleBeginDrag(fClickEvent, motion);
            }
        }
    }
}

lazy<YTimer> YWindow::fToolTipTimer;

void YWindow::setToolTip(const ustring &tip) {
    if (fToolTip) {
        if (tip == null) {
            delete fToolTip; fToolTip = 0;
        } else {
            fToolTip->setText(tip);
            fToolTip->repaint();
        }
    }
    else if (tip != null) {
        fToolTip = new YToolTip();
        fToolTip->setText(tip);
    }
}

bool YWindow::toolTipVisible() {
    return (fToolTip && fToolTip->visible());
}

void YWindow::updateToolTip() {
}

void YWindow::handleCrossing(const XCrossingEvent &crossing) {
    if (fToolTip) {
        if (crossing.type == EnterNotify && crossing.mode == NotifyNormal) {
            fToolTipTimer->setTimer(ToolTipDelay, fToolTip, true);
            updateToolTip();
            fToolTip->locate(this, crossing);
        } else if (crossing.type == LeaveNotify) {
            fToolTip->hide();
            if (fToolTipTimer)
                fToolTipTimer->disableTimerListener(fToolTip);
        }
    }
}

void YWindow::handleClientMessage(const XClientMessageEvent &message) {
    if (message.message_type == _XA_WM_PROTOCOLS
        && message.format == 32
        && message.data.l[0] == (long)_XA_WM_DELETE_WINDOW)
    {
        handleClose();
    } else if (message.message_type == _XA_WM_PROTOCOLS
        && message.format == 32
        && message.data.l[0] == (long)_XA_WM_TAKE_FOCUS)
    {
        gotFocus();
#if 0
        YWindow *w = getFocusWindow();
        if (w)
            w->gotFocus();
        else
            gotFocus();
#endif
    } else if (message.message_type == XA_XdndEnter ||
               message.message_type == XA_XdndLeave ||
               message.message_type == XA_XdndPosition ||
               message.message_type == XA_XdndStatus ||
               message.message_type == XA_XdndDrop ||
               message.message_type == XA_XdndFinished)
    {
        handleXdnd(message);
    }
}

void YWindow::handleVisibility(const XVisibilityEvent& visibility) {
}

#if 0
    virtual void handleCreateWindow(const XCreateWindowEvent &createWindow);
#endif

void YWindow::handleMapNotify(const XMapEvent &) {
    // ignore "map notify" not implemented or needed due to MapRequest event
}

void YWindow::handleUnmapNotify(const XUnmapEvent &xunmap) {
    if (xunmap.window == xunmap.event || xunmap.send_event) {
        if (!ignoreUnmap(xunmap.window)) {
            flags &= ~wfVisible;
            handleUnmap(xunmap);
        }
    }
}

void YWindow::handleUnmap(const XUnmapEvent &) {
}

void YWindow::handleConfigureRequest(const XConfigureRequestEvent&) {
}

void YWindow::handleMapRequest(const XMapRequestEvent&) {
}

void YWindow::handleDestroyWindow(const XDestroyWindowEvent &destroyWindow) {
    if (destroyWindow.window == fHandle) {
        flags |= wfDestroyed;
        removeAllIgnoreUnmap(destroyWindow.window);
    }
}

void YWindow::paint(Graphics &g, const YRect &r) {
    g.fillRect(r.x(), r.y(), r.width(), r.height());
}

bool YWindow::nullGeometry() {
    bool zero = (fWidth == 0 || fHeight == 0);

    if (zero && !(flags & wfNullSize)) {
        flags |= wfNullSize;
        if (flags & wfVisible) {
            addIgnoreUnmap(handle());
            XUnmapWindow(xapp->display(), handle());
        }
    } else if ((flags & wfNullSize) && !zero) {
        flags &= ~wfNullSize;
        if (flags & wfVisible)
            XMapWindow(xapp->display(), handle());
    }
    return zero;
}

void YWindow::setGeometry(const YRect &r) {
    const bool resized = (r.width() != fWidth || r.height() != fHeight);

    if (r.x() != fX || r.y() != fY || resized) {
        fX = r.x();
        fY = r.y();
        fWidth = r.width();
        fHeight = r.height();

        if (flags & wfCreated) {
            if (!nullGeometry())
                XMoveResizeWindow(xapp->display(),
                                  fHandle,
                                  fX, fY, fWidth, fHeight);
        }

        configure(geometry());
    }
}

void YWindow::setPosition(int x, int y) {
    if (x != fX || y != fY) {
        fX = x;
        fY = y;

        if (flags & wfCreated)
            XMoveWindow(xapp->display(), fHandle, fX, fY);

        configure(geometry());
    }
}

void YWindow::setSize(unsigned width, unsigned height) {
    if (width != fWidth || height != fHeight) {
        fWidth = width;
        fHeight = height;

        if (flags & wfCreated)
            if (!nullGeometry())
                XResizeWindow(xapp->display(), fHandle, fWidth, fHeight);

        configure(geometry());
    }
}

void YWindow::setBorderWidth(unsigned width) {
    XSetWindowBorderWidth(xapp->display(), handle(), width);
}

void YWindow::setBackground(unsigned long pixel) {
    XSetWindowBackground(xapp->display(), handle(), pixel);
}

void YWindow::setBackgroundPixmap(Pixmap pixmap) {
    XSetWindowBackgroundPixmap(xapp->display(), handle(), pixmap);
}

void YWindow::setParentRelative(void) {
    setBackgroundPixmap(ParentRelative);
}

void YWindow::mapToGlobal(int &x, int &y) {
    int dx, dy;
    Window child;

    XTranslateCoordinates(xapp->display(),
                          handle(),
                          desktop->handle(),
                          x, y,
                          &dx, &dy, &child);
    x = dx;
    y = dy;
}

void YWindow::mapToLocal(int &x, int &y) {
    int dx, dy;
    Window child;

    XTranslateCoordinates(xapp->display(),
                          desktop->handle(),
                          handle(),
                          x, y,
                          &dx, &dy, &child);
    x = dx;
    y = dy;
}

void YWindow::configure(const YRect &/*r*/)
{
}

void YWindow::setPointer(const YCursor& pointer) {
    fPointer = pointer;

    if (flags & wfCreated) {
        XSetWindowAttributes attributes;
        attributes.cursor = fPointer.handle();
        XChangeWindowAttributes(xapp->display(), handle(),
                                CWCursor, &attributes);
    }
}

void YWindow::setGrabPointer(const YCursor& pointer) {
    XChangeActivePointerGrab(xapp->display(),
                             ButtonPressMask|PointerMotionMask|
                             ButtonReleaseMask,
                             pointer.handle(), CurrentTime);
                             //app->getEventTime());
}

void YWindow::grabKeyM(int keycode, unsigned int modifiers) {
    MSG(("grabKey %d %d %s", keycode, modifiers,
         XKeysymToString(keyCodeToKeySym(keycode))));

    XGrabKey(xapp->display(), keycode, modifiers, handle(), False,
             GrabModeAsync, GrabModeAsync);
}

void YWindow::grabKey(int key, unsigned int modifiers) {
    KeyCode keycode = XKeysymToKeycode(xapp->display(), key);
    if (keycode != 0) {
        grabKeyM(keycode, modifiers);
        if (modifiers != AnyModifier) {
            grabKeyM(keycode, modifiers | LockMask);
            if (xapp->NumLockMask != 0) {
                grabKeyM(keycode, modifiers | xapp->NumLockMask);
                grabKeyM(keycode, modifiers | xapp->NumLockMask | LockMask);
            }
        }
    }
}

void YWindow::grabButtonM(int button, unsigned int modifiers) {
    XGrabButton(xapp->display(), button, modifiers,
                handle(), True, ButtonPressMask,
                GrabModeAsync, GrabModeAsync, None, None);
}

void YWindow::grabButton(int button, unsigned int modifiers) {
    grabButtonM(button, modifiers);
    if (modifiers != AnyModifier) {
        grabButtonM(button, modifiers | LockMask);
        if (xapp->NumLockMask != 0) {
            grabButtonM(button, modifiers | xapp->NumLockMask);
            grabButtonM(button, modifiers | xapp->NumLockMask | LockMask);
        }
    }
}

void YWindow::captureEvents() {
    xapp->captureGrabEvents(this);
}

void YWindow::releaseEvents() {
    xapp->releaseGrabEvents(this);
}

void YWindow::donePopup(YPopupWindow * /*command*/) {
}

void YWindow::handleClose() {
}

void YWindow::setEnabled(bool enable) {
    if (enable != fEnabled) {
        fEnabled = enable;
        repaint();
    }
}

void YWindow::handleFocus(const XFocusChangeEvent &xfocus) {
    if (isToplevel()) {
        if (xfocus.type == FocusIn) {
            gotFocus();
        } else if (xfocus.type == FocusOut) {
            lostFocus();
        }
    }
}

bool YWindow::isFocusTraversable() {
    return false;
}

bool YWindow::isFocused() {
    return (flags & wfFocused) != 0;
#if 0
    if (parent() == 0)
        return true;
    else if (isToplevel())
        return (flags & wfFocused) != 0;
    else
        return (parent()->fFocusedWindow == this) && parent()->isFocused();
#endif
}

void YWindow::requestFocus(bool requestUserFocus) {
//    if (!toplevel())
//        return ;

//    setFocus(0);///!!! is this the right place?
    if (isToplevel()) {
        if (visible() && requestUserFocus)
            setWindowFocus();
    } else {
        if (parent()) {
            parent()->requestFocus(requestUserFocus);
            parent()->setFocus(this);
        }
    }
}


YWindow *YWindow::toplevel() {
    YWindow *w = this;

    while (w) {
        if (w->isToplevel() == true)
            return w;
        w = w->fParentWindow;
    }
    return 0;
}

void YWindow::nextFocus() {
    YWindow *t = toplevel();

    if (t)
        t->changeFocus(true);
}

void YWindow::prevFocus() {
    YWindow *t = toplevel();

    if (t)
        t->changeFocus(false);
}

YWindow *YWindow::getFocusWindow() {
    YWindow *w = this;

    while (w) {
        if (w->fFocusedWindow)
            w = w->fFocusedWindow;
        else
            break;
    }
    return w;
}

bool YWindow::changeFocus(bool next) {
    YWindow *cur = getFocusWindow();

    if (cur == 0) {
        if (next)
            cur = lastWindow();
        else
            cur = firstWindow();
    }

    YWindow *org = cur;
    if (cur) do {
        ///!!! need focus ordering

        if (next) {
            if (cur->lastWindow())
                cur = cur->lastWindow();
            else if (cur->prevWindow())
                cur = cur->prevWindow();
            else if (cur->isToplevel())
            {}
            else {
                while (cur->parent()) {
                    cur = cur->parent();
                    if (cur->isToplevel())
                        break;
                    if (cur->prevWindow()) {
                        cur = cur->prevWindow();
                        break;
                    }
                }
            }
        } else {
            // is reverse tabbing of nested windows correct?
            if (cur->firstWindow())
                cur = cur->firstWindow();
            else if (cur->nextWindow())
                cur = cur->nextWindow();
            else if (cur->isToplevel())
                /**/;
            else {
                while (cur->parent()) {
                    cur = cur->parent();
                    if (cur->isToplevel())
                        break;
                    if (cur->nextWindow()) {
                        cur = cur->nextWindow();
                        break;
                    }
                }
            }
        }

        if (cur->isFocusTraversable()) {
            cur->requestFocus(false);
            return true;
        }
    } while (cur != org);

    return false;
}

void YWindow::setFocus(YWindow *window) {
    if (window != fFocusedWindow) {
        YWindow *oldFocus = fFocusedWindow;

        fFocusedWindow = window;

        if (focused()) {
            if (oldFocus)
                oldFocus->lostFocus();
            if (fFocusedWindow)
                fFocusedWindow->gotFocus();
        }
    }
}
void YWindow::gotFocus() {
    if (parent() == 0 || isToplevel() || parent()->focused()) {
        if (!(flags & wfFocused)) {
            flags |= wfFocused;
            repaintFocus();
            if (fFocusedWindow)
                fFocusedWindow->gotFocus();
        }
    }
}

void YWindow::lostFocus() {
    if (flags & wfFocused) {
        if (fFocusedWindow)
            fFocusedWindow->lostFocus();
        flags &= ~wfFocused;
        repaintFocus();
    }
}

void YWindow::installAccelerator(unsigned int key, unsigned int mod, YWindow *win) {
    if (key < 128)
        key = ASCII::toUpper((char)key);
    if (fToplevel || fParentWindow == 0) {
        YAccelerator **pa = &accel, *a;

        while (*pa) {
            a = *pa;
            if (a->key == key &&
                a->mod == mod &&
                a->win == win)
            {
                assert(1 == 0);
                return ;
            } else
                pa = &(a->next);
        }

        a = new YAccelerator;
        if (a == 0)
            return ;

        a->key = key;
        a->mod = mod;
        a->win = win;
        a->next = accel;
        accel = a;
    } else parent()->installAccelerator(key, mod, win);
}

void YWindow::removeAccelerator(unsigned int key, unsigned int mod, YWindow *win) {
    if (key < 128)
        key = ASCII::toUpper((char)key);
    if (fToplevel || fParentWindow == 0) {
        YAccelerator **pa = &accel, *a;

        while (*pa) {
            a = *pa;
            if (a->key == key &&
                a->mod == mod &&
                a->win == win)
            {
                *pa = a->next;
                delete a;
            } else
                pa = &(a->next);
        }
    } else parent()->removeAccelerator(key, mod, win);
}

const Atom XdndCurrentVersion = 3;

void YWindow::setDND(bool enabled) {
    if (fDND != enabled) {
        fDND = enabled;

        if (fDND) {
            XChangeProperty(xapp->display(), handle(),
                            XA_XdndAware, XA_ATOM, // !!! ATOM?
                            32, PropModeReplace,
                            (const unsigned char *)&XdndCurrentVersion, 1);
        } else {
            XDeleteProperty(xapp->display(), handle(), XA_XdndAware);
        }
    }
}

void YWindow::XdndStatus(bool acceptDrop, Atom dropAction) {
    XClientMessageEvent msg;
    int x_root = 0, y_root = 0;

    mapToGlobal(x_root, y_root);

    memset(&msg, 0, sizeof(msg));
    msg.type = ClientMessage;
    msg.display = xapp->display();
    msg.window = XdndDragSource;
    msg.message_type = XA_XdndStatus;
    msg.format = 32;
    msg.data.l[0] = handle();
    msg.data.l[1] = (acceptDrop ? 0x00000001 : 0x00000000) | 2;
    msg.data.l[2] = (x_root << 16) + y_root;
    msg.data.l[3] = (width() << 16) + height();
    msg.data.l[4] = dropAction;
    XSendEvent(xapp->display(), XdndDragSource, False, 0L, (XEvent *)&msg);
}

void YWindow::handleXdnd(const XClientMessageEvent &message) {
    if (message.message_type == XA_XdndEnter) {
        MSG(("XdndEnter source=%lX", message.data.l[0]));
        XdndDragSource = message.data.l[0];
    } else if (message.message_type == XA_XdndLeave) {
        MSG(("XdndLeave source=%lX", message.data.l[0]));
        if (XdndDropTarget) {
            YWindow *win;

            if (XFindContext(xapp->display(), XdndDropTarget, windowContext,
                             (XPointer *)(void *)&win) == 0)
                win->handleDNDLeave();
            XdndDropTarget = None;
        }
        XdndDragSource = None;
    } else if (message.message_type == XA_XdndPosition &&
               XdndDragSource != 0)
    {
        Window target, child;
        int x, y, nx, ny;
        YWindow *pwin = 0;

        XdndDragSource = message.data.l[0];
        x = int(message.data.l[2] >> 16);
        y = int(message.data.l[2] & 0xFFFF);

        target = handle();

        MSG(("XdndPosition source=%lX %d:%d time=%ld action=%ld window=%ld",
                 message.data.l[0],
                 x, y,
                 message.data.l[3],
                 message.data.l[4],
                 XdndDropTarget));


        do {
            if (XTranslateCoordinates(xapp->display(),
                                      desktop->handle(), target,
                                      x, y, &nx, &ny, &child))
            {
                if (child)
                    target = child;
            } else {
                target = 0;
                break;
            }
        } while (child);

        if (target != XdndDropTarget) {
            if (XdndDropTarget) {
                union {
                    YWindow *ptr;
                    XPointer xptr;
                } win;

                if (XFindContext(xapp->display(), XdndDropTarget, windowContext,
                                 &win.xptr) == 0)
                    win.ptr->handleDNDLeave();
            }
            XdndDropTarget = target;
            if (XdndDropTarget) {
                union {
                    YWindow *ptr;
                    XPointer xptr;
                } win;

                if (XFindContext(xapp->display(), XdndDropTarget, windowContext,
                                 &win.xptr) == 0)
                {
                    win.ptr->handleDNDEnter();
                    pwin = win.ptr;
                }
            }
        }
        if (pwin == 0 && XdndDropTarget) { // !!! optimize this
            union {
                YWindow *ptr;
                XPointer xptr;
            } win;
            if (XFindContext(xapp->display(), XdndDropTarget, windowContext,
                             &win.xptr) == 0)
            {
                pwin = win.ptr;
            } else
                pwin = 0;
        }
        if (pwin)
            pwin->handleDNDPosition(nx, ny);
        MSG(("XdndPosition %d:%d target=%ld", nx, ny, XdndDropTarget));
        XdndStatus(false, None);
        /*{
            XClientMessageEvent msg;

            msg.data.l[0] = handle();
            msg.data.l[1] = (false ? 0x00000001 : 0x00000000) | 2;
            msg.data.l[2] = 0; //(x << 16) + y;
            msg.data.l[3] = 0;//(1 << 16) + 1;
            msg.data.l[4] = None;
            XSendEvent(app->display(), XdndDragSource, True, 0L, (XEvent *)&msg);
        }*/
    } else if (message.message_type == XA_XdndStatus) {
        MSG(("XdndStatus"));
    } else if (message.message_type == XA_XdndDrop) {
        MSG(("XdndDrop"));
    } else if (message.message_type == XA_XdndFinished) {
        MSG(("XdndFinished"));
    }
}

void YWindow::handleDNDEnter() {
}

void YWindow::handleDNDLeave() {
}

void YWindow::handleDNDPosition(int /*x*/, int /*y*/) {
}

bool YWindow::handleAutoScroll(const XMotionEvent & /*mouse*/) {
    return false;
}

void YWindow::beginAutoScroll(bool doScroll, const XMotionEvent *motion) {
    if (fAutoScroll == 0)
        fAutoScroll = new YAutoScroll();
    if (fAutoScroll)
        fAutoScroll->autoScroll(this, doScroll, motion);
}

void YWindow::handleSelectionClear(const XSelectionClearEvent &/*clear*/) {
}

void YWindow::handleSelectionRequest(const XSelectionRequestEvent &/*request*/) {
}

void YWindow::handleSelection(const XSelectionEvent &/*selection*/) {
}

void YWindow::acquireSelection(bool selection) {
    Atom sel = selection ? XA_PRIMARY : _XA_CLIPBOARD;

    XSetSelectionOwner(xapp->display(), sel, handle(),
                       xapp->getEventTime("acquireSelection"));
}

void YWindow::clearSelection(bool selection) {
    Atom sel = selection ? XA_PRIMARY : _XA_CLIPBOARD;

    XSetSelectionOwner(xapp->display(), sel, None,
                       xapp->getEventTime("clearSelection"));
}

void YWindow::requestSelection(bool selection) {
    Atom sel = selection ? XA_PRIMARY : _XA_CLIPBOARD;

    XConvertSelection(xapp->display(),
                      sel, XA_STRING,
                      sel, handle(), xapp->getEventTime("requestSelection"));
}

void YWindow::handleEndPopup(YPopupWindow *popup) {
    if (parent())
        parent()->handleEndPopup(popup);
}

bool YWindow::hasPopup() {
    YPopupWindow *p = xapp->popup();
    while (p && p->prevPopup())
        p = p->prevPopup();
    if (p) {
        YWindow *w = p->owner();
        while (w) {
            if (w == this)
                return true;
            w = w->parent();
        }
    }
    return false;
}

YDesktop::YDesktop(YWindow *aParent, Window win):
    YWindow(aParent, win)
{
    desktop = this;
    setDoubleBuffer(false);
    unsigned w, h;
    updateXineramaInfo(w, h);
}

YDesktop::~YDesktop() {
    for (YWindow* w; (w = firstWindow()) != 0; delete w) {
        char* name = demangle(typeid(*w).name());
        INFO("deleting stray %s", name);
        free(name);
    }
}

void YWindow::grabVKey(int key, unsigned int vm) {
    int m = 0;

    if (vm & kfShift)
        m |= ShiftMask;
    if (vm & kfCtrl)
        m |= ControlMask;
    if (vm & kfAlt)
        m |= xapp->AltMask;
    if (vm & kfMeta)
        m |= xapp->MetaMask;
    if (vm & kfSuper)
       m |= xapp->SuperMask;
    if (vm & kfHyper)
       m |= xapp->HyperMask;
    if (vm & kfAltGr)
        m |= xapp->ModeSwitchMask;

    MSG(("grabVKey %d %d %d", key, vm, m));
    if (key != 0 && (vm == 0 || m != 0)) {
        if ((!(vm & kfMeta) || xapp->MetaMask) &&
            (!(vm & kfAlt) || xapp->AltMask) &&
           (!(vm & kfSuper) || xapp->SuperMask) &&
            (!(vm & kfHyper) || xapp->HyperMask) &&
            (!(vm & kfAltGr) || xapp->ModeSwitchMask))
        {
            grabKey(key, m);
        }

        // !!! recheck this
        if (((vm & (kfAlt | kfCtrl)) == (kfAlt | kfCtrl)) &&
            modSuperIsCtrlAlt &&
            xapp->WinMask)
        {
            m = xapp->WinMask;
            if (vm & kfShift)
                m |= ShiftMask;
            if (vm & kfSuper)
                m |= xapp->SuperMask;
            if (vm & kfHyper)
                m |= xapp->HyperMask;
            if (vm & kfAltGr)
                m |= xapp->ModeSwitchMask;
            grabKey(key, m);
        }
    }
}

void YWindow::grabVButton(int button, unsigned int vm) {
    int m = 0;

    if (vm & kfShift)
        m |= ShiftMask;
    if (vm & kfCtrl)
        m |= ControlMask;
    if (vm & kfAlt)
        m |= xapp->AltMask;
    if (vm & kfMeta)
        m |= xapp->MetaMask;
    if (vm & kfSuper)
       m |= xapp->SuperMask;
    if (vm & kfHyper)
       m |= xapp->HyperMask;
    if (vm & kfAltGr)
       m |= xapp->ModeSwitchMask;

    MSG(("grabVButton %d %d %d", button, vm, m));

    if (button != 0 && (vm == 0 || m != 0)) {
        if ((!(vm & kfMeta) || xapp->MetaMask) &&
            (!(vm & kfAlt) || xapp->AltMask) &&
           (!(vm & kfSuper) || xapp->SuperMask) &&
            (!(vm & kfHyper) || xapp->HyperMask) &&
            (!(vm & kfAltGr) || xapp->ModeSwitchMask))
        {
            grabButton(button, m);
        }

        // !!! recheck this
        if (((vm & (kfAlt | kfCtrl)) == (kfAlt | kfCtrl)) &&
            modSuperIsCtrlAlt &&
            xapp->WinMask)
        {
            m = xapp->WinMask;
            if (vm & kfShift)
                m |= ShiftMask;
            if (vm & kfSuper)
                m |= xapp->SuperMask;
            if (vm & kfHyper)
                m |= xapp->HyperMask;
            if (vm & kfAltGr)
                m |= xapp->ModeSwitchMask;
            grabButton(button, m);
        }
    }
}

unsigned int YWindow::VMod(int m) {
    int vm = 0;
    int m1 = m & ~xapp->WinMask;

    if (m & xapp->WinMask) {
        if (modSuperIsCtrlAlt) {
            vm |= kfCtrl + kfAlt;
        } else if (xapp->WinMask == xapp->SuperMask) {
            vm |= kfSuper;
        }
    }

    if (m1 & ShiftMask)
        vm |= kfShift;
    if (m1 & ControlMask)
        vm |= kfCtrl;
    if (m1 & xapp->AltMask)
        vm |= kfAlt;
    if (m1 & xapp->MetaMask)
        vm |= kfMeta;
    if (m1 & xapp->SuperMask)
       vm |= kfSuper;
    if (m1 & xapp->HyperMask)
       vm |= kfHyper;
    if (m1 & xapp->ModeSwitchMask)
       vm |= kfAltGr;

    return vm;
}

bool YWindow::getCharFromEvent(const XKeyEvent &key, char *s, int maxLen) {
    char keyBuf[16];
    KeySym ksym;
    XKeyEvent kev = key;

    // FIXME:
    int klen = XLookupString(&kev, keyBuf, sizeof(keyBuf), &ksym, NULL);
#ifndef USE_XmbLookupString
    if ((klen == 0)  && (ksym < 0x1000)) {
        klen = 1;
        keyBuf[0] = (char)(ksym & 0xFF);
    }
#endif
    if (klen >= 1 && klen < maxLen - 1) {
        memcpy(s, keyBuf, klen);
        s[klen] = '\0';
        return true;
    }
    return false;
}

void YWindow::scrollWindow(int dx, int dy) {
    if (dx == 0 && dy == 0)
        return ;

    if (dx >= int(width()) || dx <= -int(width()) ||
        dy >= int(height()) || dy <= -int(height()))
    {
        repaint();
        return ;
    }

    Graphics &g = getGraphics();
    XRectangle r[2];
    int nr = 0;

    GC scrollGC = XCreateGC(xapp->display(), handle(), 0, NULL);

    XCopyArea(xapp->display(), handle(), handle(), scrollGC,
              dx, dy, width(), height(), 0, 0);

    XFreeGC(xapp->display(), scrollGC);

    dx = - dx;
    dy = - dy;

    if (dy != 0) {
        r[nr].x = 0;
        r[nr].width = width();

        if (dy >= 0) {
            r[nr].y = 0;
            r[nr].height = dy;
        } else {
            r[nr].height = - dy;
            r[nr].y = height() - r[nr].height;
        }
        nr++;
    }
    if (dx != 0) {
        r[nr].y = 0;
        r[nr].height = height(); // !!! optimize

        if (dx >= 0) {
            r[nr].x = 0;
            r[nr].width = dx;
        } else {
            r[nr].width = - dx;
            r[nr].x = width() - r[nr].width;
        }
        nr++;
    }
    //msg("nr=%d", nr);

    g.setClipRectangles(r, nr);

    XRectangle re;
    if (nr == 1)
        re = r[0];
    else {
        re.x = 0;
        re.y = 0;
        re.width = width();
        re.height = height();
    }

    paint(g, YRect(re.x, re.y, re.width, re.height)); // !!! add flag to do minimal redraws

    g.resetClip();

    {
        XEvent e;

        while (XCheckTypedWindowEvent(xapp->display(), handle(), GraphicsExpose, &e)) {
            handleGraphicsExpose(e.xgraphicsexpose);
            if (e.xgraphicsexpose.count == 0)
                break;
        }
    }
}

void YDesktop::updateXineramaInfo(unsigned &w, unsigned &h) {
    xiInfo.clear();

#ifdef CONFIG_XRANDR
    bool gotLayout = false;
    MSG(("xrr: %d", xrandr12 ? 1 : 0));
    if (xrandr12 && !xrrDisable) {
        XRRScreenResources *xrrsr =
            XRRGetScreenResources(xapp->display(), handle());

        for (int i = 0; i < xrrsr->ncrtc; i++)
        {
            XRRCrtcInfo *ci = XRRGetCrtcInfo(xapp->display(), xrrsr, xrrsr->crtcs[i]);

            MSG(("xrr %d (%lu): %d %d %d %d", i, xrrsr->crtcs[i], ci->x, ci->y, ci->width, ci->height));

            if (!gotLayout && ci->width > 0 && ci->height > 0) {
                DesktopScreenInfo si;
                si.screen_number = xrrsr->crtcs[i];
                si.x_org = ci->x;
                si.y_org = ci->y;
                si.width = ci->width;
                si.height = ci->height;
                xiInfo.append(si);
            }
            XRRFreeCrtcInfo(ci);
        }

        MSG(("xinerama primary screen name: %s", xineramaPrimaryScreenName));
        for (int o = 0; o < xrrsr->noutput; o++) {
            XRROutputInfo *oinfo = XRRGetOutputInfo(xapp->display(), xrrsr, xrrsr->outputs[o]);

            MSG(("output: %s -> %lu", oinfo->name, oinfo->crtc));

            if (xineramaPrimaryScreenName != 0 && oinfo->name != NULL) {
                if (strcmp(xineramaPrimaryScreenName, oinfo->name) == 0)
                {
                    int s = oinfo->crtc;
                    for (int sc = 0; sc < xiInfo.getCount(); sc++) {
                         if (xiInfo[sc].screen_number == s) {
                             xineramaPrimaryScreen = o;
                             MSG(("xinerama primary screen: %s -> %d", oinfo->name, o));
                         }
                    }
                }
            }
            XRRFreeOutputInfo(oinfo);
        }
        XRRFreeScreenResources(xrrsr);
    }
#endif
    if (xiInfo.getCount() < 2) { // use xinerama if no XRANDR screens (nvidia hack)
       xiInfo.clear();
#ifdef XINERAMA
        if (XineramaIsActive(xapp->display())) {
            int nxsi;
            XineramaScreenInfo *xsi = XineramaQueryScreens(xapp->display(), &nxsi);

            MSG(("xinerama: heads=%d", nxsi));
            for (int i = 0; i < nxsi; i++) {
                MSG(("xinerama: %d +%d+%d %dx%d",
                    xsi[i].screen_number,
                    xsi[i].x_org,
                    xsi[i].y_org,
                    xsi[i].width,
                    xsi[i].height));

                DesktopScreenInfo si;
                si.screen_number = i;
                si.x_org = xsi[i].x_org;
                si.y_org = xsi[i].y_org;
                si.width = xsi[i].width;
                si.height = xsi[i].height;
                xiInfo.append(si);
            }
            if (xsi)
                    XFree(xsi);
        }
#endif
    }
    if (xiInfo.getCount() == 0) {
        DesktopScreenInfo si;
        si.screen_number = 0;
        si.x_org = 0;
        si.y_org = 0;
        si.width = xapp->displayWidth();
        si.height = xapp->displayHeight();
        xiInfo.append(si);
    }
    {
        w = xiInfo[0].x_org + xiInfo[0].width;
        h = xiInfo[0].y_org + xiInfo[0].height;
        for (int i = 0; i < xiInfo.getCount(); i++)
        {
            if (xiInfo[i].x_org + xiInfo[i].width > w)
                w = xiInfo[i].width + xiInfo[i].x_org;
            if (xiInfo[i].y_org + xiInfo[i].height > h)
                h = xiInfo[i].height + xiInfo[i].y_org;

            MSG(("screen %d (%d): %d %d %d %d", i, xiInfo[i].screen_number, xiInfo[i].x_org, xiInfo[i].y_org, xiInfo[i].width, xiInfo[i].height));
        }
    }
    MSG(("desktop screen area: %d %d", w, h));
}


void YDesktop::getScreenGeometry(int *x, int *y,
                                 unsigned *width, unsigned *height,
                                 int screen_no)
{
    if (screen_no == -1)
        screen_no = xineramaPrimaryScreen;
    if (screen_no < 0 || screen_no >= xiInfo.getCount())
        screen_no = 0;
    if (screen_no >= xiInfo.getCount() || xiInfo.getCount() == 0) {
    } else {
        for (int s = 0; s < xiInfo.getCount(); s++) {
            if (s != screen_no)
                continue;
            *x = xiInfo[s].x_org;
            *y = xiInfo[s].y_org;
            *width = xiInfo[s].width;
            *height = xiInfo[s].height;
            return;
        }
    }

    *x = 0;
    *y = 0;
    *width = desktop->width();
    *height = desktop->height();
}

int YDesktop::getScreenForRect(int x, int y, unsigned width, unsigned height) {
    int screen = -1;
    long coverage = -1;

    if (xiInfo.getCount() == 0)
        return 0;
    for (int s = 0; s < xiInfo.getCount(); s++) {
        int x_i = intersection(x, x + int(width),
                               xiInfo[s].x_org, xiInfo[s].x_org + xiInfo[s].width);
        //MSG(("x_i %d %d %d %d %d", x_i, x, width, xiInfo[s].x_org, xiInfo[s].width));
        int y_i = intersection(y, y + height,
                               xiInfo[s].y_org, xiInfo[s].y_org + xiInfo[s].height);
        //MSG(("y_i %d %d %d %d %d", y_i, y, height, xiInfo[s].y_org, xiInfo[s].height));

        int cov = (1 + x_i) * (1 + y_i);

        //MSG(("cov=%d %d %d s:%d xc:%d yc:%d %d %d %d %d", cov, x, y, s, x_i, y_i, xiInfo[s].x_org, xiInfo[s].y_org, xiInfo[s].width, xiInfo[s].height));

        if (cov > coverage) {
            screen = s;
            coverage = cov;
        }
    }
    return screen;
}


KeySym YWindow::keyCodeToKeySym(unsigned int keycode, int index) {
    KeySym k = XkbKeycodeToKeysym(xapp->display(), (KeyCode)keycode, 0, index);
    return k;
}

int YDesktop::getScreenCount() {
    return xiInfo.getCount();
}

// vim: set sw=4 ts=4 et:
