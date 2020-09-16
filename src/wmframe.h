#ifndef WMFRAME_H
#define WMFRAME_H

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
    private YTimerListener,
    private YPopDownListener,
    private YMsgBoxListener,
    public ClientData,
    public YLayeredNode,
    public YCreatedNode,
    public YFocusedNode
{
public:
    YFrameWindow(YActionListener *wmActionListener,
                 unsigned depth = CopyFromParent,
                 Visual* visual = nullptr,
                 Colormap clmap = CopyFromParent);
    virtual ~YFrameWindow();

    void doManage(YFrameClient *client, bool &doActivate, bool &requestFocus);
    void afterManage();
    void manage();
    void unmanage(bool reparent = true);
    void sendConfigure();

    Window createPointerWindow(Cursor cursor, int gravity);
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
    virtual void handleMotion(const XMotionEvent &motion);
    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual void handleFocus(const XFocusChangeEvent &focus);
    virtual void handleConfigure(const XConfigureEvent &configure);
    virtual void handleExpose(const XExposeEvent &expose);

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
    static YMsgBox* wmConfirmKill(const mstring& title, YMsgBoxListener *recvr);
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

    void drawMoveSizeFX(int x, int y, int w, int h);
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
    bool canShow() const;
    bool canHide() const;
    bool canLower() const;
    bool canRaise();
    bool canFullscreen() const;
    bool overlaps(bool below);
    unsigned overlap(YFrameWindow *other);

    void insertFrame(bool top);
    void removeFrame();
    bool setAbove(YFrameWindow *aboveFrame);
    bool setBelow(YFrameWindow *belowFrame);

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
    virtual void updateSubmenus();

    virtual void raise();
    virtual void lower();

    void popupSystemMenu(YWindow *owner, int x, int y,
                         unsigned int flags,
                         YWindow *forWindow = nullptr);
    virtual void popupSystemMenu(YWindow *owner);
    virtual void handlePopDown(YPopupWindow *popup);

    virtual void configure(const YRect2& r);

    void getNewPos(const XConfigureRequestEvent &cr,
                   int &cx, int &cy, int &cw, int &ch);
    void configureClient(const XConfigureRequestEvent &configureRequest);
    void configureClient(int cx, int cy, int cwidth, int cheight);

    void setShape();

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
        foAllWorkspaces            = (1 << 0),  // WinStateSticky
        foMinimized                = (1 << 1),  // WinStateMinimized
        foMaximizedVert            = (1 << 2),  // WinStateMaximizedVert
        foMaximizedHorz            = (1 << 3),  // WinStateMaximizedHoriz
        foMaximizedBoth            = (3 << 2),  // WinStateMaximizedBoth
        foAppTakesFocus            = (1 << 4),
        foDoNotCover               = (1 << 5),
        foDoNotFocus               = (1 << 6),
        foForcedClose              = (1 << 7),
        foFullKeys                 = (1 << 8),
        foFullscreen               = (1 << 9),
        foIgnoreNoFocusHint        = (1 << 10),
        foIgnorePagerPreview       = (1 << 11),
        foIgnorePosition           = (1 << 12),
        foIgnoreQSwitch            = (1 << 13),
        foIgnoreTaskBar            = (1 << 14),
        foIgnoreUrgent             = (1 << 15),
        foIgnoreWinList            = (1 << 16),
        foIgnoreActivationMessages = (1 << 17),
        foNoFocusOnAppRaise        = (1 << 18),
        foNoFocusOnMap             = (1 << 19),
        foNoIgnoreTaskBar          = (1 << 20),
        foNonICCCMConfigureRequest = (1 << 21),
        foClose                    = (1 << 22),
    };

    unsigned frameFunctions() const { return fFrameFunctions; }
    unsigned frameDecors() const { return fFrameDecors; }
    unsigned frameOptions() const { return fFrameOptions; }
    bool frameOption(YFrameOptions o) const { return hasbit(fFrameOptions, o); }
    void updateAllowed();
    void updateNetWMState();
    void getFrameHints();
    bool haveHintOption() const { return fHintOption; }
    WindowOption& getHintOption() { return *fHintOption; }
    WindowOption getWindowOption();
    void getWindowOptions(WindowOptions* list, WindowOption& opt, bool remove);

    YMenu *windowMenu();

    long getState() const { return fWinState; }
    void setState(long mask, long state);
    bool hasState(long bit) const { return hasbit(fWinState, bit); }
    bool notState(long bit) const { return !hasbit(fWinState, bit); }
    long oldState() const { return fOldState; }

    bool isFullscreen() const { return hasState(WinStateFullscreen); }

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

    bool addAsTransient();
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

    void getNormalGeometryInner(int *x, int *y, int *w, int *h) const;
    void setNormalGeometryOuter(int x, int y, int w, int h);
    void setNormalPositionOuter(int x, int y);
    void setNormalGeometryInner(int x, int y, int w, int h);
    void updateDerivedSize(long flagmask);

    void setCurrentGeometryOuter(YRect newSize);
    void setCurrentPositionOuter(int x, int y);
    void limitOuterPosition();
    void updateNormalSize();

    void updateTitle();
    void updateIconTitle();
    void updateIcon();
    void updateState();
    void updateLayer(bool restack = true);
    void updateIconPosition();
    void updateLayout();
    void updateExtents();
    void performLayout();

    void updateMwmHints();
    void updateProperties();
    void updateTaskBar();
    void updateAppStatus();

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
    bool isTypeDock() { return (fWindowType == wtDock); }

    int getWorkspace() const { return fWinWorkspace; }
    int getTrayOrder() const { return fTrayOrder; }
    void setWorkspace(int workspace);
    void setWorkspaceHint(long workspace);
    long getActiveLayer() const { return fWinActiveLayer; }
    void setRequestedLayer(long layer);
    long getRequestedLayer() const { return fWinRequestedLayer; }
    long getTrayOption() const { return fWinTrayOption; }
    void setTrayOption(long option);
    void setDoNotCover(bool flag);
    bool isMaximized() const { return hasState(WinStateMaximizedBoth); }
    bool isMaximizedVert() const { return hasState(WinStateMaximizedVert); }
    bool isMaximizedHoriz() const { return hasState(WinStateMaximizedHoriz); }
    bool isMaximizedFully() const { return isMaximizedVert() && isMaximizedHoriz(); }
    bool isMinimized() const { return hasState(WinStateMinimized); }
    bool isHidden() const { return hasState(WinStateHidden); }
    bool isSkipPager() const { return hasState(WinStateSkipPager); }
    bool isSkipTaskBar() const { return hasState(WinStateSkipTaskBar); }
    bool isRollup() const { return hasState(WinStateRollup); }
    bool isSticky() const { return hasState(WinStateSticky); }
    bool isAllWorkspaces() const { return (getWorkspace() == AllWorkspaces); }
    //bool isHidWorkspace() { return hasState(WinStateHidWorkspace); }
    //bool isHidTransient() { return hasState(WinStateHidTransient); }

    bool wasMinimized() const { return hasState(WinStateWasMinimized); }
    bool wasHidden() const { return hasState(WinStateWasHidden); }

    bool isIconic() const { return isMinimized() && fMiniIcon; }
    bool hasMiniIcon() const { return fMiniIcon != nullptr; }
    MiniIcon *getMiniIcon();

    bool isManaged() const { return fManaged; }
    void setManaged(bool isManaged) { fManaged = isManaged; }

    void setAllWorkspaces();

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

    bool doNotCover() const { return frameOption(foDoNotCover); }

    virtual ref<YIcon> getIcon() const { return clientIcon(); }

    virtual mstring getTitle() const { return client()->windowTitle(); }
    virtual mstring getIconTitle() const { return client()->iconTitle(); }

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
    bool haveStruts() const { return fHaveStruts; }

    void updateUrgency();
    void setWmUrgency(bool wmUrgency);
    bool isUrgent() { return fWmUrgency || fClientUrgency; }

    int getScreen() const;
    void refresh();

    long getOldLayer() { return fOldLayer; }
    void saveOldLayer() { fOldLayer = fWinActiveLayer; }

    bool hasIndicators() const { return indicatorsCreated; }
    Window topSideIndicator() const { return topSide; }
    Window topLeftIndicator() const { return topLeft; }
    Window topRightIndicator() const { return topRight; }
    Time since() const { return fStartManaged; }
    bool startMinimized() const { return frameOption(foMinimized); }

    void addToWindowList();
    void removeFromWindowList();

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
    bool indicatorsCreated;
    bool indicatorsVisible;

    TaskBarApp *fTaskBarApp;
    TrayApp *fTrayApp;
    MiniIcon *fMiniIcon;
    WindowListItem *fWinListItem;
    ref<YIcon> fFrameIcon;
    lazy<WindowOption> fHintOption;

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
    long fOldState;
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
    Time fStartManaged;

    unsigned fShapeWidth;
    unsigned fShapeHeight;
    int fShapeTitleY;
    int fShapeBorderX;
    int fShapeBorderY;
    unsigned fShapeDecors;
    mstring fShapeTitle;

    bool fHaveStruts;
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

    void repaint();
    void setGeometry(const YRect &) = delete;
    void setPosition(int, int) = delete;
    void setSize(int, int) = delete;
    void setWindowGeometry(const YRect &r) {
        YWindow::setGeometry(r);
    }
};

#endif


// vim: set sw=4 ts=4 et:
