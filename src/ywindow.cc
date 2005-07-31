/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "yutil.h"
#include "ywindow.h"

#include "ytooltip.h"
#include "yxapp.h"
#include "sysdep.h"
#include "yprefs.h"
#include "yrect.h"
#include "ascii.h"

#include "ytimer.h"
#include "ypopup.h"

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
    YTimer *fAutoScrollTimer;
    XMotionEvent *fMotion;
    YWindow *fWindow;
    bool fScrolling;
};


YAutoScroll::YAutoScroll() {
    fWindow = 0;
    fAutoScrollTimer = 0;
    fScrolling = false;
    fMotion = new XMotionEvent;
}

YAutoScroll::~YAutoScroll() {
    delete fAutoScrollTimer; fAutoScrollTimer = 0;
    delete fMotion; fMotion = 0;
}

bool YAutoScroll::handleTimer(YTimer *timer) {
    if (timer == fAutoScrollTimer && fWindow) {
        fAutoScrollTimer->setInterval(autoScrollDelay);
        return fWindow->handleAutoScroll(*fMotion);
    }
    return false;
}

void YAutoScroll::autoScroll(YWindow *w, bool autoScroll, const XMotionEvent *motion) {
    if (motion && fMotion)
        *fMotion = *motion;
    else
        w = 0;
    fWindow = w;
    if (w == 0)
        autoScroll = false;
    fScrolling = autoScroll;
    if (autoScroll && fAutoScrollTimer == 0) {
        fAutoScrollTimer = new YTimer(autoScrollStartDelay);
        fAutoScrollTimer->setTimerListener(this);
    }
    if (fAutoScrollTimer) {
        if (autoScroll) {
            if (!fAutoScrollTimer->isRunning()) {
                fAutoScrollTimer->startTimer(autoScrollStartDelay);
            }
        } else
            fAutoScrollTimer->stopTimer();
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

/******************************************************************************/
/******************************************************************************/

YWindow::YWindow(YWindow *parent, Window win):
    fParentWindow(parent),
    fNextWindow(0), fPrevWindow(0), fFirstWindow(0), fLastWindow(0),
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
#ifdef CONFIG_TOOLTIP
    fToolTip(0),
#endif
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
    removeWindow();
    while (accel) {
        YAccelerator *next = accel->next;
        delete accel;
        accel = next;
    }
#ifdef CONFIG_TOOLTIP
    if (fToolTip) {
        fToolTip->hide();
        if (fToolTipTimer && fToolTipTimer->getTimerListener() == fToolTip) {
            fToolTipTimer->stopTimer();
            fToolTipTimer->setTimerListener(0);
        }
        delete fToolTip; fToolTip = 0;
    }
#endif
    if (fClickWindow == this)
        fClickWindow = 0;
    if (fGraphics) {
        delete fGraphics; fGraphics = 0;
    }
    if (flags & wfCreated)
        destroy();
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

void YWindow::setStyle(unsigned long aStyle) {
    if (fStyle != aStyle) {
        fStyle = aStyle;

        if (flags & wfCreated) {
            if (fStyle & wsPointerMotion)
                fEventMask |= PointerMotionMask;


            if ((fStyle & wsDesktopAware) || (fStyle & wsManager) ||
                (fHandle != RootWindow(xapp->display(), DefaultScreen(xapp->display()))))
                fEventMask |=
                    StructureNotifyMask |
                    ColormapChangeMask |
                    PropertyChangeMask;

            if (fStyle & wsManager)
                fEventMask |=
                    FocusChangeMask |
                    SubstructureRedirectMask | SubstructureNotifyMask;

            fEventMask |= ButtonPressMask | ButtonReleaseMask | ButtonMotionMask;
            if (fHandle == RootWindow(xapp->display(), DefaultScreen(xapp->display())))
                if (!(fStyle & wsManager) || !grabRootWindow)
                    fEventMask &= ~(ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);

            XSelectInput(xapp->display(), fHandle, fEventMask);
        }
    }
}

Graphics &YWindow::getGraphics() {
    return *(NULL == fGraphics ? fGraphics = new Graphics(*this) : fGraphics);
}

void YWindow::repaint() {
    XClearArea(xapp->display(), handle(), 0, 0, width(), height(), True);
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

void YWindow::create() {
    if (flags & wfCreated) return;

    if (fHandle == None) {
        XSetWindowAttributes attributes;
        unsigned int attrmask = CWEventMask;

        fEventMask |=
            ExposureMask |
            ButtonPressMask | ButtonReleaseMask | ButtonMotionMask;

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
                                CopyFromParent,
                                (fStyle & wsInputOnly) ? InputOnly : InputOutput,
                                CopyFromParent,
                                attrmask,
                                &attributes);

        if (parent() == desktop &&
            !(flags & (wsManager || wsOverrideRedirect)))
            XSetWMProtocols(xapp->display(), fHandle, &_XA_WM_DELETE_WINDOW, 1);

        if ((flags & wfVisible) && !(flags & wfNullSize))
            XMapWindow(xapp->display(), fHandle);
    } else {
        XWindowAttributes attributes;

        XGetWindowAttributes(xapp->display(),
                             fHandle,
                             &attributes);
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

        fEventMask = 0;

        if ((fStyle & wsDesktopAware) || (fStyle & wsManager) ||
            (fHandle != RootWindow(xapp->display(), DefaultScreen(xapp->display()))))
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
                fHandle == RootWindow(xapp->display(), DefaultScreen(xapp->display())))
                fEventMask &= ~(ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);
        }

        XSelectInput(xapp->display(), fHandle, fEventMask);
    }
    XSaveContext(xapp->display(), fHandle, windowContext, (XPointer)this);
    flags |= wfCreated;
}

void YWindow::destroy() {
    if (flags & wfCreated) {
        if (!(flags & wfDestroyed)) {
            if (!(flags & wfAdopted)) {
                MSG(("----------------------destroy %X", fHandle));
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
        if (fParentWindow->fFocusedWindow == this)
            fParentWindow->fFocusedWindow = 0;
        if (fPrevWindow)
            fPrevWindow->fNextWindow = fNextWindow;
        else
            fParentWindow->fFirstWindow = fNextWindow;

        if (fNextWindow)
            fNextWindow->fPrevWindow = fPrevWindow;
        else
            fParentWindow->fLastWindow = fPrevWindow;
        fParentWindow = 0;
    }
    fPrevWindow = fNextWindow = 0;
}

void YWindow::insertWindow() {
    if (fParentWindow) {
        fNextWindow = fParentWindow->fFirstWindow;
        fPrevWindow = 0;

        if (fNextWindow)
            fNextWindow->fPrevWindow = this;
        else
            fParentWindow->fLastWindow = this;
        fParentWindow->fFirstWindow = this;
    }
}

void YWindow::reparent(YWindow *parent, int x, int y) {
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

Window YWindow::handle() {
    if (!(flags & wfCreated))
        create();

    return fHandle;
}

void YWindow::show() {
    if (!(flags & wfVisible)) {
        flags |= wfVisible;
        if (!(flags & wfNullSize))
            XMapWindow(xapp->display(), handle());
    }
}

void YWindow::hide() {
    if (flags & wfVisible) {
        flags &= ~wfVisible;
        if (!(flags & wfNullSize)) {
            addIgnoreUnmap(handle());
            XUnmapWindow(xapp->display(), handle());
        }
    }
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
                 w = w->parent());
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
#if 1
         {
             XEvent new_event, old_event;

             old_event = event;
             while (/*XPending(app->display()) > 0 &&*/
                    XCheckTypedWindowEvent(xapp->display(), handle(), ConfigureNotify,
                                 &new_event) == True)
             {
                 if (event.type != new_event.type ||
                     event.xmotion.window != new_event.xmotion.window)
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
        handleGraphicsExpose(event.xgraphicsexpose); break;

    case MapNotify:
        handleMapNotify(event.xmap);
        break;

    case UnmapNotify:
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

    default:
#ifdef CONFIG_SHAPE
        if (shapesSupported && event.type == (shapeEventBase + ShapeNotify))
            handleShapeNotify(*(const XShapeEvent *)&event);
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
    ref<YPixmap> pix(new YPixmap(r.width(), r.height()));
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
    if (ex + ew < width()) {
        ew += ee;
    } else {
        ew = width() - ex;
    }
    if (ey + eh + ee < height()) {
        eh += ee;
    } else {
        eh = height() - ey;
    }


    YRect r1(ex, ey, ew, eh);
    if (fDoubleBuffer) {
        ref<YPixmap> pixmap = beginPaint(r1);
        Graphics g1(pixmap, ex, ey);
        paint(g1, r1);
        endPaint(g, pixmap, r1);
    } else {
        paint(g, r1);
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
        const bool resized(configure.width != fWidth ||
                           configure.height != fHeight);
        
        if (configure.x != fX ||
            configure.y != fY ||
            resized)
        {
            fX = configure.x;
            fY = configure.y;
            fWidth = configure.width;
            fHeight = configure.height;

            this->configure(YRect(fX, fY, fWidth, fHeight), resized);
        }
    }   
}

bool YWindow::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        KeySym k = XKeycodeToKeysym(xapp->display(), key.keycode, 0);
        unsigned int m = KEY_MODMASK(key.state);

        if (accel) {
            YAccelerator *a;

            for (a = accel; a; a = a->next) {
                //msg("%c %d - %c %d %d", k, k, a->key, a->key, a->mod);
                if (m == a->mod && k == a->key)
                    if (a->win->handleKey(key) == true)
                        return true;
            }
            if (ASCII::isLower(k)) {
                k = ASCII::toUpper(k);
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
#ifdef CONFIG_TOOLTIP
    if (fToolTip) {
        fToolTip->hide();
        if (fToolTipTimer && fToolTipTimer->getTimerListener() == fToolTip) {
            fToolTipTimer->stopTimer();
            fToolTipTimer->setTimerListener(0);
        }
    }
#endif

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

#ifndef CONFIG_TOOLTIP
void YWindow::setToolTip(const char */*tip*/) {
#else
YTimer *YWindow::fToolTipTimer = 0;

void YWindow::setToolTip(const char *tip) {
    if (fToolTip) {
        if (!tip) {
            delete fToolTip; fToolTip = 0;
        } else {
            fToolTip->setText(tip);
            fToolTip->repaint();
        }
    }
    if (tip) {
        if (!fToolTip)
            fToolTip = new YToolTip();
        if (fToolTip)
            fToolTip->setText(tip);
    }
#endif
}

bool YWindow::toolTipVisible() {
#ifdef CONFIG_TOOLTIP
    return (fToolTip && fToolTip->visible());
#else    
    return false;
#endif
}

void YWindow::updateToolTip() {
}

#ifndef CONFIG_TOOLTIP
void YWindow::handleCrossing(const XCrossingEvent &/*crossing*/) {
#else
void YWindow::handleCrossing(const XCrossingEvent &crossing) {
    if (fToolTip) {
        if (crossing.type == EnterNotify && crossing.mode == NotifyNormal) {
            if (fToolTipTimer == 0)
                fToolTipTimer = new YTimer(ToolTipDelay);
            if (fToolTipTimer) {
                fToolTipTimer->setTimerListener(fToolTip);
                fToolTipTimer->startTimer();
                updateToolTip();
                if (fToolTip)
                    fToolTip->locate(this, crossing);
            }
        } else if (crossing.type == LeaveNotify) {
            fToolTip->hide();
            if (fToolTipTimer && fToolTipTimer->getTimerListener() == fToolTip) {
                fToolTipTimer->stopTimer();
                fToolTipTimer->setTimerListener(0);
            }
        }
    }
#endif
}

void YWindow::handleClientMessage(const XClientMessageEvent &message) {
    if (message.message_type == _XA_WM_PROTOCOLS
        && message.format == 32
        && message.data.l[0] == (long)_XA_WM_DELETE_WINDOW)
    {
        handleClose();
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

#if 0
    virtual void handleVisibility(const XVisibilityEvent &visibility);
    virtual void handleCreateWindow(const XCreateWindowEvent &createWindow);
#endif

void YWindow::handleMapNotify(const XMapEvent &) {
    // ignore "map notify" not implemented or needed due to MapRequest event
}

void YWindow::handleUnmapNotify(const XUnmapEvent &xunmap) {
    if (xunmap.window == xunmap.event) {
        if (!ignoreUnmap(xunmap.window)) {
            flags &= ~wfVisible;
            handleUnmap(xunmap);
        }
    }
}

void YWindow::handleUnmap(const XUnmapEvent &) {
}

void YWindow::handleConfigureRequest(const XConfigureRequestEvent & /*configureRequest*/) {
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
    bool zero;

    if (fWidth == 0 || fHeight == 0)
        zero = true;
    else
        zero = false;

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

        configure(YRect(fX, fY, fWidth, fHeight), resized);
    }
}

void YWindow::setPosition(int x, int y) {
    if (x != fX || y != fY) {
        fX = x;
        fY = y;

        if (flags & wfCreated)
            XMoveWindow(xapp->display(), fHandle, fX, fY);

        configure(YRect(fX, fY, width(), height()), false);
    }
}

void YWindow::setSize(int width, int height) {
    if (width != fWidth || height != fHeight) {
        fWidth = width;
        fHeight = height;

        if (flags & wfCreated)
            if (!nullGeometry())
                XResizeWindow(xapp->display(), fHandle, fWidth, fHeight);

        configure(YRect(x(), y(), fWidth, fHeight), true);
    }
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

void YWindow::configure(const YRect &/*r*/,
                        const bool /*resized*/)
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
         XKeysymToString(XKeycodeToKeysym(xapp->display(), keycode, 0))));

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

bool YWindow::isFocusTraversable() {
    return false;
}

bool YWindow::isFocused() {
    if (parent() == 0 || isToplevel()) /// !!! fix
        return true;
    else
        return (parent()->fFocusedWindow == this) && parent()->isFocused();
}

void YWindow::requestFocus() {
    if (!toplevel())
        return ;

    if (parent()) {
        if (!isToplevel())
            parent()->requestFocus();
        parent()->setFocus(this);
        setFocus(0);///!!! is this the right place?
    }
    if (parent() && parent()->isFocused())
        setWindowFocus();
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
            cur = fLastWindow;
        else
            cur = fFirstWindow;
    }

    YWindow *org = cur;
    if (cur) do {
        ///!!! need focus ordering

        if (next) {
            if (cur->fLastWindow)
                cur = cur->fLastWindow;
            else if (cur->fPrevWindow)
                cur = cur->fPrevWindow;
            else if (cur->isToplevel())
                /**/;
            else {
                while (cur->fParentWindow) {
                    cur = cur->fParentWindow;
                    if (cur->isToplevel())
                        break;
                    if (cur->fPrevWindow) {
                        cur = cur->fPrevWindow;
                        break;
                    }
                }
            }
        } else {
            // is reverse tabbing of nested windows correct?
            if (cur->fFirstWindow)
                cur = cur->fFirstWindow;
            else if (cur->fNextWindow)
                cur = cur->fNextWindow;
            else if (cur->isToplevel())
                /**/;
            else {
                while (cur->fParentWindow) {
                    cur = cur->fParentWindow;
                    if (cur->isToplevel())
                        break;
                    if (cur->fNextWindow) {
                        cur = cur->fNextWindow;
                        break;
                    }
                }
            }
        }

        if (cur->isFocusTraversable()) {
            cur->requestFocus();
            return true;
        }
    } while (cur != org);

    return false;
}

void YWindow::setFocus(YWindow *window) {
    if (window != fFocusedWindow) {
        YWindow *oldFocus = fFocusedWindow;

        fFocusedWindow = window;

        if (oldFocus)
            oldFocus->lostFocus();
        if (fFocusedWindow)
            fFocusedWindow->gotFocus();
    }
}
void YWindow::gotFocus() {
    if (fFocusedWindow)
        fFocusedWindow->gotFocus();
    repaintFocus();
}

void YWindow::lostFocus() {
    if (fFocusedWindow)
        fFocusedWindow->lostFocus();
    repaintFocus();
}

void YWindow::installAccelerator(unsigned int key, unsigned int mod, YWindow *win) {
    key = ASCII::toUpper(key);
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
    key = ASCII::toUpper(key);
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
                             (XPointer *)&win) == 0)
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
                YWindow *win;

                if (XFindContext(xapp->display(), XdndDropTarget, windowContext,
                                 (XPointer *)&win) == 0)
                    win->handleDNDLeave();
            }
            XdndDropTarget = target;
            if (XdndDropTarget) {
                YWindow *win;

                if (XFindContext(xapp->display(), XdndDropTarget, windowContext,
                                 (XPointer *)&win) == 0)
                {
                    win->handleDNDEnter();
                    pwin = win;
                }
            }
        }
        if (pwin == 0 && XdndDropTarget) { // !!! optimize this
            if (XFindContext(xapp->display(), XdndDropTarget, windowContext,
                             (XPointer *)&pwin) != 0)
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
    updateXineramaInfo();
}

YDesktop::~YDesktop() {
}

void YDesktop::resetColormapFocus(bool /*active*/) {
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

    MSG(("grabVKey %d %d %d", key, vm, m));
    if (key != 0 && (vm == 0 || m != 0)) {
        if ((!(vm & kfMeta) || xapp->MetaMask) &&
            (!(vm & kfAlt) || xapp->AltMask) &&
           (!(vm & kfSuper) || xapp->SuperMask) &&
            (!(vm & kfHyper) || xapp->HyperMask))
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

    MSG(("grabVButton %d %d %d", button, vm, m));

    if (button != 0 && (vm == 0 || m != 0)) {
        if ((!(vm & kfMeta) || xapp->MetaMask) &&
            (!(vm & kfAlt) || xapp->AltMask) &&
           (!(vm & kfSuper) || xapp->SuperMask) &&
            (!(vm & kfHyper) || xapp->HyperMask))
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
        keyBuf[0] = ksym & 0xFF;
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

    static GC scrollGC = None;

    if (scrollGC == None) {
        scrollGC = XCreateGC(xapp->display(), desktop->handle(), 0, NULL);
    }

    XCopyArea(xapp->display(), handle(), handle(), scrollGC,
              dx, dy, width(), height(), 0, 0);

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

void YDesktop::updateXineramaInfo() {
#ifdef XINERAMA
    xiHeads = 0;
    xiInfo = NULL;

    if (XineramaIsActive(xapp->display())) {
        xiInfo = XineramaQueryScreens(xapp->display(), &xiHeads);
        msg("xinerama: heads=%d", xiHeads);
        for (int i = 0; i < xiHeads; i++) {
            msg("xinerama: %d +%d+%d %dx%d",
                xiInfo[i].screen_number,
                xiInfo[i].x_org,
                xiInfo[i].y_org,
                xiInfo[i].width,
                xiInfo[i].height);
        }
    } else {
        xiHeads = 1;
        xiInfo = new XineramaScreenInfo[1];
        xiInfo[0].screen_number = 0;
        xiInfo[0].x_org = 0;
        xiInfo[0].y_org = 0;
        xiInfo[0].width = width();
        xiInfo[0].height = height();
    }
#endif
}


void YDesktop::getScreenGeometry(int *x, int *y,
                                 int *width, int *height,
                                 int screen_no)
{
#ifdef XINERAMA
    if (screen_no == -1)
        screen_no = xineramaPrimaryScreen;
    if (screen_no < 0 || screen_no >= xiHeads)
        screen_no = 0;
    if (screen_no >= xiHeads || xiInfo == NULL) {
    } else {
        for (int s = 0; s < xiHeads; s++) {
            if (xiInfo[s].screen_number != screen_no)
                continue;
            *x = xiInfo[s].x_org;
            *y = xiInfo[s].y_org;
            *width = xiInfo[s].width;
            *height = xiInfo[s].height;
            return;
        }
    }
#endif
    *x = 0;
    *y = 0;
    *width = desktop->width();
    *height = desktop->height();
}

int YDesktop::getScreenForRect(int x, int y, int width, int height) {
    int screen = -1;
    long coverage = -1;

#ifdef XINERAMA
    if (xiInfo == NULL || xiHeads == 0)
        return 0;
    for (int s = 0; s < xiHeads; s++) {
        int x_i = intersection(x, x + width,
                               xiInfo[s].x_org, xiInfo[s].x_org + xiInfo[s].width);
        MSG(("x_i %d %d %d %d %d", x_i, x, width, xiInfo[s].x_org, xiInfo[s].width));
        int y_i = intersection(y, y + height,
                               xiInfo[s].y_org, xiInfo[s].y_org + xiInfo[s].height);
        MSG(("y_i %d %d %d %d %d", y_i, y, height, xiInfo[s].y_org, xiInfo[s].height));

        int cov = (1 + x_i) * (1 + y_i);

        MSG(("cov=%d %d %d s:%d xc:%d yc:%d %d %d %d %d", cov, x, y, s, x_i, y_i, xiInfo[s].x_org, xiInfo[s].y_org, xiInfo[s].width, xiInfo[s].height));

        if (cov > coverage) {
            screen = s;
            coverage = cov;
        }
    }
    return screen;
#else
    return 0;
#endif
}
