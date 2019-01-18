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
#include "acpustatus.h"

#include "ymenuitem.h"
#include "wmmgr.h"
#include "wmframe.h"
#include "wmclient.h"
#include "wmapp.h"
#include "wmaction.h"
#include "wmprog.h"
#include "sysdep.h"
#include "wmwinlist.h"

#include "aaddressbar.h"
#include "aclock.h"
#include "acpustatus.h"
#include "amemstatus.h"
#include "apppstatus.h"
#include "amailbox.h"
#include "objbar.h"
#include "objbutton.h"
#include "objmenu.h"
#include "atasks.h"
#include "atray.h"
#include "aworkspaces.h"
#include "yrect.h"
#include "yxtray.h"
#include "prefs.h"
#include "yicon.h"
#include "wpixmaps.h"
#include "aapm.h"
#include "upath.h"

#include "intl.h"

lazy<YTimer> WorkspaceButton::fRaiseTimer;

TaskBar *taskBar = 0;

YColorName taskBarBg(&clrDefaultTaskBar);

static void initPixmaps() {
    if (taskbarStartImage == null || !taskbarStartImage->valid())
        taskbarStartImage = taskbarLinuxImage;
}


EdgeTrigger::EdgeTrigger(TaskBar *owner) {
    setStyle(wsOverrideRedirect | wsInputOnly);
    setPointer(YXApplication::leftPointer);
    setDND(true);
    if (taskBarAutoHide) {
        setTitle("IceEdge");
    }

    fTaskBar = owner;

    fAutoHideTimer->setTimer(autoShowDelay, this, false);
    fDoShow = false;
}

EdgeTrigger::~EdgeTrigger() {
}

void EdgeTrigger::startHide() {
    fDoShow = false;
    if (fAutoHideTimer)
        fAutoHideTimer->startTimer(autoHideDelay);
}

void EdgeTrigger::stopHide() {
    fDoShow = false;
    if (fAutoHideTimer)
        fAutoHideTimer->stopTimer();
}

void EdgeTrigger::handleCrossing(const XCrossingEvent &crossing) {
    if (crossing.type == EnterNotify /* && crossing.mode != NotifyNormal */) {
        unsigned long last = YWindow::getLastEnterNotifySerial();
        if (crossing.serial != last && crossing.serial != last + 1) {
            MSG(("enter notify %d %d", crossing.mode, crossing.detail));
            fDoShow = true;
            if (fAutoHideTimer) {
                fAutoHideTimer->startTimer(autoShowDelay);
            }
        }
    } else if (crossing.type == LeaveNotify /* && crossing.mode != NotifyNormal */) {
        fDoShow = false;
        MSG(("leave notify"));
        if (fAutoHideTimer)
            fAutoHideTimer->stopTimer();
    }
}

void EdgeTrigger::handleDNDEnter() {
    fDoShow = true;
    if (fAutoHideTimer)
        fAutoHideTimer->startTimer(autoShowDelay);
}

void EdgeTrigger::handleDNDLeave() {
    fDoShow = false;
    if (fAutoHideTimer)
        fAutoHideTimer->startTimer(autoHideDelay);
}


bool EdgeTrigger::handleTimer(YTimer *t) {
    MSG(("taskbar handle timer"));
    if (t == fAutoHideTimer) {
        fTaskBar->autoTimer(fDoShow);
        return false;
    }
    return false;
}

TaskBar::TaskBar(IApp *app, YWindow *aParent, YActionListener *wmActionListener, YSMListener *smActionListener):
    YFrameClient(aParent, 0),
    fTasks(0),
    fCollapseButton(0),
    fWindowTray(0),
    fMailBoxStatus(0),
    fMEMStatus(0),
    fCPUStatus(0),
    fApm(0),
    fObjectBar(0),
    fApplications(0),
    fWinList(0),
    fShowDesktop(0),
    fAddressBar(0),
    fWorkspaces(0),
    fDesktopTray(0),
    wmActionListener(wmActionListener),
    smActionListener(smActionListener),
    app(app),
    fIsHidden(taskBarAutoHide),
    fFullscreen(false),
    fIsCollapsed(false),
    fIsMapped(false),
    fMenuShown(false),
    taskBarMenu(0),
    fNeedRelayout(false),
    fEdgeTrigger(0)
{
    taskBar = this;

    ///setToplevel(true);
    setBackground(taskBarBg.pixel());
    if (taskbackPixmap != null)
        setBackgroundPixmap(taskbackPixmap->pixmap());

    initPixmaps();

    setTitle("TaskBar");
    setWindowTitle(_("Task Bar"));
    setIconTitle(_("Task Bar"));
    setClassHint("icewm", "TaskBar");
    //!!!setWinStateHint(WinStateDockHorizontal, WinStateDockHorizontal);

    setWinHintsHint(WinHintsSkipFocus |
                    WinHintsSkipWindowMenu |
                    WinHintsSkipTaskBar);

    setWinWorkspaceHint(-1);
    setWinLayerHint((taskBarAutoHide || fFullscreen) ? WinLayerAboveAll :
                    fIsCollapsed ? WinLayerAboveDock :
                    taskBarKeepBelow ? WinLayerBelow : WinLayerDock);
    Atom protocols[2] = {
      _XA_WM_DELETE_WINDOW,
      _XA_WM_TAKE_FOCUS
      //_NET_WM_PING,
      //_NET_WM_SYNC_REQUEST,
    };
    XSetWMProtocols(xapp->display(), handle(), protocols, 2);
    getProtocols(false);

    {
        XWMHints wmh = {};
        wmh.flags = InputHint;
        wmh.input = False;
        wmh.initial_state = WithdrawnState;
        XSetWMHints(xapp->display(), handle(), &wmh);
    }
    {
        long wk[4] = { 0, 0, 0, 0 };

        XChangeProperty(xapp->display(),
                        handle(),
                        _XA_NET_WM_STRUT,
                        XA_CARDINAL,
                        32, PropModeReplace,
                        (unsigned char *)&wk, 4);
    }

    setMwmHints(MwmHints(
       MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS,
       MWM_FUNC_MOVE,
       0,
       0,
       0));

    {
        long arg[2];
        arg[0] = NormalState;
        arg[1] = 0;
        XChangeProperty(xapp->display(), handle(),
                        _XA_WM_STATE, _XA_WM_STATE,
                        32, PropModeReplace,
                        (unsigned char *)arg, 2);
    }
    setPointer(YXApplication::leftPointer);
    setDND(true);

    fEdgeTrigger = new EdgeTrigger(this);

    initMenu();
    initApplets();

    getPropertiesList();
    getWMHints();
    getClassHint();
    fIsMapped = true;
}

TaskBar::~TaskBar() {
    detachDesktopTray();
    delete fEdgeTrigger; fEdgeTrigger = 0;
    delete fClock; fClock = 0;
    delete fMailBoxStatus; fMailBoxStatus = 0;
#ifdef MEM_STATES
    delete fMEMStatus; fMEMStatus = 0;
#endif
    delete fWinList; fWinList = 0;
    delete fApplications; fApplications = 0;
    delete fObjectBar; fObjectBar = 0;
    delete fWorkspaces; fWorkspaces = 0;
#ifdef MAX_ACPI_BATTERY_NUM
    delete fApm; fApm = 0;
#endif
#ifdef IWM_STATES
    delete fCPUStatus; fCPUStatus = 0;
#endif
    delete fAddressBar; fAddressBar = 0;
    delete fTasks; fTasks = 0;
    delete fWindowTray; fWindowTray = 0;
    delete fCollapseButton; fCollapseButton = 0;
    delete fShowDesktop; fShowDesktop = 0;
    delete taskBarMenu; taskBarMenu = 0;
    taskBar = 0;
    MSG(("taskBar delete"));
}

void TaskBar::initMenu() {
    taskBarMenu = new YMenu();
    if (taskBarMenu) {
        taskBarMenu->setActionListener(this);
        taskBarMenu->addItem(_("Tile _Vertically"), -2, KEY_NAME(gKeySysTileVertical), actionTileVertical);
        taskBarMenu->addItem(_("T_ile Horizontally"), -2, KEY_NAME(gKeySysTileHorizontal), actionTileHorizontal);
        taskBarMenu->addItem(_("Ca_scade"), -2, KEY_NAME(gKeySysCascade), actionCascade);
        taskBarMenu->addItem(_("_Arrange"), -2, KEY_NAME(gKeySysArrange), actionArrange);
        taskBarMenu->addItem(_("_Minimize All"), -2, KEY_NAME(gKeySysMinimizeAll), actionMinimizeAll);
        taskBarMenu->addItem(_("_Hide All"), -2, KEY_NAME(gKeySysHideAll), actionHideAll);
        taskBarMenu->addItem(_("_Undo"), -2, KEY_NAME(gKeySysUndoArrange), actionUndoArrange);
        if (minimizeToDesktop)
            taskBarMenu->addItem(_("Arrange _Icons"), -2, KEY_NAME(gKeySysArrangeIcons), actionArrangeIcons)->setEnabled(false);
        taskBarMenu->addSeparator();
        taskBarMenu->addItem(_("_Windows"), -2, actionWindowList, windowListMenu);
        taskBarMenu->addSeparator();
        taskBarMenu->addItem(_("_Refresh"), -2, null, actionRefresh);

#if 0
        YMenu *helpMenu; // !!!

        helpMenu = new YMenu();
        helpMenu->addItem(_("_License"), -2, null, actionLicense);
        helpMenu->addSeparator();
        helpMenu->addItem(_("_About"), -2, null, actionAbout);
#endif

        taskBarMenu->addItem(_("_About"), -2, actionAbout, 0);
        if (logoutMenu) {
            taskBarMenu->addSeparator();
            if (showLogoutSubMenu)
                taskBarMenu->addItem(_("_Logout..."), -2, actionLogout, logoutMenu);
            else
                taskBarMenu->addItem(_("_Logout..."), -2, null, actionLogout);
        }
    }

}

void TaskBar::initApplets() {
#ifdef MEM_STATES
    if (taskBarShowMEMStatus)
        fMEMStatus = new MEMStatus(this, this);
    else
        fMEMStatus = 0;
#endif

#ifdef IWM_STATES
    if (taskBarShowCPUStatus)
        fCPUStatus = new CPUStatusControl(smActionListener, this, this);
    else
        fCPUStatus = 0;
#endif

    if (taskBarShowNetStatus)
        fNetStatus.init(new NetStatusControl(app, smActionListener, this, this));
    if (taskBarShowClock)
        fClock = new YClock(smActionListener, this, this);
    else
        fClock = 0;

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
                fApm = 0;
        }
        else fApm->setTitle("IceAPM");
    }
    else
        fApm = 0;
#endif

    if (taskBarShowCollapseButton) {
        fCollapseButton = new YButton(this, actionCollapseTaskbar);
        if (fCollapseButton) {
            fCollapseButton->setText(">");
            fCollapseButton->setImage(taskbarCollapseImage);
            fCollapseButton->setActionListener(this);
            fCollapseButton->setToolTip(_("Hide Taskbar"));
            fCollapseButton->setTitle("Collapse");
        }
    } else
        fCollapseButton = 0;

    if (taskBarShowMailboxStatus) {
        fMailBoxStatus = new MailBoxControl(app, smActionListener, this, this);
    } else
        fMailBoxStatus = 0;

    if (taskBarShowStartMenu) {
        fApplications = new ObjectButton(this, rootMenu);
        fApplications->setActionListener(this);
        fApplications->setImage(taskbarStartImage);
        fApplications->setToolTip(_("Favorite Applications"));
        fApplications->setTitle("TaskBarMenu");
    } else
        fApplications = 0;

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
        fWinList = new ObjectButton(this, windowListMenu);
        fWinList->setImage(taskbarWindowsImage);
        fWinList->setActionListener(this);
        fWinList->setToolTip(_("Window List Menu"));
        fWinList->setTitle("ShowWindowList");
    } else
        fWinList = 0;
    if (taskBarShowShowDesktopButton) {
        fShowDesktop = new ObjectButton(this, actionShowDesktop);
        fShowDesktop->setText("__");
        fShowDesktop->setImage(taskbarShowDesktopImage);
        fShowDesktop->setActionListener(wmActionListener);
        fShowDesktop->setToolTip(_("Show Desktop"));
        fShowDesktop->setTitle("ShowDesktop");
    }
    if (taskBarShowWorkspaces && workspaceCount > 0) {
        fWorkspaces = new WorkspacesPane(this);
        fWorkspaces->setTitle("Workspaces");
    } else
        fWorkspaces = 0;
    if (enableAddressBar) {
        fAddressBar = new AddressBar(app, this);
        fAddressBar->setTitle("AddressBar");
    }
    if (taskBarShowWindows) {
        fTasks = new TaskPane(this, this);
        fTasks->setTitle("TaskPane");
    } else
        fTasks = 0;
    if (taskBarShowTray) {
        fWindowTray = new TrayPane(this, this);
        fWindowTray->setTitle("TrayPane");
    } else
        fWindowTray = 0;

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
        fDesktopTray = 0;
}

void TaskBar::trayChanged() {
    relayout();
    //    updateLayout();
}

struct LayoutInfo {
    YWindow *w;
    bool left;
    int row; // 0 = bottom, 1 = top
    bool show;
    int pre, post;
    bool expand;

    LayoutInfo() :
        w(0), left(false), row(0), show(false), pre(0), post(0), expand(false) {}
    LayoutInfo(YWindow *_w, bool l, int r, bool s, int p, int o, bool e) :
        w(_w), left(l), row(r), show(s), pre(p), post(o), expand(e) {}
};

bool operator==(const LayoutInfo &l1, const LayoutInfo &l2)
{
    return memcmp(&l1, &l2, sizeof(LayoutInfo)) == 0;
}

void TaskBar::updateLayout(unsigned &size_w, unsigned &size_h) {
    LayoutInfo nw;
    YArray<LayoutInfo> wlist;
    wlist.setCapacity(13);

    nw = LayoutInfo( fApplications, true, 1, true, 0, 0, true );
    wlist.append(nw);
    nw = LayoutInfo( fShowDesktop, true, 0, true, 0, 0, true );
    wlist.append(nw);
    nw = LayoutInfo( fWinList, true, 0, true, 0, 0, true );
    wlist.append(nw);
    nw = LayoutInfo( fObjectBar, true, 1, true, 4, 0, true );
    wlist.append(nw);
    nw = LayoutInfo( fCollapseButton, false, 0, true, 0, 2, true );
    wlist.append(nw);
    nw = LayoutInfo( fWorkspaces, taskBarWorkspacesLeft,
                     taskBarDoubleHeight && taskBarWorkspacesTop,
                     taskBarShowWorkspaces && workspaceCount > 0,
                     4, 4, true );
    wlist.append(nw);

    nw = LayoutInfo( fClock, false, 1, false, 2, 2, false );
    wlist.append(nw);
    if (taskBarShowMailboxStatus) {
        for (MailBoxControl::IterType m = fMailBoxStatus->iterator(); ++m; ) {
            nw = LayoutInfo( *m, false, 1, true, 1, 1, false );
            wlist.append(nw);
        }
    }

#ifdef IWM_STATES
    if (taskBarShowCPUStatus) {
        CPUStatusControl::IterType it = fCPUStatus->getIterator();
        while (++it)
        {
            nw = LayoutInfo(*it, false, 1, true, 2, 2, false );
            wlist.append(nw);
        }
    }
#endif

#ifdef MEM_STATES
    nw = LayoutInfo( fMEMStatus, false, 1, false, 2, 2, false );
    wlist.append(nw);
#endif

    if (taskBarShowNetStatus) {
        NetStatusControl::IterType it = fNetStatus->getIterator();
        while (++it)
        {
            if (*it != 0) {
                nw = LayoutInfo(*it, false, 1, false, 2, 2, false);
                wlist.append(nw);
            }
        }
    }
#ifdef MAX_ACPI_BATTERY_NUM
    nw = LayoutInfo( fApm, false, 1, true, 0, 2, false );
    wlist.append(nw);
#endif
    nw = LayoutInfo( fDesktopTray, false, 1, true, 1, 1, false );
    wlist.append(nw);
    nw = LayoutInfo( fWindowTray, false, 0, true, 1, 1, true );
    wlist.append(nw);
    const int wcount = wlist.getCount();

    unsigned w = 0;
    int y[2] = { 0, 0 };
    unsigned h[2] = { 0, 0 };
    int left[2] = { 0, 0 };
    int right[2] = { 0, 0 };

    if (!taskBarDoubleHeight)
        for (int i = 0; i < wcount; i++)
            wlist[i].row = 0;
    for (int i = 0; i < wcount; i++) {
        if (!wlist[i].w)
             continue;
        if (wlist[i].w->height() > h[wlist[i].row])
            h[wlist[i].row] = wlist[i].w->height();
    }

    {
        int dx, dy;
        unsigned dw, dh;
        manager->getScreenGeometry(&dx, &dy, &dw, &dh);
        w = (dw/100.0) * taskBarWidthPercentage;
    }

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
    if (taskBarShowWindows && fTasks != 0) {
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
        int row = taskBarDoubleHeight ? 1 : 0;

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
    if (windowTrayPane())
        windowTrayPane()->relayoutNow();
    if (fNeedRelayout) {
        updateLocation();
    }
    if (taskPane())
        taskPane()->relayoutNow();
}

void TaskBar::updateFullscreen(bool fullscreen) {
    fFullscreen = fullscreen;
    if ((fFullscreen || fIsHidden) && taskBarAutoHide)
        fEdgeTrigger->show();
    else
        fEdgeTrigger->hide();
}

void TaskBar::updateLocation() {
    fNeedRelayout = false;

    if (fIsHidden) {
        if (fIsMapped && getFrame())
            getFrame()->wmHide();
        else
            hide();
        xapp->sync();
    }

    int dx, dy;
    unsigned dw, dh;
    manager->getScreenGeometry(&dx, &dy, &dw, &dh, -1);

    int x = dx;
    unsigned int w = 0;
    unsigned int h = 0;

    w = (dw/100.0) * taskBarWidthPercentage;
    if (strcmp(taskBarJustify, "right") == 0) x = dw - w;
    if (strcmp(taskBarJustify, "center") == 0) x = (dw - w)/2;

    updateLayout(w, h);

    if (fIsCollapsed) {
        if (fCollapseButton) {
            w = fCollapseButton->width();
            h = fCollapseButton->height();
        } else {
            w = h = 0;
        }

        x = dw - w;

        if (fCollapseButton) {
            fCollapseButton->show();
            fCollapseButton->raise();
            fCollapseButton->setPosition(0, 0);
        }
    }

    int by = taskBarAtTop ? dy : dy + dh - 1;

    if (taskBarAutoHide) {
        fEdgeTrigger->setGeometry(YRect(x, by, w, 1));
    }

    int y = taskBarAtTop ? dy : dy + dh - h;

    if ( !fIsHidden) {
        if (fIsMapped && getFrame()) {
            getFrame()->configureClient(x, y, w, h);
            getFrame()->wmShow();
        } else
            setGeometry(YRect(x, y, w, h));
    }
    if ((fIsHidden || fFullscreen) && taskBarAutoHide)
        fEdgeTrigger->show();
    else
        fEdgeTrigger->hide();

    MwmHints mwm(
       MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS,
       MWM_FUNC_MOVE,
       0,
       0,
       0);
    setMwmHints(mwm);

    XChangeProperty(xapp->display(), handle(),
                    _XATOM_MWM_HINTS, _XATOM_MWM_HINTS,
                    32, PropModeReplace,
                    (unsigned char *)&mwm, PROP_MWM_HINTS_ELEMENTS);
    getMwmHints();
    if (getFrame())
        getFrame()->updateMwmHints();

    ///!!! fix
    updateWMHints();
}

void TaskBar::updateWMHints() {
    int dx, dy;
    unsigned dw, dh;
    manager->getScreenGeometry(&dx, &dy, &dw, &dh);

    long wk[4] = { 0, 0, 0, 0 };
    if (!taskBarAutoHide && !fIsCollapsed && getFrame()) {
        wk[taskBarAtTop ? 2 : 3] = getFrame()->height();
    }

    MSG(("SET NET WM STRUT"));

    XChangeProperty(xapp->display(),
                    handle(),
                    _XA_NET_WM_STRUT,
                    XA_CARDINAL,
                    32, PropModeReplace,
                    (unsigned char *)&wk, 4);
    if (getFrame())
    {
        getFrame()->updateNetWMStrut();
    }
}


void TaskBar::handleCrossing(const XCrossingEvent &crossing) {
    unsigned long last = YWindow::getLastEnterNotifySerial();
    if (crossing.serial != last &&
        (crossing.serial != last + 1 || crossing.detail != NotifyVirtual))
    {
        if (crossing.type == EnterNotify /* && crossing.mode != NotifyNormal */) {
            fEdgeTrigger->stopHide();
        } else if (crossing.type == LeaveNotify /* && crossing.mode != NotifyNormal */) {
            if (crossing.detail != NotifyInferior &&
                !(crossing.detail == NotifyVirtual &&
                  crossing.mode == NotifyGrab) &&
                !(crossing.detail == NotifyAncestor &&
                  crossing.mode != NotifyNormal))
            {
                MSG(("taskbar hide: %d", crossing.detail));
                fEdgeTrigger->startHide();
            } else {
                fEdgeTrigger->stopHide();
            }
        }
    }
}


void TaskBar::handleEndPopup(YPopupWindow *popup) {
    if (!hasPopup()) {
        MSG(("taskbar hide2"));
        //fEdgeTrigger->startHide();
    }
    YWindow::handleEndPopup(popup);
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
    if (fGradient != null)
        g.drawImage(fGradient, r.x(), r.y(), r.width(), r.height(),
                    r.x(), r.y());
    else
    if (taskbackPixmap != null)
        g.fillPixmap(taskbackPixmap, r.x(), r.y(), r.width(), r.height(),
                     r.x(), r.y());
    else {
        int y = taskBarAtTop ? 0 : 1;
        g.fillRect(r.x(), y + r.y(), r.width(), r.height() - 1);
        if (!taskBarAtTop) {
            y++;
            g.setColor(taskBarBg->brighter());
            g.drawLine(r.x(), 0, r.width(), 0);
        } else {
            g.setColor(taskBarBg->darker());
            g.drawLine(r.x(), height() - 1, r.width(), height() - 1);
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
        if (windowList)
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
    taskBarMenu->popup(this, 0, 0, x_root, y_root,
                       YPopupWindow::pfCanFlipVertical |
                       YPopupWindow::pfCanFlipHorizontal);
}

void TaskBar::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 1) {
    } else if (up.button == 2) {
        if (windowList)
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
        //setPosition(x(), taskBarAtTop ? -1 : int(manager->height() - height() + 1));
        manager->setWorkAreaMoveWindows(true);
        updateLocation();
        repaint();
        //manager->updateWorkArea();
        manager->setWorkAreaMoveWindows(false);
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
    if (taskBarAutoHide == true) {
        fIsHidden = doShow ? false : true;
        if (hasPopup())
            fIsHidden = false;
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

void TaskBar::showBar(bool visible) {
    if (visible) {
        if (getFrame() == 0)
            manager->mapClient(handle());
        if (getFrame() != 0) {
            setWinLayerHint((taskBarAutoHide || fFullscreen) ? WinLayerAboveAll :
                            fIsCollapsed ? WinLayerAboveDock :
                            taskBarKeepBelow ? WinLayerBelow : WinLayerDock);
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

    relayout();
    updateLocation();
}

void TaskBar::handlePopDown(YPopupWindow * /*popup*/) {
}

void TaskBar::configure(const YRect &r) {
    YWindow::configure(r);
}

void TaskBar::detachDesktopTray() {
    if (fDesktopTray) {
        MSG(("detach Tray"));
        fDesktopTray->detachTray();
        delete fDesktopTray; fDesktopTray = 0;
    }
}

void TaskBar::removeTasksApp(YFrameWindow *w) {
    if (taskPane())
        taskPane()->removeApp(w);
}

TaskBarApp *TaskBar::addTasksApp(YFrameWindow *w) {
    if (taskPane())
        return taskPane()->addApp(w);
    else
        return 0;
}

void TaskBar::relayoutTasks() {
    if (taskPane())
        taskPane()->relayout();
}

void TaskBar::removeTrayApp(YFrameWindow *w) {
    if (windowTrayPane())
        windowTrayPane()->removeApp(w);
}

TrayApp *TaskBar::addTrayApp(YFrameWindow *w) {
    if (windowTrayPane())
        return windowTrayPane()->addApp(w);
    else
        return 0;
}

void TaskBar::relayoutTray() {
    if (windowTrayPane())
        windowTrayPane()->relayout();
}

void TaskBar::showAddressBar() {
    popOut();
    if (fAddressBar != 0)
        fAddressBar->showNow();
}

void TaskBar::setWorkspaceActive(long workspace, int active) {
    if (fWorkspaces != 0 &&
        fWorkspaces->workspaceButton(workspace) != 0)
    {
        fWorkspaces->workspaceButton(workspace)->setPressed(active);
    }
}

bool TaskBar::windowTrayRequestDock(Window w) {
    if (fDesktopTray) {
        fDesktopTray->trayRequestDock(w, "SystemTray");
        return true;
    }
    return false;
}

void TaskBar::switchToPrev()
{
    if (taskPane())
        taskPane()->switchToPrev();
}

void TaskBar::switchToNext()
{
    if (taskPane())
        taskPane()->switchToNext();
}

void TaskBar::movePrev()
{
    if (taskPane())
        taskPane()->movePrev();
}

void TaskBar::moveNext()
{
    if (taskPane())
        taskPane()->moveNext();
}
// vim: set sw=4 ts=4 et:
