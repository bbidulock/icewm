/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "yutil.h"
#include "ywindow.h"

#include "ytooltip.h"
#include "yapp.h"
#include "sysdep.h"
#include "prefs.h"

#include "ytimer.h"

/******************************************************************************/
/******************************************************************************/

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
};


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
        fAutoScrollTimer = new YTimer(autoScrollStartDelay);
        fAutoScrollTimer->setTimerListener(this);
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

/******************************************************************************/
/******************************************************************************/

extern XContext windowContext;
AutoScroll *YWindow::fAutoScroll = 0;
YWindow *YWindow::fClickWindow = 0;
Time YWindow::fClickTime = 0;
int YWindow::fClickCount = 0;
XButtonEvent YWindow::fClickEvent;
int YWindow::fClickDrag = 0;
unsigned int YWindow::fClickButton = 0;
unsigned int YWindow::fClickButtonDown = 0;

/******************************************************************************/
/******************************************************************************/

YWindowAttributes::YWindowAttributes(Window window) {
    if (!XGetWindowAttributes(app->display(), window, &attributes)) {
	XGetGeometry(app->display(), window, &attributes.root,
		     &attributes.x, &attributes.y,
		     (unsigned *) &attributes.width,
		     (unsigned *) &attributes.height,
		     (unsigned *) &attributes.border_width, 
		     (unsigned *) &attributes.depth);

	attributes.visual = app->visual();
	attributes.colormap = app->colormap();
   }
}

/******************************************************************************/
/******************************************************************************/

YWindow::YWindow(YWindow *parent, Window win):
    fParentWindow(parent),
    fNextWindow(0), fPrevWindow(0), fFirstWindow(0), fLastWindow(0),
    fFocusedWindow(0),

    fHandle(win), flags(0), fStyle(0), fX(0), fY(0), fWidth(1), fHeight(1),
    fPointer(), unmapCount(0), fGraphics(0),
    fEventMask(KeyPressMask|KeyReleaseMask|FocusChangeMask|
	       LeaveWindowMask|EnterWindowMask),
    fWinGravity(NorthWestGravity), fBitGravity(ForgetGravity),
    fEnabled(true), fToplevel(false), accel(0),
#ifdef CONFIG_TOOLTIP
    fToolTip(0),
#endif
    fDND(false), XdndDragSource(None), XdndDropTarget(None) {
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
    XSetInputFocus(app->display(), handle(), RevertToNone, CurrentTime);
}

void YWindow::setTitle(char const * title) {
    XStoreName(app->display(), handle(), title);
}

void YWindow::setClassHint(char const * rName, char const * rClass) {
    XClassHint wmclass;
    wmclass.res_name = (char *) rName;
    wmclass.res_class = (char *) rClass;

    XSetClassHint(app->display(), handle(), &wmclass);
}

void YWindow::setStyle(unsigned long aStyle) {
    if (fStyle != aStyle) {
        fStyle = aStyle;

        if (flags & wfCreated) {
            if (aStyle & wsManager)
                fEventMask |=
                    SubstructureRedirectMask | SubstructureNotifyMask;

            if (fStyle & wsPointerMotion)
                fEventMask |= PointerMotionMask;

            if (!grabRootWindow &&
                fHandle == RootWindow(app->display(), DefaultScreen(app->display())))
                fEventMask &= ~(ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);
            else
                fEventMask |= ButtonPressMask | ButtonReleaseMask | ButtonMotionMask;

            XSelectInput(app->display(), fHandle, fEventMask);
        }
    }
}

Graphics &YWindow::getGraphics() {
    return *(NULL == fGraphics ? fGraphics = new Graphics(*this) : fGraphics);
}

void YWindow::repaint() {
//    if ((flags & (wfCreated | wfVisible)) == (wfCreated | wfVisible)) {
    if (viewable()) paint(getGraphics(), 0, 0, width(), height());
}

void YWindow::repaintFocus() {
//    if ((flags & (wfCreated | wfVisible)) == (wfCreated | wfVisible)) {
    if (viewable()) paintFocus(getGraphics(), 0, 0, width(), height());
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

        if (!grabRootWindow &&
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
    if (!(flags & wfCreated)) create();
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
        for (YWindow *w = this; // !!! hack, fix
	     w && w->handleKey(event.xkey) == false;
	     w = w->parent());
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
	handleMap(event.xmap);
	break;

    case UnmapNotify:
	handleUnmap(event.xunmap);
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

#ifdef CONFIG_SHAPE
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
    if (configure.window == handle()) {
	const bool resized((unsigned int)configure.width != fWidth ||
			   (unsigned int)configure.height != fHeight);
	
        if (configure.x != fX ||
            configure.y != fY ||
            resized)
        {
            fX = configure.x;
            fY = configure.y;
            fWidth = configure.width;
            fHeight = configure.height;

            this->configure(fX, fY, fWidth, fHeight, resized);
        }
    }	
}

bool YWindow::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        KeySym k = XKeycodeToKeysym(app->display(), key.keycode, 0);
        unsigned int m = KEY_MODMASK(key.state);

        if (accel) {
            YAccelerator *a;

            for (a = accel; a; a = a->next) {
                //msg("%c %d - %c %d %d", k, k, a->key, a->key, a->mod);
                if (m == a->mod && k == a->key)
                    if (a->win->handleKey(key) == true)
                        return true;
            }
            if (ISLOWER(k)) {
                k = TOUPPER(k);
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

            if ((motion.time - fClickTime > (unsigned) ClickMotionDelay) ||
                (motionDelta >= ClickMotionDistance)) {
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

void YWindow::handleMap(const XMapEvent &) {
//    flags |= wfVisible; // !!! WTF does this 'cause such odd side effects?
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

void YWindow::paint(Graphics &g, int x, int y, unsigned int w, unsigned int h) {
    g.fillRect(x, y, w, h);
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
    const bool resized(width != fWidth || height != fHeight);

    if (x != fX || y != fY || resized) {
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

        configure(fX, fY, fWidth, fHeight, resized);
    }
}

void YWindow::setPosition(int x, int y) {
    if (x != fX || y != fY) {
        fX = x;
        fY = y;

        if (flags & wfCreated)
            XMoveWindow(app->display(), fHandle, fX, fY);

        configure(fX, fY, width(), height(), false);
    }
}

void YWindow::setSize(unsigned int width, unsigned int height) {
    if (width != fWidth || height != fHeight) {
        fWidth = width;
        fHeight = height;

        if (flags & wfCreated)
            if (!nullGeometry())
                XResizeWindow(app->display(), fHandle, fWidth, fHeight);

        configure(x(), y(), fWidth, fHeight, true);
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

void YWindow::configure(const int, const int, const unsigned, const unsigned, 
			const bool) {
}

void YWindow::setPointer(const YCursor& pointer) {
    fPointer = pointer;

    if (flags & wfCreated) {
        XSetWindowAttributes attributes;
        attributes.cursor = fPointer.handle();
        XChangeWindowAttributes(app->display(), handle(),
				CWCursor, &attributes);
    }
}

void YWindow::setGrabPointer(const YCursor& pointer) {
    XChangeActivePointerGrab(app->display(),
    			     ButtonPressMask|PointerMotionMask|
    			     ButtonReleaseMask,
                             pointer.handle(), CurrentTime);
			     //app->getEventTime());
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

bool YWindow::viewable(Drawable drawable) {
    XWindowAttributes attributes;
    XGetWindowAttributes(app->display(), drawable, &attributes);
    return (IsViewable == attributes.map_state);
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
        setFocus(0);///???!!! is this the right place?
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

void YWindow::removeAccelerator(unsigned int key, unsigned int mod, YWindow *win) {
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

const Atom XdndCurrentVersion = 3;

void YWindow::setDND(bool enabled) {
    if (fDND != enabled) {
        fDND = enabled;

        if (fDND) {
            XChangeProperty(app->display(), handle(),
                            XA_XdndAware, XA_ATOM, // !!! ATOM?
                            32, PropModeReplace,
                            (const unsigned char *)&XdndCurrentVersion, 1);
        } else {
            XDeleteProperty(app->display(), handle(), XA_XdndAware);
        }
    }
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
    msg.data.l[1] = (acceptDrop ? 0x00000001 : 0x00000000) | 2;
    msg.data.l[2] = (x_root << 16) + y_root;
    msg.data.l[3] = (width() << 16) + height();
    msg.data.l[4] = dropAction;
    XSendEvent(app->display(), XdndDragSource, False, 0L, (XEvent *)&msg);
}

void YWindow::handleXdnd(const XClientMessageEvent &message) {
    if (message.message_type == XA_XdndEnter) {
        //msg("XdndEnter source=%lX", message.data.l[0]);
        XdndDragSource = message.data.l[0];
    } else if (message.message_type == XA_XdndLeave) {
        //msg("XdndLeave source=%lX", message.data.l[0]);
        if (XdndDropTarget) {
            YWindow *win;

            if (XFindContext(app->display(), XdndDropTarget, windowContext,
                             (XPointer *)&win) == 0)
                win->handleDNDLeave();
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

        /*printf("XdndPosition source=%lX %d:%d time=%ld action=%ld window=%ld\n",
                 message.data.l[0],
                 x, y,
                 message.data.l[3],
                 message.data.l[4],
                 XdndDropTarget);*/


        do {
            if (XTranslateCoordinates(app->display(),
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

                if (XFindContext(app->display(), XdndDropTarget, windowContext,
                                 (XPointer *)&win) == 0)
                    win->handleDNDLeave();
            }
            XdndDropTarget = target;
            if (XdndDropTarget) {
                YWindow *win;

                if (XFindContext(app->display(), XdndDropTarget, windowContext,
                                 (XPointer *)&win) == 0)
                {
                    win->handleDNDEnter();
                    pwin = win;
                }
            }
        }
        if (pwin == 0 && XdndDropTarget) { // !!! optimize this
            if (XFindContext(app->display(), XdndDropTarget, windowContext,
                             (XPointer *)&pwin) != 0)
                pwin = 0;
        }
        if (pwin)
            pwin->handleDNDPosition(nx, ny);
        /*printf("XdndPosition %d:%d target=%ld\n",
               nx, ny, XdndDropTarget);*/
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
        //msg("XdndStatus");
    } else if (message.message_type == XA_XdndDrop) {
        //msg("XdndDrop");
    } else if (message.message_type == XA_XdndFinished) {
        //msg("XdndFinished");
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

        // !!! recheck this
        if (((vm & (kfAlt | kfCtrl)) == (kfAlt | kfCtrl)) &&
            modMetaIsCtrlAlt &&
            app->WinMask)
        {
            m = app->WinMask;
            if (vm & kfShift)
                m |= ShiftMask;
            if (vm & kfSuper)
                m |= app->SuperMask;
            if (vm & kfHyper)
                m |= app->HyperMask;
            grabKey(key, m);
        }
    }
}

unsigned int YWindow::VMod(int m) {
    int vm = 0;
    int m1 = m & ~app->WinMask;

    if (m & app->WinMask) {
        if (modMetaIsCtrlAlt) {
            vm |= kfCtrl + kfAlt;
        } else if (app->WinMask == app->MetaMask) {
            vm |= kfMeta;
        }
    }

    if (m1 & ShiftMask)
        vm |= kfShift;
    if (m1 & ControlMask)
        vm |= kfCtrl;
    if (m1 & app->AltMask)
        vm |= kfAlt;
    if (m1 & app->MetaMask)
        vm |= kfMeta;
    if (m1 & app->SuperMask)
       vm |= kfSuper;
    if (m1 & app->HyperMask)
       vm |= kfHyper;

    return vm;
}

bool YWindow::getCharFromEvent(const XKeyEvent &key, char *c) {
    char keyBuf[1];
    KeySym ksym;
    XKeyEvent kev = key;

    // FIXME:
    int klen = XLookupString(&kev, keyBuf, 1, &ksym, NULL);
#ifndef USE_XmbLookupString
    if ((klen == 0)  && (ksym < 0x1000)) {
        klen = 1;
        keyBuf[0] = ksym & 0xFF;
    }
#endif
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
    //msg("nr=%d", nr);

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
