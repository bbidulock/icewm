#ifndef __WMFRAME_H
#define __WMFRAME_H

#include "ywindow.h"
#include "ymenu.h"
#include "ytimer.h"
#include "ymsgbox.h"
#include "yaction.h"
#include "wmclient.h"
#include "wmbutton.h"
#include "wmoption.h"
#include "yconfig.h"
#include "prefs.h"
#include "WinMgr.h"
#include "wmmgr.h"

class YClientContainer;
class MiniIcon;
class TaskBarApp;
class YFrameTitleBar;

class YFrameWindow: public YWindow, public YActionListener, public YTimerListener, public PopDownListener, public YMsgBoxListener, public ClientData {
public:
    YFrameWindow(YWindow *parent, YFrameClient *client, YWindowManager *root);
    virtual ~YFrameWindow();

    void manage(YFrameClient *client);
    void unmanage();
    void sendConfigure();

    void createPointerWindows();
    void grabKeys();

    void focus(bool canWarp = false);
    void activate(bool canWarp = false);

    void activateWindow(bool raise) {
        if (raise)
            wmRaise();
        activate(true);
    }

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);

    virtual bool handleKeySym(const XKeyEvent &key, KeySym ksym, int vmod);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleBeginDrag(const XButtonEvent &down, const XMotionEvent &motion);
    virtual void handleDrag(const XButtonEvent &down, const XMotionEvent &motion);
    virtual void handleEndDrag(const XButtonEvent &down, const XButtonEvent &up);
    virtual void handleMotion(const XMotionEvent &motion);
    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual void handleFocus(const XFocusChangeEvent &focus);
    virtual void handleConfigure(const XConfigureEvent &configure);

    virtual bool handleTimer(YTimer *t);

    virtual void actionPerformed(YAction *action, unsigned int modifiers);
    virtual void handleMsgBox(YMsgBox *msgbox, int operation);
    
    void wmRestore();
    void wmMinimize();
    void wmMaximize();
    void wmMaximizeVert();
    void wmMaximizeHorz();
    void wmRollup();
    void wmHide();
    void wmShow();
    void wmLower();
    void doLower();
    void wmRaise();
    void doRaise();
    void wmClose();
    void wmConfirmKill();
    void wmKill();
    void wmNextWindow();
    void wmPrevWindow();
    void wmLastWindow();
    void wmMove();
    void wmSize();
    void wmOccupyAll();
    void wmOccupyAllOrCurrent();
    void wmOccupyWorkspace(long workspace);
    void wmOccupyOnlyWorkspace(long workspace);
    void wmMoveToWorkspace(long workspace);
    void wmSetLayer(long layer);

    void minimizeTransients();
    void restoreTransients();

    void DoMaximize(long flags);

    void loseWinFocus();
    void setWinFocus();
    bool focused() const { return fFocused; }
    void focusOnMap();

    YFrameClient *client() const { return fClient; }
    YFrameTitleBar *titlebar() const { return fTitleBar; }
    YClientContainer *container() const { return fClientContainer; }

    void startMoveSize(int doMove, int byMouse,
                       int sideX, int sideY,
                       int mouseXroot, int mouseYroot);
    void endMoveSize();
    void moveWindow(int newX, int newY);
    void manualPlace();
    void snapTo(int &wx, int &wy,
                int rx1, int ry1, int rx2, int ry2,
                int &flags);
    void snapTo(int &wx, int &wy);

    void drawOutline(int x, int y, int w, int h);
    int handleMoveKeys(const XKeyEvent &xev, int &newX, int &newY);
    int handleResizeKeys(const XKeyEvent &key,
                         int &newX, int &newY, int &newWidth, int &newHeight,
                         int incX, int incY);
    void handleMoveMouse(const XMotionEvent &motion, int &newX, int &newY);
    void handleResizeMouse(const XMotionEvent &motion,
                           int &newX, int &newY, int &newWidth, int &newHeight);

    void outlineMove();
    void outlineResize();
    
    void constrainPositionByModifier(int &x, int &y, const XMotionEvent &motion);
    void constrainMouseToWorkspace(int &x, int &y);

    void getDefaultOptions();

    bool canSize(bool boriz = true, bool vert = true);
    bool canMove();
    bool canClose();
    bool canMaximize();
    bool canMinimize();
    bool canRollup();
    bool canHide();
    bool canLower();
    bool canRaise();
    bool Overlaps(bool below);

    void insertFrame();
    void removeFrame();
    void setAbove(YFrameWindow *aboveFrame); // 0 = at the bottom
    void setBelow(YFrameWindow *belowFrame); // 0 = at the top
    YFrameWindow *next() const { return fNextFrame; }
    YFrameWindow *prev() const { return fPrevFrame; }
    void setNext(YFrameWindow *next) { fNextFrame = next; }
    void setPrev(YFrameWindow *prev) { fPrevFrame = prev; }

    YWindowManager *getRoot() const { return fRoot; }

    typedef enum {
        fwfVisible    = 1 << 0, // visible windows only
        fwfCycle      = 1 << 1, // cycle when bottom(top) reached
        fwfBackward   = 1 << 2, // go up in zorder (default=down)
        fwfNext       = 1 << 3, // start from next window
        fwfFocusable  = 1 << 4, // only focusable windows
        fwfWorkspace  = 1 << 5, // current workspace only
        fwfSame       = 1 << 6, // return same if no match and same matches
        fwfLayers     = 1 << 7, // find windows in other layers
        fwfSwitchable = 1 << 8, // window can be Alt+Tabbed
        fwfMinimized  = 1 << 9, // minimized/visible windows
        fwfUnminimized = 1 << 10, // normal/rolledup only
        fwfHidden     = 1 << 11, // hidden
        fwfNotHidden  = 1 << 12 // not hidden
    } FindWindowFlags;

    YFrameWindow *findWindow(int flag);

    void updateMenu();

    virtual void raise();
    virtual void lower();

    void popupSystemMenu(int x, int y,
                         int x_delta, int y_delta,
                         unsigned int flags,
                         YWindow *forWindow = 0);
    virtual void popupSystemMenu(void);
    virtual void handlePopDown(YPopupWindow *popup);

    virtual void configure(int x, int y, unsigned int width, unsigned int height);
    
    void configureClient(const XConfigureRequestEvent &configureRequest);
    void configureClient(int cx, int cy, int cwidth, int cheight);

#ifdef SHAPE
    void setShape();
#endif

    enum {
        ffMove          = (1 << 0),
        ffResize        = (1 << 1),
        ffClose         = (1 << 2),
        ffMinimize      = (1 << 3),
        ffMaximize      = (1 << 4),
        ffHide          = (1 << 5),
        ffRollup        = (1 << 6)
    } YFrameFunctions;

    enum {
        fdTitleBar      = (1 << 0),
        fdSysMenu       = (1 << 1),
        fdBorder        = (1 << 2),
        fdResize        = (1 << 3),
        fdClose         = (1 << 4),
        fdMinimize      = (1 << 5),
        fdMaximize      = (1 << 6),
        fdHide          = (1 << 7),
        fdRollup        = (1 << 8),
        fdDepth         = (1 << 9)
    } YFrameDecors;

    enum YFrameOptions {
        foAllWorkspaces = (1 << 1),
        foIgnoreTaskBar = (1 << 2),
        foIgnoreWinList = (1 << 3),
        foFullKeys      = (1 << 4),
        foIgnoreQSwitch = (1 << 5),
        foNoFocusOnAppRaise = (1 << 6),
        foIgnoreNoFocusHint = (1 << 7),
        foIgnorePosition = (1 << 7)
    };

    unsigned long frameFunctions() const { return fFrameFunctions; }
    unsigned long frameDecors() const { return fFrameDecors; }
    unsigned long frameOptions() const { return fFrameOptions; }
    void getFrameHints();
#ifndef NO_WINDOW_OPTIONS
    void getWindowOptions(WindowOption &opt, bool remove); /// !!! fix kludges
    void getWindowOptions(WindowOptions *list, WindowOption &opt, bool remove);
#endif

    YMenu *windowMenu();

    unsigned int borderLeft() const;
    unsigned int borderRight() const;
    unsigned int borderTop() const;
    unsigned int borderBottom() const;
    unsigned int titleY() const;
    
    void layoutTitleBar();
    void layoutResizeIndicators();
    void layoutClient();

    //void workspaceShow();
    //void workspaceHide();

    void addToMenu(YMenu *menu);

    YFrameWindow *nextLayer();
    YFrameWindow *prevLayer();

    void addAsTransient();
    void removeAsTransient();
    void addTransients();
    void removeTransients();

    void setTransient(YFrameWindow *transient) { fTransient = transient; }
    void setNextTransient(YFrameWindow *nextTransient) { fNextTransient = nextTransient; }
    void setOwner(YFrameWindow *owner) { fOwner = owner; }
    YFrameWindow *transient() const { return fTransient; }
    YFrameWindow *nextTransient() const { return fNextTransient; }
    YFrameWindow *owner() const { return fOwner; }

#ifndef LITE
    YIcon *getClientIcon() const { return fFrameIcon; }
    YIcon *clientIcon();
#endif

    void getNormalGeometry(int *x, int *y, int *w, int *h);
    void updateNormalSize();

    void updateTitle();
    void updateIconTitle();
#ifndef LITE
    void updateIcon();
#endif
    void updateState();
    //void updateWorkspace();
    void updateLayout();

    void updateMwmHints();
    void updateProperties();

    long getWorkspace() const { return fWinWorkspace; }
    void setWorkspace(long workspace);
    long getLayer() const { return fWinLayer; }
    void setLayer(long layer);
    long getState() const { return fWinState; }
    void setState(long mask, long state);

    bool isMaximized() const { return (getState() &
                                 (WinStateMaximizedHoriz |
                                  WinStateMaximizedVert)) ? true : false; }
    bool isMaximizedVert() const { return (getState() & WinStateMaximizedVert) ? true : false; }
    bool isMaximizedHoriz() const { return (getState() & WinStateMaximizedHoriz) ? true : false; }
    bool isMaximizedFully() const { return isMaximizedVert() && isMaximizedHoriz(); }
    bool isMinimized() const { return (getState() & WinStateMinimized) ? true : false; }
    bool isHidden() const { return (getState() & WinStateHidden) ? true : false; }
    bool isRollup() const { return (getState() & WinStateRollup) ? true : false; }
    bool isSticky() const { return (getState() & WinStateAllWorkspaces) ? true : false; }
    //bool isHidWorkspace() { return (getState() & WinStateHidWorkspace) ? true : false; }
    //bool isHidTransient() { return (getState() & WinStateHidTransient) ? true : false; }

    bool isIconic() const { return isMinimized() && fMiniIcon; }

    MiniIcon *getMiniIcon() const { return fMiniIcon; }

    bool isManaged() const { return fManaged; }
    void setManaged(bool isManaged) { fManaged = isManaged; }

    void setSticky(bool sticky);

    int visibleOn(long workspace) {
        return (isSticky() || getWorkspace() == workspace);
    }
    int visibleNow() { return visibleOn(getRoot()->activeWorkspace()); }

    bool isModal();
    bool hasModal();
    bool isFocusable();

    bool shouldRaise(const XButtonEvent &button);

#ifndef LITE
    virtual YIcon *getIcon() { return clientIcon(); }
#endif
    virtual const CStr *getTitle() { return client()->windowTitle(); }
    virtual const CStr *getIconTitle() { return client()->iconTitle(); }
private:
    /*typedef enum {
        fsMinimized       = 1 << 0,
        fsMaximized       = 1 << 1,
        fsRolledup        = 1 << 2,
        fsHidden          = 1 << 3,
        fsWorkspaceHidden = 1 << 4
    } FrameStateFlags;*/

    bool fFocused;
    unsigned long fFrameFunctions;
    unsigned long fFrameDecors;
    unsigned long fFrameOptions;

    int normalX, normalY;
    unsigned int normalWidth, normalHeight;
    int iconX, iconY;

    YWindowManager *fRoot;
    YFrameClient *fClient;
    YClientContainer *fClientContainer;
    YFrameTitleBar *fTitleBar;

    YPopupWindow *fPopupActive;

    int buttonDownX, buttonDownY;
    int grabX, grabY;
    bool movingWindow, sizingWindow;
    int sizeByMouse;
    int origX, origY, origW, origH;

    YFrameWindow *fNextFrame; // window below this one
    YFrameWindow *fPrevFrame; // window above this one

    Window topSide, leftSide, rightSide, bottomSide;
    Window topLeftCorner, topRightCorner, bottomLeftCorner, bottomRightCorner;
    int indicatorsVisible;

    MiniIcon *fMiniIcon;
    YIcon *fFrameIcon;

    YFrameWindow *fOwner;
    YFrameWindow *fTransient;
    YFrameWindow *fNextTransient;

    static YTimer *fAutoRaiseTimer;
    static YTimer *fDelayFocusTimer;

    long fWinWorkspace;
    long fWinLayer;
    long fWinState;
    long fWinStateMask;
    bool fManaged;

    bool fWasMinimized; // !!! bug, fix it

    YMsgBox *fKillMsgBox;

    static YNumPrefProperty gBorderL;
    static YNumPrefProperty gBorderR;
    static YNumPrefProperty gBorderT;
    static YNumPrefProperty gBorderB;
    static YNumPrefProperty gDlgBorderL;
    static YNumPrefProperty gDlgBorderR;
    static YNumPrefProperty gDlgBorderT;
    static YNumPrefProperty gDlgBorderB;
    static YNumPrefProperty gCornerX;
    static YNumPrefProperty gCornerY;
    static YNumPrefProperty gTitleHeight;
    static YNumPrefProperty gEdgeResistance;
    static YNumPrefProperty gPointerFocusDelay;
    static YNumPrefProperty gAutoRaiseDelay;
    static YNumPrefProperty gSnapDistance;
    static YNumPrefProperty gButtonRaiseMask;
    static YBoolPrefProperty gClientMouseActions;
    static YBoolPrefProperty gMinimizeToDesktop;
    static YBoolPrefProperty gAutoRaise;
    static YBoolPrefProperty gLimitPosition;
    static YBoolPrefProperty gOpaqueMove;
    static YBoolPrefProperty gOpaqueResize;
    static YBoolPrefProperty gSizeMaximized;
    static YBoolPrefProperty gSnapMove;
    static YBoolPrefProperty gFocusOnMap;
    static YBoolPrefProperty gFocusOnMapTransient;
    static YBoolPrefProperty gFocusOnMapTransientActive;
    static YBoolPrefProperty gStrongPointerFocus;
    static YBoolPrefProperty gDelayPointerFocus;
    static YBoolPrefProperty gFocusRootWindow;
    static YBoolPrefProperty gRaiseOnClickFrame;
    static YBoolPrefProperty gRaiseOnFocus;
    static YBoolPrefProperty gRaiseOnClickClient;
    static YBoolPrefProperty gFocusOnClickClient;
    static YBoolPrefProperty gClickFocus;
};

extern YPixmap *frameTL[2][2];
extern YPixmap *frameT[2][2];
extern YPixmap *frameTR[2][2];
extern YPixmap *frameL[2][2];
extern YPixmap *frameR[2][2];
extern YPixmap *frameBL[2][2];
extern YPixmap *frameB[2][2];
extern YPixmap *frameBR[2][2];

#if 0
extern YPixmap *menuButton[2];
#endif

extern Cursor movePointer;
extern Cursor sizeRightPointer;
extern Cursor sizeTopRightPointer;
extern Cursor sizeTopPointer;
extern Cursor sizeTopLeftPointer;
extern Cursor sizeLeftPointer;
extern Cursor sizeBottomLeftPointer;
extern Cursor sizeBottomPointer;
extern Cursor sizeBottomRightPointer;

#endif
