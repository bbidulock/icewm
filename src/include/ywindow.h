#ifndef __YWINDOW_H
#define __YWINDOW_H

#include "yxbase.h"

#pragma interface

class Graphics;
class YPopupWindow;
class YToolTip;
class YTimer;
class AutoScroll;
class CStr;
class YRect;
class YPointer;
class YKeyBind;

class YKeyEvent;
class YButtonEvent;
class YClickEvent;
class YMotionEvent;
class YCrossingEvent;
class YFocusEvent;

typedef unsigned long XWindowId;
typedef unsigned long XAtomId;
typedef unsigned long XTime;
typedef unsigned long XColormap;
typedef unsigned long XPixmapId;

class YWindow {
public:
    YWindow(YWindow *aParent = 0);
    YWindow(YWindow *aParent, XWindowId win);
    virtual ~YWindow();

private:
    void init();
public:

    void setStyle(unsigned long aStyle);
    unsigned long getStyle() { return fStyle; }

    void show();
    void hide();
    virtual void raise();
    virtual void lower();

    void repaint();
    void repaintFocus();

    void reparent(YWindow *parent, int x, int y);

    void setWindowFocus();

    void setGeometry(int x, int y, unsigned int width, unsigned int height);
    void setGeometry(const YRect &gr);
    void setSize(unsigned int width, unsigned int height);
    void setPosition(int x, int y);
    virtual void configure(const YRect &cr);

    virtual void paint(Graphics &g, const YRect &er);
    virtual void paintFocus(Graphics &g, const YRect &er);

    virtual bool eventKey(const YKeyEvent &key);
    virtual bool eventButton(const YButtonEvent &button);
    virtual bool eventClickDown(const YClickEvent &down);
    virtual bool eventClick(const YClickEvent &up);
    virtual bool eventMotion(const YMotionEvent &motion);
    virtual bool eventBeginDrag(const YButtonEvent &down, const YMotionEvent &motion);
    virtual bool eventDrag(const YButtonEvent &down, const YMotionEvent &motion);
    virtual bool eventEndDrag(const YButtonEvent &down, const YButtonEvent &up);
    virtual bool eventCrossing(const YCrossingEvent &crossing);
    virtual bool eventFocus(const YFocusEvent &focus);

    virtual void handleEvent(const XEvent &event);
    virtual bool handleCrossing(const XCrossingEvent &crossing);
    virtual bool handleFocus(const XFocusChangeEvent &focus);

    virtual void handleExpose(const XExposeEvent &expose);
    virtual void handleGraphicsExpose(const XGraphicsExposeEvent &graphicsExpose);
    virtual void handleConfigure(const XConfigureEvent &configure);
    virtual void handleProperty(const XPropertyEvent &property);
    virtual void handleColormap(const XColormapEvent &colormap);
    virtual void handleClientMessage(const XClientMessageEvent &message);
    virtual void handleSelectionClear(const XSelectionClearEvent &clear);
    virtual void handleSelectionRequest(const XSelectionRequestEvent &request);
    virtual void handleSelection(const XSelectionEvent &selection);
#ifdef NEVER
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

    virtual void handleClose();

    virtual bool handleAutoScroll(const YMotionEvent &mouse);
    void beginAutoScroll(bool autoScroll, const YMotionEvent *motion);

    void setPointer(YPointer *pointer);
    void grabKeyM(int key, int modifiers);
    void grabKey(int key, int modifiers);
    void grabVKey(int key, int modifiers);
    void grabButtonM(int button, int modifiers);
    void grabButton(int button, int modifiers);

    void captureEvents();
    void releaseEvents();

    XWindowId handle();
    YWindow *parent() const { return fParentWindow; }

    Graphics &beginPaint();

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
        wsPointerMotion    = 1 << 5,
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

    void installAccelerator(int key, int mod, YWindow *win);
    void removeAccelerator(int key, int mod, YWindow *win);

    void __setToolTip(const char *tip);
    void setToolTip(const CStr *tip);

    void mapToGlobal(int &x, int &y);
    void mapToLocal(int &x, int &y);

    void setWinGravity(int gravity);
    void setBitGravity(int gravity);

    void setDND(bool enabled);
    bool startDrag(int nTypes, XAtomId *types);
    void endDrag(bool drop);

    void finishDrop();

    bool isDNDAware(XWindowId w);

    void XdndStatus(bool acceptDrop, XAtomId dropAction);
    virtual void handleXdnd(const XClientMessageEvent &message);
    void handleDNDCrossing(const XCrossingEvent &crossing);
    void handleDNDMotion(const XMotionEvent &motion);
    XWindowId findDNDTarget(XWindowId w, int x, int y);
    void setDndTarget(XWindowId dnd);
    void sendNewPosition();

    virtual void handleDNDEnter(int nTypes, XAtomId *types);
    virtual void handleDNDLeave();
    virtual bool handleDNDPosition(int x, int y, XAtomId *action);
    virtual void handleDNDDrop();
    virtual void handleDNDFinish();

    int getClickCount();

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

    Graphics &getGraphics();

    bool getCharFromEvent(const XKeyEvent &key, char *c);

    YWindow *fParentWindow;
    YWindow *fNextWindow;
    YWindow *fPrevWindow;
    YWindow *fFirstWindow;
    YWindow *fLastWindow;
    YWindow *fFocusedWindow;

    XWindowId fHandle;
    unsigned long flags;
    unsigned long fStyle;
    int fX, fY;
    unsigned int fWidth, fHeight;
    YPointer *fPointer;
    int unmapCount;
    Graphics *fGraphics;
    long fEventMask;
    int fWinGravity, fBitGravity;

    bool fEnabled;
    bool fToplevel;

    class YAccelerator;

    YAccelerator *accel;
    YToolTip *fToolTip;

    XWindowId XdndDragSource;
    XWindowId XdndDropTarget;
    XWindowId XdndDragTarget;
    XAtomId XdndTargetVersion;
    XAtomId *XdndTypes;
    int XdndNumTypes;
    XTime XdndTimestamp;
    int fNewPosX;
    int fNewPosY;
    bool fWaitingForStatus;
    bool fGotStatus;
    bool fHaveNewPosition;
    bool fEndDrag;
    bool fDoDrop;
    bool fDND;
    bool fDragging;

    static YTimer *fToolTipTimer;
    static AutoScroll *fAutoScroll;
private: // not-used
    YWindow(const YWindow &);
    YWindow &operator=(const YWindow &);
};

class YDesktop: public YWindow {
public:
    YDesktop(YWindow *aParent = 0, XWindowId win = 0);
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

#endif
