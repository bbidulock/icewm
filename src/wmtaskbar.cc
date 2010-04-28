/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 *
 * TaskBar
 */

#include "config.h"

#ifdef CONFIG_TASKBAR
#include "ypixbuf.h"
#include "yfull.h"
#include "ypaint.h"
#include "wmtaskbar.h"
#include "yprefs.h"

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

#include "aapm.h"
#include "upath.h"

#include "intl.h"

#ifdef CONFIG_TRAY
YTimer *TrayApp::fRaiseTimer(NULL);
#endif
YTimer *WorkspaceButton::fRaiseTimer(NULL);

TaskBar *taskBar = 0;

YColor *taskBarBg = 0;

static ref<YImage> startImage;
static ref<YImage> windowsImage;
static ref<YImage> showDesktopImage;
static ref<YImage> collapseImage;
static ref<YImage> expandImage;

/// TODO #warning "these should be static/elsewhere"
ref<YPixmap> taskbackPixmap;
#ifdef CONFIG_GRADIENTS
ref<YImage> taskbackPixbuf;
ref<YImage> taskbuttonPixbuf;
ref<YImage> taskbuttonactivePixbuf;
ref<YImage> taskbuttonminimizedPixbuf;
#endif

static void initPixmaps() {
    upath base("taskbar");
    ref<YResourcePaths> themedirs = YResourcePaths::subdirs(base, true);
    ref<YResourcePaths> subdirs = YResourcePaths::subdirs(base);

    /*
     * that sucks, a neccessary workaround for differering startmenu pixmap
     * filename. This will be unified and be a forced standard in
     * icewm-2
     */
    startImage = themedirs->loadImage(base, "start.xpm");
#if 1
    if (startImage == null || !startImage->valid())
        startImage = themedirs->loadImage(base, "linux.xpm");
    if (startImage == null || !startImage->valid())
        startImage = themedirs->loadImage(base, "icewm.xpm");
    if (startImage == null || !startImage->valid())
        startImage = subdirs->loadImage(base, "icewm.xpm");
    if (startImage == null || !startImage->valid())
        startImage = subdirs->loadImage(base, "start.xpm");
#endif

    windowsImage = subdirs->loadImage(base, "windows.xpm");
    showDesktopImage = subdirs->loadImage(base, "desktop.xpm");
    collapseImage = subdirs->loadImage(base, "collapse.xpm");
    expandImage = subdirs->loadImage(base, "expand.xpm");

#ifdef CONFIG_GRADIENTS
    if (taskbackPixbuf == null)
        taskbackPixmap = subdirs->loadPixmap(base, "taskbarbg.xpm");
    if (taskbuttonPixbuf == null)
        taskbuttonPixmap = subdirs->loadPixmap(base, "taskbuttonbg.xpm");
    if (taskbuttonactivePixbuf == null)
        taskbuttonactivePixmap = subdirs->loadPixmap(base, "taskbuttonactive.xpm");
    if (taskbuttonminimizedPixbuf == null)
        taskbuttonminimizedPixmap = subdirs->loadPixmap(base, "taskbuttonminimized.xpm");
#else
    taskbackPixmap = subdirs->loadPixmap(base, "taskbarbg.xpm");
    taskbuttonPixmap = subdirs->loadPixmap(base, "taskbuttonbg.xpm");
    taskbuttonactivePixmap = subdirs->loadPixmap(base, "taskbuttonactive.xpm");
    taskbuttonminimizedPixmap = subdirs->loadPixmap(base, "taskbuttonminimized.xpm");
#endif

#ifdef CONFIG_APPLET_MAILBOX
    base = "mailbox/";
    subdirs = YResourcePaths::subdirs(base);
    mailPixmap = subdirs->loadPixmap(base, "mail.xpm");
    noMailPixmap = subdirs->loadPixmap(base, "nomail.xpm");
    errMailPixmap = subdirs->loadPixmap(base, "errmail.xpm");
    unreadMailPixmap = subdirs->loadPixmap(base, "unreadmail.xpm");
    newMailPixmap = subdirs->loadPixmap(base, "newmail.xpm");
#endif

#ifdef CONFIG_APPLET_CLOCK
    base = "ledclock/";
    subdirs = YResourcePaths::subdirs(base);
    PixNum[0] = subdirs->loadPixmap(base, "n0.xpm");
    PixNum[1] = subdirs->loadPixmap(base, "n1.xpm");
    PixNum[2] = subdirs->loadPixmap(base, "n2.xpm");
    PixNum[3] = subdirs->loadPixmap(base, "n3.xpm");
    PixNum[4] = subdirs->loadPixmap(base, "n4.xpm");
    PixNum[5] = subdirs->loadPixmap(base, "n5.xpm");
    PixNum[6] = subdirs->loadPixmap(base, "n6.xpm");
    PixNum[7] = subdirs->loadPixmap(base, "n7.xpm");
    PixNum[8] = subdirs->loadPixmap(base, "n8.xpm");
    PixNum[9] = subdirs->loadPixmap(base, "n9.xpm");
    PixSpace = subdirs->loadPixmap(base, "space.xpm");
    PixColon = subdirs->loadPixmap(base, "colon.xpm");
    PixSlash = subdirs->loadPixmap(base, "slash.xpm");
    PixDot = subdirs->loadPixmap(base, "dot.xpm");
    PixA = subdirs->loadPixmap(base, "a.xpm");
    PixP = subdirs->loadPixmap(base, "p.xpm");
    PixM = subdirs->loadPixmap(base, "m.xpm");
    PixPercent = subdirs->loadPixmap(base, "percent.xpm");
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

extern unsigned int ignore_enternotify_hack;

void EdgeTrigger::handleCrossing(const XCrossingEvent &crossing) {
    if (crossing.type == EnterNotify /* && crossing.mode != NotifyNormal */) {
        if (crossing.serial != ignore_enternotify_hack && crossing.serial != ignore_enternotify_hack + 1)
        {
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

TaskBar::TaskBar(YWindow *aParent):
#if 1
YFrameClient(aParent, 0) INIT_GRADIENT(fGradient, NULL)
#else
    YWindow(aParent) INIT_GRADIENT(fGradient, NULL)
#endif
{
    taskBar = this;
    fIsMapped = false;
    fIsHidden = taskBarAutoHide;
    fIsCollapsed = false;
    fFullscreen = false;
    fMenuShown = false;
    fNeedRelayout = false;
    fAddressBar = 0;
    fShowDesktop = 0;

    if (taskBarBg == 0) {
        taskBarBg = new YColor(clrDefaultTaskBar);
    }

    ///setToplevel(true);

    initPixmaps();

#if 1
    setWindowTitle(_("Task Bar"));
    setIconTitle(_("Task Bar"));
    setClassHint("icewm", "TaskBar");
    setWinStateHint(WinStateAllWorkspaces, WinStateAllWorkspaces);
    //!!!setWinStateHint(WinStateDockHorizontal, WinStateDockHorizontal);

    setWinHintsHint(WinHintsSkipFocus |
                    WinHintsSkipWindowMenu |
                    WinHintsSkipTaskBar);

    setWinWorkspaceHint(0);
    setWinLayerHint((taskBarAutoHide || fFullscreen) ? WinLayerAboveAll :
                    fIsCollapsed ? WinLayerAboveDock :
                    taskBarKeepBelow ? WinLayerBelow : WinLayerDock);

    {
        XWMHints wmh;

        memset(&wmh, 0, sizeof(wmh));
        wmh.flags = InputHint;
        wmh.input = False;
        //wmh.

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
        MwmHints mwm;

        memset(&mwm, 0, sizeof(mwm));
        mwm.flags =
            MWM_HINTS_FUNCTIONS |
            MWM_HINTS_DECORATIONS;
        mwm.functions =
            MWM_FUNC_MOVE /*|
            MWM_FUNC_RESIZE*/;
        mwm.decorations = 0;
        //MWM_DECOR_BORDER /*|MWM_DECOR_RESIZEH*/;

        setMwmHints(mwm);
    }
#else
    setStyle(wsOverrideRedirect);
#endif
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
    delete fApplications; fApplications = 0;
#ifdef CONFIG_WINMENU
    delete fWinList; fWinList = 0;
#endif
#ifndef NO_CONFIGURE_MENUS
    delete fObjectBar; fObjectBar = 0;
#endif
    delete fWorkspaces;
    taskbackPixmap = null;
    taskbuttonPixmap = null;
    taskbuttonactivePixmap = null;
    taskbuttonminimizedPixmap = null;
#ifdef CONFIG_GRADIENT
    taskbackPixbuf = null;
    taskbuttonPixbuf = null;
    taskbuttonactivePixbuf = null;
    taskbuttonminimizedPixbuf = null;
    delete fGradient;
#endif
    startImage = null;
    windowsImage = null;
    showDesktopImage = null;;
#ifdef CONFIG_APPLET_MAILBOX
    mailPixmap = null;
    noMailPixmap = null;
    errMailPixmap = null;
    unreadMailPixmap = null;
    newMailPixmap = null;
#endif
#ifdef CONFIG_APPLET_CLOCK
    PixSpace = null;
    PixSlash = null;
    PixDot = null;
    PixA = null;
    PixP = null;
    PixM = null;
    PixColon = null;
    for (int n = 0; n < 10; n++)
        PixNum[n] = null;
#endif
#ifdef CONFIG_APPLET_APM
    delete fApm; fApm = 0;
#endif
#ifdef HAVE_NET_STATUS
    delete [] fNetStatus;
#endif
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
#ifdef CONFIG_APPLET_CPU_STATUS
    if (taskBarShowCPUStatus)
        fCPUStatus = new CPUStatus(this, cpustatusShowRamUsage, cpustatusShowSwapUsage,
																	 cpustatusShowAcpiTemp, cpustatusShowCpuFreq);
    else
        fCPUStatus = 0;
#endif
#ifdef CONFIG_APPLET_NET_STATUS
    fNetStatus = 0;
#ifdef HAVE_NET_STATUS
    if (taskBarShowNetStatus && netDevice) {
        mstring networkDevices(netDevice);
        mstring s(null), r(null);
        int cnt = 0;

        for (s = networkDevices; s.splitall(' ', &s, &r); s = r)
            cnt++;

        networkDevices = netDevice;

        if (cnt) {
            fNetStatus = new NetStatus*[cnt + 1];
            fNetStatus[cnt--] = NULL;

            for (s = networkDevices; s.splitall(' ', &s, &r); s = r) {
                fNetStatus[cnt--] = new NetStatus(s, this, this);
            }
        }
    }
#endif
#endif
#ifdef CONFIG_APPLET_CLOCK
    if (taskBarShowClock) {
        fClock = new YClock(this);
    } else
        fClock = 0;
#endif
#ifdef CONFIG_APPLET_APM
    if (taskBarShowApm && (access(APMDEV, 0) == 0 ||
                           access("/proc/acpi", 0) == 0 ||
                           access("/dev/acpi", 0) == 0 ||
	                   access("/proc/pmu", R_OK|X_OK) == 0))
    {
        fApm = new YApm(this);
    } else
        fApm = 0;
#endif

    if (taskBarShowCollapseButton) {
        fCollapseButton = new YButton(this, actionCollapseTaskbar);
        if (fCollapseButton) {
            fCollapseButton->setText(">");
            fCollapseButton->setImage(collapseImage);
            fCollapseButton->setActionListener(this);
        }
    } else
        fCollapseButton = 0;

#ifdef CONFIG_APPLET_MAILBOX
    fMailBoxStatus = 0;

    if (taskBarShowMailboxStatus) {
        char const * mailboxList(mailBoxPath ? mailBoxPath : getenv("MAIL"));
        unsigned cnt = 0;

        mstring mailboxes(mailboxList);
        mstring s(null), r(null);

        for (s = mailboxes; s.splitall(' ', &s, &r); s = r)
            cnt++;

        if (cnt) {
            fMailBoxStatus = new MailBoxStatus*[cnt + 1];
            fMailBoxStatus[cnt--] = NULL;

            for (s = mailboxes; s.splitall(' ', &s, &r); s = r)
            {
                fMailBoxStatus[cnt--] = new MailBoxStatus(s, this);
            }
        } else if (getenv("MAIL")) {
            fMailBoxStatus = new MailBoxStatus*[2];
            fMailBoxStatus[0] = new MailBoxStatus(getenv("MAIL"), this);
            fMailBoxStatus[1] = NULL;
        } else if (getlogin()) {
            char * mbox = cstrJoin("/var/spool/mail/", getlogin(), NULL);

            if (!access(mbox, R_OK)) {
                fMailBoxStatus = new MailBoxStatus*[2];
                fMailBoxStatus[0] = new MailBoxStatus(mbox, this);
                fMailBoxStatus[1] = NULL;
            }

            delete[] mbox;
        }
    }
#endif
#ifndef NO_CONFIGURE_MENUS
    if (taskBarShowStartMenu) {
        fApplications = new ObjectButton(this, rootMenu);
        fApplications->setActionListener(this);
        fApplications->setImage(startImage);
        fApplications->setToolTip(_("Favorite applications"));
    } else
        fApplications = 0;

    fObjectBar = new ObjectBar(this);
    if (fObjectBar) {
        upath t = app->findConfigFile("toolbar");
        if (t != null) {
            loadMenus(t, fObjectBar);
        }
    }
#endif
#ifdef CONFIG_WINMENU
    if (taskBarShowWindowListMenu) {
        fWinList = new ObjectButton(this, windowListMenu);
        fWinList->setImage(windowsImage);
        fWinList->setActionListener(this);
        fWinList->setToolTip(_("Window list menu"));
    } else
        fWinList = 0;
#endif
    if (taskBarShowShowDesktopButton) {
        fShowDesktop = new ObjectButton(this, actionShowDesktop);
        fShowDesktop->setText("__");
        fShowDesktop->setImage(showDesktopImage);
        fShowDesktop->setActionListener(wmapp);
        fShowDesktop->setToolTip(_("Show Desktop"));
    }
    if (taskBarShowWorkspaces && workspaceCount > 0) {
        fWorkspaces = new WorkspacesPane(this);
    } else
        fWorkspaces = 0;
#ifdef CONFIG_ADDRESSBAR
    if (enableAddressBar)
        fAddressBar = new AddressBar(this);
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
    char trayatom[64];
    sprintf(trayatom,"_ICEWM_INTTRAY_S%d", xapp->screen());
    fDesktopTray = new YXTray(this, true, trayatom, this);
    fDesktopTray->relayout();
}

void TaskBar::trayChanged() {
    relayout();
    //    updateLayout();
}

void TaskBar::updateLayout(int &size_w, int &size_h) {
    struct {
        YWindow *w;
        bool left;
        int row; // 0 = bottom, 1 = top
        bool show;
        int pre, post;
        bool expand;
    } *wl, wlist[] = {
#ifndef NO_CONFIGURE_MENUS
        { fApplications, true, 1, true, 0, 0, true },
#endif
        { fShowDesktop, true, 0, true, 0, 0, true },
#ifdef CONFIG_WINMENU
        { fWinList, true, 0, true, 0, 0, true},
#endif
#ifndef NO_CONFIGURE_MENUS
        { fObjectBar, true, 1, true, 4, 0, true },
#endif
        { fWorkspaces, taskBarWorkspacesLeft, 0, true, 4, 4, true },

        { fCollapseButton, false, 0, true, 0, 2, true },
#ifdef CONFIG_APPLET_CLOCK
        { fClock, false, 1, true, 2, 2, false },
#endif
#ifdef CONFIG_APPLET_MAILBOX
        { fMailBoxStatus ? fMailBoxStatus[0] : 0, false, 1, true, 1, 1, false },
/// TODO #warning "a hack"
        { fMailBoxStatus && fMailBoxStatus[0] ? fMailBoxStatus[1] : 0, false, 1, true, 1, 1, false },
        { fMailBoxStatus && fMailBoxStatus[0] && fMailBoxStatus[1] ? fMailBoxStatus[2] : 0, false, 1, true, 1, 1, false },
        { fMailBoxStatus && fMailBoxStatus[0] && fMailBoxStatus[1] && fMailBoxStatus[2] ? fMailBoxStatus[3] : 0, false, 1, true, 1, 1, false },
        { fMailBoxStatus && fMailBoxStatus[0] && fMailBoxStatus[1] && fMailBoxStatus[2] && fMailBoxStatus[3] ? fMailBoxStatus[4] : 0, false, 1, true, 1, 1, false },
        { fMailBoxStatus && fMailBoxStatus[0] && fMailBoxStatus[1] && fMailBoxStatus[2] && fMailBoxStatus[3] && fMailBoxStatus[4] ? fMailBoxStatus[5] : 0, false, 1, true, 1, 1, false },
#endif
#ifdef CONFIG_APPLET_CPU_STATUS
        { fCPUStatus, false, 1, true, 2, 2, false },
#endif
#ifdef CONFIG_APPLET_NET_STATUS
#ifdef CONFIG_APPLET_MAILBOX
        { fNetStatus ? fNetStatus[0] : 0, false, 1, false, 1, 1, false },
/// TODO #warning "a hack"
        { fNetStatus && fNetStatus[0] ? fNetStatus[1] : 0, false, 1, false, 1, 1, false },
        { fNetStatus && fNetStatus[0] && fNetStatus[1] ? fNetStatus[2] : 0, false, 1, false, 1, 1, false },
#endif
#endif
#ifdef CONFIG_APPLET_APM
        { fApm, false, 1, true, 0, 2, false },
#endif
        { fDesktopTray, false, 1, true, 1, 1, false },
#ifdef CONFIG_TRAY
        { fWindowTray, false, 0, true, 1, 1, true },
#endif
    };
    const int wcount = sizeof(wlist)/sizeof(wlist[0]);

    int w = 0;
    int y[2] = { 0, 0 };
    int h[2] = { 0, 0 };
    int left[2] = { 0, 0 };
    int right[2] = { 0, 0 };
    int i;

    for (i = 0; wl = wlist + i, i < wcount; i++) {
        if (!taskBarDoubleHeight)
            wl->row = 0;
    }

    for (i = 0; wl = wlist + i, i < wcount; i++) {
        if (wl->w == 0)
            continue;
        if (wl->w->height() > h[wl->row])
            h[wl->row] = wl->w->height();
    }

    {
        int dx, dy, dw, dh;
        manager->getScreenGeometry(&dx, &dy, &dw, &dh);
        w = dw;
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

    for (i = 0; wl = wlist + i, i < wcount; i++) {
        if (wl->w == 0)
            continue;
        if (!wl->show && !wl->w->visible())
            continue;

        int xx = 0;
        int yy = 0;
        int ww = wl->w->width();
        int hh = h[wl->row];

        if (wl->expand) {
            yy = y[wl->row];
        } else {
            hh = wl->w->height();
            yy = y[wl->row] + (h[wl->row] - wl->w->height()) / 2;
        }

        if (wl->left) {
            xx = left[wl->row] + wl->pre;

            left[wl->row] += ww + wl->pre + wl->post;
        } else {
            xx = right[wl->row] - ww - wl->pre;

            right[wl->row] -= ww + wl->pre + wl->post;
        }

        wl->w->setGeometry(YRect(xx, yy, ww, hh));
        if (wl->show)
            wl->w->show();
    }

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
    if (taskBar && taskBar->windowTrayPane())
        taskBar->windowTrayPane()->relayoutNow();
#endif
    if (fNeedRelayout) {

        updateLocation();
        fNeedRelayout = false;
    }
    if (taskBar->taskPane())
        taskBar->taskPane()->relayoutNow();
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
    int y = dy;
    int w = 0;
    int h = 0;

    updateLayout(w, h);

    if (fIsCollapsed) {
        if (fCollapseButton) {
            w = fCollapseButton->width();
            h = fCollapseButton->height();
        } else {
            w = h = 0;
        }

        x = dw - w;
        if (taskBarAtTop)
            y = 0;
        else
            y = dh - h;

        if (fCollapseButton) {
            fCollapseButton->show();
            fCollapseButton->raise();
            fCollapseButton->setPosition(0, 0);
        }
    }

    //if (fIsHidden) {
    //    h = 1;
    //    y = taskBarAtTop ? dy : dy + dh - 1;
    //} else {
        y = taskBarAtTop ? dy : dy + dh - h;
    //}

    int by = taskBarAtTop ? dy : dy + dh - 1;

    fEdgeTrigger->setGeometry(YRect(x, by, w, 1));

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

/// TODO #warning "optimize this"
    {
        MwmHints mwm;

        memset(&mwm, 0, sizeof(mwm));
        mwm.flags =
            MWM_HINTS_FUNCTIONS |
            MWM_HINTS_DECORATIONS;
        mwm.functions =
            MWM_FUNC_MOVE /*|
            MWM_FUNC_RESIZE*/;
        if (fIsHidden)
            mwm.decorations = 0;
        else
            mwm.decorations = 0;
        //MWM_DECOR_BORDER /*|
        //MWM_DECOR_RESIZEH*/;

        XChangeProperty(xapp->display(), handle(),
                        _XATOM_MWM_HINTS, _XATOM_MWM_HINTS,
                        32, PropModeReplace,
                        (unsigned char *)&mwm, sizeof(mwm)/sizeof(long)); ///!!!
        getMwmHints();
        if (getFrame())
            getFrame()->updateMwmHints();
    }
    ///!!! fix
    updateWMHints();
}

void TaskBar::updateWMHints() {
    int dx, dy, dw, dh;
    manager->getScreenGeometry(&dx, &dy, &dw, &dh);

    long wk[4] = { 0, 0, 0, 0 };
    if (!taskBarAutoHide && !fIsCollapsed && getFrame()) {
        if (taskBarAtTop)
            wk[2] = getFrame()->y() + getFrame()->height();
        else
            wk[3] = dh - getFrame()->y();
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
    if (crossing.serial != ignore_enternotify_hack && crossing.serial != ignore_enternotify_hack + 1)
    {
        if (crossing.type == EnterNotify /* && crossing.mode != NotifyNormal */) {
            fEdgeTrigger->stopHide();
        } else if (crossing.type == LeaveNotify /* && crossing.mode != NotifyNormal */) {
            if (crossing.detail != NotifyInferior && crossing.detail != NotifyVirtual && crossing.detail != NotifyAncestor) {
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
        !(fGradient != null &&
          fGradient->width() == width() &&
          fGradient->height() == height()))
    {
        fGradient = taskbackPixbuf->scale(width(), height());
    }
#endif

    g.setColor(taskBarBg);
    //g.draw3DRect(0, 0, width() - 1, height() - 1, true);

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
                g.setColor(taskBarBg->brighter());
                g.drawLine(0, 0, width(), 0);
            } else {
                g.setColor(taskBarBg->darker());
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
    if (fApplications) {
        /*requestFocus();
         fApplications->requestFocus();
         fApplications->setFocus();*/
        popOut();
        fApplications->popupMenu();
    }
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
            setWinLayerHint((taskBarAutoHide || fFullscreen) ? WinLayerAboveAll :
                            fIsCollapsed ? WinLayerAboveDock :
                            taskBarKeepBelow ? WinLayerBelow : WinLayerDock);
            getFrame()->setState(WinStateAllWorkspaces, WinStateAllWorkspaces);
            getFrame()->activate(true);
            updateLocation();
        }
    }
}

void TaskBar::actionPerformed(YAction *action, unsigned int modifiers) {
    wmapp->actionPerformed(action, modifiers);
}

void TaskBar::handleCollapseButton() {
    fIsCollapsed = !fIsCollapsed;
    if (fCollapseButton) {
        fCollapseButton->setText(fIsCollapsed ? "<": ">");
        fCollapseButton->setImage(fIsCollapsed ? expandImage : collapseImage);
    }

    relayout();
}

void TaskBar::handlePopDown(YPopupWindow */*popup*/) {
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
