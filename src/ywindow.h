#ifndef __YWINDOW_H
#define __YWINDOW_H

#include "ypaint.h"
#include "ycursor.h"
#include "yarray.h"
#include "ylist.h"
#include "yrect.h"

class YPopupWindow;
class YToolTip;
class YTimer;
class YAutoScroll;
class YRect;

#ifdef XINERAMA
extern "C" {
#include <X11/extensions/Xinerama.h>
}
#endif

struct DesktopScreenInfo {
    int screen_number;
    int x_org;
    int y_org;
    unsigned width;
    unsigned height;
};

class YWindow : protected YWindowList, private YWindowNode {
public:
    YWindow(YWindow *aParent = 0, Window win = 0, int depth = CopyFromParent, Visual *visual = CopyFromParent);
    virtual ~YWindow();

    void setStyle(unsigned aStyle);
    unsigned getStyle() const { return fStyle; }
    long getEventMask() const { return fEventMask; }
    void addEventMask(long mask);

    void setVisible(bool enable);
    void show();
    void hide();
    virtual void raise();
    virtual void lower();

    void repaint();
    void repaintFocus();
    void repaintSync();
    void readAttributes();
    void reparent(YWindow *parent, int x, int y);
    bool getWindowAttributes(XWindowAttributes* attr);

    void setWindowFocus();

    bool fetchTitle(char** title);
    void setTitle(char const * title);
    void setClassHint(char const * rName, char const * rClass);

    void setGeometry(const YRect &r);
    void setSize(unsigned width, unsigned height);
    void setPosition(int x, int y);
    void setBorderWidth(unsigned width);
    void setBackground(unsigned long pixel);
    void setBackgroundPixmap(Pixmap pixmap);
    void setParentRelative(void);
    virtual void configure(const YRect &r);


    virtual void paint(Graphics &g, const YRect &r);
    virtual void paintFocus(Graphics &, const YRect &) {}

    virtual void handleEvent(const XEvent &event);

    virtual void handleExpose(const XExposeEvent &expose);
    virtual void handleGraphicsExpose(const XGraphicsExposeEvent &graphicsExpose);
    virtual void handleConfigure(const XConfigureEvent &configure);
    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleMotion(const XMotionEvent &motion);
    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual void handleProperty(const XPropertyEvent &) {}
    virtual void handleColormap(const XColormapEvent &) {}
    virtual void handleFocus(const XFocusChangeEvent &);
    virtual void handleClientMessage(const XClientMessageEvent &message);
    virtual void handleSelectionClear(const XSelectionClearEvent &clear);
    virtual void handleSelectionRequest(const XSelectionRequestEvent &request);
    virtual void handleSelection(const XSelectionEvent &selection);
    virtual void handleVisibility(const XVisibilityEvent &visibility);
#if 0
    virtual void handleCreateWindow(const XCreateWindowEvent &createWindow);
#endif
    virtual void handleGravityNotify(const XGravityEvent &gravity);
    virtual void handleMapNotify(const XMapEvent &map);
    virtual void handleUnmapNotify(const XUnmapEvent &unmap);
    virtual void handleUnmap(const XUnmapEvent &unmap);
    virtual void handleDestroyWindow(const XDestroyWindowEvent &destroyWindow);
    virtual void handleReparentNotify(const XReparentEvent &) {}
    virtual void handleConfigureRequest(const XConfigureRequestEvent &);
    virtual void handleMapRequest(const XMapRequestEvent &);
#ifdef CONFIG_SHAPE
    virtual void handleShapeNotify(const XShapeEvent &) {}
#endif
#ifdef CONFIG_XRANDR
    virtual void handleRRScreenChangeNotify(const XRRScreenChangeNotifyEvent &/*xrrsc*/) {}
#endif

    virtual void handleClickDown(const XButtonEvent &, int) {}
    virtual void handleClick(const XButtonEvent &, int) {}
    virtual void handleBeginDrag(const XButtonEvent &, const XMotionEvent &) {}
    virtual void handleDrag(const XButtonEvent &, const XMotionEvent &) {}
    virtual void handleEndDrag(const XButtonEvent &, const XButtonEvent &) {}

    virtual void handleEndPopup(YPopupWindow *popup);

    virtual void handleClose();

    virtual bool handleAutoScroll(const XMotionEvent &mouse);
    void beginAutoScroll(bool autoScroll, const XMotionEvent *motion);

    void setPointer(const YCursor& pointer);
    void setGrabPointer(const YCursor& pointer);
    void grabKeyM(int key, unsigned modifiers);
    void grabKey(int key, unsigned modifiers);
    void grabVKey(int key, unsigned vmodifiers);
    unsigned VMod(int modifiers);
    void grabButtonM(int button, unsigned modifiers);
    void grabButton(int button, unsigned modifiers);
    void grabVButton(int button, unsigned vmodifiers);

    void captureEvents();
    void releaseEvents();

    Window handle() { return (flags & wfCreated) ? fHandle : create(); }
    YWindow *parent() const { return fParentWindow; }
    YWindow *window() { return this; }

    ref<YPixmap> beginPaint(YRect &r);
    void endPaint(Graphics &g, ref<YPixmap> pixmap, YRect &r);
    void paintExpose(int ex, int ey, int ew, int eh);

    Graphics & getGraphics();

    virtual ref<YImage> getGradient() const {
        return (parent() ? parent()->getGradient() : null); }

    int x() const { return fX; }
    int y() const { return fY; }
    unsigned width() const { return fWidth; }
    unsigned height() const { return fHeight; }
    unsigned depth() const { return fDepth; }
    Visual *visual() const { return fVisual; }
    YRect geometry() const { return YRect(fX, fY, fWidth, fHeight); }

    bool visible() const { return (flags & wfVisible); }
    bool created() const { return (flags & wfCreated); }
    bool adopted() const { return (flags & wfAdopted); }
    bool destroyed() const { return (flags & wfDestroyed); }
    bool focused() const { return (flags & wfFocused); }

    virtual void donePopup(YPopupWindow * /*command*/);

    enum WindowStyle {
        wsOverrideRedirect = 1 << 0,
        wsSaveUnder        = 1 << 1,
        wsManager          = 1 << 2,
        wsInputOnly        = 1 << 3,
        wsOutputOnly       = 1 << 4,
        wsPointerMotion    = 1 << 5,
        wsDesktopAware     = 1 << 6,
        wsToolTip          = 1 << 7,
    };

    virtual bool isFocusTraversable();
    bool isFocused();
    bool isEnabled() const { return fEnabled; }
    void setEnabled(bool enable);
    void nextFocus();
    void prevFocus();
    bool changeFocus(bool next);
    void requestFocus(bool requestUserFocus);
    void setFocus(YWindow *window);
    YWindow *getFocusWindow();
    virtual void gotFocus();
    virtual void lostFocus();

    bool isToplevel() const { return fToplevel; }
    void setToplevel(bool toplevel) { fToplevel = toplevel; }

    YWindow *toplevel();

    void installAccelerator(unsigned key, unsigned mod, YWindow *win);
    void removeAccelerator(unsigned key, unsigned mod, YWindow *win);

    void setToolTip(const ustring &tip);

    void mapToGlobal(int &x, int &y);
    void mapToLocal(int &x, int &y);

    void setWinGravity(int gravity);
    void setBitGravity(int gravity);

    void setDND(bool enabled);

    void XdndStatus(bool acceptDrop, Atom dropAction);
    virtual void handleXdnd(const XClientMessageEvent &message);

    virtual void handleDNDEnter();
    virtual void handleDNDLeave();
    virtual void handleDNDPosition(int x, int y);

    bool getCharFromEvent(const XKeyEvent &key, char *s, int maxLen);
    int getClickCount() { return fClickCount; }

    void scrollWindow(int dx, int dy);

    bool toolTipVisible();
    virtual void updateToolTip();

    void acquireSelection(bool selection);
    void clearSelection(bool selection);
    void requestSelection(bool selection);

    bool hasPopup();
    void setDoubleBuffer(bool doubleBuffer);

    KeySym keyCodeToKeySym(unsigned int keycode, int index = 0);
    static unsigned long getLastEnterNotifySerial();

    void unmanageWindow() { removeWindow(); }

private:
    enum WindowFlags {
        wfVisible   = 1 << 0,
        wfCreated   = 1 << 1,
        wfAdopted   = 1 << 2,
        wfDestroyed = 1 << 3,
        wfNullSize  = 1 << 5,
        wfFocused   = 1 << 6
    };

    Window create();
    void destroy();

    void insertWindow();
    void removeWindow();

    bool nullGeometry();

    unsigned fDepth;
    Visual *fVisual;
    Colormap fAllocColormap;

    YWindow *fParentWindow;
    YWindow *fFocusedWindow;

    Window fHandle;
    unsigned flags;
    unsigned fStyle;
    int fX, fY;
    unsigned fWidth, fHeight;
    YCursor fPointer;
    int unmapCount;
    Graphics *fGraphics;
    long fEventMask;
    int fWinGravity, fBitGravity;

    bool fEnabled;
    bool fToplevel;
    bool fDoubleBuffer;

    struct YAccelerator {
        unsigned key;
        unsigned mod;
        YWindow *win;
        YAccelerator *next;
    };

    YAccelerator *accel;
    YToolTip *fToolTip;

    static XButtonEvent fClickEvent;
    static YWindow *fClickWindow;
    static Time fClickTime;
    static int fClickCount;
    static int fClickDrag;
    static unsigned fClickButton;
    static unsigned fClickButtonDown;
    static lazy<YTimer> fToolTipTimer;
    static unsigned long lastEnterNotifySerial;
    static void updateEnterNotifySerial(const XEvent& event);

    bool fDND;
    Window XdndDragSource;
    Window XdndDropTarget;

    static YAutoScroll *fAutoScroll;

    void addIgnoreUnmap(Window w);
    bool ignoreUnmap(Window w);
    void removeAllIgnoreUnmap(Window w);
};

class YDesktop: public YWindow {
public:
    YDesktop(YWindow *aParent = 0, Window win = 0);
    virtual ~YDesktop();

    void updateXineramaInfo(unsigned &w, unsigned &h);

    void getScreenGeometry(int *x, int *y,
                           unsigned *width, unsigned *height,
                           int screen_no = -1);
    int getScreenForRect(int x, int y, unsigned width, unsigned height);

    int getScreenCount();

    virtual void grabKeys() {}

protected:
    YArray<DesktopScreenInfo> xiInfo;
};

extern YDesktop *desktop;

#ifdef CONFIG_SHAPE
extern int shapesSupported;
extern int shapeEventBase, shapeErrorBase;
#endif

#ifdef CONFIG_XRANDR
extern int xrandrSupported;
extern bool xrandr12;
extern int xrandrEventBase, xrandrErrorBase;
#endif


extern Atom _XA_WM_CHANGE_STATE;
extern Atom _XA_WM_CLASS;
extern Atom _XA_WM_COLORMAP_NOTIFY;
extern Atom _XA_WM_COLORMAP_WINDOWS;
extern Atom _XA_WM_COMMAND;
extern Atom _XA_WM_DELETE_WINDOW;
extern Atom _XA_WM_DESKTOP;
extern Atom _XA_WM_HINTS;
extern Atom _XA_WM_ICON_NAME;
extern Atom _XA_WM_ICON_SIZE;
extern Atom _XA_WM_LOCALE_NAME;
extern Atom _XA_WM_NAME;
extern Atom _XA_WM_NORMAL_HINTS;
extern Atom _XA_WM_PROTOCOLS;
extern Atom _XA_WM_SIZE_HINTS;
extern Atom _XA_WM_STATE;
extern Atom _XA_WM_TAKE_FOCUS;
extern Atom _XA_WM_TRANSIENT_FOR;
extern Atom _XA_WM_ZOOM_HINTS;

extern Atom _XATOM_MWM_HINTS;
extern Atom _XA_CLIPBOARD;
extern Atom _XA_XEMBED_INFO;

/* Xdnd */
extern Atom XA_XdndAware;
extern Atom XA_XdndEnter;
extern Atom XA_XdndLeave;
extern Atom XA_XdndPosition;
extern Atom XA_XdndProxy;
extern Atom XA_XdndStatus;
extern Atom XA_XdndDrop;
extern Atom XA_XdndFinished;

#endif

// vim: set sw=4 ts=4 et:
