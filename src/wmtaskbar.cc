/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 *
 * TaskBar
 */

#include "config.h"

#include "yfull.h"
#include "ypaint.h"
#include "wmtaskbar.h"
#include "yprefs.h"

#include "wmmgr.h"
#include "wmframe.h"
#include "wmclient.h"
#include "wmconfig.h"
#include "wmaction.h"
#include "wmprog.h"
#include "workspaces.h"
#include "wmwinmenu.h"
#include "wmapp.h"
#include "sysdep.h"
#include "wmwinlist.h"

#include "aaddressbar.h"
#include "aclock.h"
#include "akeyboard.h"
#include "acpustatus.h"
#include "amemstatus.h"
#include "apppstatus.h"
#include "amailbox.h"
#include "objbar.h"
#include "objbutton.h"
#include "atasks.h"
#include "atray.h"
#include "aworkspaces.h"
#include "yxtray.h"
#include "prefs.h"
#include "wpixmaps.h"
#include "aapm.h"

#include "intl.h"

TaskBar *taskBar;

YColorName taskBarBg(&clrDefaultTaskBar);

EdgeTrigger::EdgeTrigger(TaskBar *owner):
    fTaskBar(owner),
    fDoShow(false)
{
    setStyle(wsOverrideRedirect | wsInputOnly);
    setPointer(YXApplication::leftPointer);
    setDND(true);
    setTitle("IceEdge");
    fAutoHideTimer->setTimer(autoShowDelay, this, false);
}

EdgeTrigger::~EdgeTrigger() {
}

void EdgeTrigger::startHide() {
    fDoShow = false;
    fAutoHideTimer->startTimer(autoHideDelay);
}

void EdgeTrigger::stopHide() {
    fDoShow = false;
    fAutoHideTimer->stopTimer();
}

bool EdgeTrigger::enabled() const {
    return (taskBarAutoHide | (taskBarFullscreenAutoShow & !taskBarKeepBelow));
}

void EdgeTrigger::show() {
    if (enabled()) {
        YWindow::show();
    }
}

void EdgeTrigger::handleCrossing(const XCrossingEvent &crossing) {
    if (crossing.type == EnterNotify /* && crossing.mode != NotifyNormal */) {
        unsigned long last = YWindow::getLastEnterNotifySerial();
        if (crossing.serial != last && crossing.serial != last + 1) {
            MSG(("enter notify %d %d", crossing.mode, crossing.detail));
            fDoShow = true;
            fAutoHideTimer->startTimer(autoShowDelay);
        }
    } else if (crossing.type == LeaveNotify /* && crossing.mode != NotifyNormal */) {
        fDoShow = false;
        MSG(("leave notify"));
        fAutoHideTimer->stopTimer();
    }
}

void EdgeTrigger::handleDNDEnter() {
    fDoShow = true;
    fAutoHideTimer->startTimer(autoShowDelay);
}

void EdgeTrigger::handleDNDLeave() {
    fDoShow = false;
    fAutoHideTimer->startTimer(autoHideDelay);
}


bool EdgeTrigger::handleTimer(YTimer *t) {
    MSG(("taskbar handle timer"));
    return fTaskBar->autoTimer(fDoShow);
}

TaskBar::TaskBar(IApp *app, YWindow *aParent, YActionListener *wmActionListener, YSMListener *smActionListener):
    YFrameClient(aParent, nullptr),
    fSurface(taskBarBg, taskbackPixmap, taskbackPixbuf),
    fTasks(nullptr),
    fCollapseButton(nullptr),
    fWindowTray(nullptr),
    fKeyboardStatus(nullptr),
    fMailBoxControl(nullptr),
    fMEMStatus(nullptr),
    fCPUStatus(nullptr),
    fApm(nullptr),
    fNetStatus(nullptr),
    fObjectBar(nullptr),
    fApplications(nullptr),
    fWinList(nullptr),
    fShowDesktop(nullptr),
    fAddressBar(nullptr),
    fWorkspaces(nullptr),
    fDesktopTray(nullptr),
    fEdgeTrigger(nullptr),
    wmActionListener(wmActionListener),
    smActionListener(smActionListener),
    app(app),
    fIsHidden(taskBarAutoHide),
    fFullscreen(false),
    fIsCollapsed(false),
    fIsMapped(false),
    fMenuShown(false),
    fNeedRelayout(false),
    fButtonUpdate(false)
{
    taskBar = this;

    addStyle(wsNoExpose);
    setWinHintsHint(WinHintsSkipFocus |
                    WinHintsSkipWindowMenu |
                    WinHintsSkipTaskBar);

    setWinWorkspaceHint(AllWorkspaces);
    updateWinLayer();
    Atom protocols[2] = {
      _XA_WM_DELETE_WINDOW,
      _XA_WM_TAKE_FOCUS
      //_NET_WM_PING,
      //_NET_WM_SYNC_REQUEST,
    };
    XSetWMProtocols(xapp->display(), handle(), protocols, 2);
    XWMHints wmhints = { InputHint, False, };
    ClassHint clhint("icewm", "TaskBar");
    YTextProperty text("TaskBar");
    XSetWMProperties(xapp->display(), handle(), &text, &text,
                     nullptr, 0, nullptr, &wmhints, &clhint);
    setNetPid();

    setMwmHints(MwmHints(
       MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS,
       MWM_FUNC_MOVE));
    setFrameState(NormalState);
    setPointer(YXApplication::leftPointer);
    setDND(true);

    fEdgeTrigger = new EdgeTrigger(this);

    initApplets();

    getPropertiesList();
    getWMHints();
    getClassHint();
    fIsMapped = true;

    MSG(("taskbar"));
}

TaskBar::~TaskBar() {
    detachDesktopTray();
    delete fEdgeTrigger; fEdgeTrigger = nullptr;
    delete fClock; fClock = nullptr;
    delete fKeyboardStatus; fKeyboardStatus = nullptr;
    delete fMailBoxControl; fMailBoxControl = nullptr;
#ifdef MEM_STATES
    delete fMEMStatus; fMEMStatus = nullptr;
#endif
    delete fWinList; fWinList = nullptr;
    delete fApplications; fApplications = nullptr;
    delete fObjectBar; fObjectBar = nullptr;
    delete fWorkspaces; fWorkspaces = nullptr;
#ifdef MAX_ACPI_BATTERY_NUM
    delete fApm; fApm = nullptr;
#endif
#ifdef IWM_STATES
    delete fCPUStatus; fCPUStatus = nullptr;
#endif
    delete fNetStatus; fNetStatus = nullptr;
    delete fAddressBar; fAddressBar = nullptr;
    delete fTasks; fTasks = nullptr;
    delete fWindowTray; fWindowTray = nullptr;
    delete fCollapseButton; fCollapseButton = nullptr;
    delete fShowDesktop; fShowDesktop = nullptr;
    xapp->dropClipboard();
    taskBar = nullptr;
    if (getFrame())
        getFrame()->unmanage(false);
    MSG(("taskBar delete"));
}

class TaskBarMenu : public YMenu {
public:
    void updatePopup() {
        if (0 < itemCount())
            return;

        setActionListener(taskBar);
        // TRANSLATORS: This appears in a group with others items, so please make the hotkeys unique in the set: # T_ile Horizontally, Ca_scade, _Arrange, _Minimize All, _Hide All, _Undo, Arrange _Icons, _Windows, _Refresh, _About, _Logout
        addItem(_("Tile _Vertically"), -2, KEY_NAME(gKeySysTileVertical), actionTileVertical);
        // TRANSLATORS: This appears in a group with others items, so please make the hotkeys unique in the set: # T_ile Horizontally, Ca_scade, _Arrange, _Minimize All, _Hide All, _Undo, Arrange _Icons, _Windows, _Refresh, _About, _Logout
        addItem(_("T_ile Horizontally"), -2, KEY_NAME(gKeySysTileHorizontal), actionTileHorizontal);
        // TRANSLATORS: This appears in a group with others items, so please make the hotkeys unique in the set: # T_ile Horizontally, Ca_scade, _Arrange, _Minimize All, _Hide All, _Undo, Arrange _Icons, _Windows, _Refresh, _About, _Logout
        addItem(_("Ca_scade"), -2, KEY_NAME(gKeySysCascade), actionCascade);
        // TRANSLATORS: This appears in a group with others items, so please make the hotkeys unique in the set: # T_ile Horizontally, Ca_scade, _Arrange, _Minimize All, _Hide All, _Undo, Arrange _Icons, _Windows, _Refresh, _About, _Logout
        addItem(_("_Arrange"), -2, KEY_NAME(gKeySysArrange), actionArrange);
        // TRANSLATORS: This appears in a group with others items, so please make the hotkeys unique in the set: # T_ile Horizontally, Ca_scade, _Arrange, _Minimize All, _Hide All, _Undo, Arrange _Icons, _Windows, _Refresh, _About, _Logout
        addItem(_("_Minimize All"), -2, KEY_NAME(gKeySysMinimizeAll), actionMinimizeAll);
        // TRANSLATORS: This appears in a group with others items, so please make the hotkeys unique in the set: # T_ile Horizontally, Ca_scade, _Arrange, _Minimize All, _Hide All, _Undo, Arrange _Icons, _Windows, _Refresh, _About, _Logout
        addItem(_("_Hide All"), -2, KEY_NAME(gKeySysHideAll), actionHideAll);
        // TRANSLATORS: This appears in a group with others items, so please make the hotkeys unique in the set: # T_ile Horizontally, Ca_scade, _Arrange, _Minimize All, _Hide All, _Undo, Arrange _Icons, _Windows, _Refresh, _About, _Logout
        addItem(_("_Undo"), -2, KEY_NAME(gKeySysUndoArrange), actionUndoArrange);
        if (minimizeToDesktop)
        // TRANSLATORS: This appears in a group with others items, so please make the hotkeys unique in the set: # T_ile Horizontally, Ca_scade, _Arrange, _Minimize All, _Hide All, _Undo, Arrange _Icons, _Windows, _Refresh, _About, _Logout
            addItem(_("Arrange _Icons"), -2, KEY_NAME(gKeySysArrangeIcons), actionArrangeIcons);
        addSeparator();
        // TRANSLATORS: This appears in a group with others items, so please make the hotkeys unique in the set: # T_ile Horizontally, Ca_scade, _Arrange, _Minimize All, _Hide All, _Undo, Arrange _Icons, _Windows, _Refresh, _About, _Logout
        addItem(_("_Windows"), -2, actionWindowList, windowListMenu);
        addSeparator();
        // TRANSLATORS: This appears in a group with others items, so please make the hotkeys unique in the set: # T_ile Horizontally, Ca_scade, _Arrange, _Minimize All, _Hide All, _Undo, Arrange _Icons, _Windows, _Refresh, _About, _Logout
        addItem(_("_Refresh"), -2, null, actionRefresh);
        // TRANSLATORS: This appears in a group with others items, so please make the hotkeys unique in the set: # T_ile Horizontally, Ca_scade, _Arrange, _Minimize All, _Hide All, _Undo, Arrange _Icons, _Windows, _Refresh, _About, _Logout
        addItem(_("_About"), -2, actionAbout, nullptr);
        if (showLogoutMenu) {
            addSeparator();
            if (showLogoutSubMenu)
        // TRANSLATORS: This appears in a group with others items, so please make the hotkeys unique in the set: # T_ile Horizontally, Ca_scade, _Arrange, _Minimize All, _Hide All, _Undo, Arrange _Icons, _Windows, _Refresh, _About, _Logout
                addItem(_("_Logout..."), -2, actionLogout, logoutMenu);
            else
                addItem(_("_Logout..."), -2, null, actionLogout);
        }

        setTitle("IceMenu");
        setClassHint("icemenu", "TaskBar");
        setNetWindowType(_XA_NET_WM_WINDOW_TYPE_POPUP_MENU);
    }
};

void TaskBar::initApplets() {
#ifdef MEM_STATES
    if (taskBarShowMEMStatus)
        fMEMStatus = new MEMStatus(this, this);
    else
        fMEMStatus = nullptr;
#endif

#ifdef IWM_STATES
    if (taskBarShowCPUStatus)
        fCPUStatus = new CPUStatusControl(smActionListener, this, this);
    else
        fCPUStatus = nullptr;
#endif

    if (taskBarShowNetStatus)
        fNetStatus = new NetStatusControl(app, smActionListener, this, this);
    else
        fNetStatus = nullptr;

    if (taskBarShowClock)
        fClock = new YClock(smActionListener, this, this);
    else
        fClock = nullptr;

#ifdef MAX_ACPI_BATTERY_NUM
    if (taskBarShowApm && (access(APMDEV, 0) == 0 ||
                           access("/sys/class/power_supply", 0) == 0 ||
                           access("/proc/acpi", 0) == 0 ||
                           access("/dev/acpi", 0) == 0 ||
                           access("/proc/pmu", R_OK|X_OK) == 0))
    {
        fApm = new YApm(this);
        fApm->setTitle("IceAPM");
    }
    else if (!taskBarShowApm && taskBarShowApmAuto)
    {
        fApm = new YApm(this, true);
        if ( ! fApm->hasBatteries()) {
                delete fApm;
                fApm = nullptr;
        }
        else fApm->setTitle("IceAPM");
    }
    else
        fApm = nullptr;
#endif

    if (taskBarShowCollapseButton) {
        fCollapseButton = new ObjectButton(this, actionCollapseTaskbar);
        if (fCollapseButton) {
            fCollapseButton->setWinGravity(StaticGravity);
            fCollapseButton->setText(">");
            fCollapseButton->setImage(taskbarCollapseImage);
            fCollapseButton->setActionListener(this);
            fCollapseButton->setToolTip(_("Hide Taskbar"));
            fCollapseButton->setTitle("Collapse");
        }
    } else
        fCollapseButton = nullptr;

    if (taskBarShowMailboxStatus) {
        fMailBoxControl = new MailBoxControl(app, smActionListener, this, this);
    } else
        fMailBoxControl = nullptr;

    if (configKeyboards.nonempty()) {
        fKeyboardStatus = new KeyboardStatus(this, this);
    } else
        fKeyboardStatus = nullptr;

    if (taskBarShowStartMenu) {
        class LazyRootMenu : public LazyMenu {
            YMenu* ymenu() { return rootMenu; }
        };
        fApplications = new ObjectButton(this, new LazyRootMenu);
        fApplications->setActionListener(this);
        fApplications->setImage(taskbarStartImage);
        fApplications->setToolTip(_("Favorite Applications"));
        fApplications->setTitle("TaskBarMenu");
    } else
        fApplications = nullptr;

    fObjectBar = new ObjectBar(this);
    if (fObjectBar) {
        upath t = app->findConfigFile("toolbar");
        if (t != null) {
            MenuLoader(app, smActionListener, wmActionListener)
            .loadMenus(t, fObjectBar);
        }
        fObjectBar->setTitle("IceToolbar");
    }
    if (taskBarShowWindowListMenu) {
        class LazyWindowListMenu : public LazyMenu {
            YMenu* ymenu() { return windowListMenu; }
        };
        fWinList = new ObjectButton(this, new LazyWindowListMenu);
        fWinList->setImage(taskbarWindowsImage);
        fWinList->setActionListener(this);
        fWinList->setToolTip(_("Window List Menu"));
        fWinList->setTitle("ShowWindowList");
    } else
        fWinList = nullptr;
    if (taskBarShowShowDesktopButton) {
        fShowDesktop = new ObjectButton(this, actionShowDesktop);
        fShowDesktop->setText("__");
        fShowDesktop->setImage(taskbarShowDesktopImage);
        fShowDesktop->setActionListener(wmActionListener);
        fShowDesktop->setToolTip(_("Show Desktop"));
        fShowDesktop->setTitle("ShowDesktop");
    }

    fWorkspaces = taskBarShowWorkspaces
                ? new WorkspacesPane(this)
                : new AWorkspaces(this);
    fWorkspaces->setTitle("Workspaces");

    if (enableAddressBar) {
        fAddressBar = new AddressBar(app, this);
        fAddressBar->setTitle("AddressBar");
    }
    if (taskBarShowWindows) {
        fTasks = new TaskPane(this, this);
        fTasks->setTitle("TaskPane");
    } else
        fTasks = nullptr;
    if (taskBarShowTray) {
        fWindowTray = new TrayPane(this, this);
        fWindowTray->setTitle("TrayPane");
    } else
        fWindowTray = nullptr;

    if (taskBarEnableSystemTray) {
        const char atomstr[] =
#ifdef CONFIG_EXTERNAL_TRAY
                   "_ICEWM_INTTRAY_S"
#else
                   "_NET_SYSTEM_TRAY_S"
#endif
                   ;
        YAtom trayatom(atomstr, true);
        bool isInternal = ('I' == atomstr[1]);

        fDesktopTray = new YXTray(this, isInternal, trayatom,
                                  this, trayDrawBevel);
        fDesktopTray->setTitle("SystemTray");
        fDesktopTray->relayout();
    } else
        fDesktopTray = nullptr;

    if (fCollapseButton) {
        fCollapseButton->raise();
    }
}

void TaskBar::trayChanged() {
    relayout();
    //    updateLayout();
}

struct LayoutInfo {
    YWindow *w;
    bool left;
    bool row; // 0 = bottom, 1 = top
    bool show;
    bool expand;
    short pre, post;

    LayoutInfo() :
        w(nullptr), left(false), row(false), show(false),
        expand(false), pre(0), post(0) {}
    LayoutInfo(YWindow *w, bool l, bool r, bool s, bool e, short p, short o) :
        w(w), left(l), row(r), show(s), expand(e), pre(p), post(o) {}
};

void TaskBar::updateLayout(unsigned &size_w, unsigned &size_h) {
    enum { Over, Here };
    enum { Bot, Top };
    enum { Same, Show };
    enum { Keep, Grow };
    LayoutInfo nw;
    YArray<LayoutInfo> wlist(16);

    bool issue314 = taskBarAtTop;
    nw = LayoutInfo( fApplications, Here, issue314, Show, Grow, 0, 0 );
    wlist.append(nw);
    nw = LayoutInfo( fShowDesktop, Here, !issue314, Show, Grow, 0, 0 );
    wlist.append(nw);
    nw = LayoutInfo( fWinList, Here, !issue314, Show, Grow, 0, 0 );
    wlist.append(nw);
    nw = LayoutInfo( fObjectBar, Here, Top, Show, Grow, 4, 0 );
    wlist.append(nw);
    nw = LayoutInfo( fCollapseButton, Over, Bot, Show, Grow, 0, 2 );
    wlist.append(nw);
    nw = LayoutInfo( fWorkspaces, taskBarWorkspacesLeft,
                     taskBarDoubleHeight && taskBarWorkspacesTop,
                     taskBarShowWorkspaces && workspaceCount > 0,
                     Grow, 4, 4 );
    wlist.append(nw);

    nw = LayoutInfo( fClock, Over, Top, Same, Keep, 2, 2 );
    wlist.append(nw);
    if (taskBarShowMailboxStatus) {
        for (MailBoxControl::IterType m = fMailBoxControl->iterator(); ++m; ) {
            nw = LayoutInfo( *m, Over, Top, Show, Keep, 1, 1 );
            wlist.append(nw);
        }
    }
    if (fKeyboardStatus) {
        nw = LayoutInfo( fKeyboardStatus, Over, Top, Show, Keep, 1, 1 );
        wlist.append(nw);
    }

#ifdef IWM_STATES
    if (taskBarShowCPUStatus) {
        CPUStatusControl::IterType it = fCPUStatus->getIterator();
        while (++it)
        {
            nw = LayoutInfo(*it, Over, Top, Show, Keep, 2, 2 );
            wlist.append(nw);
        }
    }
#endif

#ifdef MEM_STATES
    nw = LayoutInfo( fMEMStatus, Over, Top, Same, Keep, 2, 2 );
    wlist.append(nw);
#endif

    if (taskBarShowNetStatus) {
        NetStatusControl::IterType it = fNetStatus->getIterator();
        while (++it)
        {
            if (*it != 0) {
                nw = LayoutInfo(*it, Over, Top, Same, Keep, 2, 2 );
                wlist.append(nw);
            }
        }
    }
#ifdef MAX_ACPI_BATTERY_NUM
    nw = LayoutInfo( fApm, Over, Top, Show, Keep, 0, 2 );
    wlist.append(nw);
#endif
    nw = LayoutInfo( fDesktopTray, Over, Top, Show, Keep, 1, 1 );
    wlist.append(nw);
    nw = LayoutInfo( fWindowTray, Over, Bot, Show, Grow, 1, 1 );
    wlist.append(nw);
    const int wcount = wlist.getCount();

    int y[2] = { 0, 0 };
    unsigned h[2] = { 0, 0 };
    int left[2] = { 0, 0 };
    int right[2] = { 0, 0 };

    if (!taskBarDoubleHeight)
        for (int i = 0; i < wcount; i++)
            wlist[i].row = false;
    for (int i = 0; i < wcount; i++) {
        if (wlist[i].w) {
            if (h[wlist[i].row] < wlist[i].w->height())
                h[wlist[i].row] = wlist[i].w->height();
        }
    }

    unsigned w = (desktop->getScreenGeometry().width()
                  * unsigned(taskBarWidthPercentage)) / 100U;

    if (taskBarAtTop) { // !!! for now
        y[1] = 0;
        y[0] = h[1] + y[1];
#if 0
        y[0] = 0;
        if (fIsHidden)
            y[0]++;
        y[1] = h[0] + y[0];
#endif
    } else {
        y[1] = 1;
        y[0] = h[1] + y[1];
    }

    right[0] = w;
    right[1] = w;
    if (taskBarShowWindows && fTasks != nullptr) {
        h[0] = max(h[0], max(YIcon::smallSize() + 8, fTasks->maxHeight()));
    }

    for (int i = 0; i < wcount; i++) {
        if (!wlist[i].w)
            continue;
        if (!wlist[i].show && !wlist[i].w->visible())
            continue;

        int xx = 0;
        int yy = 0;
        int ww = wlist[i].w->width();
        int hh = h[wlist[i].row];

        if (wlist[i].expand) {
            yy = y[wlist[i].row];
        } else {
            hh = wlist[i].w->height();
            yy = y[wlist[i].row] + (h[wlist[i].row] - wlist[i].w->height()) / 2;
        }

        if (wlist[i].left) {
            xx = left[wlist[i].row] + wlist[i].pre;

            left[wlist[i].row] += ww + wlist[i].pre + wlist[i].post;
        } else {
            xx = right[wlist[i].row] - ww - wlist[i].pre;

            right[wlist[i].row] -= ww + wlist[i].pre + wlist[i].post;
        }
        wlist[i].w->setGeometry(YRect(xx, yy, ww, hh));
        if (wlist[i].show)
            wlist[i].w->show();
    }

    wlist.clear();
    /* ----------------------------------------------------------------- */

    if (taskBarShowWindows) {
        if (fTasks) {
            fTasks->setGeometry(YRect(left[0],
                                      y[0],
                                      max(0, right[0] - left[0]),
                                      h[0]));
            fTasks->show();
            fTasks->relayout();
        }
    }
    if (fAddressBar) {
        int row = taskBarDoubleHeight;

        fAddressBar->setGeometry(YRect(left[row],
                                       y[row] + 2,
                                       max(0, right[row] - left[row]),
                                       h[row] - 4));
        fAddressBar->raise();
        if (::showAddressBar) {
            if (taskBarDoubleHeight || !taskBarShowWindows)
                fAddressBar->show();
        }
    }

    size_w = w;
    size_h = h[0] + h[1] + 1;
}

void TaskBar::relayoutNow() {
    if (fUpdates.nonempty()) {
        for (YFrameWindow* frame : fUpdates) {
            frame->updateAppStatus();
        }
        fUpdates.clear();
    }
    if (windowTrayPane())
        windowTrayPane()->relayoutNow();
    if (fNeedRelayout) {
        updateLocation();
    }
    if (taskPane())
        taskPane()->relayoutNow();
    if (fButtonUpdate) {
        fButtonUpdate = false;
        buttonUpdate();
    }
}

void TaskBar::updateFullscreen(bool fullscreen) {
    fFullscreen = fullscreen;
    if (fFullscreen || fIsHidden)
        fEdgeTrigger->show();
    else
        fEdgeTrigger->hide();
}

void TaskBar::updateLocation() {
    fNeedRelayout = false;

    if (fIsHidden) {
        if (fIsMapped && getFrame() && visible())
            getFrame()->wmHide();
        xapp->sync();
    }

    int dx, dy;
    unsigned dw, dh;
    desktop->getScreenGeometry(&dx, &dy, &dw, &dh, -1);

    int x = dx;
    unsigned int w = 0;
    unsigned int h = 0;

    if (taskBarWidthPercentage < 100) {
        w = (dw * taskBarWidthPercentage + 50) / 100;
        if (strcmp(taskBarJustify, "right") == 0)
            x = dx + (dw - w);
        if (strcmp(taskBarJustify, "center") == 0)
            x = dx + (dw - w)/2;
    }

    updateLayout(w, h);

    if (fIsCollapsed) {
        if (fCollapseButton) {
            w = fCollapseButton->width();
            h = fCollapseButton->height() + 1;
            fCollapseButton->setPosition(0, 1);
            fCollapseButton->raise();
            fCollapseButton->show();
        }
        else {
            w = h = 0;
        }

        x = dx + (dw - w);
    }

    int by = taskBarAtTop ? dy : dy + dh - 1;

    fEdgeTrigger->setGeometry(YRect(x, by, w, 1));

    int y = taskBarAtTop ? dy : dy + dh - h;

    if ( !fIsHidden) {
        if (fIsMapped && getFrame()) {
            XConfigureRequestEvent conf;
            conf.type = ConfigureRequest;
            conf.window = handle();
            conf.x = x;
            conf.y = y;
            conf.width = int(w);
            conf.height = int(h);
            conf.value_mask = CWX | CWY | CWWidth | CWHeight;
            getFrame()->configureClient(conf);
            getFrame()->wmShow();
        } else
            setGeometry(YRect(x, y, w, h));
    }
    if (fIsHidden || fFullscreen)
        fEdgeTrigger->show();
    else
        fEdgeTrigger->hide();

    ///!!! fix
    updateWMHints();
}

void TaskBar::updateWMHints() {
    YStrut strut;
    if (!taskBarAutoHide && !fIsCollapsed) {
        YRect geo = desktop->getScreenGeometry();
        if (y() + height() == geo.y() + geo.height()) {
            strut.bottom = Atom(height());
        }
        else if (y() == geo.y()) {
            strut.top = Atom(height());
        }
    }
    if (fStrut != strut) {
        fStrut = strut;
        MSG(("SET NET WM STRUT"));
        setProperty(_XA_NET_WM_STRUT, XA_CARDINAL, &strut, 4);
    }
}

void TaskBar::updateWinLayer() {
    long layer = (taskBarAutoHide || fFullscreen) ? WinLayerAboveAll
               : fIsCollapsed ? WinLayerAboveDock
               : taskBarKeepBelow ? WinLayerBelow : WinLayerDock;
    if (getFrame()) {
        getFrame()->wmSetLayer(layer);
    } else {
        setWinLayerHint(layer);
    }
}

void TaskBar::handleFocus(const XFocusChangeEvent& focus) {
    if (focus.type == FocusOut) {
        if (focus.mode == NotifyUngrab) {
            if (focus.detail == NotifyPointer) {
                updateWMHints();
            }
        }
    }
}

void TaskBar::handleCrossing(const XCrossingEvent &crossing) {
    unsigned long last = YWindow::getLastEnterNotifySerial();
    bool ahwm_hack = (crossing.serial != last &&
        (crossing.serial != last + 1 || crossing.detail != NotifyVirtual));

    if (crossing.type == EnterNotify) {
        fEdgeTrigger->stopHide();
    }
    else if (crossing.type == LeaveNotify) {
        if (crossing.detail == NotifyInferior ||
           (crossing.detail == NotifyVirtual && crossing.mode == NotifyGrab) ||
           (crossing.detail == NotifyAncestor && crossing.mode != NotifyNormal))
        {
            if (ahwm_hack) {
                fEdgeTrigger->stopHide();
            }
        } else {
            if (ahwm_hack) {
                fEdgeTrigger->startHide();
            }
        }
    }
}

void TaskBar::paint(Graphics &g, const YRect& r) {
    if (taskbackPixbuf != null &&
        (fGradient == null ||
         fGradient->width() != width() ||
         fGradient->height() != height()))
    {
        int gradientHeight = height() / (1 + taskBarDoubleHeight);
        fGradient = taskbackPixbuf->scale(width(), gradientHeight);
    }

    g.setColor(taskBarBg);
    //g.draw3DRect(0, 0, width() - 1, height() - 1, true);

    // When TaskBarDoubleHeight=1 this draws the upper half.
    if (fGradient != null) {
        g.drawImage(fGradient, r.x(), r.y(), r.width(), r.height(),
                    r.x(), r.y());
    }
    else if (taskbackPixmap != null) {
        g.fillPixmap(taskbackPixmap, r.x(), r.y(), r.width(), r.height(),
                     r.x(), r.y());
    }
    else if (taskBarAtTop) {
        bool dh = (r.y() + r.height() == height());
        g.fillRect(r.x(), r.y(), r.width(), r.height() - dh);
        if (dh) {
            g.setColor(taskBarBg->darker());
            g.drawLine(r.x(), height() - 1, r.x() + r.width(), height() - 1);
        }
    }
    else {
        bool dy = (r.y() == 0);
        g.fillRect(r.x(), r.y() + dy, r.width(), r.height() - dy);
        if (dy) {
            g.setColor(taskBarBg->brighter());
            g.drawLine(r.x(), 0, r.x() + r.width(), 0);
        }
    }
}

bool TaskBar::handleKey(const XKeyEvent &key) {
    return YWindow::handleKey(key);
}

void TaskBar::handleButton(const XButtonEvent &button) {
    if ((button.type == ButtonRelease) &&
        (button.button == 1 || button.button == 3) &&
        (BUTTON_MODMASK(button.state) == Button1Mask + Button3Mask))
    {
        windowList->showFocused(button.x_root, button.y_root);
    }
    else {
        if (button.type == ButtonPress) {
            manager->updateWorkArea();
            if (button.button == 1) {
                if (button.state & xapp->AltMask)
                    lower();
                else if (!(button.state & ControlMask))
                    raise();
            }
        }
    }
    YWindow::handleButton(button);
}

void TaskBar::contextMenu(int x_root, int y_root) {
    taskBarMenu->popup(this, nullptr, nullptr, x_root, y_root,
                       YPopupWindow::pfCanFlipVertical |
                       YPopupWindow::pfCanFlipHorizontal);
}

void TaskBar::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 1) {
    } else if (up.button == 2) {
        windowList->showFocused(up.x_root, up.y_root);
    } else {
        if (up.button == 3 && count == 1 && IS_BUTTON(up.state, Button3Mask)) {
            contextMenu(up.x_root, up.y_root);
        }
    }
}

void TaskBar::handleEndDrag(const XButtonEvent &/*down*/, const XButtonEvent &/*up*/) {
    xapp->releaseEvents();
}
void TaskBar::handleDrag(const XButtonEvent &/*down*/, const XMotionEvent &motion) {
    int newPosition = 0;

    xapp->grabEvents(this, YXApplication::movePointer.handle(),
                         ButtonPressMask |
                         ButtonReleaseMask |
                         PointerMotionMask);


    if (motion.y_root < int(desktop->height() / 2))
        newPosition = 1;

    if (taskBarAtTop != newPosition) {
        taskBarAtTop = newPosition;
        //setPosition(x(), taskBarAtTop ? -1 : int(desktop->height() - height() + 1));
        updateLocation();
        //repaint();
        //manager->updateWorkArea();
    }
}

void TaskBar::popupStartMenu() {
    if (fApplications) {
        /*requestFocus();
         fApplications->requestFocus();
         fApplications->setFocus();*/
        popOut();
        fApplications->popupMenu();
    }
}

void TaskBar::popupWindowListMenu() {
    if (fWinList) {
        popOut();
        fWinList->popupMenu();
    }
}

bool TaskBar::autoTimer(bool doShow) {
    MSG(("hide taskbar"));
    if (fFullscreen && doShow && taskBarFullscreenAutoShow) {
        fIsHidden = false;
        getFrame()->focus();
        manager->switchFocusTo(getFrame(), true);
        manager->updateFullscreenLayer();
    }
    if (taskBarAutoHide) {
        fIsHidden = !doShow && !hasPopup();
        if (taskBarDoubleHeight == false && taskBarShowWindows) {
            fIsHidden &= !(addressBar() && addressBar()->visible());
        }
        updateLocation();
    }
    return fIsHidden == doShow;
}

void TaskBar::popOut() {
    if (fIsCollapsed) {
        handleCollapseButton();
    }
    if (taskBarAutoHide) {
        fIsHidden = false;
        updateLocation();
        fIsHidden = taskBarAutoHide;
        if (fEdgeTrigger) {
            MSG(("start hide 4"));
            fEdgeTrigger->startHide();
        }
    }
    relayoutNow();
}

void TaskBar::showBar() {
    if (getFrame() == nullptr) {
        manager->mapClient(handle());
        updateWinLayer();
        if (getFrame() != nullptr) {
            getFrame()->setAllWorkspaces();
            if (enableAddressBar && ::showAddressBar && taskBarDoubleHeight)
                getFrame()->activate(true);
            updateLocation();
            parent()->setTitle("TaskBarFrame");
            getFrame()->updateLayer();
        }
    }
}

void TaskBar::actionPerformed(YAction action, unsigned int modifiers) {
    wmActionListener->actionPerformed(action, modifiers);
}

void TaskBar::handleCollapseButton() {
    fIsCollapsed = !fIsCollapsed;
    if (fCollapseButton) {
        fCollapseButton->setText(fIsCollapsed ? "<": ">");
        fCollapseButton->setImage(fIsCollapsed ? taskbarExpandImage : taskbarCollapseImage);
        fCollapseButton->setToolTip(fIsCollapsed ? _("Show Taskbar") : _("Hide Taskbar"));
        fCollapseButton->repaint();
    }

    if (fIsCollapsed)
        updateWinLayer();
    relayout();
    updateLocation();
    if (fIsCollapsed == false)
        updateWinLayer();
    xapp->sync();
}

void TaskBar::handlePopDown(YPopupWindow * /*popup*/) {
}

void TaskBar::configure(const YRect2& r) {
    if (r.resized() && 1 < r.width()) {
        repaint();
    }
    updateWMHints();
}

void TaskBar::repaint() {
    GraphicsBuffer(this).paint();
}

void TaskBar::detachDesktopTray() {
    if (fDesktopTray) {
        MSG(("detach Tray"));
        fDesktopTray->detachTray();
        delete fDesktopTray; fDesktopTray = nullptr;
    }
}

void TaskBar::updateFrame(YFrameWindow* frame) {
    if (find(fUpdates, frame) < 0)
        fUpdates += frame;
}

void TaskBar::delistFrame(YFrameWindow* frame, TaskBarApp* task, TrayApp* tray) {
    findRemove(fUpdates, frame);
    if (taskPane() && task)
        taskPane()->remove(task);
    if (windowTrayPane() && tray)
        windowTrayPane()->remove(tray);
}

TaskBarApp *TaskBar::addTasksApp(YFrameWindow* frame) {
    return taskPane() ? taskPane()->addApp(frame) : nullptr;
}

void TaskBar::relayoutTasks() {
    if (taskPane())
        taskPane()->relayout();
}

TrayApp *TaskBar::addTrayApp(YFrameWindow* frame) {
    return windowTrayPane() ? windowTrayPane()->addApp(frame) : nullptr;
}

void TaskBar::relayoutTray() {
    if (windowTrayPane())
        windowTrayPane()->relayout();
}

void TaskBar::showAddressBar() {
    popOut();
    if (fAddressBar != nullptr)
        fAddressBar->showNow();
}

void TaskBar::setWorkspaceActive(long workspace, bool active) {
    if (taskBarShowWorkspaces && fWorkspaces) {
        YDimension dim(fWorkspaces->dimension());
        fWorkspaces->setPressed(workspace, active);
        if (dim != fWorkspaces->dimension()) {
            relayout();
        }
    }
}

void TaskBar::workspacesRepaint() {
    if (taskBarShowWorkspaces && fWorkspaces) {
        fWorkspaces->repaint();
    }
}

void TaskBar::workspacesUpdateButtons() {
    fButtonUpdate = true;
}

void TaskBar::buttonUpdate() {
    if (taskBarShowWorkspaces && fWorkspaces) {
        YDimension dim(fWorkspaces->dimension());
        fWorkspaces->updateButtons();
        if (dim != fWorkspaces->dimension()) {
            relayout();
        }
    }
}

void TaskBar::workspacesRelabelButtons() {
    if (taskBarShowWorkspaces && fWorkspaces) {
        YDimension dim(fWorkspaces->dimension());
        fWorkspaces->relabelButtons();
        if (dim != fWorkspaces->dimension()) {
            relayout();
        }
    }
}

void TaskBar::keyboardUpdate(mstring keyboard) {
    if (fKeyboardStatus) {
        fKeyboardStatus->updateKeyboard(keyboard);
    }
}

bool TaskBar::windowTrayRequestDock(Window w) {
    if (fDesktopTray) {
        fDesktopTray->trayRequestDock(w, "SystemTray");
        return true;
    }
    return false;
}

void TaskBar::switchToPrev() {
    if (taskPane())
        taskPane()->switchToPrev();
}

void TaskBar::switchToNext() {
    if (taskPane())
        taskPane()->switchToNext();
}

void TaskBar::movePrev() {
    if (taskPane())
        taskPane()->movePrev();
}

void TaskBar::moveNext() {
    if (taskPane())
        taskPane()->moveNext();
}

void TaskBar::refresh() {
    if (fApplications)
        fApplications->repaint();
    if (fWinList)
        fWinList->repaint();
    if (fShowDesktop)
        fShowDesktop->repaint();
    if (fObjectBar)
        fObjectBar->refresh();
}

// vim: set sw=4 ts=4 et:
