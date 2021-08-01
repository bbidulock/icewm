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
#include "yxcontext.h"
#include <typeinfo>

#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

/******************************************************************************/
/******************************************************************************/

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
    fWindow = nullptr;
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
        w = nullptr;
    fWindow = w;
    if (w == nullptr)
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

YAutoScroll *YWindow::fAutoScroll = nullptr;
YWindow *YWindow::fClickWindow = nullptr;
Time YWindow::fClickTime = 0;
int YWindow::fClickCount = 0;
XButtonEvent YWindow::fClickEvent;
bool YWindow::fClickDrag;
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

YWindow::YWindow(YWindow *parent, Window win, int depth,
                 Visual *visual, Colormap colormap):
    fDepth(depth ? unsigned(depth) : xapp->depth()),
    fVisual(visual ? visual : xapp->visual()),
    fColormap(colormap),
    fParentWindow(parent),
    fFocusedWindow(nullptr),
    fHandle(win), flags(0), fStyle(0),
    fX(0), fY(0), fWidth(1), fHeight(1),
    unmapCount(0),
    fPointer(),
    fGraphics(nullptr),
    fEventMask(KeyPressMask|KeyReleaseMask|FocusChangeMask|
               LeaveWindowMask|EnterWindowMask),
    fWinGravity(NorthWestGravity), fBitGravity(ForgetGravity),
    accel(nullptr)
{
    if (fHandle != None) {
        MSG(("adopting window %lX", fHandle));
        flags |= wfAdopted;
        adopt();
    }
    else if (fParentWindow == nullptr) {
        PRECONDITION(desktop);
        fParentWindow = desktop;
    }
    insertWindow();
}

YWindow::~YWindow() {
    if (fAutoScroll &&
        fAutoScroll->getWindow() == this)
    {
        delete fAutoScroll;
        fAutoScroll = nullptr;
    }
    fFocusedWindow = nullptr;
    removeWindow();
    while (nextWindow() != nullptr)
           nextWindow()->removeWindow();
    while (accel) {
        YAccelerator *next = accel->next;
        delete accel;
        accel = next;
    }
    if (fClickWindow == this)
        fClickWindow = nullptr;
    if (fGraphics) {
        delete fGraphics; fGraphics = nullptr;
    }
    if (flags & wfCreated)
        destroy();
}

Colormap YWindow::colormap() {
    if (fColormap == None) {
        if (fParentWindow
            && (fVisual == CopyFromParent || fVisual == fParentWindow->fVisual)
            && (fDepth == CopyFromParent || fDepth == fParentWindow->fDepth))
        {
            fColormap = fParentWindow->colormap();
        }
        if (fVisual && fColormap == None) {
            fColormap = xapp->colormapForVisual(fVisual);
        }
        if (fDepth && fColormap == None) {
            fColormap = xapp->colormapForDepth(fDepth);
        }
        if (fColormap == None) {
            // unexpected
            fColormap = XCreateColormap(xapp->display(), xapp->root(),
                                        fVisual ? fVisual : xapp->visual(),
                                        AllocNone);
        }
    }
    return fColormap;
}

void YWindow::setWindowFocus(Time timestamp) {
    XSetInputFocus(xapp->display(), handle(), RevertToNone, timestamp);
}

void YWindow::setTitle(char const * title) {
    XStoreName(xapp->display(), handle(), title);
}

bool YWindow::fetchTitle(char** title) {
    return XFetchName(xapp->display(), handle(), title);
}

void YWindow::setClassHint(char const * rName, char const * rClass) {
    XClassHint wmclass;
    wmclass.res_name = const_cast<char *>(rName);
    wmclass.res_class = const_cast<char *>(rClass);

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

            if (fStyle & wsNoExpose)
                fEventMask &= ~ExposureMask;

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
    return *(fGraphics ? fGraphics : fGraphics = new Graphics(*this));
}

void YWindow::repaint() {
    XClearArea(xapp->display(), handle(), 0, 0, 0, 0, True);
}

void YWindow::repaintFocus() {
    repaint();
}

bool YWindow::getWindowAttributes(XWindowAttributes* attr) {
    if (fHandle == None)
        return false;

    if (XGetWindowAttributes(xapp->display(), fHandle, attr))
        return true;

    setDestroyed();
    return false;
}

void YWindow::readAttributes() {
    XWindowAttributes attributes;

    if (!getWindowAttributes(&attributes))
        return;

    fX = attributes.x;
    fY = attributes.y;
    fWidth = unsigned(attributes.width);
    fHeight = unsigned(attributes.height);

    if (fHandle != xapp->root()) {
        fDepth = attributes.depth;
        fVisual = attributes.visual;
        fColormap = attributes.colormap;
    }

    //MSG(("window initial geometry (%d:%d %dx%d)",
    //     fX, fY, fWidth, fHeight));

    if (attributes.map_state != IsUnmapped)
        flags |= wfVisible;
    else
        flags &= unsigned(~wfVisible);
}

Window YWindow::create() {
    if (flags & wfCreated)
        return fHandle;

    XSetWindowAttributes attributes = { 0, };
    unsigned int attrmask = CWEventMask;
    const bool output = notbit(fStyle, wsInputOnly);

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
    if (fStyle & (wsInputOnly | wsNoExpose)) {
        fEventMask &= ~(ExposureMask);
        if (fStyle & wsInputOnly)
            fEventMask &= ~(FocusChangeMask);
    }

    if (fStyle & wsSaveUnder) {
        attributes.save_under = True;
        attrmask |= CWSaveUnder;
    }
    if (fStyle & wsOverrideRedirect) {
        attributes.override_redirect = True;
        attrmask |= CWOverrideRedirect;
    }
    if (fStyle & wsBackingMapped) {
        attributes.backing_store = WhenMapped;
        attrmask |= CWBackingStore;
    }
    if (fPointer) {
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
    if (fDepth && output) {
        attributes.background_pixel = xapp->black();
        attrmask |= CWBackPixel;
        attributes.border_pixel = xapp->black();
        attrmask |= CWBorderPixel;
    }
    if (fVisual && output) {
        attributes.colormap = colormap();
        attrmask |= CWColormap;
    }

    attributes.event_mask = fEventMask;
    unsigned zw = width();
    unsigned zh = height();
    if (zw == 0 || zh == 0) {
        zw = 1;
        zh = 1;
        flags |= wfNullSize;
    }
    fHandle = XCreateWindow(xapp->display(),
                            parent()->handle(),
                            x(), y(), zw, zh,
                            0,
                            output ? fDepth : CopyFromParent,
                            output ? InputOutput : InputOnly,
                            output ? fVisual : CopyFromParent,
                            attrmask,
                            &attributes);

    XWindowAttributes wa;
    if (XGetWindowAttributes(xapp->display(), fHandle, &wa) == False) {
        flags |= (wfCreated | wfDestroyed);
        return None;
    }
    fDepth = unsigned(wa.depth);
    fVisual = wa.visual;
    if (parent() == desktop && !(flags & (wsManager | wsOverrideRedirect))) {
        Atom prot[] = { _XA_WM_DELETE_WINDOW, _XA_WM_TAKE_FOCUS };
        const int n = 1 + hasbit(flags, wsTakeFocus);
        XSetWMProtocols(xapp->display(), fHandle, prot, n);
    }

    if ((flags & wfVisible) && !(flags & wfNullSize))
        XMapWindow(xapp->display(), fHandle);

    windowContext.save(fHandle, this);
    flags |= wfCreated;

    return fHandle;
}

void YWindow::adopt() {
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


        if (!grabRootWindow && fHandle == xapp->root()) {
            fEventMask &= ~(ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);
        }
    }

    if (destroyed() == false)
        XSelectInput(xapp->display(), fHandle, fEventMask);

    windowContext.save(fHandle, this);
    flags |= wfCreated;
}

void YWindow::destroy() {
    if (flags & wfCreated) {
        if (!(flags & wfDestroyed)) {
            setDestroyed();
            if (!(flags & wfAdopted)) {
                MSG(("----------------------destroy %lX", fHandle));
                XDestroyWindow(xapp->display(), fHandle);
                removeAllIgnoreUnmap();
            } else {
                XSelectInput(xapp->display(), fHandle, NoEventMask);
            }
        }
        windowContext.remove(fHandle);
        fHandle = None;
        flags &= unsigned(~wfCreated);
    }
}
void YWindow::removeWindow() {
    if (fParentWindow) {
        fParentWindow->remove(this);
        fParentWindow = nullptr;
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
    if ((flags & (wfVisible | wfDestroyed)) == wfVisible) {
        addIgnoreUnmap();
    }

    removeWindow();
    fParentWindow = parent;
    insertWindow();

    if (notbit(flags, wfDestroyed)) {
        MSG(("--- reparent %lX to %lX", handle(), parent->handle()));
        XReparentWindow(xapp->display(), handle(), parent->handle(), x, y);
    }
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
        flags &= unsigned(~wfVisible);
        if (!(flags & (wfNullSize | wfDestroyed))) {
            addIgnoreUnmap();
            XUnmapWindow(xapp->display(), handle());
        }
    }
}

void YWindow::setDestroyed() {
    flags |= wfDestroyed;
}

bool YWindow::testDestroyed() {
    XWindowAttributes attr;
    return fHandle && (destroyed() || !getWindowAttributes(&attr));
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

void YWindow::beneath(YWindow* superior) {
    if (superior) {
        Window stack[] = { superior->handle(), handle(), };
        XRestackWindows(xapp->display(), stack, 2);
    }
}

void YWindow::raiseTo(YWindow* inferior) {
    if (inferior) {
        unsigned mask = CWSibling | CWStackMode;
        XWindowChanges xwc;
        xwc.sibling = inferior->handle();
        xwc.stack_mode = Above;
        XConfigureWindow(xapp->display(), handle(), mask, &xwc);
    }
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
             const long mask = KeyPressMask |
                               KeyReleaseMask |
                               ButtonPressMask |
                               ButtonReleaseMask |
                               ButtonMotionMask;
             XEvent old_event(event), new_event;

             while (XCheckMaskEvent(xapp->display(), mask, &new_event)) {
                 if (event.type != new_event.type ||
                     event.xmotion.window != new_event.xmotion.window ||
                     event.xmotion.subwindow != new_event.xmotion.subwindow)
                 {
                     XPutBackEvent(xapp->display(), &new_event);
                     break;
                 } else {
                     xapp->sync();
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
        if (damage.isEvent(event.type, XDamageNotify)) {
            handleDamageNotify(
                *reinterpret_cast<const XDamageNotifyEvent *>(&event));
            break;
        }
#ifdef CONFIG_SHAPE
        if (shapes.isEvent(event.type, ShapeNotify)) {
            handleShapeNotify(*reinterpret_cast<const XShapeEvent *>(&event));
            break;
        }
#endif
#ifdef CONFIG_XRANDR
        if (xrandr.isEvent(event.type, RRScreenChangeNotify)) {
            handleRRScreenChangeNotify(
                reinterpret_cast<const XRRScreenChangeNotifyEvent&>(event));
            break;
        }
        else if (xrandr.isEvent(event.type, RRNotify)) {
            handleRRNotify(reinterpret_cast<const XRRNotifyEvent&>(event));
            break;
        }
#endif
        break;
    }
}

/// TODO #warning "implement expose compression"
void YWindow::paintExpose(int ex, int ey, int ew, int eh) {
    if (ex < 0) {
        ew += ex;
        ex = 0;
    }
    if (ey < 0) {
        eh += ey;
        ey = 0;
    }
    ew = min(ew, int(width()) - ex);
    eh = min(eh, int(height()) - ey);
    if (ew > 0 && eh > 0) {
        Graphics& g(getGraphics());
        XRectangle r = {
            short(ex),
            short(ey),
            static_cast<unsigned short>(ew),
            static_cast<unsigned short>(eh),
        };
        g.setClipRectangles(&r, 1);
        paint(g, r);
        g.resetClip();
    }
}

void YWindow::handleExpose(const XExposeEvent &expose) {
    paintExpose(expose.x, expose.y, expose.width, expose.height);
}

void YWindow::handleGraphicsExpose(const XGraphicsExposeEvent &expose) {
    paintExpose(expose.x, expose.y, expose.width, expose.height);
}

void YWindow::handleConfigure(const XConfigureEvent &configure) {
    if (configure.window == handle()) {
        YRect evt(configure.x, configure.y, configure.width, configure.height);
        YRect old(geometry());
        if (evt != old)
        {
            fX = configure.x;
            fY = configure.y;
            fWidth = configure.width;
            fHeight = configure.height;

            this->configure(YRect2(evt, old));
        }
    }
}

void YWindow::handleGravityNotify(const XGravityEvent& gravity) {
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
                    if (a->win->handleKey(key))
                        return true;
            }
            if (ASCII::isLower(char(k))) {
                k = ASCII::toUpper(char(k));
                for (a = accel; a; a = a->next)
                    if (m == a->mod && k == a->key)
                        if (a->win->handleKey(key))
                            return true;
            }
        }
        if (k == XK_Tab) {
            if (m == 0) {
                nextFocus();
                return true;
            }
            else if (m == ShiftMask) {
                prevFocus();
                return true;
            }
        }
    }
    return false;
}

void YWindow::handleButton(const XButtonEvent &button) {
    if (fToolTip) {
        fToolTip->leave();
    }

    int const dx(abs(button.x_root - fClickEvent.x_root));
    int const dy(abs(button.y_root - fClickEvent.y_root));
    int const motionDelta(max(dx, dy));

    if (button.type == ButtonPress) {
        fClickDrag = false;

        if (fClickWindow != this) {
            fClickWindow = this;
            fClickCount = 1;
        } else {
            if (button.time < fClickTime + MultiClickTime &&
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
    }
    else if (button.type == ButtonRelease) {
        if ((fClickWindow == this) &&
            !fClickDrag &&
            fClickCount > 0 &&
            fClickButtonDown == button.button &&
            // motionDelta <= ClickMotionDistance &&
            button.x >= 0 && button.y >= 0 &&
            button.x < int(width()) && button.y < int(height()))
        {
            fClickButtonDown = 0;
            handleClick(button, fClickCount);
        } else {
            fClickWindow = nullptr;
            fClickCount = 1;
            fClickButtonDown = 0;
            fClickButton = 0;
            if (fClickDrag) {
                handleEndDrag(fClickEvent, button);
                fClickDrag = false;
            }
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
            if (motion.time > fClickTime + ClickMotionDelay ||
                motionDelta >= ClickMotionDistance)
            {
                if ((motion.state & xapp->ButtonMask) / Button1Mask
                    == fClickButton)
                {
                    fClickDrag = handleBeginDrag(fClickEvent, motion);
                }
            }
        }
    }
}

bool YWindow::handleBeginDrag(const XButtonEvent &, const XMotionEvent &) {
    return false;
}

void YWindow::setToolTip(const mstring& tip) {
    if (tip == null) {
        fToolTip = null;
    } else {
        fToolTip->setText(tip);
    }
}

mstring YWindow::getToolTip() {
    return fToolTip ? fToolTip->getText() : null;
}

bool YWindow::toolTipVisible() {
    return fToolTip && fToolTip->visible();
}

void YWindow::updateToolTip() {
}

void YWindow::handleCrossing(const XCrossingEvent& crossing) {
    if (fToolTip || fStyle & wsToolTipping) {
        if (crossing.type == EnterNotify && crossing.mode == NotifyNormal) {
            updateToolTip();
            if (fToolTip) {
                fToolTip->enter(this);
            }
        }
        else if (crossing.type == LeaveNotify) {
            if (fToolTip) {
                fToolTip->leave();
            }
        }
    }
}

void YWindow::handleClientMessage(const XClientMessageEvent& message) {
    if (message.message_type == _XA_WM_PROTOCOLS
        && message.format == 32
        && message.data.l[0] == long(_XA_WM_DELETE_WINDOW))
    {
        handleClose();
    }
    else if (message.message_type == _XA_WM_PROTOCOLS
        && message.format == 32
        && message.data.l[0] == long(_XA_WM_TAKE_FOCUS))
    {
        gotFocus();
#if 0
        YWindow *w = getFocusWindow();
        if (w)
            w->gotFocus();
        else
            gotFocus();
#endif
    }
}

void YDndWindow::handleClientMessage(const XClientMessageEvent& message) {
    if (message.message_type == XA_XdndEnter) {
        handleXdndEnter(message);
    }
    else if (message.message_type == XA_XdndLeave) {
        handleXdndLeave(message);
    }
    else if (message.message_type == XA_XdndPosition) {
        handleXdndPosition(message);
    }
    else if (message.message_type == XA_XdndStatus) {
        handleXdndStatus(message);
    }
    else if (message.message_type == XA_XdndDrop) {
        handleXdndDrop(message);
    }
    else if (message.message_type == XA_XdndFinished) {
        handleXdndFinished(message);
    }
    else {
        YWindow::handleClientMessage(message);
    }
}

void YWindow::handleVisibility(const XVisibilityEvent& visibility) {
}

void YWindow::handleMapNotify(const XMapEvent &) {
    // ignore "map notify" not implemented or needed due to MapRequest event
}

void YWindow::handleUnmapNotify(const XUnmapEvent &xunmap) {
    if (xunmap.window == xunmap.event || xunmap.send_event) {
        if (ignoreUnmap() == false) {
            flags &= unsigned(~wfVisible);
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
        setDestroyed();
        removeAllIgnoreUnmap();
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
            addIgnoreUnmap();
            XUnmapWindow(xapp->display(), handle());
        }
    } else if ((flags & wfNullSize) && !zero) {
        flags &= unsigned(~wfNullSize);
        if (flags & wfVisible)
            XMapWindow(xapp->display(), handle());
    }
    return zero;
}

void YWindow::setGeometry(const YRect &r) {
    YRect old(geometry());

    if (r != old) {
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

        configure(YRect2(r, old));
    }
}

void YWindow::setPosition(int x, int y) {
    if (x != fX || y != fY) {
        YRect old(geometry());

        fX = x;
        fY = y;

        if (flags & wfCreated)
            XMoveWindow(xapp->display(), fHandle, fX, fY);

        configure(YRect2(geometry(), old));
    }
}

void YWindow::setSize(unsigned width, unsigned height) {
    if (width != fWidth || height != fHeight) {
        YRect old(geometry());

        fWidth = width;
        fHeight = height;

        if (flags & wfCreated)
            if (!nullGeometry())
                XResizeWindow(xapp->display(), fHandle, fWidth, fHeight);

        configure(YRect2(geometry(), old));
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

void YWindow::setBackgroundPixmap(ref<YPixmap> pixmap) {
    setBackgroundPixmap(pixmap->pixmap(depth()));
}

void YWindow::setParentRelative() {
    setBackgroundPixmap(ParentRelative);
}

void YWindow::mapToGlobal(int& x, int& y) {
    Window child;

    XTranslateCoordinates(xapp->display(),
                          handle(),
                          desktop->handle(),
                          x, y,
                          &x, &y, &child);
}

void YWindow::mapToLocal(int& x, int& y) {
    Window child;

    XTranslateCoordinates(xapp->display(),
                          desktop->handle(),
                          handle(),
                          x, y,
                          &x, &y, &child);
}

void YWindow::configure(const YRect &/*r*/)
{
}

void YWindow::configure(const YRect2& r2)
{
    configure((const YRect &) r2);
}

void YWindow::setPointer(Cursor pointer) {
    fPointer = pointer;

    if (flags & wfCreated) {
        XSetWindowAttributes attributes;
        attributes.cursor = fPointer;
        XChangeWindowAttributes(xapp->display(), handle(),
                                CWCursor, &attributes);
    }
}

void YWindow::grabKeyM(int keycode, unsigned int modifiers) {
    MSG(("grabKey %d %d %s", keycode, modifiers,
         XKeysymToString(keyCodeToKeySym(keycode))));

    XGrabKey(xapp->display(), keycode, modifiers, handle(), False,
             GrabModeAsync, GrabModeAsync);
}

void YWindow::grabKey(int key, unsigned int modifiers) {
    KeyCode keycode = XKeysymToKeycode(xapp->display(), KeySym(key));
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
    XGrabButton(xapp->display(), unsigned(button), modifiers,
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

void YWindow::requestFocus(bool requestUserFocus) {
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
    for (YWindow *w = this; w; w = w->fParentWindow) {
        if (w->isToplevel())
            return w;
    }
    return nullptr;
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
    while (w->fFocusedWindow) {
        w = w->fFocusedWindow;
    }
    return w;
}

bool YWindow::changeFocus(bool next) {
    YWindow *cur = getFocusWindow();

    if (cur == nullptr) {
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
    if (parent() == nullptr || isToplevel() || parent()->focused()) {
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
        flags &= unsigned(~wfFocused);
        repaintFocus();
    }
}

void YWindow::installAccelerator(unsigned int key, unsigned int mod, YWindow *win) {
    if (key < 128)
        key = ASCII::toUpper(char(key));
    if (isToplevel() || fParentWindow == nullptr) {
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
        if (a == nullptr)
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
        key = ASCII::toUpper(char(key));
    if (isToplevel() || fParentWindow == nullptr) {
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

void YWindow::deleteProperty(Atom property) {
    XDeleteProperty(xapp->display(), handle(), property);
}

void YWindow::setProperty(Atom prop, Atom type, const Atom* values, int count) {
    XChangeProperty(xapp->display(), handle(), prop, type, 32, PropModeReplace,
                    reinterpret_cast<const unsigned char *>(values), count);
}

void YWindow::setProperty(Atom property, Atom propType, Atom value) {
    setProperty(property, propType, &value, 1);
}

void YWindow::setNetName(const char* name) {
    int length = int(strlen(name));
    XChangeProperty(xapp->display(), handle(), _XA_NET_WM_NAME, _XA_UTF8_STRING,
                    8, PropModeReplace, (const unsigned char *) name, length);
}

void YWindow::setNetWindowType(Atom window_type) {
    setProperty(_XA_NET_WM_WINDOW_TYPE, XA_ATOM, window_type);
}

void YWindow::setNetOpacity(Atom opacity) {
    setProperty(_XA_NET_WM_WINDOW_OPACITY, XA_CARDINAL, opacity);
}

void YWindow::setNetPid() {
    setProperty(_XA_NET_WM_PID, XA_CARDINAL, getpid());
}

void YWindow::setToplevel(bool enabled) {
    if (isToplevel() != enabled) {
        flags = enabled ? (flags | wfToplevel) : (flags &~ wfToplevel);
    }
}

YDndWindow::YDndWindow(YWindow* parent, Window win, int depth,
                       Visual* visual, Colormap colormap) :
    YWindow(parent, win, depth, visual, colormap),
    XdndDragSource(None),
    XdndDropTarget(None)
{
}

void YDndWindow::setDND(bool enabled) {
    if (enabled) {
        setProperty(XA_XdndAware, XA_ATOM, XdndCurrentVersion);
    } else {
        deleteProperty(XA_XdndAware);
    }
}

void YDndWindow::sendXdndStatus(bool acceptDrop, Atom dropAction) {
    if (XdndDragSource) {
        int x_root = 0, y_root = 0;
        mapToGlobal(x_root, y_root);
        XClientMessageEvent msg = { ClientMessage, None, False, nullptr, };
        msg.window = XdndDragSource;
        msg.message_type = XA_XdndStatus;
        msg.format = 32;
        msg.data.l[0] = long(handle());
        msg.data.l[1] = (2 | acceptDrop);
        msg.data.l[2] = (x_root << 16) | (y_root & 0xFFFF);
        msg.data.l[3] = (width() << 16) | (height() & 0xFFFF);
        msg.data.l[4] = long(dropAction);
        xapp->send(msg, XdndDragSource);
    }
}

void YDndWindow::handleXdndEnter(const XClientMessageEvent& message) {
    if (message.message_type == XA_XdndEnter) {
        const long* data = message.data.l;
        long version = ((data[1] >> 24) & 0xF);
        MSG(("XdndEnter source=0x%lX version=%ld %ld %ld %ld",
              data[0], version, data[2], data[3], data[4]));
        if (version <= XdndCurrentVersion) {
            XdndDragSource = static_cast<Window>(data[0]);
        } else {
            XdndDragSource = None;
        }
        XdndDropTarget = None;
    }
}

void YDndWindow::handleXdndLeave(const XClientMessageEvent& message) {
    if (message.message_type == XA_XdndLeave) {
        Window source = static_cast<Window>(message.data.l[0]);
        MSG(("XdndLeave source=0x%lX", source));
        if (source == XdndDragSource && XdndDragSource) {
            if (XdndDropTarget) {
                YWindow* win = windowContext.find(XdndDropTarget);
                if (win)
                    win->handleDNDLeave();
                XdndDropTarget = None;
            }
            XdndDragSource = None;
        }
    }
}

void YDndWindow::handleXdndPosition(const XClientMessageEvent& message) {
    if (message.message_type == XA_XdndPosition && XdndDragSource) {
        const long* data = message.data.l;
        XdndDragSource = static_cast<Window>(data[0]);
        int x = short(data[2] >> 16);
        int y = short(data[2] & 0xFFFF);

        MSG(("XdndPosition 0x%lX %s 0x%lX %d:%d",
             data[0], xapp->atomName(data[4]), handle(), x, y));

        Window target = handle(), child = None;
        int nx, ny;
        do {
            if (XTranslateCoordinates(xapp->display(), desktop->handle(),
                                      target, x, y, &nx, &ny, &child)) {
                if (child) {
                    target = child;
                }
            } else {
                target = None;
                break;
            }
        } while (child);

        YWindow* pwin = nullptr;
        if (target != XdndDropTarget) {
            if (XdndDropTarget) {
                YWindow* ptr = windowContext.find(XdndDropTarget);
                if (ptr)
                    ptr->handleDNDLeave();
            }
            XdndDropTarget = target;
            if (XdndDropTarget) {
                YWindow* ptr = windowContext.find(XdndDropTarget);
                if (ptr) {
                    ptr->handleDNDEnter();
                    pwin = ptr;
                }
            }
        }
        if (pwin == nullptr && XdndDropTarget) {
            pwin = windowContext.find(XdndDropTarget);
        }
        if (pwin)
            pwin->handleDNDPosition(nx, ny);
        MSG(("XdndPosition 0x%lX %s 0x%lX %d:%d",
             data[0], xapp->atomName(data[4]), XdndDropTarget, nx, ny));
        sendXdndStatus();
    }
}

void YDndWindow::handleXdndStatus(const XClientMessageEvent& message) {
    if (message.message_type == XA_XdndStatus) {
        MSG(("XdndStatus target=0x%lX accept=%s",
             message.data.l[0], boolstr(message.data.l[1] & 1)));
    }
}

void YDndWindow::handleXdndDrop(const XClientMessageEvent& message) {
    if (message.message_type == XA_XdndDrop) {
        MSG(("XdndDrop source=0x%lx time=%ld.%03ld", message.data.l[0],
              message.data.l[2] / 1000, message.data.l[2] % 1000));
    }
}

void YDndWindow::handleXdndFinished(const XClientMessageEvent& message) {
    if (message.message_type == XA_XdndFinished) {
        MSG(("XdndFinished target=0x%lX accept=%s action=%s",
              message.data.l[0], boolstr(message.data.l[1] & 1),
              xapp->atomName(message.data.l[2])));
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
    if (fAutoScroll == nullptr)
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
                      sel, _XA_UTF8_STRING,
                      sel, handle(), xapp->getEventTime("requestSelection"));
}

bool YWindow::hasPopup() {
    YPopupWindow *p = xapp->popup();
    if (p) {
        while (p->prevPopup()) {
            p = p->prevPopup();
        }
        for (YWindow *w = p->popupOwner(); w; w = w->parent()) {
            if (w == this)
                return true;
        }
    }
    return false;
}

YDesktop::YDesktop(YWindow *aParent, Window win):
    YWindow(aParent, win)
{
    desktop = this;
    unsigned w = 0, h = 0;
    updateXineramaInfo(w, h);
}

YDesktop::~YDesktop() {
#if DEBUG || PRECON
    for (YWindow* w; (w = firstWindow()) != nullptr; delete w) {
        char* name = demangle(typeid(*w).name());
        INFO("deleting stray %s", name);
        free(name);
    }
#endif
    if (desktop == this)
        desktop = nullptr;
}

void YWindow::grabVKey(int key, unsigned int vm) {
    unsigned m = 0;

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
    unsigned m = 0;

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

unsigned YWindow::VMod(unsigned m) {
    unsigned vm = 0;
    unsigned m1 = m & ~xapp->WinMask;

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
    int klen = XLookupString(&kev, keyBuf, sizeof(keyBuf), &ksym, nullptr);
#ifndef USE_XmbLookupString
    if ((klen == 0)  && (ksym < 0x1000)) {
        klen = 1;
        keyBuf[0] = char(ksym & 0xFF);
    }
#endif
    if (klen >= 1 && klen < maxLen - 1) {
        memcpy(s, keyBuf, size_t(klen));
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

    XGCValues gcv;
    gcv.graphics_exposures = False;
    unsigned long gcvflags = GCGraphicsExposures;
    GC scrollGC = XCreateGC(xapp->display(), handle(), gcvflags, &gcv);

    XCopyArea(xapp->display(), handle(), handle(), scrollGC,
              dx, dy, width(), height(), 0, 0);

    XFreeGC(xapp->display(), scrollGC);

    dx = - dx;
    dy = - dy;

    if (dy != 0) {
        r[nr].x = 0;
        r[nr].width = static_cast<unsigned short>(width());

        if (dy >= 0) {
            r[nr].y = 0;
            r[nr].height = static_cast<unsigned short>(dy);
        } else {
            r[nr].height = static_cast<unsigned short>(- dy);
            r[nr].y = short(int(height()) - int(r[nr].height));
        }
        nr++;
    }
    if (dx != 0) {
        r[nr].y = 0;
        r[nr].height = static_cast<unsigned short>(height()); // !!! optimize

        if (dx >= 0) {
            r[nr].x = 0;
            r[nr].width = static_cast<unsigned short>(dx);
        } else {
            r[nr].width = static_cast<unsigned short>(- dx);
            r[nr].x = short(int(width()) - int(r[nr].width));
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
        re.width = static_cast<unsigned short>(width());
        re.height = static_cast<unsigned short>(height());
    }

    paint(g, YRect(re.x, re.y, re.width, re.height)); // !!! add flag to do minimal redraws

    g.resetClip();
}

void YWindow::clearWindow() {
    XClearWindow(xapp->display(), handle());
}

void YWindow::clearArea(int x, int y, unsigned w, unsigned h, bool exposures) {
    XClearArea(xapp->display(), handle(), x, y, w, h, exposures);
}

Pixmap YWindow::createPixmap() {
    return XCreatePixmap(xapp->display(), xapp->root(), fWidth, fHeight, fDepth);
}

XRenderPictFormat* YWindow::format() {
    return xapp->formatForDepth(depth());
}

Picture YWindow::createPicture() {
    Picture picture = None;
    XRenderPictFormat* format = this->format();
    if (format) {
        XRenderPictureAttributes attr;
        unsigned long mask = None;
        picture = XRenderCreatePicture(xapp->display(), handle(),
                                       format, mask, &attr);
    }
    return picture;
}

int YWindow::getScreen() {
    int dx = 0, dy = 0;
    mapToGlobal(dx, dy);
    return desktop->getScreenForRect(dx, dy, width(), height());
}

bool YDesktop::updateXineramaInfo(unsigned& horizontal, unsigned& vertical) {
    xiInfo.clear();

#ifdef CONFIG_XRANDR
    if (xrandr.supported && !xrrDisable) {
        XRRScreenResources *xrrsr =
            XRRGetScreenResources(xapp->display(), handle());
        for (int i = 0; xrrsr && i < xrrsr->ncrtc; i++) {
            XRRCrtcInfo *ci = XRRGetCrtcInfo(xapp->display(), xrrsr,
                                             xrrsr->crtcs[i]);
            MSG(("xrr %d (%lu): %d %d %u %u", i, xrrsr->crtcs[i],
                        ci->x, ci->y, ci->width, ci->height));
            if (ci->width && ci->height) {
                DesktopScreenInfo si(int(xrrsr->crtcs[i]),
                                     ci->x, ci->y, ci->width, ci->height);
                xiInfo.append(si);
            }
            XRRFreeCrtcInfo(ci);
        }

        if (xineramaPrimaryScreenName) {
            const char* name = xineramaPrimaryScreenName;
            MSG(("xinerama primary screen name: %s", name));
            for (int o = 0; xrrsr && o < xrrsr->noutput; o++) {
                XRROutputInfo *oinfo = XRRGetOutputInfo(xapp->display(), xrrsr,
                                                        xrrsr->outputs[o]);
                MSG(("output: %s -> %lu", oinfo->name, oinfo->crtc));
                if (oinfo->name && strcmp(oinfo->name, name) == 0) {
                    for (int k = 0; k < xiInfo.getCount(); k++) {
                         if (xiInfo[k].screen_number == int(oinfo->crtc)) {
                             xineramaPrimaryScreen = o;
                             MSG(("xinerama primary screen: %s -> %d",
                                         oinfo->name, o));
                         }
                    }
                }
                XRRFreeOutputInfo(oinfo);
            }
        }
        XRRFreeScreenResources(xrrsr);
    }
#endif

#ifdef XINERAMA
    if (xiInfo.getCount() < 2 && xinerama.supported) {
        // use xinerama if no XRANDR screens (nvidia hack)
        int count = 0;
        xsmart<XineramaScreenInfo> screens(
               XineramaQueryScreens(xapp->display(), &count));
        MSG(("xinerama: heads=%d", count));
        if (screens && count > xiInfo.getCount()) {
            xiInfo.clear();
            for (int i = 0; i < count; i++) {
                const XineramaScreenInfo& xine(screens[i]);
                MSG(("xinerama: %d +%d+%d %dx%d", xine.screen_number,
                    xine.x_org, xine.y_org, xine.width, xine.height));
                DesktopScreenInfo si(i, xine.x_org, xine.y_org,
                                     unsigned(xine.width),
                                     unsigned(xine.height));
                xiInfo.append(si);
            }
        }
    }
#endif

    if (xiInfo.isEmpty()) {
        DesktopScreenInfo si(0, 0, 0,
                             unsigned(xapp->displayWidth()),
                             unsigned(xapp->displayHeight()));
        xiInfo.append(si);
    }

    unsigned w = 0;
    unsigned h = 0;
    for (int i = 0; i < xiInfo.getCount(); i++) {
        const DesktopScreenInfo& info(xiInfo[i]);
        w = max(w, info.horizontal());
        h = max(h, info.vertical());
        MSG(("screen %d (%d): %d %d %u %u", i, info.screen_number,
            info.x_org, info.y_org, info.width, info.height));
    }
    swap(w, horizontal);
    swap(h, vertical);
    MSG(("desktop screen area: %u %u -> %u %u", w, h, horizontal, vertical));
    return w != horizontal || h != vertical;
}

const DesktopScreenInfo& YDesktop::getScreenInfo(int screen_no) {
    int s = (screen_no == -1) ? xineramaPrimaryScreen : screen_no;
    if (s < 0 || s >= xiInfo.getCount())
        s = 0;
    if (xiInfo.isEmpty()) {
        xiInfo.append(DesktopScreenInfo(0, 0, 0,
                      desktop->width(), desktop->height()));
    }
    return xiInfo[s];
}

YRect YDesktop::getScreenGeometry(int screen_no) {
    return getScreenInfo(screen_no);
}

void YDesktop::getScreenGeometry(int *x, int *y,
                                 unsigned *width, unsigned *height,
                                 int screen_no)
{
    const DesktopScreenInfo& info(getScreenInfo(screen_no));
    *x = info.x_org;
    *y = info.y_org;
    *width = info.width;
    *height = info.height;
}

int YDesktop::getScreenForRect(int x, int y, unsigned width, unsigned height) {
    int screen = -1;
    long coverage = -1;

    if (xiInfo.getCount() < 2)
        return 0;
    for (int s = 0; s < xiInfo.getCount(); s++) {
        int x_i = intersection(x, x + int(width),
                               xiInfo[s].x_org,
                               xiInfo[s].x_org + int(xiInfo[s].width));
        //MSG(("x_i %d %d %d %d %d", x_i, x, width, xiInfo[s].x_org, xiInfo[s].width));
        int y_i = intersection(y, y + int(height),
                               xiInfo[s].y_org,
                               xiInfo[s].y_org + int(xiInfo[s].height));
        //MSG(("y_i %d %d %d %d %d", y_i, y, height, xiInfo[s].y_org, xiInfo[s].height));

        long cov = (1L + x_i) * (1L + y_i);

        //MSG(("cov=%ld %d %d s:%d xc:%d yc:%d %d %d %d %d", cov, x, y, s, x_i, y_i, xiInfo[s].x_org, xiInfo[s].y_org, xiInfo[s].width, xiInfo[s].height));

        if (cov > coverage) {
            screen = s;
            coverage = cov;
        }
    }
    return screen;
}


KeySym YWindow::keyCodeToKeySym(unsigned keycode, unsigned index) {
    KeySym k = XkbKeycodeToKeysym(xapp->display(), KeyCode(keycode), 0, index);
    return k;
}

int YDesktop::getScreenCount() {
    return xiInfo.getCount();
}

// vim: set sw=4 ts=4 et:
