#ifndef WMFRAME_H
#define WMFRAME_H

#include "ymsgbox.h"
#include "yicon.h"
#include "ylist.h"
#include "WinMgr.h"
#include "workspaces.h"

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

    YClientContainer* allocateContainer(YFrameClient* client);
    void doManage(YFrameClient *client, bool &doActivate, bool &requestFocus);
    void afterManage();
    void manage(YFrameClient* client, YClientContainer* conter);
    void unmanage();
    void sendConfigure();

    void untab(YFrameClient* dest);
    bool hasTab(YFrameClient* dest);
    bool lessTabs();
    bool moreTabs();
    void closeTab(YFrameClient* client);
    void removeTab(YFrameClient* client);
    void selectTab(YFrameClient* client);
    void changeTab(int delta);
    void createTab(YFrameClient* client, int place = -1);
    void mergeTabs(YFrameWindow* frame);
    void independer(YFrameClient* client);

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
    virtual bool handleBeginDrag(const XButtonEvent &down, const XMotionEvent &motion);
    virtual void handleMotion(const XMotionEvent &motion);
    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual void handleFocus(const XFocusChangeEvent &focus);
    virtual void handleConfigure(const XConfigureEvent &configure);
    virtual void handleExpose(const XExposeEvent &expose);

    virtual bool handleTimer(YTimer *t);

    virtual void actionPerformed(YAction action, unsigned modifiers = 0);
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
    void wmCloseClient(YFrameClient* client, bool* confirm);
    void wmConfirmKill(const char* message = nullptr);
    void wmKill();
    void wmNextWindow();
    void wmPrevWindow();
    void wmMove();
    void wmSize();
    void wmOccupyAllOrCurrent();
    void wmOccupyWorkspace(int workspace);
    void wmSetLayer(int layer);
    void wmSetTrayOption(int option);
    void wmTile(YAction action);
    void wmToggleTray();
#if DO_NOT_COVER_OLD
    void wmToggleDoNotCover();
#endif
    void wmToggleFullscreen();

    void minimizeTransients();
    void restoreMinimizedTransients();
    void hideTransients();
    void restoreHiddenTransients();

    void doMaximize(int flags);

    void loseWinFocus();
    void setWinFocus();
    bool focused() const { return fFocused; }
    void updateFocusOnMap(bool &doActivate);

    YFrameClient *client() const { return fClient; }
    YFrameTitleBar *titlebar();
    YClientContainer *container() const { return fContainer; }

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
    void checkEdgeSwitch(int mouseX, int mouseY);
    void outlineMove();
    void outlineResize();

    void constrainPositionByModifier(int &x, int &y, const XMotionEvent &motion);
    void constrainMouseToWorkspace(int &x, int &y);

    void getDefaultOptions(bool &doActivate);

    bool canSize(bool horiz = true, bool vert = true);
    bool canMove() const { return hasbit(frameFunctions(), ffMove); }
    bool canClose() const { return hasbit(frameFunctions(), ffClose); }
    bool canMaximize() const { return hasbit(frameFunctions(), ffMaximize); }
    bool canMinimize() const { return hasbit(frameFunctions(), ffMinimize); }
    bool canRestore() const;
    bool canRollup() const { return (frameFunctions() & ffRollup) && titleY(); }
    bool canShow() const;
    bool canHide() const { return hasbit(frameFunctions(), ffHide); }
    bool canLower() const;
    bool canRaise(bool ignoreTaskBar = false) const;
    bool canFullscreen() const;
    bool overlaps(bool below);
    unsigned overlap(YFrameWindow *other);

    void insertFrame(bool top);
    void removeFrame();
    bool setAbove(YFrameWindow *aboveFrame);
    bool setBelow(YFrameWindow *belowFrame);
    bool isBefore(YFrameWindow* afterFrame);

    enum FindWindowFlags {
        fwfVisible    = 1 << 0, // visible windows only
        fwfCycle      = 1 << 1, // cycle when bottom(top) reached
        fwfBackward   = 1 << 2, // go up in zorder (default=down)
        fwfNext       = 1 << 3, // start from next window
        fwfFocusable  = 1 << 4, // only focusable windows
        fwfWorkspace  = 1 << 5, // current workspace only
        fwfSame       = 1 << 6, // return same if no match and same matches
        fwfLayers     = 1 << 7, // find windows in other layers
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
    void netRestackWindow(Window window, int detail);

    void setShape();

    enum YFrameFunctions {
        ffMove          = (1 << 0),
        ffResize        = (1 << 1),
        ffClose         = (1 << 2),
        ffMinimize      = (1 << 3),
        ffMaximize      = (1 << 4),
        ffRollup        = (1 << 5),
        ffHide          = (1 << 6),
    };

    enum YFrameDecors {
        fdTitleBar      = (1 << 0),
        fdResize        = (1 << 1),
        fdClose         = (1 << 2),
        fdMinimize      = (1 << 3),
        fdMaximize      = (1 << 4),
        fdRollup        = (1 << 5),
        fdHide          = (1 << 6),
        fdDepth         = (1 << 7),
        fdBorder        = (1 << 8),
        fdSysMenu       = (1 << 9),
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
        foClose                    = (1 << 22),
        foIgnoreOverrideRedirect   = (1 << 23),
    };

    unsigned frameFunctions() const { return fFrameFunctions; }
    unsigned frameDecors() const { return fFrameDecors; }
    unsigned frameOptions() const { return fFrameOptions; }
    bool frameOption(YFrameOptions o) const { return hasbit(fFrameOptions, o); }
    void updateAllowed();
    void getFrameHints();
    YMenu *windowMenu();

    int getState() const { return fWinState; }
    void setState(int mask, int state);
    bool hasState(int bit) const { return hasbit(fWinState, bit); }
    bool hasStates(int bits) const { return hasbits(fWinState, bits); }
    bool notState(int bit) const { return !hasbit(fWinState, bit); }

    bool isFullscreen() const { return hasState(WinStateFullscreen); }
    bool isResizable() const { return hasbit(frameFunctions(), ffResize); }
    bool isUnmapped() const { return hasState(WinStateUnmapped); }
    bool isMapped() const { return notState(WinStateUnmapped); }
    void makeMapped() { return setState(WinStateUnmapped, None); }
    bool hasBorders() const;
    bool hasTitleBar() const { return fTitleBar; }
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

    bool addAsTransient();
    void addTransients();
    void removeFromGroupModals();

    YFrameClient* transient() const;
    YFrameWindow* owner() const;
    YFrameWindow* mainOwner();

    void getNormalGeometryInner(int *x, int *y, int *w, int *h) const;
    void setNormalGeometryOuter(int x, int y, int w, int h);
    void setNormalPositionOuter(int x, int y);
    void setNormalGeometryInner(int x, int y, int w, int h);
    void updateDerivedSize(int flagmask);

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
    void performLayout();

    void updateMwmHints(XSizeHints* sh);
    void updateProperties(YFrameClient* client = nullptr);
    void updateTaskBar();
    void updateAppStatus();
    void removeAppStatus();

    void setWindowType(WindowType winType) { fWindowType = winType; }
    bool isTypeDock() { return (fWindowType == wtDock); }

    int getWorkspace() const { return fWinWorkspace; }
    int getTrayOrder() const { return fTrayOrder; }
    void setWorkspace(int workspace);
    int getActiveLayer() const { return fWinActiveLayer; }
    void setRequestedLayer(int layer);
    int getRequestedLayer() const { return fWinRequestedLayer; }
    int getTrayOption() const { return fWinTrayOption; }
    void setTrayOption(int option);
    void setDoNotCover(bool flag);
    unsigned getFrameName() const { return fFrameName; }
    void setFrameName(unsigned name);
    bool isMaximized() const { return hasState(WinStateMaximizedBoth); }
    bool isMaximizedVert() const { return hasState(WinStateMaximizedVert); }
    bool isMaximizedHoriz() const { return hasState(WinStateMaximizedHoriz); }
    bool isMaximizedFully() const { return hasStates(WinStateMaximizedBoth); }
    bool isMinimized() const { return hasState(WinStateMinimized); }
    bool isHidden() const { return hasState(WinStateHidden); }
    bool isSkipPager() const { return hasState(WinStateSkipPager); }
    bool isSkipTaskBar() const { return hasState(WinStateSkipTaskBar); }
    bool isRollup() const { return hasState(WinStateRollup); }
    bool isSticky() const { return hasState(WinStateSticky); }
    bool isAllWorkspaces() const { return fWinWorkspace == AllWorkspaces; }
    bool wasMinimized() const { return hasState(WinStateWasMinimized); }
    bool wasHidden() const { return hasState(WinStateWasHidden); }

    bool isIconic() const { return isMinimized() && fMiniIcon; }
    bool hasMiniIcon() const { return fMiniIcon != nullptr; }
    MiniIcon *getMiniIcon();
    ClassHint* classHint() const { return client()->classHint(); }
    Window clientLeader() const { return client()->clientLeader(); }

    bool isManaged() const { return fManaged; }
    void setManaged(bool isManaged) { fManaged = isManaged; }

    void setAllWorkspaces();

    bool visibleOn(int workspace) const {
        return fWinWorkspace == workspace || fWinWorkspace == AllWorkspaces;
    }
    bool visibleNow() const;

    bool isModal();
    bool hasModal();
    bool canFocus();
    bool canFocusByMouse();
    bool avoidFocus();
    bool getInputFocusHint();

    bool inWorkArea() const;
    bool affectsWorkArea() const;
    bool doNotCover() const { return frameOption(foDoNotCover); }

    virtual ref<YIcon> getIcon() const;
    virtual mstring getTitle() const { return client()->windowTitle(); }
    virtual mstring getIconTitle() const { return client()->iconTitle(); }

    void updateNetWMStrut();
    void updateNetWMStrutPartial();
    void updateNetStartupId();
    void updateNetWMUserTime();
    void updateNetWMUserTimeWindow();
    void updateNetWMWindowOpacity();
    void updateNetWMFullscreenMonitors(int top, int bottom, int left, int right);

    int strutLeft() { return fStrutLeft; }
    int strutRight() { return fStrutRight; }
    int strutTop() { return fStrutTop; }
    int strutBottom() { return fStrutBottom; }
    bool haveStruts() const { return fHaveStruts; }

    void setWmUrgency(bool wmUrgency);
    bool isUrgent() const;
    bool isPassive() const;

    int getScreen() const;
    void refresh();

    int windowTypeLayer() const;
    int tabCount() const { return fTabs.getCount(); }
    bool isEmpty() const { return fTabs.isEmpty(); }

    bool hasIndicators() const { return indicatorsCreated; }
    Window topSideIndicator() const { return topSide; }
    Window topLeftIndicator() const { return topLeft; }
    Window topRightIndicator() const { return topRight; }
    Time since() const { return fStartManaged; }
    bool startMinimized() const { return frameOption(foMinimized); }
    bool ignoreActivation() const { return frameOption(foIgnoreActivationMessages); }

    void addToWindowList();
    void removeFromWindowList();

private:
    bool fManaged;
    bool fFocused;
    unsigned fFrameFunctions;
    unsigned fFrameDecors;
    unsigned fFrameOptions;

    int normalX, normalY, normalW, normalH;
    int posX, posY, posW, posH;

    YFrameClient *fClient;
    YClientContainer *fContainer;
    YFrameTitleBar *fTitleBar;

    YPopupWindow *fPopupActive;

    int buttonDownX, buttonDownY;
    int grabX, grabY;
    bool movingWindow, sizingWindow;
    int origX, origY, origW, origH;

    Window topSide, leftSide, rightSide, bottomSide;
    Window topLeft, topRight, bottomLeft, bottomRight;

    TaskBarApp *fTaskBarApp;
    TrayApp *fTrayApp;
    MiniIcon *fMiniIcon;
    ref<YIcon> fFrameIcon;
    lazy<YTimer> fFocusEventTimer;
    YArray<YFrameClient*> fTabs;
    static YArray<YFrameWindow*> tabbedFrames;
    static YArray<YFrameWindow*> namedFrames;
public:
    typedef YArray<YFrameClient*>::IterType IterType;
    IterType iterator() { return fTabs.iterator(); }
    YArray<YFrameClient*>& clients() { return fTabs; }
    static YArray<YFrameWindow*>& tabbing() { return tabbedFrames; }
    static YArray<YFrameWindow*>& fnaming() { return namedFrames; }
private:

    YMsgBox *fKillMsgBox;
    YActionListener *wmActionListener;

    static lazy<YTimer> fAutoRaiseTimer;
    static lazy<YTimer> fDelayFocusTimer;
    static lazy<YTimer> fEdgeSwitchTimer;

    int fWinWorkspace;
    int fWinRequestedLayer;
    int fWinActiveLayer;
    int fWinTrayOption;
    int fWinState;
    int fWinOptionMask;
    int fTrayOrder;
    unsigned fFrameName;

    int fFullscreenMonitorsTop;
    int fFullscreenMonitorsBottom;
    int fFullscreenMonitorsLeft;
    int fFullscreenMonitorsRight;

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
    int fShapeTabCount;
    unsigned fShapeDecors;
    mstring fShapeTitle;
    bool fShapeLessTabs;
    bool fShapeMoreTabs;

    bool fHaveStruts;
    bool indicatorsCreated;

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

    void repaint();
    void setGeometry(const YRect &) = delete;
    void setPosition(int, int) = delete;
    void setSize(int, int) = delete;
    void setWindowGeometry(const YRect &r) {
        YWindow::setGeometry(r);
    }

    struct GroupModal {
        GroupModal(Window g, YFrameWindow* f) : group(g), frame(f) { }
        Window group;
        YFrameWindow* frame;
        bool operator==(Window window) const { return window == group; }
        operator YFrameWindow*() const { return frame; }
        YFrameWindow* operator->() const { return frame; }
    };
    static YArray<GroupModal> groupModals;
    bool isGroupModalFor(const YFrameWindow* other) const;
    bool isTransientFor(const YFrameWindow* other) const;
};

#endif


// vim: set sw=4 ts=4 et:
