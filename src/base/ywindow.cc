/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "ywindow.h"

#include "ytooltip.h"
#include "yapp.h"
#include "sysdep.h"
///#include "prefs.h"

extern XContext windowContext;

#include "ytimer.h"

class AutoScroll: public YTimerListener {
public:
    AutoScroll();
    virtual ~AutoScroll();

    virtual bool handleTimer(YTimer *timer);

    void autoScroll(YWindow *w, bool autoScroll, const XMotionEvent *motion);

    YWindow *getWindow() const { return fWindow; }
    bool isScrolling() const { return fScrolling; }
private:
    YTimer *fAutoScrollTimer;
    XMotionEvent *fMotion;
    YWindow *fWindow;
    bool fScrolling;

    static unsigned int autoScrollDelay;
    static unsigned int autoScrollStartDelay;
};

unsigned int AutoScroll::autoScrollDelay = 50;
unsigned int AutoScroll::autoScrollStartDelay = 400;

AutoScroll::AutoScroll() {
    fWindow = 0;
    fAutoScrollTimer = 0;
    fScrolling = false;
    fMotion = new XMotionEvent;
}

AutoScroll::~AutoScroll() {
    delete fAutoScrollTimer; fAutoScrollTimer = 0;
    delete fMotion; fMotion = 0;
}

bool AutoScroll::handleTimer(YTimer *timer) {
    if (timer == fAutoScrollTimer && fWindow) {
        fAutoScrollTimer->setInterval(autoScrollDelay);
        return fWindow->handleAutoScroll(*fMotion);
    }
    return false;
}

void AutoScroll::autoScroll(YWindow *w, bool autoScroll, const XMotionEvent *motion) {
    if (motion && fMotion)
        *fMotion = *motion;
    else
        w = 0;
    fWindow = w;
    if (w == 0)
        autoScroll = false;
    fScrolling = autoScroll;
    if (autoScroll && fAutoScrollTimer == 0) {
        fAutoScrollTimer = new YTimer(this, autoScrollStartDelay);
        //fAutoScrollTimer->setTimerListener(this);
    }
    if (fAutoScrollTimer) {
        if (autoScroll) {
            if (!fAutoScrollTimer->isRunning())
                fAutoScrollTimer->setInterval(autoScrollStartDelay);
            fAutoScrollTimer->startTimer();
        } else
            fAutoScrollTimer->stopTimer();
    }
}

AutoScroll *YWindow::fAutoScroll = 0;

YWindow *YWindow::fClickWindow = 0;
Time YWindow::fClickTime = 0;
int YWindow::fClickCount = 0;
XButtonEvent YWindow::fClickEvent;
int YWindow::fClickDrag = 0;
unsigned int YWindow::fClickButton = 0;
unsigned int YWindow::fClickButtonDown = 0;

unsigned int YWindow::MultiClickTime = 400;
unsigned int YWindow::ClickMotionDistance = 4;
unsigned int YWindow::ClickMotionDelay = 200;
unsigned int YWindow::ToolTipDelay = 1000;

void YWindow::init() {
    fFirstWindow = fLastWindow = fPrevWindow = fNextWindow = 0;
    flags = 0;
    fX = fY = 0;
    fWidth = fHeight = 1;
    fPointer = None;
    fStyle = 0;
    fGraphics = 0;
    fEnabled = true;
    fToplevel = false;
    fFocusedWindow = 0;
    fWinGravity = NorthWestGravity;
    fBitGravity = ForgetGravity;
    unmapCount = 0;
    accel = 0;
    fToolTip = 0;
    fDND = false;
    fDragging = false;
    XdndDragSource = None;
    XdndDragTarget = None;
    XdndDropTarget = None;
    XdndTargetVersion = None;
    XdndNumTypes = 0;
    XdndTypes = 0;
    XdndTimestamp = 0;
    fWaitingForStatus = false;
    fGotStatus = false;
    fHaveNewPosition = false;
    fEndDrag = false;
    fDoDrop = false;

    fEventMask =
        KeyPressMask | KeyReleaseMask | FocusChangeMask |
        LeaveWindowMask | EnterWindowMask;

 }

YWindow::YWindow(YWindow *parent) {
    fParentWindow = parent;
    fHandle = None;
    init();

    if (fParentWindow == 0) {
        PRECONDITION(desktop != 0);
        fParentWindow = desktop;
    }
    insertWindow();
}

YWindow::YWindow(YWindow *parent, Window win) {
    fParentWindow = parent;
    fHandle = win;
    PRECONDITION(fHandle != None);
    init();

    MSG(("adopting window %lX\n", fHandle));
    flags |= wfAdopted;
    create();
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
    if (fToolTip) {
        fToolTip->hide();
        if (fToolTipTimer && fToolTipTimer->getTimerListener() == fToolTip) {
            fToolTipTimer->stopTimer();
            fToolTipTimer->setTimerListener(0);
        }
        delete fToolTip; fToolTip = 0;
    }
    if (fClickWindow == this)
        fClickWindow = 0;
    if (fGraphics) {
        delete fGraphics; fGraphics = 0;
    }
    if (flags & wfCreated)
        destroy();
}

void YWindow::setWindowFocus() {
    XSetInputFocus(app->display(), handle(), RevertToNone, CurrentTime);
}

void YWindow::setStyle(unsigned long aStyle) {
    if (fStyle != aStyle) {
        fStyle = aStyle;

        if (flags & wfCreated) {
            if (aStyle & wsManager)
                fEventMask |=
                    SubstructureRedirectMask | SubstructureNotifyMask ;

            if (fStyle & wsPointerMotion)
                fEventMask |= PointerMotionMask;

            if (!(aStyle & wsManager) && ///grabRootWindow &&
                fHandle == RootWindow(app->display(), DefaultScreen(app->display())))
                fEventMask &= ~(ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);
            else
                fEventMask |= ButtonPressMask | ButtonReleaseMask | ButtonMotionMask;

            XSelectInput(app->display(), fHandle, fEventMask);
        }
    }
}

Graphics &YWindow::getGraphics() {
    if (fGraphics == 0)
        fGraphics = new Graphics(this);
    return *fGraphics;
}

void YWindow::repaint() {
    if ((flags & (wfCreated | wfVisible)) == (wfCreated | wfVisible)) {
        Graphics &g = getGraphics();

        paint(g, 0, 0, width(), height());
    }
}

void YWindow::repaintFocus() {
    if ((flags & (wfCreated | wfVisible)) == (wfCreated | wfVisible)) {
        Graphics &g = getGraphics();

        paintFocus(g, 0, 0, width(), height());
    }
}

void YWindow::create() {
    if (flags & wfCreated)
        return ;
    if (fHandle == None) {
        XSetWindowAttributes attributes;
        unsigned int attrmask = CWEventMask;

        fEventMask |=
            ExposureMask |
            ButtonPressMask | ButtonReleaseMask | ButtonMotionMask;

        if (fStyle & wsPointerMotion)
            fEventMask |= PointerMotionMask;

        if (fParentWindow == desktop && !(fStyle & wsOverrideRedirect))
            fEventMask |= StructureNotifyMask;
        if (fStyle & wsManager)
            fEventMask |=
                SubstructureRedirectMask | SubstructureNotifyMask;

        if (fStyle & wsSaveUnder) {
            attributes.save_under = True;
            attrmask |= CWSaveUnder;
        }
        if (fStyle & wsOverrideRedirect) {
            attributes.override_redirect = True;
            attrmask |= CWOverrideRedirect;
        }
        if (fPointer != None) {
            attrmask |= CWCursor;
            attributes.cursor = fPointer;
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
        fHandle = XCreateWindow(app->display(),
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
            XSetWMProtocols(app->display(), fHandle, &_XA_WM_DELETE_WINDOW, 1);

        if ((flags & wfVisible) && !(flags & wfNullSize))
            XMapWindow(app->display(), fHandle);
    } else {
        XWindowAttributes attributes;

        XGetWindowAttributes(app->display(),
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

        fEventMask |=
            StructureNotifyMask |
            ColormapChangeMask |
            PropertyChangeMask;

        if (fStyle & wsManager)
            fEventMask |=
                SubstructureRedirectMask | SubstructureNotifyMask |
                ButtonPressMask | ButtonReleaseMask | ButtonMotionMask;

        if (!(fStyle & wsManager) && ///grabRootWindow &&
            fHandle == RootWindow(app->display(), DefaultScreen(app->display())))
            fEventMask &= ~(ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);

        XSelectInput(app->display(), fHandle, fEventMask);
    }
    XSaveContext(app->display(), fHandle, windowContext, (XPointer)this);
    flags |= wfCreated;
}

void YWindow::destroy() {
    if (flags & wfCreated) {
        if (!(flags & wfDestroyed)) {
            if (!(flags & wfAdopted)) {
                XDestroyWindow(app->display(), fHandle);
            } else {
                XSelectInput(app->display(), fHandle, NoEventMask);
            }
            flags |= wfDestroyed;
        }
        XDeleteContext(app->display(), fHandle, windowContext);
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
    //flags &= ~wfVisible; // don't unmap when we get UnmapNotify
    if (flags & wfVisible) {
        flags |= wfUnmapped;
        unmapCount++;
    }

    removeWindow();
    fParentWindow = parent;
    insertWindow();

    XReparentWindow(app->display(),
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
            XMapWindow(app->display(), handle());
    }
}

void YWindow::hide() {
    if (flags & wfVisible) {
        flags &= ~wfVisible;
        if (!(flags & wfNullSize)) {
            flags |= wfUnmapped;
            unmapCount++;
            XUnmapWindow(app->display(), handle());
        }
    }
}

void YWindow::setWinGravity(int gravity) {
    if (flags & wfCreated) {
        unsigned long eventmask = CWWinGravity;
        XSetWindowAttributes attributes;
        attributes.win_gravity = gravity;
        fWinGravity = gravity;

        XChangeWindowAttributes(app->display(), handle(), eventmask, &attributes);
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

        XChangeWindowAttributes(app->display(), handle(), eventmask, &attributes);
    } else {
        fBitGravity = gravity;
    }
}

void YWindow::raise() {
    XRaiseWindow(app->display(), handle());
}

void YWindow::lower() {
    XLowerWindow(app->display(), handle());
}

void YWindow::handleEvent(const XEvent &event) {
    switch (event.type) {
    case KeyPress:
    case KeyRelease:
    { // !!! hack, fix
        YWindow *w = this;
        while (w && w->handleKeyEvent(event.xkey) == false) w = w->parent();
        break;
    }
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
                    XCheckMaskEvent(app->display(),
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
                     XPutBackEvent(app->display(), &new_event);
                     break;
                 } else {
                     XFlush(app->display());
                     old_event = new_event;
                 }
             }
             if (fDragging == true)
                 handleDNDMotion(old_event.xmotion);
             else
                 handleMotion(old_event.xmotion);
         }
         break;

    case EnterNotify:
    case LeaveNotify:
        handleCrossing(event.xcrossing);
        break;
    case FocusIn:
    case FocusOut: handleFocus(event.xfocus); break;
    case PropertyNotify: handleProperty(event.xproperty); break;
    case ColormapNotify: handleColormap(event.xcolormap); break;
    case MapRequest: handleMapRequest(event.xmaprequest); break;
    case ConfigureRequest:
        handleConfigureRequest(event.xconfigurerequest);
        break;
    case ConfigureNotify:
#if 1
         {
             XEvent new_event, old_event;

             old_event = event;
             while (/*XPending(app->display()) > 0 &&*/
                    XCheckTypedWindowEvent(app->display(), handle(), ConfigureNotify,
                                 &new_event) == True)
             {
                 if (event.type != new_event.type ||
                     event.xmotion.window != new_event.xmotion.window)
                 {
                     XPutBackEvent(app->display(), &new_event);
                     break;
                 } else {
                     XFlush(app->display());
                     old_event = new_event;
                 }
             }
             handleConfigure(old_event.xconfigure);
         }
#else
         handleConfigure(event.xconfigure);
#endif
        break;
    case DestroyNotify: handleDestroyWindow(event.xdestroywindow); break;
    case Expose: handleExpose(event.xexpose); break;
    case GraphicsExpose: handleGraphicsExpose(event.xgraphicsexpose); break;
    case MapNotify: handleMap(event.xmap); break;
    case UnmapNotify: handleUnmap(event.xunmap); break;
    case ClientMessage: handleClientMessage(event.xclient); break;
    case SelectionClear: handleSelectionClear(event.xselectionclear); break;
    case SelectionRequest: handleSelectionRequest(event.xselectionrequest); break;
    case SelectionNotify: handleSelection(event.xselection); break;
#ifdef SHAPE
    default:
        if (shapesSupported && event.type == (shapeEventBase + ShapeNotify))
            handleShapeNotify(*(const XShapeEvent *)&event);
        break;
#endif
    }
}

void YWindow::handleExpose(const XExposeEvent &expose) {
    Graphics &g = getGraphics();
    XRectangle r;

    r.x = expose.x;
    r.y = expose.y;
    r.width = expose.width;
    r.height = expose.height;

    XSetClipRectangles(app->display(), g.handle(),
                       0, 0, &r, 1, Unsorted);
    paint(g,
          expose.x,
          expose.y,
          expose.width,
          expose.height);

    XSetClipMask(app->display(), g.handle(), None);
    //XFlush(app->display());
}

void YWindow::handleGraphicsExpose(const XGraphicsExposeEvent &graphicsExpose) {
    Graphics &g = getGraphics();
    XRectangle r;

    r.x = graphicsExpose.x;
    r.y = graphicsExpose.y;
    r.width = graphicsExpose.width;
    r.height = graphicsExpose.height;

    XSetClipRectangles(app->display(), g.handle(),
                       0, 0, &r, 1, Unsorted);
    paint(g,
          graphicsExpose.x,
          graphicsExpose.y,
          graphicsExpose.width,
          graphicsExpose.height);

    XSetClipMask(app->display(), g.handle(), None);
}

void YWindow::handleConfigure(const XConfigureEvent &configure) {
    if (configure.window == handle())
        if (configure.x != fX ||
            configure.y != fY ||
            (unsigned int)configure.width != fWidth ||
            (unsigned int)configure.height != fHeight)
        {
            fX = configure.x;
            fY = configure.y;
            fWidth = configure.width;
            fHeight = configure.height;

            this->configure(fX, fY, fWidth, fHeight);
        }
}

bool YWindow::handleKeyEvent(const XKeyEvent &key) {
    KeySym k = XKeycodeToKeysym(app->display(), key.keycode, 0);
    unsigned int m = KEY_MODMASK(key.state);

    return handleKeySym(key, k, VMod(m));
}

bool YWindow::handleKeySym(const XKeyEvent &key, KeySym ksym, int vmod) {
    if (key.type == KeyPress) {
        if (accel) {
            YAccelerator *a;

            for (a = accel; a; a = a->next) {
                //printf("%c %d - %c %d %d\n", k, k, a->key, a->key, a->mod);
                if (vmod == a->mod && ksym == a->key)
                    if (a->win->handleKeySym(key, ksym, vmod) == true)
                        return true;
            }
            if (ISLOWER(ksym)) {
                ksym = TOUPPER(ksym);
                for (a = accel; a; a = a->next)
                    if (vmod == a->mod && ksym == a->key)
                        if (a->win->handleKeySym(key, ksym, vmod) == true)
                            return true;
            }
        }
        if (ksym == XK_Tab) {
            if (vmod == 0) {
                nextFocus();
                return true;
            } else if (vmod == kfShift) {
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
        if (fToolTipTimer && fToolTipTimer->getTimerListener() == fToolTip) {
            fToolTipTimer->stopTimer();
            fToolTipTimer->setTimerListener(0);
        }
    }

    int x_dif = button.x_root - fClickEvent.x_root;
    int y_dif = button.y_root - fClickEvent.y_root;

    unsigned int motionDelta = 0;
    unsigned int motionDeltaX = (x_dif < 0) ? - x_dif : x_dif;
    unsigned int motionDeltaY = (y_dif < 0) ? - y_dif : y_dif;

    if (motionDeltaX > motionDeltaY)
        motionDelta = motionDeltaX;
    else
        motionDelta = motionDeltaY;

    if (button.type == ButtonPress) {
        fClickDrag = 0;

        if (fClickWindow != this) {
            fClickWindow = this;
            fClickCount = 1;
        } else {
            if ((button.time - fClickTime < MultiClickTime) &&
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
            int x_dif = motion.x_root - fClickEvent.x_root;
            int y_dif = motion.y_root - fClickEvent.y_root;

            unsigned int motionDelta = 0;
            unsigned int motionDeltaX = (x_dif < 0) ? - x_dif : x_dif;
            unsigned int motionDeltaY = (y_dif < 0) ? - y_dif : y_dif;

            if (motionDeltaX > motionDeltaY)
                motionDelta = motionDeltaX;
            else
                motionDelta = motionDeltaY;

            if ((motion.time - fClickTime > ClickMotionDelay) ||
                (motionDelta >= ClickMotionDistance)) {
                fClickDrag = 1;
                handleBeginDrag(fClickEvent, motion);
            }
        }
    }
}

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
}

bool YWindow::toolTipVisible() {
    if (fToolTip && fToolTip->visible())
        return true;
    return false;
}

void YWindow::updateToolTip() {
}

void YWindow::handleCrossing(const XCrossingEvent &crossing) {
    if (fDragging) {
        handleDNDCrossing(crossing);
    } else if (fToolTip) {
        if (crossing.type == EnterNotify && crossing.mode == NotifyNormal) {
            if (fToolTipTimer == 0)
                fToolTipTimer = new YTimer(fToolTip, ToolTipDelay);
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
}

void YWindow::handleProperty(const XPropertyEvent &) {
}

void YWindow::handleColormap(const XColormapEvent &) {
}

void YWindow::handleFocus(const XFocusChangeEvent &) {
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

void YWindow::handleMap(const XMapEvent &) {
    //flags |= wfVisible;
}

void YWindow::handleUnmap(const XUnmapEvent &) {
    if (flags & wfUnmapped) {
        unmapCount--;
        if (unmapCount == 0)
            flags &= ~wfUnmapped;
    }
//    else
 //       flags &= ~wfVisible;
}

void YWindow::handleDestroyWindow(const XDestroyWindowEvent &destroyWindow) {
    if (destroyWindow.window == fHandle)
        flags |= wfDestroyed;
}

void YWindow::handleConfigureRequest(const XConfigureRequestEvent &) {
}

void YWindow::handleMapRequest(const XMapRequestEvent &) {
}

#ifdef SHAPE
void YWindow::handleShapeNotify(const XShapeEvent &) {
}
#endif

void YWindow::handleClickDown(const XButtonEvent &/*down*/, int /*count*/) {
}

void YWindow::handleClick(const XButtonEvent &/*up*/, int /*count*/) {
}

void YWindow::handleBeginDrag(const XButtonEvent &/*down*/, const XMotionEvent &/*motion*/) {
}

void YWindow::handleDrag(const XButtonEvent &/*down*/, const XMotionEvent &/*motion*/) {
}

void YWindow::handleEndDrag(const XButtonEvent &/*down*/, const XButtonEvent &/*up*/) {
}

void YWindow::paint(Graphics &g, int x, int y, unsigned int w, unsigned int h) {
    g.fillRect(x, y, w, h);
}

void YWindow::paintFocus(Graphics &/*g*/, int /*x*/, int /*y*/, unsigned int /*w*/, unsigned int /*h*/) {
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
            flags |= wfUnmapped; ///!!!???
            unmapCount++;
            XUnmapWindow(app->display(), handle());
        }
    } else if ((flags & wfNullSize) && !zero) {
        flags &= ~wfNullSize;
        if (flags & wfVisible)
            XMapWindow(app->display(), handle());
    }
    return zero;
}

void YWindow::setGeometry(int x, int y, unsigned int width, unsigned int height) {
    if (x != fX ||
        y != fY ||
        width != fWidth ||
        height != fHeight)
    {
        fX = x;
        fY = y;
        fWidth = width;
        fHeight = height;

        if (flags & wfCreated) {
            if (!nullGeometry())
                XMoveResizeWindow(app->display(),
                                  fHandle,
                                  fX, fY, fWidth, fHeight);
        }

        configure(fX, fY, fWidth, fHeight);
    }
}

void YWindow::setPosition(int x, int y) {
    if (x != fX || y != fY) {
        fX = x;
        fY = y;

        if (flags & wfCreated)
            XMoveWindow(app->display(), fHandle, fX, fY);

        configure(fX, fY, width(), height());
    }
}

void YWindow::setSize(unsigned int width, unsigned int height) {
    if (width != fWidth || height != fHeight) {
        fWidth = width;
        fHeight = height;

        if (flags & wfCreated)
            if (!nullGeometry())
                XResizeWindow(app->display(), fHandle, fWidth, fHeight);

        configure(x(), y(), fWidth, fHeight);
    }
}

void YWindow::mapToGlobal(int &x, int &y) {
    int dx, dy;
    Window child;

    XTranslateCoordinates(app->display(),
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

    XTranslateCoordinates(app->display(),
                          desktop->handle(),
                          handle(),
                          x, y,
                          &dx, &dy, &child);
    x = dx;
    y = dy;
}

void YWindow::configure(int, int, unsigned int, unsigned int) {
}

void YWindow::setPointer(Cursor pointer) {
    fPointer = pointer;

    if (flags & wfCreated) {
        XSetWindowAttributes attributes;

        attributes.cursor = fPointer;

        XChangeWindowAttributes(app->display(),
                                handle(),
                                CWCursor,
                                &attributes);
    }
}

void YWindow::setGrabPointer(Cursor pointer) {
    XChangeActivePointerGrab(app->display(),
                             ButtonPressMask | PointerMotionMask | ButtonReleaseMask,
                             pointer, CurrentTime);//app->getEventTime());
}

void YWindow::grabKeyM(int keycode, unsigned int modifiers) {
    XGrabKey(app->display(), keycode, modifiers, handle(), False,
             GrabModeAsync, GrabModeAsync);
}

void YWindow::grabKey(int key, unsigned int modifiers) {
    KeyCode keycode = XKeysymToKeycode(app->display(), key);
    if (keycode != 0) {
        grabKeyM(keycode, modifiers);
        if (modifiers != AnyModifier) {
            grabKeyM(keycode, modifiers | LockMask);
            if (app->NumLockMask != 0) {
                grabKeyM(keycode, modifiers | app->NumLockMask);
                grabKeyM(keycode, modifiers | app->NumLockMask | LockMask);
            }
        }
    }
}

void YWindow::grabButtonM(int button, unsigned int modifiers) {
    XGrabButton(app->display(), button, modifiers,
                handle(), True, ButtonPressMask,
                GrabModeAsync, GrabModeAsync, None, None);
}

void YWindow::grabButton(int button, unsigned int modifiers) {
    grabButtonM(button, modifiers);
    if (modifiers != AnyModifier) {
        grabButtonM(button, modifiers | LockMask);
        if (app->NumLockMask != 0) {
            grabButtonM(button, modifiers | app->NumLockMask);
            grabButtonM(button, modifiers | app->NumLockMask | LockMask);
        }
    }
}

void YWindow::captureEvents() {
    app->captureGrabEvents(this);
}

void YWindow::releaseEvents() {
    app->releaseGrabEvents(this);
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
        setFocus(0);///??? !!! is this the right place?
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

    if (cur == 0)
        if (next)
            cur = fLastWindow;
        else
            cur = fFirstWindow;

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
    if (parent() && parent()->isFocused())
        setWindowFocus();
    repaintFocus();
}

void YWindow::lostFocus() {
    if (fFocusedWindow)
        fFocusedWindow->lostFocus();
    repaintFocus();
}

void YWindow::installAccelerator(unsigned int key, int mod, YWindow *win) {
    key = TOUPPER(key);
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

void YWindow::removeAccelerator(unsigned int key, int mod, YWindow *win) {
    key = TOUPPER(key);
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

const Atom XdndCurrentVersion = 4;
const Atom XdndMinimumVersion = 3;

void YWindow::setDND(bool enabled) {
    if (parent() != 0 && parent() != desktop) {
        YWindow *w = this;
        while (w->parent() && w->parent() != desktop)
            w = w->parent();
        w->setDND(enabled);
    } else if (fDND != enabled) {
        fDND = enabled;

        if (fDND) {
            XChangeProperty(app->display(), handle(),
                            XA_XdndAware, XA_ATOM,
                            32, PropModeReplace,
                            (const unsigned char *)&XdndCurrentVersion, 1);
        }
    }
}

bool YWindow::startDrag(int nTypes, Atom *types) {
    Atom sel = XA_XdndSelection;

    XdndNumTypes = nTypes;
    if (XdndNumTypes > 0) { // !!! this should go away (just here for testing)
        XdndTypes = new Atom[nTypes];
        if (XdndTypes == 0)
            return false;
        memcpy(XdndTypes, types, sizeof(Atom) * nTypes);
    } else
        XdndTypes = 0;


    if (app->grabEvents(this, None,
                   ButtonPressMask |
                   ButtonReleaseMask |
                   PointerMotionMask) == 0)
    {
        return false;
    }

    fDragging = true;
    XdndDragTarget = None;
    fWaitingForStatus = false;
    fGotStatus = false;
    fHaveNewPosition = false;
    fEndDrag = false;
    fDoDrop = false;

    puts("start drag");
    XSetSelectionOwner(app->display(), sel, handle(), app->getEventTime());

    return true;
}

void YWindow::endDrag(bool drop) {
    puts("endDrag");
    if (fDragging) {
        if (fWaitingForStatus) {
            fEndDrag = true;
            fDoDrop = drop;
            puts("delay end");
            return ;
        }

        if (fGotStatus == true) {
            if (drop) {
                if (XdndDragTarget) { // XdndLeave
                    XClientMessageEvent msg;

                    memset(&msg, 0, sizeof(msg));
                    msg.type = ClientMessage;
                    msg.display = app->display();
                    msg.window = XdndDragTarget;
                    msg.message_type = XA_XdndDrop;
                    msg.format = 32;
                    msg.data.l[0] = handle();
                    msg.data.l[1] = 0;
                    msg.data.l[2] = XdndTimestamp;
                    XSendEvent(app->display(), XdndDragTarget, False, 0L, (XEvent *)&msg);
                    puts("send: XdndDrop");
                }
                XdndDragTarget = 0;
            }
        }
        setDndTarget(None);
        fDragging = false;
        app->releaseEvents();
        delete [] XdndTypes; XdndTypes = 0;
        XdndNumTypes = 0;
    }
}

void YWindow::finishDrop() {

}

void YWindow::XdndStatus(bool acceptDrop, Atom dropAction) {
    XClientMessageEvent msg;
    int x_root = 0, y_root = 0;

    mapToGlobal(x_root, y_root);

    memset(&msg, 0, sizeof(msg));
    msg.type = ClientMessage;
    msg.display = app->display();
    msg.window = XdndDragSource;
    msg.message_type = XA_XdndStatus;
    msg.format = 32;
    msg.data.l[0] = handle();
    msg.data.l[1] = (acceptDrop ? 0x1 : 0x0) | 2;
    msg.data.l[2] = (x_root << 16) + y_root;
    msg.data.l[3] = (width() << 16) + height();
    msg.data.l[4] = dropAction;
    XSendEvent(app->display(), XdndDragSource, False, 0L, (XEvent *)&msg);
}

bool YWindow::isDNDAware(Window w) {
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop;
    bool is = false;

    if (XGetWindowProperty(app->display(), w,
                           XA_XdndAware,
                           0, 1, False, XA_ATOM,
                           &r_type, &r_format,
                           &count, &bytes_remain, &prop) == Success && prop)
    {
        Atom *data = (Atom *)prop;
        if (data[0] >= XdndMinimumVersion) {
            if (data[0] <= XdndCurrentVersion)
                XdndTargetVersion = data[0];
            else
                XdndTargetVersion  = XdndCurrentVersion;
            is = true;
        }
        XFree(prop);
    }
    return is;
}

Window YWindow::findDNDTarget(Window w, int x, int y) {
    Window root = RootWindow(app->display(), DefaultScreen(app->display()));
    Window w1 = w;
    Window w2 = root;
    int cx, cy;
    Window child;

    while (XTranslateCoordinates(app->display(),
                                 w1,
                                 w2,
                                 x, y,
                                 &cx, &cy,
                                 &child))
    {
        if (w2 != root && isDNDAware(w2)) {
            //printf("found target x=%d y=%d", cx, cy);
            return w2;
        } else if (child == None) {
            return 0;
        }
        w1 = w2;
        w2 = child;
        x = cx;
        y = cy;
    }
    return 0;
}

void YWindow::setDndTarget(Window dnd) {
    if (dnd != XdndDragTarget) {
        if (XdndDragTarget) { // XdndLeave
            XClientMessageEvent msg;

            memset(&msg, 0, sizeof(msg));
            msg.type = ClientMessage;
            msg.display = app->display();
            msg.window = XdndDragTarget;
            msg.message_type = XA_XdndLeave;
            msg.format = 32;
            msg.data.l[0] = handle();
            msg.data.l[1] = 0;
            XSendEvent(app->display(), XdndDragTarget, False, 0L, (XEvent *)&msg);
            puts("send: XdndLeave");
        }
        XdndDragTarget = dnd;
        if (dnd) { // XdndEnter
            XClientMessageEvent msg;

            memset(&msg, 0, sizeof(msg));
            msg.type = ClientMessage;
            msg.display = app->display();
            msg.window = XdndDragTarget;
            msg.message_type = XA_XdndEnter;
            msg.format = 32;
            msg.data.l[0] = handle();
            msg.data.l[1] =
                ((XdndNumTypes > 3) ? 0x1 : 0x0) |
                (XdndTargetVersion << 24);
            msg.data.l[2] = (XdndNumTypes > 0) ? XdndTypes[0] : None;
            msg.data.l[3] = (XdndNumTypes > 1) ? XdndTypes[1] : None;
            msg.data.l[4] = (XdndNumTypes > 2) ? XdndTypes[2] : None;
            XSendEvent(app->display(), XdndDragTarget, False, 0L, (XEvent *)&msg);
            puts("send: XdndEnter");
        }
    }
}

void YWindow::handleDNDCrossing(const XCrossingEvent &crossing) {
    if (crossing.type == EnterNotify) {
    }
}

void YWindow::sendNewPosition() {
    XClientMessageEvent msg;
    //int x_root = 0, y_root = 0;

    //mapToGlobal(x_root, y_root);

    memset(&msg, 0, sizeof(msg));
    msg.type = ClientMessage;
    msg.display = app->display();
    msg.window = XdndDragTarget;
    msg.message_type = XA_XdndPosition;
    msg.format = 32;
    msg.data.l[0] = handle();
    msg.data.l[1] = 0;
    msg.data.l[2] = (fNewPosX << 16) | fNewPosY;
    msg.data.l[3] = XdndTimestamp = app->getEventTime();
    msg.data.l[4] = None;
    XSendEvent(app->display(), XdndDragTarget, False, 0L, (XEvent *)&msg);
    puts("sendNew: XdndPosition");
    fWaitingForStatus = true;
}

void YWindow::handleDNDMotion(const XMotionEvent &motion) {
    puts("motion");
    Window dnd = findDNDTarget(motion.window, motion.x, motion.y);

    setDndTarget(dnd);
    fHaveNewPosition = false;

    if (XdndDragTarget != None) {
        if (fWaitingForStatus) {
            fHaveNewPosition = true;

            fNewPosX = motion.x_root;
            fNewPosY = motion.y_root;
            puts("send position later");

        } else {
            XClientMessageEvent msg;
            //int x_root = 0, y_root = 0;

            //mapToGlobal(x_root, y_root);

            memset(&msg, 0, sizeof(msg));
            msg.type = ClientMessage;
            msg.display = app->display();
            msg.window = XdndDragTarget;
            msg.message_type = XA_XdndPosition;
            msg.format = 32;
            msg.data.l[0] = handle();
            msg.data.l[1] = 0;
            msg.data.l[2] = (motion.x_root << 16) | motion.y_root;
            msg.data.l[3] = XdndTimestamp = app->getEventTime();
            msg.data.l[4] = None;
            XSendEvent(app->display(), XdndDragTarget, False, 0L, (XEvent *)&msg);
            puts("send: XdndPosition");
            fWaitingForStatus = true;
        }
    }
}

void YWindow::handleXdnd(const XClientMessageEvent &message) {
    //puts("handleXdnd");
    if (message.message_type == XA_XdndEnter) {
        puts("-- enter");
        //printf("XdndEnter source=%lX\n", message.data.l[0]);
        XdndDragSource = message.data.l[0];
        //?XdndDropTarget = 0;

        delete [] XdndTypes; XdndTypes = 0;
        XdndNumTypes = 0;

        int nTargets = 0;
        Atom *targets;

        if (message.data.l[1] & 1) { // more than 3 Types/XdndTypeList
            Atom r_type;
            int r_format;
            unsigned long count;
            unsigned long bytes_remain;
            unsigned char *prop;

            //puts(">3 Types");

            if (XGetWindowProperty(app->display(),
                                   XdndDragSource,
                                   XA_XdndTypeList,
                                   0, 256, False, XA_ATOM,
                                   &r_type, &r_format,
                                   &count, &bytes_remain, &prop) == Success && prop)
            {
                Atom *atoms = (Atom *)prop;
                //printf("r_format=%d count=%d\n", r_format, count);
                if (r_format == 32) {
                    nTargets = count;
                    if (nTargets > 0)
                        targets = new Atom[nTargets];
                    else
                        targets = 0;
                    for (int i = 0; i < count; i++)
                        targets[i] = atoms[i];
                }
                XFree(prop);
            }
        } else {
            nTargets = 0;
            if (message.data.l[2]) nTargets++;
            if (message.data.l[3]) nTargets++;
            if (message.data.l[4]) nTargets++;

            if (nTargets > 0)
                targets = new Atom[nTargets];
            else
                targets = 0;
            if (targets != 0) {
                if (nTargets >= 1)
                    targets[0] = message.data.l[2];
                if (nTargets >= 2)
                    targets[1] = message.data.l[3];
                if (nTargets >= 3)
                    targets[2] = message.data.l[4];
            }
        }
        XdndNumTypes = nTargets;
        XdndTypes = targets;
    } else if (message.message_type == XA_XdndLeave) {
        puts("-- leave");
        //printf("XdndLeave source=%lX\n", message.data.l[0]);
        if (XdndDropTarget) {
            YWindow *win;

            if (XFindContext(app->display(), XdndDropTarget, windowContext,
                             (XPointer *)&win) == 0)
                win->handleDNDLeave();
        }
        XdndDragSource = None;
        XdndDropTarget = None;
        delete [] XdndTypes; XdndTypes = 0;
        XdndNumTypes = 0;
    } else if (message.message_type == XA_XdndPosition &&
               XdndDragSource != 0)
    {
        Window target, child;
        int x, y, nx, ny;
        YWindow *pwin = 0;
        Atom action = None;

        puts("-- position");
        XdndDragSource = message.data.l[0];
        x = int(message.data.l[2] >> 16);
        y = int(message.data.l[2] & 0xFFFF);

        target = handle();
        action = message.data.l[4];
        XdndTimestamp = message.data.l[3];//??

        /*printf("XdndPosition source=%lX %d:%d time=%ld action=%ld window=%ld\n",
                 message.data.l[0],
                 x, y,
                 message.data.l[3],
                 message.data.l[4],
                 XdndDropTarget);*/


        //do {
        while (XTranslateCoordinates(app->display(),
                                     desktop->handle(), target,
                                     x, y, &nx, &ny, &child))
        {
            if (child)
                target = child;
            else
                break;
        }

        if (target != XdndDropTarget) {
            if (XdndDropTarget) {
                YWindow *win;

                if (XFindContext(app->display(), XdndDropTarget, windowContext,
                                 (XPointer *)&win) == 0)
                    win->handleDNDLeave();
            }
            XdndDropTarget = target;
            //printf("drop target=%lX\n", XdndDropTarget);
            if (XdndDropTarget) {
                YWindow *win;

                if (XFindContext(app->display(), XdndDropTarget, windowContext,
                                 (XPointer *)&win) == 0)
                {
                    win->handleDNDEnter(XdndNumTypes, XdndTypes);
                    pwin = win;
                }
            }
        }

        if (pwin == 0 && XdndDropTarget) { // !!! optimize this
            if (XFindContext(app->display(), XdndDropTarget, windowContext,
                             (XPointer *)&pwin) != 0)
                pwin = 0;
        }

        bool willAccept = false;
        if (pwin)
            willAccept = pwin->handleDNDPosition(nx, ny, &action);
        //printf("XdndPosition %d:%d target=%ld\n", nx, ny, XdndDropTarget);
        XdndStatus(willAccept, willAccept ? action : None);
    } else if (message.message_type == XA_XdndStatus) {
        printf("XdndStatus\n");
        fGotStatus = true;
        fWaitingForStatus = false;
        if (fHaveNewPosition) {
            sendNewPosition();
            fHaveNewPosition = false;
        }
        if (fEndDrag) {
            endDrag(fDoDrop);
            fEndDrag = false;
        }
    } else if (message.message_type == XA_XdndDrop) {
        handleDNDDrop();
        //printf("XdndDrop\n");
        if (XdndDropTarget) {
            YWindow *win;

            if (XFindContext(app->display(), XdndDropTarget, windowContext,
                             (XPointer *)&win) == 0)
                win->handleDNDLeave();
        }
        XdndDragSource = None;
        XdndDropTarget = None;
        delete [] XdndTypes; XdndTypes = 0;
        XdndNumTypes = 0;
    } else if (message.message_type == XA_XdndFinished) {
        handleDNDFinish();
        //printf("XdndFinished\n");
    }
}

void YWindow::handleDNDDrop() {
    finishDrop();
}

void YWindow::handleDNDFinish() {
}

void YWindow::handleDNDEnter(int /*nTypes*/, Atom * /*types*/) {
}

void YWindow::handleDNDLeave() {
}

bool YWindow::handleDNDPosition(int /*x*/, int /*y*/, Atom * /*action*/) {
    return false;
}

bool YWindow::handleAutoScroll(const XMotionEvent & /*mouse*/) {
    return false;
}

void YWindow::beginAutoScroll(bool doScroll, const XMotionEvent *motion) {
    if (fAutoScroll == 0)
        fAutoScroll = new AutoScroll();
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

    XSetSelectionOwner(app->display(), sel, handle(), app->getEventTime());
}

void YWindow::clearSelection(bool selection) {
    Atom sel = selection ? XA_PRIMARY : _XA_CLIPBOARD;

    XSetSelectionOwner(app->display(), sel, None, app->getEventTime());
}

void YWindow::requestSelection(bool selection) {
    Atom sel = selection ? XA_PRIMARY : _XA_CLIPBOARD;

    XConvertSelection(app->display(),
                      sel, XA_STRING,
                      sel, handle(), app->getEventTime());
}

YDesktop::YDesktop(YWindow *aParent, Window win):
    YWindow(aParent, win)
{
    desktop = this;
}
YDesktop::~YDesktop() {
}

void YDesktop::resetColormapFocus(bool /*active*/) {
}

void YDesktop::manageWindow(YWindow *w, bool mapWindow) {
    if (mapWindow) //!!! ???
        w->show();
}

void YDesktop::unmanageWindow(YWindow *w) {
    w->hide();
}

void YWindow::grabVKey(int key, unsigned int vm) {
    int m = 0;

    if (vm & kfShift)
        m |= ShiftMask;
    if (vm & kfCtrl)
        m |= ControlMask;
    if (vm & kfAlt)
        m |= app->AltMask;
    if (vm & kfMeta)
        m |= app->MetaMask;
    if (vm & kfSuper)
       m |= app->SuperMask;
    if (vm & kfHyper)
       m |= app->HyperMask;

    if (key != 0 && (vm == 0 || m != 0)) {
        if ((!(vm & kfMeta) || app->MetaMask) &&
            (!(vm & kfAlt) || app->AltMask) &&
           (!(vm & kfSuper) || app->SuperMask) &&
           (!(vm & kfHyper) || app->HyperMask))
            grabKey(key, m);

#if 0
        if (((vm & (kfAlt | kfCtrl)) == (kfAlt | kfCtrl)) &&
            modMetaIsCtrlAlt &&
            app->MetaMask)
        {
            m = app->MetaMask;
            if (vm & kfShift)
                m |= ShiftMask;
           if (vm & kfSuper)
               m |= app->SuperMask;
           if (vm & kfHyper)
               m |= app->HyperMask;
            grabKey(key, m);
        }
#endif
    }
}

unsigned int YWindow::VMod(int m) {
    int vm = 0;

    if (m & ShiftMask)
        vm |= kfShift;
    if (m & ControlMask)
        vm |= kfCtrl;
    if (m & app->AltMask)
        vm |= kfAlt;
    if (m & app->MetaMask)
        vm |= kfMeta;
    if (m & app->SuperMask)
       vm |= kfSuper;
    if (m & app->HyperMask)
       vm |= kfHyper;

#if 0
    if (modMetaIsCtrlAlt && (vm & (kfCtrl | kfAlt | kfMeta)) == kfMeta)
        vm = (vm & ~kfMeta) | kfCtrl | kfAlt;
#endif

    return vm;
}

bool YWindow::getCharFromEvent(const XKeyEvent &key, char *c) {
    int klen;
    char keyBuf[1];
    KeySym ksym;
    XKeyEvent kev = key;

    //printf("key=%d\n", key.keycode);
    klen = XLookupString(&kev, keyBuf, 1, &ksym, NULL);
    if (klen == 1) {
        *c = keyBuf[0];
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
        scrollGC = XCreateGC(app->display(), desktop->handle(), 0, NULL);
    }

    XCopyArea(app->display(), handle(), handle(), scrollGC,
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
    //printf("nr=%d\n", nr);

    XSetClipRectangles(app->display(), g.handle(),
                       0, 0, r, nr, Unsorted); // !!! optimize Unsorted?

    XRectangle re;
    if (nr == 1)
        re = r[0];
    else {
        re.x = 0;
        re.y = 0;
        re.width = width();
        re.height = height();
    }

    paint(g, re.x, re.y, re.width, re.height); // !!! add flag to do minimal redraws

    XSetClipMask(app->display(), g.handle(), None);

    {
        XEvent e;

        while (XCheckTypedWindowEvent(app->display(), handle(), GraphicsExpose, &e)) {
            handleGraphicsExpose(e.xgraphicsexpose);
            if (e.xgraphicsexpose.count == 0)
                break;
        }
    }
}

