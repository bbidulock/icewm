#ifndef __YWINDOW_H
#define __YWINDOW_H

#include "ypaint.h"

class YPopupWindow;
class YToolTip;
class YTimer;
class AutoScroll;
class CStr;

class YWindow {
public:
    YWindow(YWindow *aParent = 0);
    YWindow(YWindow *aParent, Window win);
    virtual ~YWindow();

private:
    void init();
public:

    void setStyle(unsigned long aStyle);

    void show();
    void hide();
    virtual void raise();
    virtual void lower();

    void repaint();
    void repaintFocus();

    void reparent(YWindow *parent, int x, int y);

    void setWindowFocus();

    void setGeometry(int x, int y, unsigned int width, unsigned int height);
    void setSize(unsigned int width, unsigned int height);
    void setPosition(int x, int y);
    virtual void configure(int x, int y, unsigned int width, unsigned int height);

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);
    virtual void paintFocus(Graphics &g, int x, int y, unsigned int w, unsigned int h);

    virtual void handleEvent(const XEvent &event);

    virtual void handleExpose(const XExposeEvent &expose);
    virtual void handleGraphicsExpose(const XGraphicsExposeEvent &graphicsExpose);
    virtual void handleConfigure(const XConfigureEvent &configure);
    virtual bool handleKeyEvent(const XKeyEvent &key);
    virtual bool handleKeySym(const XKeyEvent &key, KeySym ksym, int vmod);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleMotion(const XMotionEvent &motion);
    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual void handleProperty(const XPropertyEvent &property);
    virtual void handleColormap(const XColormapEvent &colormap);
    virtual void handleFocus(const XFocusChangeEvent &focus);
    virtual void handleClientMessage(const XClientMessageEvent &message);
    virtual void handleSelectionClear(const XSelectionClearEvent &clear);
    virtual void handleSelectionRequest(const XSelectionRequestEvent &request);
    virtual void handleSelection(const XSelectionEvent &selection);
#if 0
    virtual void handleVisibility(const XVisibilityEvent &visibility);
    virtual void handleCreateWindow(const XCreateWindowEvent &createWindow);
#endif
    virtual void handleMap(const XMapEvent &map);
    virtual void handleUnmap(const XUnmapEvent &unmap);
    virtual void handleDestroyWindow(const XDestroyWindowEvent &destroyWindow);
    virtual void handleConfigureRequest(const XConfigureRequestEvent &configureRequest);
    virtual void handleMapRequest(const XMapRequestEvent &mapRequest);
#ifdef SHAPE
    virtual void handleShapeNotify(const XShapeEvent &shape);
#endif

    virtual void handleClickDown(const XButtonEvent &down, int count);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleBeginDrag(const XButtonEvent &down, const XMotionEvent &motion);
    virtual void handleDrag(const XButtonEvent &down, const XMotionEvent &motion);
    virtual void handleEndDrag(const XButtonEvent &down, const XButtonEvent &up);

    virtual void handleClose();

    virtual bool handleAutoScroll(const XMotionEvent &mouse);
    void beginAutoScroll(bool autoScroll, const XMotionEvent *motion);

    void setPointer(Cursor pointer);
    void setGrabPointer(Cursor pointer);
    void grabKeyM(int key, unsigned int modifiers);
    void grabKey(int key, unsigned int modifiers);
    void grabVKey(int key, unsigned int vmodifiers);
    unsigned int VMod(int modifiers);
    void grabButtonM(int button, unsigned int modifiers);
    void grabButton(int button, unsigned int modifiers);

    void captureEvents();
    void releaseEvents();

    Window handle();
    YWindow *parent() const { return fParentWindow; }

    Graphics &getGraphics();

    int x() const { return fX; }
    int y() const { return fY; }
    unsigned int width() const { return fWidth; }
    unsigned int height() const { return fHeight; }

    int visible() const { return (flags & wfVisible) ? 1 : 0; }
    int created() const { return (flags & wfCreated) ? 1 : 0; }
    int adopted() const { return (flags & wfAdopted) ? 1 : 0; }
    int destroyed() const { return (flags & wfDestroyed) ? 1 : 0; }
    int unmapped() const { return (flags & wfUnmapped) ? 1 : 0; }

    virtual void donePopup(YPopupWindow * /*command*/);

    typedef enum {
        wsOverrideRedirect = 1 << 0,
        wsSaveUnder        = 1 << 1,
        wsManager          = 1 << 2,
        wsInputOnly        = 1 << 3,
        wsOutputOnly       = 1 << 4,
        wsPointerMotion    = 1 << 5
    } WindowStyle;

    virtual bool isFocusTraversable();
    bool isFocused();
    bool isEnabled() const { return fEnabled; }
    void setEnabled(bool enable);
    void nextFocus();
    void prevFocus();
    bool changeFocus(bool next);
    void requestFocus();
    void setFocus(YWindow *window);
    YWindow *getFocusWindow();
    virtual void gotFocus();
    virtual void lostFocus();

    bool isToplevel() const { return fToplevel; }
    void setToplevel(bool toplevel) { fToplevel = toplevel; }

    YWindow *toplevel();

    void installAccelerator(unsigned int key, int vmod, YWindow *win);
    void removeAccelerator(unsigned int key, int vmod, YWindow *win);

    void _setToolTip(const char *tip);
    void setToolTip(const CStr *tip);

    void mapToGlobal(int &x, int &y);
    void mapToLocal(int &x, int &y);

    void setWinGravity(int gravity);
    void setBitGravity(int gravity);

    void setDND(bool enabled);
    bool startDrag(int nTypes, Atom *types);
    void endDrag(bool drop);

    void finishDrop();

    bool isDNDAware(Window w);

    void XdndStatus(bool acceptDrop, Atom dropAction);
    virtual void handleXdnd(const XClientMessageEvent &message);
    void handleDNDCrossing(const XCrossingEvent &crossing);
    void handleDNDMotion(const XMotionEvent &motion);
    Window findDNDTarget(Window w, int x, int y);
    void setDndTarget(Window dnd);
    void sendNewPosition();

    virtual void handleDNDEnter(int nTypes, Atom *types);
    virtual void handleDNDLeave();
    virtual bool handleDNDPosition(int x, int y, Atom *action);
    virtual void handleDNDDrop();
    virtual void handleDNDFinish();

    bool getCharFromEvent(const XKeyEvent &key, char *c);
    int getClickCount() { return fClickCount; }

    void scrollWindow(int dx, int dy);

    bool toolTipVisible();
    virtual void updateToolTip();

    void acquireSelection(bool selection);
    void clearSelection(bool selection);
    void requestSelection(bool selection);

    YWindow *firstChild() { return fFirstWindow; }

private:
    typedef enum {
        wfVisible   = 1 << 0,
        wfCreated   = 1 << 1,
        wfAdopted   = 1 << 2,
        wfDestroyed = 1 << 3,
        wfUnmapped  = 1 << 4,
        wfNullSize  = 1 << 5
    } WindowFlags;

    void create();
    void destroy();

    void insertWindow();
    void removeWindow();

    bool nullGeometry();
    
    YWindow *fParentWindow;
    YWindow *fNextWindow;
    YWindow *fPrevWindow;
    YWindow *fFirstWindow;
    YWindow *fLastWindow;
    YWindow *fFocusedWindow;

    Window fHandle;
    unsigned long flags;
    unsigned long fStyle;
    int fX, fY;
    unsigned int fWidth, fHeight;
    Cursor fPointer;
    int unmapCount;
    Graphics *fGraphics;
    long fEventMask;
    int fWinGravity, fBitGravity;

    bool fEnabled;
    bool fToplevel;

    typedef struct _YAccelerator {
        unsigned int key;
        int mod;
        YWindow *win;
        struct _YAccelerator *next;
    } YAccelerator;

    YAccelerator *accel;
    YToolTip *fToolTip;

    static XButtonEvent fClickEvent;
    static YWindow *fClickWindow;
    static Time fClickTime;
    static int fClickCount;
    static int fClickDrag;
    static unsigned int fClickButton;
    static unsigned int fClickButtonDown;
    static YTimer *fToolTipTimer;

    bool fDND;
    bool fDragging;
    Window XdndDragSource;
    Window XdndDropTarget;
    Window XdndDragTarget;
    Atom XdndTargetVersion;
    Atom *XdndTypes;
    int XdndNumTypes;
    Time XdndTimestamp;
    bool fWaitingForStatus;
    bool fGotStatus;
    bool fHaveNewPosition;
    int fNewPosX;
    int fNewPosY;
    bool fEndDrag;
    bool fDoDrop;

    static unsigned int MultiClickTime;
    static unsigned int ClickMotionDistance;
    static unsigned int ClickMotionDelay;
    static unsigned int ToolTipDelay;
    static AutoScroll *fAutoScroll;
private: // not-used
    YWindow(const YWindow &);
    YWindow &operator=(const YWindow &);
};

class YDesktop: public YWindow {
public:
    YDesktop(YWindow *aParent = 0, Window win = 0);
    virtual ~YDesktop();
    
    virtual void resetColormapFocus(bool active);
    virtual void manageWindow(YWindow *w, bool mapWindow);
    virtual void unmanageWindow(YWindow *w);
};

extern YDesktop *desktop;

#ifdef SHAPE
extern int shapesSupported;
extern int shapeEventBase, shapeErrorBase;
#endif

extern Atom _XA_WM_PROTOCOLS;
extern Atom _XA_WM_DELETE_WINDOW;
extern Atom _XA_WM_TAKE_FOCUS;
extern Atom _XA_WM_STATE;
extern Atom _XA_WM_CHANGE_STATE;
extern Atom _XATOM_MWM_HINTS;
extern Atom _XA_WM_COLORMAP_WINDOWS;
extern Atom _XA_CLIPBOARD;

/* Xdnd */
extern Atom XA_XdndAware;
extern Atom XA_XdndEnter;
extern Atom XA_XdndLeave;
extern Atom XA_XdndPosition;
extern Atom XA_XdndStatus;
extern Atom XA_XdndDrop;
extern Atom XA_XdndFinished;
extern Atom XA_XdndSelection;
extern Atom XA_XdndTypeList;

#ifdef GNOME1_HINTS
extern Atom _XA_WIN_PROTOCOLS;
extern Atom _XA_WIN_WORKSPACE;
extern Atom _XA_WIN_WORKSPACE_COUNT;
extern Atom _XA_WIN_WORKSPACE_NAMES;
extern Atom _XA_WIN_WORKAREA;
extern Atom _XA_WIN_LAYER;
extern Atom _XA_WIN_ICONS;
extern Atom _XA_WIN_HINTS;
extern Atom _XA_WIN_STATE;
extern Atom _XA_WIN_SUPPORTING_WM_CHECK;
extern Atom _XA_WIN_CLIENT_LIST;
extern Atom _XA_WIN_DESKTOP_BUTTON_PROXY;
extern Atom _XA_WIN_AREA;
extern Atom _XA_WIN_AREA_COUNT;
#endif
extern Atom _XA_WM_CLIENT_LEADER;
extern Atom _XA_SM_CLIENT_ID;
#ifdef WMSPEC_HINTS
extern Atom _XA_NET_SUPPORTED;
extern Atom _XA_NET_CLIENT_LIST;
extern Atom _XA_NET_NUMBER_OF_DESKTOPS;
///extern Atom _XA_NET_DESKTOP_GEOMETRY;
///extern Atom _XA_NET_DESKTOP_VIEWPORT;
extern Atom _XA_NET_CURRENT_DESKTOP;
extern Atom _XA_NET_ACTIVE_WINDOW;
extern Atom _XA_NET_WORKAREA;
extern Atom _XA_NET_SUPPORTING_WM_CHECK;
//extern Atom _XA_NET_CLOSE_WINDOW;
//extern Atom _XA_NET_WM_MOVERESIZE;
extern Atom _XA_NET_WM_DESKTOP;
extern Atom _XA_NET_WINDOW_TYPE;
extern Atom _XA_NET_WM_STRUT;
extern Atom _XA_NET_WM_HANDLED_ICONS;
extern Atom _XA_NET_WM_PID;
extern Atom _XA_NET_WM_PING;

extern Atom _XA_NET_WM_STATE;
extern Atom _XA_NET_WM_STATE_ADD;
extern Atom _XA_NET_WM_STATE_REMOVE;
extern Atom _XA_NET_WM_STATE_TOGGLE;
//extern Atom _XA_NET_WM_STATE_RESET; //!!! propose to wm-spec
extern Atom _XA_NET_WM_STATE_MODAL;
extern Atom _XA_NET_WM_STATE_STICKY;
extern Atom _XA_NET_WM_STATE_MAXIMIZED_VERT;
extern Atom _XA_NET_WM_STATE_MAXIMIZED_HORZ;
extern Atom _XA_NET_WM_STATE_SHADED;
extern Atom _XA_NET_WM_STATE_SKIP_TASKBAR;
extern Atom _XA_NET_WM_STATE_SKIP_PAGER;

#endif

/* KDE specific */
extern Atom _XA_KWM_WIN_ICON;

extern Atom XA_IcewmWinOptHint;

#endif
