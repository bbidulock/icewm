#ifndef __WMFRAME_H
#define __WMFRAME_H

#include "ymsgbox.h"
#include "wmoption.h"
#include "wmmgr.h"
#include "yicon.h"
#include "ylist.h"

class YClientContainer;
class MiniIcon;
class TaskBarApp;
class TrayApp;
class YFrameTitleBar;

class YFrameWindow:
    public YWindow,
    public YActionListener,
    public YTimerListener,
    public YPopDownListener,
    public YMsgBoxListener,
    public ClientData,
    public YLayeredNode,
    public YCreatedNode,
    public YFocusedNode
{
public:
    YFrameWindow(YActionListener *wmActionListener, YWindow *parent = 0, int depth = CopyFromParent, Visual *visual = CopyFromParent);
    virtual ~YFrameWindow();

    void doManage(YFrameClient *client, bool &doActivate, bool &requestFocus);
    void afterManage();
    void manage(YFrameClient *client);
    void unmanage(bool reparent = true);
    void sendConfigure();

    Window createPointerWindow(Cursor cursor, Window parent);
    void createPointerWindows();
    void grabKeys();

    void focus(bool canWarp = false);
    void activate(bool canWarp = false, bool curWork = true);
    void activateWindow(bool raise, bool curWork = true);

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

    virtual void actionPerformed(YAction action, unsigned int modifiers);
    virtual void handleMsgBox(YMsgBox *msgbox, int operation);
    virtual YFrameWindow* frame() { return this; }

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
    static YMsgBox* wmConfirmKill(const ustring& title, YMsgBoxListener *recvr);
    void wmKill();
    void wmNextWindow();
    void wmPrevWindow();
    void wmMove();
    void wmSize();
    void wmOccupyAll();
    void wmOccupyAllOrCurrent();
    void wmOccupyWorkspace(int workspace);
    void wmOccupyOnlyWorkspace(int workspace);
    void wmMoveToWorkspace(int workspace);
    void wmSetLayer(long layer);
    void wmSetTrayOption(long option);
    void wmToggleTray();
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
    YFrameTitleBar *titlebar();
    YClientContainer *container() const { return fClientContainer; }

    void startMoveSize(int x, int y, int direction);

    void startMoveSize(bool doMove, bool byMouse,
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
    bool canMove() const;
    bool canClose() const;
    bool canMaximize() const;
    bool canMinimize() const;
    bool canRestore() const;
    bool canRollup() const;
    bool canHide() const;
    bool canLower() const;
    bool canRaise();
    bool canFullscreen() { return true; }
    bool overlaps(bool below);
    unsigned overlap(YFrameWindow *other);

    void insertFrame(bool top);
    void removeFrame();
    void setAbove(YFrameWindow *aboveFrame); // 0 = at the bottom
    void setBelow(YFrameWindow *belowFrame); // 0 = at the top

    enum FindWindowFlags {
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
    };

    YFrameWindow *findWindow(int flag);

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

    enum YFrameFunctions {
        ffMove          = (1 << 0),
        ffResize        = (1 << 1),
        ffClose         = (1 << 2),
        ffMinimize      = (1 << 3),
        ffMaximize      = (1 << 4),
        ffHide          = (1 << 5),
        ffRollup        = (1 << 6)
    };

    enum YFrameDecors {
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
    };

    enum YFrameOptions {
        foAllWorkspaces            = (1 << 0),
        foAppTakesFocus            = (1 << 1),
        foDoNotCover               = (1 << 2),
        foDoNotFocus               = (1 << 3),
        foForcedClose              = (1 << 4),
        foFullKeys                 = (1 << 5),
        foFullscreen               = (1 << 6),
        foIgnoreNoFocusHint        = (1 << 7),
        foIgnorePagerPreview       = (1 << 8),
        foIgnorePosition           = (1 << 9),
        foIgnoreQSwitch            = (1 << 10),
        foIgnoreTaskBar            = (1 << 11),
        foIgnoreUrgent             = (1 << 12),
        foIgnoreWinList            = (1 << 13),
        foMaximizedHorz            = (1 << 14),
        foMaximizedVert            = (1 << 15),
        foMinimized                = (1 << 16),
        foNoFocusOnAppRaise        = (1 << 17),
        foNoFocusOnMap             = (1 << 18),
        foNoIgnoreTaskBar          = (1 << 19),
        foNonICCCMConfigureRequest = (1 << 20),
    };

    unsigned frameFunctions() const { return fFrameFunctions; }
    unsigned frameDecors() const { return fFrameDecors; }
    unsigned frameOptions() const { return fFrameOptions; }
    void updateAllowed();
    void updateNetWMState();
    void getFrameHints();
    void getWindowOptions(WindowOption &opt, bool remove); /// !!! fix kludges
    void getWindowOptions(WindowOptions *list, WindowOption &opt, bool remove);

    YMenu *windowMenu();

    long getState() const { return fWinState; }
    void setState(long mask, long state);

    bool isFullscreen() const { return (getState() & WinStateFullscreen); }

    int borderXN() const;
    int borderYN() const;
    int titleYN() const;

    int borderX() const;
    int borderY() const;
    int titleY() const;

    void layoutTitleBar();
    void layoutResizeIndicators();
    void layoutShape();
    void layoutClient();

    YFrameWindow *nextLayer();
    YFrameWindow *prevLayer();
    WindowListItem *winListItem() const { return fWinListItem; }
    void setWinListItem(WindowListItem *i) { fWinListItem = i; }

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

    ref<YIcon> getClientIcon() const { return fFrameIcon; }
    ref<YIcon> clientIcon() const;

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
    void updateIcon();
    void updateState();
    void updateLayer(bool restack = true);
    //void updateWorkspace();
    void updateLayout();
    void updateExtents();
    void performLayout();

    void updateMwmHints();
    void updateProperties();
    void updateTaskBar();

    enum WindowType {
        wtCombo,
        wtDesktop,
        wtDialog,
        wtDND,
        wtDock,
        wtDropdownMenu,
        wtMenu,
        wtNormal,
        wtNotification,
        wtPopupMenu,
        wtSplash,
        wtToolbar,
        wtTooltip,
        wtUtility
    };

    void setWindowType(enum WindowType winType) { fWindowType = winType; }
    bool isTypeDock(void) { return (fWindowType == wtDock); }

    int getWorkspace() const { return fWinWorkspace; }
    int getTrayOrder() const { return fTrayOrder; }
    void setWorkspace(int workspace);
    void setWorkspaceHint(long workspace);
    long getActiveLayer() const { return fWinActiveLayer; }
    void setRequestedLayer(long layer);
    long getTrayOption() const { return fWinTrayOption; }
    void setTrayOption(long option);
    void setDoNotCover(bool flag);
    bool isMaximized() const { return (getState() &
                                 (WinStateMaximizedHoriz |
                                  WinStateMaximizedVert)) ? true : false; }
    bool isMaximizedVert() const { return (getState() & WinStateMaximizedVert) ? true : false; }
    bool isMaximizedHoriz() const { return (getState() & WinStateMaximizedHoriz) ? true : false; }
    bool isMaximizedFully() const { return isMaximizedVert() && isMaximizedHoriz(); }
    bool isMinimized() const { return (getState() & WinStateMinimized) ? true : false; }
    bool isHidden() const { return (getState() & WinStateHidden) ? true : false; }
    bool isSkipPager() const { return (getState() & WinStateSkipPager) ? true : false; }
    bool isSkipTaskBar() const { return (getState() & WinStateSkipTaskBar) ? true : false; }
    bool isRollup() const { return (getState() & WinStateRollup) ? true : false; }
    bool isSticky() const { return (getState() & WinStateSticky) ? true : false; }
    bool isAllWorkspaces() const { return (getWorkspace() == -1) ? true : false; }
    //bool isHidWorkspace() { return (getState() & WinStateHidWorkspace) ? true : false; }
    //bool isHidTransient() { return (getState() & WinStateHidTransient) ? true : false; }

    bool wasMinimized() const { return (getState() & WinStateWasMinimized) ? true : false; }
    bool wasHidden() const { return (getState() & WinStateWasHidden) ? true : false; }

    bool isIconic() const { return isMinimized() && fMiniIcon; }

    MiniIcon *getMiniIcon();

    bool isManaged() const { return fManaged; }
    void setManaged(bool isManaged) { fManaged = isManaged; }

    void setAllWorkspaces(void);

    bool visibleOn(int workspace) const {
        return (isAllWorkspaces() || getWorkspace() == workspace);
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

    virtual ref<YIcon> getIcon() const { return clientIcon(); }

    virtual ustring getTitle() const { return client()->windowTitle(); }
    virtual ustring getIconTitle() const { return client()->iconTitle(); }

    void updateNetWMStrut();
    void updateNetWMStrutPartial();
    void updateNetStartupId();
    void updateNetWMUserTime();
    void updateNetWMUserTimeWindow();
    void updateNetWMWindowOpacity();
    void updateNetWMFullscreenMonitors(int, int, int, int);

    int strutLeft() { return fStrutLeft; }
    int strutRight() { return fStrutRight; }
    int strutTop() { return fStrutTop; }
    int strutBottom() { return fStrutBottom; }

    void updateUrgency();
    void setWmUrgency(bool wmUrgency);
    bool isUrgent() { return fWmUrgency || fClientUrgency; }

    int getScreen();

    long getOldLayer() { return fOldLayer; }
    void saveOldLayer() { fOldLayer = fWinActiveLayer; }

    bool hasIndicators() const { return indicatorsCreated; }
    Window topSideIndicator() const { return topSide; }
    Window topLeftIndicator() const { return topLeft; }
    Window topRightIndicator() const { return topRight; }

private:
    /*typedef enum {
        fsMinimized       = 1 << 0,
        fsMaximized       = 1 << 1,
        fsRolledup        = 1 << 2,
        fsHidden          = 1 << 3,
        fsWorkspaceHidden = 1 << 4
    } FrameStateFlags;*/

    bool fManaged;
    bool fFocused;
    unsigned fFrameFunctions;
    unsigned fFrameDecors;
    unsigned fFrameOptions;

    int normalX, normalY, normalW, normalH;
    int posX, posY, posW, posH;
    int extentLeft, extentRight, extentTop, extentBottom;

    int iconX, iconY;

    YFrameClient *fClient;
    YClientContainer *fClientContainer;
    YFrameTitleBar *fTitleBar;

    YPopupWindow *fPopupActive;

    int buttonDownX, buttonDownY;
    int grabX, grabY;
    bool movingWindow, sizingWindow;
    int origX, origY, origW, origH;

    Window topSide, leftSide, rightSide, bottomSide;
    Window topLeft, topRight, bottomLeft, bottomRight;
    Window topLeftSide, topRightSide;
    bool indicatorsCreated;
    bool indicatorsVisible;

    TaskBarApp *fTaskBarApp;
    TrayApp *fTrayApp;
    MiniIcon *fMiniIcon;
    WindowListItem *fWinListItem;
    ref<YIcon> fFrameIcon;

    YFrameWindow *fOwner;
    YFrameWindow *fTransient;
    YFrameWindow *fNextTransient;
    YActionListener *wmActionListener;

    static lazy<YTimer> fAutoRaiseTimer;
    static lazy<YTimer> fDelayFocusTimer;

    int fWinWorkspace;
    long fWinRequestedLayer;
    long fWinActiveLayer;
    long fWinTrayOption;
    long fWinState;
    long fWinOptionMask;
    long fOldLayer;
    int fTrayOrder;

    int fFullscreenMonitorsTop;
    int fFullscreenMonitorsBottom;
    int fFullscreenMonitorsLeft;
    int fFullscreenMonitorsRight;

    YMsgBox *fKillMsgBox;

    // _NET_WM_STRUT support
    int fStrutLeft;
    int fStrutRight;
    int fStrutTop;
    int fStrutBottom;

    // _NET_WM_USER_TIME support
    UserTime fUserTime;
    Window fUserTimeWindow;

    unsigned fShapeWidth;
    unsigned fShapeHeight;
    int fShapeTitleY;
    int fShapeBorderX;
    int fShapeBorderY;

    bool fWmUrgency;
    bool fClientUrgency;

    enum WindowType fWindowType;

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
    }
    friend class MiniIcon;
};

#endif


// vim: set sw=4 ts=4 et:
