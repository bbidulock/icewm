/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 *
 * TaskBar
 */

#include "config.h"

#ifdef CONFIG_TASKBAR
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

#ifdef CONFIG_TRAY
YTimer *TrayApp::fRaiseTimer(NULL);
#endif
YTimer *WorkspaceButton::fRaiseTimer(NULL);

TaskBar *taskBar = 0;

static YColor* taskBarBg = 0;
YColor* getTaskBarBg() {
    if (taskBarBg == 0) {
        taskBarBg = new YColor(clrDefaultTaskBar);
    }
    return taskBarBg;
}

static void initPixmaps() {
#ifdef CONFIG_GRADIENTS
    if (taskbarStartImage == null || !taskbarStartImage->valid())
        taskbarStartImage = taskbarLinuxImage;
#endif
}


EdgeTrigger::EdgeTrigger(TaskBar *owner) {
    setStyle(wsOverrideRedirect | wsInputOnly);
    setPointer(YXApplication::leftPointer);
    setDND(true);

    fTaskBar = owner;

    fAutoHideTimer = new YTimer(autoShowDelay);
    if (fAutoHideTimer) {
        fAutoHideTimer->setTimerListener(this);
    }
    fDoShow = false;
}

EdgeTrigger::~EdgeTrigger() {
    if (fAutoHideTimer) {
        fAutoHideTimer->stopTimer();
        fAutoHideTimer->setTimerListener(0);
        delete fAutoHideTimer; fAutoHideTimer = 0;
    }
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
    YFrameClient(aParent, 0) INIT_GRADIENT(fGradient, null)
{
    taskBar = this;
 
    this->app = app;
    this->wmActionListener = wmActionListener;
    this->smActionListener = smActionListener;
    fIsMapped = false;
    fIsHidden = taskBarAutoHide;
    fIsCollapsed = false;
    fFullscreen = false;
    fMenuShown = false;
    fNeedRelayout = false;
    fAddressBar = 0;
    fShowDesktop = 0;

    ///setToplevel(true);

    initPixmaps();

    setWindowTitle(_("Task Bar"));
    setIconTitle(_("Task Bar"));
    setClassHint("icewm", "TaskBar");
    setWinStateHint(WinStateAllWorkspaces, WinStateAllWorkspaces);
    //!!!setWinStateHint(WinStateDockHorizontal, WinStateDockHorizontal);

#ifdef GNOME1_HINTS
    setWinHintsHint(WinHintsSkipFocus |
                    WinHintsSkipWindowMenu |
                    WinHintsSkipTaskBar);
#endif

#if defined(GNOME1_HINTS) || defined(WMSPEC_HINTS)
    setWinWorkspaceHint(0);
#endif
#ifdef GNOME1_HINTS
    setWinLayerHint((taskBarAutoHide || fFullscreen) ? WinLayerAboveAll :
                    fIsCollapsed ? WinLayerAboveDock :
                    taskBarKeepBelow ? WinLayerBelow : WinLayerDock);
#endif
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
    {
        MwmHints mwm =
        { MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS
        , MWM_FUNC_MOVE // | MWM_FUNC_RESIZE
        , 0 // MWM_DECOR_BORDER | MWM_DECOR_RESIZEH
        , 0
        , 0
        };

        setMwmHints(mwm);
    }
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
    fIsMapped = true;
}

TaskBar::~TaskBar() {
    detachDesktopTray();
    delete fEdgeTrigger; fEdgeTrigger = 0;
#ifdef CONFIG_APPLET_CLOCK
    delete fClock; fClock = 0;
#endif
#ifdef CONFIG_APPLET_MAILBOX
    for (MailBoxStatus ** m(fMailBoxStatus); m && *m; ++m) delete *m;
    delete[] fMailBoxStatus; fMailBoxStatus = 0;
#endif
#ifdef CONFIG_WINMENU
    delete fWinList; fWinList = 0;
#endif
#ifndef NO_CONFIGURE_MENUS
    delete fApplications; fApplications = 0;
    delete fObjectBar; fObjectBar = 0;
#endif
    delete fWorkspaces; fWorkspaces = 0;
#ifdef CONFIG_APPLET_APM
    delete fApm; fApm = 0;
#endif
#ifdef CONFIG_APPLET_CPU_STATUS
    if (fCPUStatus) {
        for (int i = 0; fCPUStatus[i]; ++i)
            delete fCPUStatus[i];
        delete[] fCPUStatus; fCPUStatus = 0;
    }
#endif
#ifdef CONFIG_ADDRESSBAR
    delete fAddressBar; fAddressBar = 0;
#endif
    delete fTasks; fTasks = 0;
#ifdef CONFIG_TRAY
    delete fWindowTray; fWindowTray = 0;
#endif
    delete fCollapseButton; fCollapseButton = 0;
    delete fShowDesktop; fShowDesktop = 0;
    delete taskBarMenu; taskBarMenu = 0;
    delete taskBarBg; taskBarBg = 0;
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
#ifdef CONFIG_WINMENU
        taskBarMenu->addItem(_("_Windows"), -2, actionWindowList, windowListMenu);
#endif
        taskBarMenu->addSeparator();
        taskBarMenu->addItem(_("_Refresh"), -2, null, actionRefresh);

#ifndef LITE
#if 0
        YMenu *helpMenu; // !!!

        helpMenu = new YMenu();
        helpMenu->addItem(_("_License"), -2, "", actionLicense);
        helpMenu->addSeparator();
        helpMenu->addItem(_("_About"), -2, "", actionAbout);
#endif

        taskBarMenu->addItem(_("_About"), -2, actionAbout, 0);
#endif
        if (logoutMenu) {
            taskBarMenu->addSeparator();
            if (showLogoutSubMenu)
                taskBarMenu->addItem(_("_Logout..."), -2, actionLogout, logoutMenu);
            else
                taskBarMenu->addItem(_("_Logout..."), -2, "", actionLogout);
        }
    }

}

void TaskBar::initApplets() {
#ifdef CONFIG_APPLET_MEM_STATUS
    if (taskBarShowMEMStatus)
        fMEMStatus = new MEMStatus(this);
    else
        fMEMStatus = 0;
#endif
#ifdef CONFIG_APPLET_CPU_STATUS
    fCPUStatus = 0;
    if (taskBarShowCPUStatus)
        CPUStatus::GetCPUStatus(smActionListener, this, fCPUStatus, cpuCombine);
#endif
#ifdef CONFIG_APPLET_NET_STATUS
    fNetStatus.init(new NetStatusControl(app, smActionListener, this, this));
#endif
#ifdef CONFIG_APPLET_CLOCK
    if (taskBarShowClock) {
        fClock = new YClock(smActionListener, this);
    } else
        fClock = 0;
#endif
#ifdef CONFIG_APPLET_APM

    if (taskBarShowApm && (access(APMDEV, 0) == 0 ||
                           access("/sys/class/power_supply", 0) == 0 ||
                           access("/proc/acpi", 0) == 0 ||
                           access("/dev/acpi", 0) == 0 ||
	                   access("/proc/pmu", R_OK|X_OK) == 0))
    {
        fApm = new YApm(this);
    }
    else if(!taskBarShowApm && taskBarShowApmAuto)
    {
    	fApm = new YApm(this, true);
    	if( ! fApm->hasBatteries()) {
    		delete fApm;
    		fApm = 0;
    	}
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
        }
    } else
        fCollapseButton = 0;

#ifdef CONFIG_APPLET_MAILBOX
    fMailBoxStatus = 0;

    if (taskBarShowMailboxStatus) {
        char const *envMail = getenv("MAIL");
        char const *mailboxList(mailBoxPath ? mailBoxPath : envMail);
        unsigned cnt = 0;

        mstring mailboxes(mailboxList);
        mstring s(null), r(null);

        for (s = mailboxes; s.splitall(' ', &s, &r); s = r)
            if (s.nonempty())
                cnt++;

        if (cnt) {
            fMailBoxStatus = new MailBoxStatus*[cnt + 1];
            fMailBoxStatus[cnt--] = NULL;

            for (s = mailboxes; s.splitall(' ', &s, &r); s = r)
            {
                if (s.isEmpty())
                    continue;

                fMailBoxStatus[cnt--] = new MailBoxStatus(app, smActionListener, s, this);
            }
        } else if (envMail) {
            fMailBoxStatus = new MailBoxStatus*[2];
            fMailBoxStatus[0] = new MailBoxStatus(app, smActionListener, envMail, this);
            fMailBoxStatus[1] = NULL;
        } else if (getlogin()) {
            upath mbox(mstring("/var/spool/mail/", getlogin()));
            if (mbox.isReadable()) {
                fMailBoxStatus = new MailBoxStatus*[2];
                fMailBoxStatus[0] = new MailBoxStatus(app, smActionListener, mbox, this);
                fMailBoxStatus[1] = NULL;
            }
        }
    }
#endif
#ifndef NO_CONFIGURE_MENUS
    if (taskBarShowStartMenu) {
        fApplications = new ObjectButton(this, rootMenu);
        fApplications->setActionListener(this);
        fApplications->setImage(taskbarStartImage);
        fApplications->setToolTip(_("Favorite applications"));
    } else
        fApplications = 0;

    fObjectBar = new ObjectBar(this);
    if (fObjectBar) {
        upath t = app->findConfigFile("toolbar");
        if (t != null) {
            loadMenus(app, smActionListener, wmActionListener, t, fObjectBar);
        }
    }
#endif
#ifdef CONFIG_WINMENU
    if (taskBarShowWindowListMenu) {
        fWinList = new ObjectButton(this, windowListMenu);
        fWinList->setImage(taskbarWindowsImage);
        fWinList->setActionListener(this);
        fWinList->setToolTip(_("Window list menu"));
    } else
        fWinList = 0;
#endif
    if (taskBarShowShowDesktopButton) {
        fShowDesktop = new ObjectButton(this, actionShowDesktop);
        fShowDesktop->setText("__");
        fShowDesktop->setImage(taskbarShowDesktopImage);
        fShowDesktop->setActionListener(wmActionListener);
        fShowDesktop->setToolTip(_("Show Desktop"));
    }
    if (taskBarShowWorkspaces && workspaceCount > 0) {
        fWorkspaces = new WorkspacesPane(this);
    } else
        fWorkspaces = 0;
#ifdef CONFIG_ADDRESSBAR
    if (enableAddressBar)
        fAddressBar = new AddressBar(app, this);
#endif
    if (taskBarShowWindows) {
        fTasks = new TaskPane(this, this);
    } else
        fTasks = 0;
#ifdef CONFIG_TRAY
    if (taskBarShowTray) {
        fWindowTray = new TrayPane(this, this);
    } else
        fWindowTray = 0;
#endif
    YAtom trayatom("_ICEWM_INTTRAY_S", true);
    fDesktopTray = new YXTray(this, true, trayatom, this);
    fDesktopTray->relayout();
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

void TaskBar::updateLayout(int &size_w, int &size_h) {
    LayoutInfo nw;
    YArray<LayoutInfo> wlist;
    wlist.setCapacity(13);

#ifndef NO_CONFIGURE_MENUS
    nw = LayoutInfo( fApplications, true, 1, true, 0, 0, true );
    wlist.append(nw);
#endif
    nw = LayoutInfo( fShowDesktop, true, 0, true, 0, 0, true );
    wlist.append(nw);
#ifdef CONFIG_WINMENU
    nw = LayoutInfo( fWinList, true, 0, true, 0, 0, true );
    wlist.append(nw);
#endif
#ifndef NO_CONFIGURE_MENUS
    nw = LayoutInfo( fObjectBar, true, 1, true, 4, 0, true );
    wlist.append(nw);
#endif
    nw = LayoutInfo( fWorkspaces, taskBarWorkspacesLeft, taskBarDoubleHeight && taskBarWorkspacesTop, true, 4, 4, true );
    wlist.append(nw);

    nw = LayoutInfo( fCollapseButton, false, 0, true, 0, 2, true );
    wlist.append(nw);
#ifdef CONFIG_APPLET_CLOCK
    nw = LayoutInfo( fClock, false, 1, true, 2, 2, false );
    wlist.append(nw);
#endif
#ifdef CONFIG_APPLET_MAILBOX
    for (MailBoxStatus ** m(fMailBoxStatus); m && *m; ++m) {
        nw = LayoutInfo( *m, false, 1, true, 1, 1, false );
        wlist.append(nw);
    }
#endif
#ifdef CONFIG_APPLET_CPU_STATUS
    for (CPUStatus ** c(fCPUStatus); c && *c; ++c) {
        nw = LayoutInfo( *c, false, 1, true, 2, 2, false );
        wlist.append(nw);
    }
#endif
#ifdef CONFIG_APPLET_MEM_STATUS
    nw = LayoutInfo( fMEMStatus, false, 1, true, 2, 2, false );
    wlist.append(nw);
#endif
#ifdef CONFIG_APPLET_NET_STATUS
    YVec<NetStatus*>::iterator it = fNetStatus->getIterator();
    while(it.hasNext()) {
        nw = LayoutInfo( it.next(), false, 1, false, 2, 2, false );
        wlist.append(nw);
    }
#endif
#ifdef CONFIG_APPLET_APM
    nw = LayoutInfo( fApm, false, 1, true, 0, 2, false );
    wlist.append(nw);
#endif
    nw = LayoutInfo( fDesktopTray, false, 1, true, 1, 1, false );
    wlist.append(nw);
#ifdef CONFIG_TRAY
    nw = LayoutInfo( fWindowTray, false, 0, true, 1, 1, true );
    wlist.append(nw);
#endif
    const int wcount = wlist.getCount();

    int w = 0;
    int y[2] = { 0, 0 };
    int h[2] = { 0, 0 };
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
        int dx, dy, dw, dh;
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
#ifdef LITE
        if (h[0] < 16)
            h[0] = 16;
#else
        if (h[0] < YIcon::smallSize() + 8)
            h[0] = YIcon::smallSize() + 8;
#endif
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
                                      right[0] - left[0],
                                      h[0]));
            fTasks->show();
            fTasks->relayout();
        }
    }
#ifdef CONFIG_ADDRESSBAR
    if (fAddressBar) {
        int row = taskBarDoubleHeight ? 1 : 0;

        fAddressBar->setGeometry(YRect(left[row],
                                       y[row] + 2,
                                       right[row] - left[row],
                                       h[row] - 4));
        fAddressBar->raise();
        if (::showAddressBar) {
            if (taskBarDoubleHeight || !taskBarShowWindows)
                fAddressBar->show();
        }
    }
#endif

    size_w = w;
    size_h = h[0] + h[1] + 1;
}

void TaskBar::relayoutNow() {
#ifdef CONFIG_TRAY
    if (windowTrayPane())
        windowTrayPane()->relayoutNow();
#endif
    if (fNeedRelayout) {

        updateLocation();
        fNeedRelayout = false;
    }
    if (taskPane())
        taskPane()->relayoutNow();
}

void TaskBar::updateFullscreen(bool fullscreen) {
    fFullscreen = fullscreen;
    if (fFullscreen || fIsHidden)
        fEdgeTrigger->show();
    else
        fEdgeTrigger->hide();
}

void TaskBar::updateLocation() {
    int dx, dy, dw, dh;
    manager->getScreenGeometry(&dx, &dy, &dw, &dh, -1);

    int x = dx;
    int w = 0;
    int h = 0;

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

    fEdgeTrigger->setGeometry(YRect(x, by, w, 1));

    int y = taskBarAtTop ? dy : dy + dh - h;

    if (fIsHidden) {
        if (fIsMapped && getFrame())
            getFrame()->wmHide();
        else
            hide();
    } else {
        if (fIsMapped && getFrame()) {
            getFrame()->configureClient(x, y, w, h);
            getFrame()->wmShow();
        } else
            setGeometry(YRect(x, y, w, h));
    }
    if (fIsHidden || fFullscreen)
        fEdgeTrigger->show();
    else
        fEdgeTrigger->hide();

    {
        MwmHints mwm =
        { MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS
        , MWM_FUNC_MOVE // | MWM_FUNC_RESIZE
        , 0 // MWM_DECOR_BORDER | MWM_DECOR_RESIZEH
        , 0
        , 0
        };

        XChangeProperty(xapp->display(), handle(),
                        _XATOM_MWM_HINTS, _XATOM_MWM_HINTS,
                        32, PropModeReplace,
                        (unsigned char *)&mwm, sizeof(mwm)/sizeof(long)); ///!!!
        getMwmHints();
        if (getFrame())
            getFrame()->updateMwmHints();
    }
#ifdef WMSPEC_HINTS
    ///!!! fix
    updateWMHints();
#endif
}

#ifdef WMSPEC_HINTS
void TaskBar::updateWMHints() {
    int dx, dy, dw, dh;
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
#endif


void TaskBar::handleCrossing(const XCrossingEvent &crossing) {
    unsigned long last = YWindow::getLastEnterNotifySerial();
    if (crossing.serial != last &&
        (crossing.serial != last + 1 || crossing.detail != NotifyVirtual))
    {
        if (crossing.type == EnterNotify /* && crossing.mode != NotifyNormal */) {
            fEdgeTrigger->stopHide();
        } else if (crossing.type == LeaveNotify /* && crossing.mode != NotifyNormal */) {
            if (crossing.detail != NotifyInferior && !(crossing.detail == NotifyVirtual && crossing.mode == NotifyGrab) && !(crossing.detail == NotifyAncestor && crossing.mode != NotifyNormal)) {
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

void TaskBar::paint(Graphics &g, const YRect &/*r*/) {
#ifdef CONFIG_GRADIENTS
    if (taskbackPixbuf != null &&
        (fGradient == null ||
         fGradient->width() != width() ||
         fGradient->height() != height()))
    {
        int gradientHeight = height() / (1 + taskBarDoubleHeight);
        fGradient = taskbackPixbuf->scale(width(), gradientHeight);
    }
#endif

    g.setColor(getTaskBarBg());
    //g.draw3DRect(0, 0, width() - 1, height() - 1, true);

    // When TaskBarDoubleHeight=1 this draws the upper half.
#ifdef CONFIG_GRADIENTS
    if (fGradient != null)
        g.drawImage(fGradient, 0, 0, width(), height(), 0, 0);
    else
#endif
        if (taskbackPixmap != null)
            g.fillPixmap(taskbackPixmap, 0, 0, width(), height());
        else {
            int y = taskBarAtTop ? 0 : 1;
            g.fillRect(0, y, width(), height() - 1);
            if (!taskBarAtTop) {
                y++;
                g.setColor(getTaskBarBg()->brighter());
                g.drawLine(0, 0, width(), 0);
            } else {
                g.setColor(getTaskBarBg()->darker());
                g.drawLine(0, height() - 1, width(), height() - 1);
            }
        }
}

bool TaskBar::handleKey(const XKeyEvent &key) {
    return YWindow::handleKey(key);
}

void TaskBar::handleButton(const XButtonEvent &button) {
#ifdef CONFIG_WINLIST
    if ((button.type == ButtonRelease) &&
        (button.button == 1 || button.button == 3) &&
        (BUTTON_MODMASK(button.state) == Button1Mask + Button3Mask))
    {
        if (windowList)
            windowList->showFocused(button.x_root, button.y_root);
    } else
#endif
    {
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
#ifdef CONFIG_WINLIST
        if (windowList)
            windowList->showFocused(up.x_root, up.y_root);
#endif
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
#ifndef NO_CONFIGURE
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
#endif
}

void TaskBar::popupStartMenu() {
#ifndef NO_CONFIGURE_MENUS
    if (fApplications) {
        /*requestFocus();
         fApplications->requestFocus();
         fApplications->setFocus();*/
        popOut();
        fApplications->popupMenu();
    }
#endif
}

void TaskBar::popupWindowListMenu() {
#ifdef CONFIG_WINMENU
    if (fWinList) {
        popOut();
        fWinList->popupMenu();
    }
#endif
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
#if defined(GNOME1_HINTS) || defined(WMSPEC_HINTS)
            setWinLayerHint((taskBarAutoHide || fFullscreen) ? WinLayerAboveAll :
                            fIsCollapsed ? WinLayerAboveDock :
                            taskBarKeepBelow ? WinLayerBelow : WinLayerDock);
#endif
            getFrame()->setState(WinStateAllWorkspaces, WinStateAllWorkspaces);
            getFrame()->activate(true);
            updateLocation();
        }
    }
}

void TaskBar::actionPerformed(YAction *action, unsigned int modifiers) {
    wmActionListener->actionPerformed(action, modifiers);
}

void TaskBar::handleCollapseButton() {
    fIsCollapsed = !fIsCollapsed;
    if (fCollapseButton) {
        fCollapseButton->setText(fIsCollapsed ? "<": ">");
        fCollapseButton->setImage(fIsCollapsed ? taskbarExpandImage : taskbarCollapseImage);
    }

    relayout();
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
#ifdef CONFIG_TRAY
    if (windowTrayPane())
        windowTrayPane()->removeApp(w);
#endif
}

TrayApp *TaskBar::addTrayApp(YFrameWindow *w) {
#ifdef CONFIG_TRAY
    if (windowTrayPane())
        return windowTrayPane()->addApp(w);
    else
#endif
        return 0;
}

void TaskBar::relayoutTray() {
#ifdef CONFIG_TRAY
    if (windowTrayPane())
        windowTrayPane()->relayout();
#endif
}

void TaskBar::showAddressBar() {
    popOut();
#ifdef CONFIG_ADDRESSBAR
    if (fAddressBar != 0)
        fAddressBar->showNow();
#endif
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
        fDesktopTray->trayRequestDock(w);
        return true;
    }
    return false;
}

#endif
