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
#include "WinMgr.h"
#include "wmmgr.h"
#include "yicon.h"

class YClientContainer;
class MiniIcon;
class TaskBarApp;
class TrayApp;
class YFrameTitleBar;

class YFrameWindow: public YWindow, public YActionListener, public YTimerListener, public YPopDownListener, public YMsgBoxListener, public ClientData {
public:
    YFrameWindow(YWindow *parent);
    virtual ~YFrameWindow();

    void doManage(YFrameClient *client, bool &doActivate, bool &requestFocus);
    void afterManage();
    void manage(YFrameClient *client);
    void unmanage(bool reparent = true);
    void sendConfigure();

    void createPointerWindows();
    void grabKeys();

    void focus(bool canWarp = false);
    void activate(bool canWarp = false);
    void activateWindow(bool raise);

    virtual void paint(Graphics &g, const YRect &r);

    virtual bool handleKey(const XKeyEvent &key);
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
#ifdef CONFIG_TRAY
    void wmSetTrayOption(long option);
    void wmToggleTray();
#endif
#if DO_NOT_COVER_OLD
    void wmToggleDoNotCover();
#endif
    void wmToggleFullscreen();

    void minimizeTransients();
    void restoreMinimizedTransients();
    void hideTransients();
    void restoreHiddenTransients();

    void DoMaximize(long flags);

    void loseWinFocus();
    void setWinFocus();
    bool focused() const { return fFocused; }
    void updateFocusOnMap(bool &doActivate);

    YFrameClient *client() const { return fClient; }
    YFrameTitleBar *titlebar() const { return fTitleBar; }
    YClientContainer *container() const { return fClientContainer; }

#ifdef WMSPEC_HINTS
    void startMoveSize(int x, int y,
                                     int direction);
#endif

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

    void drawMoveSizeFX(int x, int y, int w, int h, bool interior = true);
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

    void getDefaultOptions(bool &doActivate);

    bool canSize(bool boriz = true, bool vert = true);
    bool canMove();
    bool canClose();
    bool canMaximize();
    bool canMinimize();
    bool canRollup();
    bool canHide();
    bool canLower();
    bool canRaise();
    bool canFullscreen() { return true; }
    bool Overlaps(bool below);

    void insertFrame(bool top);
    void removeFrame();
    void setAbove(YFrameWindow *aboveFrame); // 0 = at the bottom
    void setBelow(YFrameWindow *belowFrame); // 0 = at the top
    YFrameWindow *next() const { return fNextFrame; }
    YFrameWindow *prev() const { return fPrevFrame; }
    void setNext(YFrameWindow *next) { fNextFrame = next; }
    void setPrev(YFrameWindow *prev) { fPrevFrame = prev; }

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

    YFrameButton *menuButton() const { return fMenuButton; }
    YFrameButton *closeButton() const { return fCloseButton; }
    YFrameButton *minimizeButton() const { return fMinimizeButton; }
    YFrameButton *maximizeButton() const { return fMaximizeButton; }
    YFrameButton *hideButton() const { return fHideButton; }
    YFrameButton *rollupButton() const { return fRollupButton; }
    YFrameButton *depthButton() const { return fDepthButton; }
    void updateMenu();

    virtual void raise();
    virtual void lower();

    void popupSystemMenu(YWindow *owner, int x, int y,
                         unsigned int flags,
                         YWindow *forWindow = 0);
    virtual void popupSystemMenu(YWindow *owner);
    virtual void handlePopDown(YPopupWindow *popup);

    virtual void configure(const YRect &r);
    
    void getNewPos(const XConfigureRequestEvent &cr,
                   int &cx, int &cy, int &cw, int &ch);
    void configureClient(const XConfigureRequestEvent &configureRequest);
    void configureClient(int cx, int cy, int cwidth, int cheight);

#ifdef CONFIG_SHAPE
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

    /// !!! needs refactoring (some are not optional right now)
    /// should be #ifndef NO_WINDOW_OPTIONS

    enum YFrameOptions {
        foAllWorkspaces         = (1 << 0),
        foIgnoreTaskBar         = (1 << 1),
        foIgnoreWinList         = (1 << 2),
        foFullKeys              = (1 << 3),
        foIgnoreQSwitch         = (1 << 4),
        foNoFocusOnAppRaise     = (1 << 5),
        foIgnoreNoFocusHint     = (1 << 6),
        foIgnorePosition        = (1 << 7),
        foDoNotCover            = (1 << 8),
        foFullscreen            = (1 << 9),
        foMaximizedVert         = (1 << 10),
        foMaximizedHorz         = (1 << 11),
        foNonICCCMConfigureRequest = (1 << 12),
        foMinimized             = (1 << 13),
        foDoNotFocus            = (1 << 14),
        foForcedClose           = (1 << 15),
        foNoFocusOnMap          = (1 << 16),
        foNoIgnoreTaskBar       = (1 << 17),
        foAppTakesFocus         = (1 << 18)
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

    long getState() const { return fWinState; }
    void setState(long mask, long state);

    bool isFullscreen() const { return (getState() & WinStateFullscreen) ? true : false; }

    int borderXN() const;
    int borderYN() const;
    int titleYN() const;

    int borderX() const;
    int borderY() const;
    int titleY() const;
    
    void layoutTitleBar();
    void layoutButtons();
    void layoutResizeIndicators();
    void layoutShape();
    void layoutClient();

    //void workspaceShow();
    //void workspaceHide();

    YFrameWindow *nextLayer();
    YFrameWindow *prevLayer();
#ifdef CONFIG_WINLIST
    WindowListItem *winListItem() const { return fWinListItem; }
    void setWinListItem(WindowListItem *i) { fWinListItem = i; }
#endif

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
    YFrameWindow *mainOwner();

#ifndef LITE
    ref<YIcon> getClientIcon() const { return fFrameIcon; }
    ref<YIcon> clientIcon() const;
#endif

    void getNormalGeometryInner(int *x, int *y, int *w, int *h);
    void setNormalGeometryOuter(int x, int y, int w, int h);
    void setNormalPositionOuter(int x, int y);
    void setNormalGeometryInner(int x, int y, int w, int h);
    void updateDerivedSize(long flagmask);

    void setCurrentGeometryOuter(YRect newSize);
    void setCurrentPositionOuter(int x, int y);
    void updateNormalSize();

    void updateTitle();
    void updateIconTitle();
#ifndef LITE
    void updateIcon();
#endif
    void updateState();
    void updateLayer(bool restack = true);
    //void updateWorkspace();
    void updateLayout();
    void performLayout();

    void updateMwmHints();
    void updateProperties();
#ifdef CONFIG_TASKBAR
    void updateTaskBar();
#endif

    void setTypeDesktop(bool typeDesktop) { fTypeDesktop = typeDesktop; }
    void setTypeDock(bool typeDock) { fTypeDock = typeDock; }
    void setTypeSplash(bool typeSplash) { fTypeSplash = typeSplash; }
    bool isTypeDock() { return fTypeDock; }

    long getWorkspace() const { return fWinWorkspace; }
    void setWorkspace(long workspace);
    void setWorkspaceHint(long workspace);
    long getActiveLayer() const { return fWinActiveLayer; }
    void setRequestedLayer(long layer);
#ifdef CONFIG_TRAY
    long getTrayOption() const { return fWinTrayOption; }
    void setTrayOption(long option);
#endif
    void setDoNotCover(bool flag);
    bool isMaximized() const { return (getState() &
                                 (WinStateMaximizedHoriz |
                                  WinStateMaximizedVert)) ? true : false; }
    bool isMaximizedVert() const { return (getState() & WinStateMaximizedVert) ? true : false; }
    bool isMaximizedHoriz() const { return (getState() & WinStateMaximizedHoriz) ? true : false; }
    bool isMaximizedFully() const { return isMaximizedVert() && isMaximizedHoriz(); }
    bool isMinimized() const { return (getState() & WinStateMinimized) ? true : false; }
    bool isHidden() const { return (getState() & WinStateHidden) ? true : false; }
    bool isSkipTaskBar() const { return (getState() & WinStateSkipTaskBar) ? true : false; }
    bool isRollup() const { return (getState() & WinStateRollup) ? true : false; }
    bool isSticky() const { return (getState() & WinStateAllWorkspaces) ? true : false; }
    //bool isHidWorkspace() { return (getState() & WinStateHidWorkspace) ? true : false; }
    //bool isHidTransient() { return (getState() & WinStateHidTransient) ? true : false; }

    bool wasMinimized() const { return (getState() & WinStateWasMinimized) ? true : false; }
    bool wasHidden() const { return (getState() & WinStateWasHidden) ? true : false; }

    bool isIconic() const { return isMinimized() && fMiniIcon; }

    MiniIcon *getMiniIcon() const { return fMiniIcon; }

    bool isManaged() const { return fManaged; }
    void setManaged(bool isManaged) { fManaged = isManaged; }

    void setSticky(bool sticky);

    bool visibleOn(long workspace) const {
        return (isSticky() || getWorkspace() == workspace);
    }
    bool visibleNow() const { return visibleOn(manager->activeWorkspace()); }

    bool isModal();
    bool hasModal();
    bool canFocus();
    bool canFocusByMouse();
    bool avoidFocus();
    bool getInputFocusHint();

    bool inWorkArea() const;
    bool affectsWorkArea() const;

    bool doNotCover() const {
        return (frameOptions() & foDoNotCover) ? true : false;
    }

#ifndef LITE
    virtual ref<YIcon> getIcon() const { return clientIcon(); }
#endif

    virtual ustring getTitle() const { return client()->windowTitle(); }
    virtual ustring getIconTitle() const { return client()->iconTitle(); }

    YFrameButton *getButton(char c);
    void positionButton(YFrameButton *b, int &xPos, bool onRight);
    bool isButton(char c);

#ifdef WMSPEC_HINTS
    void updateNetWMStrut();
#endif
    int strutLeft() { return fStrutLeft; }
    int strutRight() { return fStrutRight; }
    int strutTop() { return fStrutTop; }
    int strutBottom() { return fStrutBottom; }

    YFrameWindow *nextCreated() { return fNextCreatedFrame; }
    YFrameWindow *prevCreated() { return fPrevCreatedFrame; }
    void setNextCreated(YFrameWindow *f) { fNextCreatedFrame = f; }
    void setPrevCreated(YFrameWindow *f) { fPrevCreatedFrame = f; }

    YFrameWindow *nextFocus() { return fNextFocusFrame; }
    YFrameWindow *prevFocus() { return fPrevFocusFrame; }
    void setNextFocus(YFrameWindow *f) { fNextFocusFrame = f; }
    void setPrevFocus(YFrameWindow *f) { fPrevFocusFrame = f; }

    void insertFocusFrame(bool focus);
    void insertLastFocusFrame();
    void removeFocusFrame();

    void updateUrgency();
    void setWmUrgency(bool wmUrgency);
    bool isUrgent() { return fWmUrgency || fClientUrgency; }

    int getScreen();

    long getOldLayer() { return fOldLayer; }
    void saveOldLayer() { fOldLayer = fWinActiveLayer; }

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

    int normalX, normalY, normalW, normalH;
    int posX, posY, posW, posH;

    int iconX, iconY;

    YFrameClient *fClient;
    YClientContainer *fClientContainer;
    YFrameTitleBar *fTitleBar;
    YFrameButton *fCloseButton;
    YFrameButton *fMenuButton;
    YFrameButton *fMaximizeButton;
    YFrameButton *fMinimizeButton;
    YFrameButton *fHideButton;
    YFrameButton *fRollupButton;
    YFrameButton *fDepthButton;

    YPopupWindow *fPopupActive;

    int buttonDownX, buttonDownY;
    int grabX, grabY;
    int movingWindow, sizingWindow;
    int sizeByMouse;
    int origX, origY, origW, origH;

    YFrameWindow *fNextFrame; // window below this one
    YFrameWindow *fPrevFrame; // window above this one

    YFrameWindow *fNextCreatedFrame;
    YFrameWindow *fPrevCreatedFrame;

    YFrameWindow *fNextFocusFrame;
    YFrameWindow *fPrevFocusFrame;

    Window topSide, leftSide, rightSide, bottomSide;
    Window topLeftCorner, topRightCorner, bottomLeftCorner, bottomRightCorner;
    int indicatorsVisible;

#ifdef CONFIG_TASKBAR
    TaskBarApp *fTaskBarApp;
#endif
#ifdef CONFIG_TRAY
    TrayApp *fTrayApp;
#endif
    MiniIcon *fMiniIcon;
#ifdef CONFIG_WINLIST
    WindowListItem *fWinListItem;
#endif
#ifndef LITE
    ref<YIcon> fFrameIcon;
#endif

    YFrameWindow *fOwner;
    YFrameWindow *fTransient;
    YFrameWindow *fNextTransient;

    static YTimer *fAutoRaiseTimer;
    static YTimer *fDelayFocusTimer;

    long fWinWorkspace;
    long fWinRequestedLayer;
    long fWinActiveLayer;
#ifdef CONFIG_TRAY
    long fWinTrayOption;
#endif
    long fWinState;
    long fWinStateMask;
    bool fManaged;
    long fWinOptionMask;
    long fOldLayer;

    YMsgBox *fKillMsgBox;

    // _NET_WM_STRUT support
    int fStrutLeft;
    int fStrutRight;
    int fStrutTop;
    int fStrutBottom;

    int fShapeWidth;
    int fShapeHeight;
    int fShapeTitleY;
    int fShapeBorderX;
    int fShapeBorderY;

    bool fWmUrgency;
    bool fClientUrgency;
    bool fTypeDesktop;
    bool fTypeDock;
    bool fTypeSplash;

    enum WindowArranges {
        waTop,
        waBottom,
        waCenter,
        waLeft,
        waRight
    };

    void wmArrange(int tcb, int lcr);
    void wmSnapMove(int tcb, int lcr);
    int getTopCoord(int my, YFrameWindow **w, int count);
    int getBottomCoord(int My, YFrameWindow **w, int count);
    int getLeftCoord(int mx, YFrameWindow **w, int count);
    int getRightCoord(int Mx, YFrameWindow **w, int count);

    // only focus if mouse moves
    //static int fMouseFocusX, fMouseFocusY;

    void setGeometry(const YRect &);
    void setPosition(int, int);
    void setSize(int, int);
    void setWindowGeometry(const YRect &r) {
        YWindow::setGeometry(r);
        performLayout();
    }
    friend class MiniIcon;
};

//!!! remove this
extern ref<YPixmap> frameTL[2][2];
extern ref<YPixmap> frameT[2][2];
extern ref<YPixmap> frameTR[2][2];
extern ref<YPixmap> frameL[2][2];
extern ref<YPixmap> frameR[2][2];
extern ref<YPixmap> frameBL[2][2];
extern ref<YPixmap> frameB[2][2];
extern ref<YPixmap> frameBR[2][2];

extern ref<YPixmap> titleJ[2];
extern ref<YPixmap> titleL[2];
extern ref<YPixmap> titleS[2];
extern ref<YPixmap> titleP[2];
extern ref<YPixmap> titleT[2];
extern ref<YPixmap> titleM[2];
extern ref<YPixmap> titleB[2];
extern ref<YPixmap> titleR[2];
extern ref<YPixmap> titleQ[2];

extern ref<YPixmap> menuButton[3];

#ifdef CONFIG_GRADIENTS
extern ref<YImage> rgbFrameT[2][2];
extern ref<YImage> rgbFrameL[2][2];
extern ref<YImage> rgbFrameR[2][2];
extern ref<YImage> rgbFrameB[2][2];
extern ref<YImage> rgbTitleS[2];
extern ref<YImage> rgbTitleT[2];
extern ref<YImage> rgbTitleB[2];
#endif

#endif

