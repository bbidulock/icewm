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
#include "prefs.h"
#include "WinMgr.h"
#include "wmmgr.h"

class YClientContainer;
class MiniIcon;
class TaskBarApp;
class TrayApp;
class YFrameTitleBar;

class YFrameWindow: public YWindow, public YActionListener, public YTimerListener, public PopDownListener, public YMsgBoxListener, public ClientData {
public:
    YFrameWindow(YWindow *parent, YFrameClient *client);
    virtual ~YFrameWindow();

    void manage(YFrameClient *client);
    void unmanage(bool reparent = true);
    void sendConfigure();

    void createPointerWindows();
    void grabKeys();

    void focus(bool canWarp = false);
    void activate(bool canWarp = false);

    void activateWindow(bool raise) {
        if (raise) wmRaise();
        activate(true);
    }

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);

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
#endif
    void wmToggleDoNotCover();

    void minimizeTransients();
    void restoreMinimizedTransients();
    void hideTransients();
    void restoreHiddenTransients();

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

    void popupSystemMenu(int x, int y,
                         int x_delta, int y_delta,
                         unsigned int flags,
                         YWindow *forWindow = 0);
    virtual void popupSystemMenu(void);
    virtual void handlePopDown(YPopupWindow *popup);

    virtual void configure(const int x, const int y, 
    			   const unsigned width, const unsigned height,
			   const bool resized);
    
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

    enum YFrameOptions {
        foAllWorkspaces		= (1 << 0),
        foIgnoreTaskBar		= (1 << 1),
        foIgnoreWinList		= (1 << 2),
        foFullKeys		= (1 << 3),
        foIgnoreQSwitch		= (1 << 4),
        foNoFocusOnAppRaise	= (1 << 5),
        foIgnoreNoFocusHint	= (1 << 6),
        foIgnorePosition	= (1 << 7),
        foDoNotCover		= (1 << 8)
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

    unsigned int borderX() const {
        return
            (frameDecors() & fdBorder) ?
            ((frameDecors() & fdResize) ? wsBorderX : wsDlgBorderX) : 0;
    }
    unsigned int borderY() const {
        return
            (frameDecors() & fdBorder) ?
            ((frameDecors() & fdResize) ? wsBorderY : wsDlgBorderY) : 0;
    }
    unsigned int titleY() const {
        return (frameDecors() & fdTitleBar) ? wsTitleBar : 0;
    }
    
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

#ifndef LITE
    YIcon *getClientIcon() const { return fFrameIcon; }
    YIcon *clientIcon() const;
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
#ifdef CONFIG_TASKBAR
    void updateTaskBar();
#endif

    long getWorkspace() const { return fWinWorkspace; }
    void setWorkspace(long workspace);
    long getLayer() const { return fWinLayer; }
    void setLayer(long layer);
#ifdef CONFIG_TRAY
    long getTrayOption() const { return fWinTrayOption; }
    void setTrayOption(long option);
#endif
    void setDoNotCover(bool flag);
    long getState() const { return fWinState; }
    void setState(long mask, long state);

    bool isMaximized() const { return (getState() & (WinStateMaximizedHoriz |
                                                     WinStateMaximizedVert)); }
    bool isMaximizedVert() const { return (getState() & WinStateMaximizedVert); }
    bool isMaximizedHoriz() const { return (getState() & WinStateMaximizedHoriz); }
    bool isMaximizedFully() const { return isMaximizedVert() && isMaximizedHoriz(); }
    bool isMinimized() const { return (getState() & WinStateMinimized); }
    bool isHidden() const { return (getState() & WinStateHidden); }
    bool isRollup() const { return (getState() & WinStateRollup); }
    bool isSticky() const { return (getState() & WinStateAllWorkspaces); }
    //bool isHidWorkspace() { return (getState() & WinStateHidWorkspace); }
    //bool isHidTransient() { return (getState() & WinStateHidTransient); }

    bool wasMinimized() const { return (getState() & WinStateWasMinimized); }
    bool wasHidden() const { return (getState() & WinStateWasHidden); }

    bool isIconic() const { return isMinimized() && minimizeToDesktop && fMiniIcon; }

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
    bool isFocusable();

    bool doNotCover() const { return limitByDockLayer
    			      ? getLayer() == WinLayerDock
    			      : frameOptions() & foDoNotCover; }

#ifndef LITE
    virtual YIcon *getIcon() const { return clientIcon(); }
#endif

    virtual const char *getTitle() const { return client()->windowTitle(); }
    virtual const char *getIconTitle() const { return client()->iconTitle(); }

    YFrameButton *getButton(char c);
    void positionButton(YFrameButton *b, int &xPos, bool onRight);
    bool isButton(char c);
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
    YIcon *fFrameIcon;

    YFrameWindow *fOwner;
    YFrameWindow *fTransient;
    YFrameWindow *fNextTransient;

    static YTimer *fAutoRaiseTimer;
    static YTimer *fDelayFocusTimer;

    long fWinWorkspace;
    long fWinLayer;
#ifdef CONFIG_TRAY
    long fWinTrayOption;
#endif
    long fWinState;
    long fWinStateMask;
    bool fManaged;
    long fWinOptionMask;

    YMsgBox *fKillMsgBox;
};

//!!! remove this
#ifdef CONFIG_LOOK_PIXMAP
extern YPixmap *frameTL[2][2];
extern YPixmap *frameT[2][2];
extern YPixmap *frameTR[2][2];
extern YPixmap *frameL[2][2];
extern YPixmap *frameR[2][2];
extern YPixmap *frameBL[2][2];
extern YPixmap *frameB[2][2];
extern YPixmap *frameBR[2][2];

extern YPixmap *titleJ[2];
extern YPixmap *titleL[2];
extern YPixmap *titleS[2];
extern YPixmap *titleP[2];
extern YPixmap *titleT[2];
extern YPixmap *titleM[2];
extern YPixmap *titleB[2];
extern YPixmap *titleR[2];
extern YPixmap *titleQ[2];

extern YPixmap *menuButton[2];

#ifdef CONFIG_GRADIENTS
class YPixbuf;

extern YPixbuf *rgbFrameT[2][2];
extern YPixbuf *rgbFrameL[2][2];
extern YPixbuf *rgbFrameR[2][2];
extern YPixbuf *rgbFrameB[2][2];
extern YPixbuf *rgbTitleS[2];
extern YPixbuf *rgbTitleT[2];
extern YPixbuf *rgbTitleB[2];
#endif

#endif

#endif
