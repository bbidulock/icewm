#ifndef YWINDOW_H
#define YWINDOW_H

#include "ypaint.h"
#include "yarray.h"
#include "ylist.h"
#include "yrect.h"
#include "logevent.h"

class YPopupWindow;
class YToolTip;
class YTimer;
class YAutoScroll;
class YRect;
class YRect2;
class YIcon;

struct DesktopScreenInfo {
    int screen_number;
    int x_org;
    int y_org;
    unsigned width;
    unsigned height;

    DesktopScreenInfo(int i, int x, int y, unsigned w, unsigned h) :
        screen_number(i), x_org(x), y_org(y), width(w), height(h)
    { }
    operator YRect() const {
        return YRect(x_org, y_org, width, height);
    }
    unsigned horizontal() const {
        return width + unsigned(x_org);
    }
    unsigned vertical() const {
        return height + unsigned(y_org);
    }
    int horizontalCoverage(int x, unsigned w) {
        return intersection(x, x + int(w), x_org, x_org + int(width));
    }
    int verticalCoverage(int y, unsigned h) {
        return intersection(y, y + int(h), y_org, y_org + int(height));
    }
    long coverage(int x, int y, unsigned w, unsigned h) {
        return (1L + horizontalCoverage(x, w))
             * (1L + verticalCoverage(y, h));
    }
};

class AToolTip {
public:
    virtual ~AToolTip() { }

    virtual void setText(mstring tip, ref<YIcon> icon) { }
    virtual void enter(YWindow* w) { }
    virtual void leave() { }
    virtual bool visible() const { return false; }
    virtual bool nonempty() const { return false; }
};

class YWindow : protected YWindowList, private YWindowNode {
public:
    YWindow(YWindow *aParent = nullptr,
            Window window = None,
            int depth = CopyFromParent,
            Visual *visual = nullptr,
            Colormap colormap = CopyFromParent);
    virtual ~YWindow();

    void setStyle(unsigned aStyle);
    void addStyle(unsigned aStyle) { setStyle(fStyle | aStyle); }
    unsigned getStyle() const { return fStyle; }
    long getEventMask() const { return fEventMask; }
    void addEventMask(long mask);

    void setVisible(bool enable);
    void show();
    void hide();
    virtual void raise();
    virtual void lower();

    virtual void repaint();
    virtual void repaintFocus();

    void readAttributes();
    void reparent(YWindow *parent, int x, int y);
    bool getWindowAttributes(XWindowAttributes* attr);
    void beneath(YWindow* superior);
    void raiseTo(YWindow* inferior);
    void setWindowFocus(Time timestamp = CurrentTime);

    bool fetchTitle(char** title);
    void setTitle(char const * title);
    void setClassHint(char const * rName, char const * rClass);

    void setGeometry(const YRect &r);
    void setSize(unsigned width, unsigned height);
    void setPosition(int x, int y);
    void setBorderWidth(unsigned width);
    void setBackground(unsigned long pixel);
    void setBackgroundPixmap(ref<YPixmap> pixmap);
    void setBackgroundPixmap(Pixmap pixmap);
    void setParentRelative();
    virtual void configure(const YRect &r);
    virtual void configure(const YRect2& r2);

    virtual void paint(Graphics &g, const YRect &r);

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
    virtual void handleGravityNotify(const XGravityEvent &gravity);
    virtual void handleMapNotify(const XMapEvent &map);
    virtual void handleUnmapNotify(const XUnmapEvent &unmap);
    virtual void handleUnmap(const XUnmapEvent &unmap);
    virtual void handleDestroyWindow(const XDestroyWindowEvent &destroyWindow);
    virtual void handleReparentNotify(const XReparentEvent &) {}
    virtual void handleConfigureRequest(const XConfigureRequestEvent &);
    virtual void handleMapRequest(const XMapRequestEvent &);
    virtual void handleDamageNotify(const XDamageNotifyEvent &) {}
#ifdef CONFIG_SHAPE
    virtual void handleShapeNotify(const XShapeEvent &) {}
#endif
#ifdef CONFIG_XRANDR
    virtual void handleRRScreenChangeNotify(const XRRScreenChangeNotifyEvent &) {}
    virtual void handleRRNotify(const XRRNotifyEvent &) {}
#endif

    virtual void handleClickDown(const XButtonEvent &, int) {}
    virtual void handleClick(const XButtonEvent &, int) {}
    virtual bool handleBeginDrag(const XButtonEvent &, const XMotionEvent &);
    virtual void handleDrag(const XButtonEvent &, const XMotionEvent &) {}
    virtual void handleEndDrag(const XButtonEvent &, const XButtonEvent &) {}

    virtual void handleClose();

    virtual bool handleAutoScroll(const XMotionEvent &mouse);
    void beginAutoScroll(bool autoScroll, const XMotionEvent *motion);

    void setPointer(Cursor pointer);
    void grabKeyM(unsigned key, unsigned modifiers);
    void grabKey(unsigned key, unsigned modifiers);
    void grabVKey(unsigned key, unsigned modifiers);
    void grab(const struct WMKey& wmkey);
    unsigned VMod(unsigned modifiers);
    void grabButtonM(int button, unsigned modifiers);
    void grabButton(int button, unsigned modifiers);
    void grabVButton(int button, unsigned vmodifiers);

    void captureEvents();
    void releaseEvents();

    Window handle() { return (flags & wfCreated) ? fHandle : create(); }
    YWindow *parent() const { return fParent; }
    YWindow *window() { return this; }

    void paintExpose(int ex, int ey, int ew, int eh);

    Graphics& getGraphics();
    virtual ref<YImage> getGradient() {
        return (parent() ? parent()->getGradient() : null); }

    int x() const { return fX; }
    int y() const { return fY; }
    unsigned width() const { return fWidth; }
    unsigned height() const { return fHeight; }
    unsigned depth() const { return fDepth; }
    Visual *visual() const { return fVisual; }
    Colormap colormap();
    Region region();
    YRect geometry() const { return YRect(fX, fY, fWidth, fHeight); }
    YDimension dimension() const { return YDimension(fWidth, fHeight); }

    bool visible() const { return (flags & wfVisible); }
    bool created() const { return (flags & wfCreated); }
    bool adopted() const { return (flags & wfAdopted); }
    bool focused() const { return (flags & wfFocused); }
    bool nullsize() const { return (flags & wfNullSize); }
    bool destroyed() const { return (flags & wfDestroyed); }
    void setDestroyed();
    bool testDestroyed();

    virtual void donePopup(YPopupWindow * /*command*/);

    enum WindowStyle {
        wsOverrideRedirect = 1 << 0,
        wsSaveUnder        = 1 << 1,
        wsManager          = 1 << 2,
        wsInputOnly        = 1 << 3,
        wsNoExpose         = 1 << 4,
        wsPointerMotion    = 1 << 5,
        wsDesktopAware     = 1 << 6,
        wsToolTip          = 1 << 7,
        wsBackingMapped    = 1 << 8,
        wsToolTipping      = 1 << 9,
        wsTakeFocus        = 1 << 10,
    };

    virtual bool isFocusTraversable();
    bool isFocused() const { return focused(); }
    void nextFocus();
    void prevFocus();
    bool changeFocus(bool next);
    virtual void requestFocus(bool requestUserFocus);
    void setFocus(YWindow *window);
    YWindow *getFocusWindow();
    virtual void gotFocus();
    virtual void lostFocus();

    bool isToplevel() const { return hasbit(flags, wfToplevel); }
    void setToplevel(bool toplevel);

    YWindow *toplevel();

    void installAccelerator(unsigned key, unsigned mod, YWindow *win);
    void removeAccelerator(unsigned key, unsigned mod, YWindow *win);

    bool hasToolTip() const;
    void setToolTip(mstring tip);
    void setToolTip(mstring tip, ref<YIcon> icon);

    void mapToGlobal(int &x, int &y);
    void mapToLocal(int &x, int &y);

    void setWinGravity(int gravity);
    void setBitGravity(int gravity);

    void deleteProperty(Atom property);
    void setProperty(Atom prop, Atom type, const char* string);
    void setProperty(Atom prop, Atom type, const Atom* values, int count);
    void setProperty(Atom property, Atom propType, Atom value);
    void setNetName(const char* name);
    void setNetWindowType(Atom window_type);
    void setNetOpacity(Atom opacity);
    void setNetPid();

    virtual void handleDNDEnter();
    virtual void handleDNDLeave();
    virtual void handleDNDPosition(int x, int y);

    bool getCharFromEvent(const XKeyEvent &key, char *s, int maxLen);
    bool dragging() const { return fClickDrag && fClickWindow == this; }
    int getClickButton() const { return fClickButton; }
    int getClickCount() const { return fClickCount; }
    int getClickX() const { return fClickEvent.x_root; }
    int getClickY() const { return fClickEvent.y_root; }
    int getScreen();

    void scrollWindow(int dx, int dy);
    void clearWindow();
    void clearArea(int x, int y, unsigned w, unsigned h, bool expos = false);
    Pixmap createPixmap();
    XRenderPictFormat* format();
    Picture createPicture();

    bool toolTipVisible();
    void toolTipVisibility(bool visible);
    virtual void updateToolTip();

    void acquireSelection(bool selection);
    void clearSelection(bool selection);
    void requestSelection(bool selection);

    bool hasPopup();

    KeySym keyCodeToKeySym(unsigned keycode, unsigned index = 0);
    static unsigned long getLastEnterNotifySerial();

    void unmanageWindow() { removeWindow(); }

private:
    enum WindowFlags {
        wfVisible   = 1 << 0,
        wfCreated   = 1 << 1,
        wfAdopted   = 1 << 2,
        wfDestroyed = 1 << 3,
        wfToplevel  = 1 << 4,
        wfNullSize  = 1 << 5,
        wfFocused   = 1 << 6,
    };

    Window create();
    void adopt();
    void destroy();

    void insertWindow();
    void removeWindow();

    bool nullGeometry();

    unsigned fDepth;
    Visual *fVisual;
    Colormap fColormap;

    YWindow *fParent;
    YWindow *fFocusedWindow;

    Window fHandle;
    unsigned flags;
    unsigned fStyle;
    int fX, fY;
    unsigned fWidth, fHeight;
    int unmapCount;
    Cursor fPointer;
    Graphics *fGraphics;
    long fEventMask;
    int fWinGravity, fBitGravity;

    struct YAccelerator {
        unsigned key;
        unsigned mod;
        YWindow *win;
        YAccelerator *next;
    };

    YAccelerator *accel;
    AToolTip* fToolTip;

    static XButtonEvent fClickEvent;
    static YWindow *fClickWindow;
    static Time fClickTime;
    static int fClickCount;
    static bool fClickDrag;
    static unsigned fClickButton;
    static unsigned fClickButtonDown;
    static unsigned long lastEnterNotifySerial;
    static void updateEnterNotifySerial(const XEvent& event);

    static YAutoScroll *fAutoScroll;

    void addIgnoreUnmap()       { ++unmapCount; }
    bool ignoreUnmap()          { return (0 < unmapCount) && unmapCount--; }
    void removeAllIgnoreUnmap() { unmapCount = 0; }
};

class YDndWindow : public YWindow {
protected:
    YDndWindow(YWindow* parent = nullptr, Window win = 0, int depth = 0,
               Visual* visual = nullptr, Colormap colormap = 0);
    void setDND(bool enabled = true);

    void handleClientMessage(const XClientMessageEvent& message) override;

private:
    void sendXdndStatus(bool acceptDrop = false, Atom dropAction = None);
    void handleXdndEnter(const XClientMessageEvent& message);
    void handleXdndLeave(const XClientMessageEvent& message);
    void handleXdndPosition(const XClientMessageEvent& message);
    void handleXdndStatus(const XClientMessageEvent& message);
    void handleXdndDrop(const XClientMessageEvent& message);
    void handleXdndFinished(const XClientMessageEvent& message);

    Window XdndDragSource;
    Window XdndDropTarget;

    enum { XdndCurrentVersion = 5, };
};

class YDesktop: public YWindow {
public:
    YDesktop(YWindow *aParent = nullptr, Window win = 0);
    virtual ~YDesktop();

    bool updateXineramaInfo(unsigned& horizontal, unsigned& vertical);

    const DesktopScreenInfo& getScreenInfo(int screen_no = -1);
    YRect getScreenGeometry(int screen_no = -1);
    void getScreenGeometry(int *x, int *y,
                           unsigned *width, unsigned *height,
                           int screen_no = -1);
    int getScreenForRect(int x, int y, unsigned width, unsigned height);

    int getScreenCount() const { return xiInfo.getCount(); }

    virtual void grabKeys() {}
    virtual void kbLayout() {}

protected:
    YArray<DesktopScreenInfo> xiInfo;
};

extern YDesktop *desktop;

struct YExtension {
    int eventBase, errorBase;
    int versionMajor, versionMinor;
    Bool parameter;
    bool supported;

    typedef int (*QueryFunc)(Display *, int *, int *);
    void init(Display* dis, QueryFunc ext, QueryFunc ver);

    typedef Bool (*ExistFunc)(Display *);
    typedef Bool (*ParamFunc)(Display *, int *, int *, Bool *);
    void init(Display* dis, ExistFunc ext, ParamFunc ver);

    bool isEvent(int type, int eventNumber) const {
        return supported && type == eventBase + eventNumber;
    }
};

extern YExtension composite;
extern YExtension damage;
extern YExtension fixes;
extern YExtension render;
extern YExtension shapes;
extern YExtension xrandr;
extern YExtension xinerama;
extern YExtension xshm;

extern Atom _XA_WM_CHANGE_STATE;
extern Atom _XA_WM_CLASS;
extern Atom _XA_WM_CLIENT_LEADER;
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
extern Atom _XA_WINDOW_ROLE;
extern Atom _XA_SM_CLIENT_ID;
extern Atom _XA_CLIPBOARD;
extern Atom _XA_MANAGER;
extern Atom _XA_TARGETS;
extern Atom _XA_XEMBED;
extern Atom _XA_XEMBED_INFO;
extern Atom _XA_UTF8_STRING;
extern Atom _XA_COMPOUND_TEXT;

/* Xdnd */
extern Atom XA_XdndAware;
extern Atom XA_XdndEnter;
extern Atom XA_XdndLeave;
extern Atom XA_XdndPosition;
extern Atom XA_XdndProxy;
extern Atom XA_XdndStatus;
extern Atom XA_XdndDrop;
extern Atom XA_XdndFinished;
extern Atom XA_XdndActionCopy;
extern Atom XA_XdndActionMove;
extern Atom XA_XdndActionLink;
extern Atom XA_XdndActionAsk;
extern Atom XA_XdndActionPrivate;

extern Atom _XA_KDE_NET_SYSTEM_TRAY_WINDOWS;
extern Atom _XA_NET_SYSTEM_TRAY_OPCODE;
extern Atom _XA_NET_SYSTEM_TRAY_ORIENTATION;
extern Atom _XA_NET_SYSTEM_TRAY_MESSAGE_DATA;
extern Atom _XA_NET_SYSTEM_TRAY_VISUAL;
extern Atom _XA_NET_WM_NAME;
extern Atom _XA_NET_WM_PID;
extern Atom _XA_NET_WM_WINDOW_OPACITY;              // OK
extern Atom _XA_NET_WM_WINDOW_TYPE;                 // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_COMBO;           // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_DESKTOP;         // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_DIALOG;          // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_DND;             // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_DOCK;            // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_DROPDOWN_MENU;   // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_MENU;            // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_NORMAL;          // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_NOTIFICATION;    // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_POPUP_MENU;      // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_SPLASH;          // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_TOOLBAR;         // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_TOOLTIP;         // OK
extern Atom _XA_NET_WM_WINDOW_TYPE_UTILITY;         // OK

inline bool operator==(const XClientMessageEvent& message, const Atom& prop) {
    return message.message_type == prop;
}

#endif

// vim: set sw=4 ts=4 et:
