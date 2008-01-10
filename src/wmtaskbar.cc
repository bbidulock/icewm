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
#include "yparse.h"

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
    if (crossing.serial != ignore_enternotify_hack && crossing.serial != ignore_enternotify_hack + 1)
    {
    if (crossing.type == EnterNotify /* && crossing.mode != NotifyNormal */) {
        fDoShow = true;
        if (fAutoHideTimer)
            fAutoHideTimer->startTimer(autoShowDelay);
    } else if (crossing.type == LeaveNotify /* && crossing.mode != NotifyNormal */) {
    }
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

    fWindowTray = 0;
    fDesktopTray = 0;
    fApplications = 0;
    fWinList = 0;
    fTasks = 0;
#if 0
    fCollapseButton = 0;
#endif
    fWorkspaces = 0;

    fLayout = 0;

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
    taskBar->showBar(true);
}

TaskBar::~TaskBar() {
    detachDesktopTray();
    delete fEdgeTrigger; fEdgeTrigger = 0;
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
                taskBarMenu->addItem(_("_Logout..."), -2, null, actionLogout);
        }
    }
}

YWindow *TaskBar::initApplet(YLayout *object_layout, ref<YElement> applet) {
    YWindow *o = 0;
    ref<YNode> n = applet->firstChild();
    if (n != null) {
        {
            ref<YElement> cpu_status = n->toElement("cpu_status");
            if (cpu_status != null) {
                o = new CPUStatus(this,
                                  cpustatusShowRamUsage,
                                  cpustatusShowSwapUsage,
                                  cpustatusShowAcpiTemp,
                                  cpustatusShowCpuFreq);
            }
        }
        {
            ref<YElement> net_status = n->toElement("net_status");
            if (net_status != null) {
                YLayout *nested = new YLayout();
                object_layout->nested.append(nested);
                nested->spacing = 1;

                mstring networkDevices(netDevice);
                mstring s(null), r(null);

                for (s = networkDevices; s.splitall(' ', &s, &r); s = r) {
                    YLayout *single = new YLayout();
                    nested->nested.append(single);
                    o = new NetStatus(s, this, this);
                    single->window = o;
                }
                o = 0;
            }
        }

        {
            ref<YElement> clock = n->toElement("clock");
            if (clock != null) {
                o = new YClock(this);
            }
        }

        {
            ref<YElement> power_status = n->toElement("power_status");
            if (power_status != null) {
                o = new YApm(this);
            }
        }

        {
            ref<YElement> mailbox_status = n->toElement("mailbox_status");
            if (mailbox_status != null) {
                YLayout *nested = new YLayout();
                object_layout->nested.append(nested);
                nested->spacing = 1;

                mstring mailboxes(mailBoxPath);

                if (mailboxes == null) {
                    mailboxes = getenv("MAIL");
                }

                if (mailboxes == null) {
                    upath mbox = upath("/var/spool/mail/").child(getlogin());
                    mailboxes = mbox.path();
                }

                mstring s(null), r(null);

                for (s = mailboxes; s.splitall(' ', &s, &r); s = r) {
                    YLayout *single = new YLayout();
                    nested->nested.append(single);
                    msg("mailbox: %s", cstring(s).c_str());
                    o = new MailBoxStatus(s, this);
                    o->show();
                    single->window = o;
                }
                o = 0;
            }
        }

        {
            ref<YElement> start_menu = n->toElement("start_menu");
            if (start_menu != null) {
                fApplications = new ObjectButton(this, rootMenu);
                fApplications->setActionListener(this);
                fApplications->setImage(startImage);
                fApplications->setToolTip(_("Favorite applications"));
                o = fApplications;
            }
        }

        {
            ref<YElement> windows_menu = n->toElement("windows_menu");
            if (windows_menu != null) {
                fWinList = new ObjectButton(this, windowListMenu);
                fWinList->setImage(windowsImage);
                fWinList->setActionListener(this);
                fWinList->setToolTip(_("Window list menu"));
                o = fWinList;
            }
        }

        {
            ref<YElement> toolbar = n->toElement("toolbar");
            if (toolbar != null) {
                ObjectBar *fObjectBar = new ObjectBar(this);
                if (fObjectBar) {
                    upath t = YApplication::findConfigFile("toolbar");
                    if (t != null)
                        loadMenus(t, fObjectBar);
                }
                o = fObjectBar;
            }
        }

        {
            ref<YElement> addressbar = n->toElement("addressbar");
            if (addressbar != null) {
                AddressBar *fAddressBar = new AddressBar(this);
                o = fAddressBar;
            }
        }

        {
            ref<YElement> showdesktop_button = n->toElement("showdesktop_button");
            if (showdesktop_button != null) {
                ObjectButton *fShowDesktop = new ObjectButton(this, actionShowDesktop);
                fShowDesktop->setText("__");
                fShowDesktop->setImage(showDesktopImage);
                fShowDesktop->setActionListener(wmapp);
                fShowDesktop->setToolTip(_("Show Desktop"));
                o = fShowDesktop;
            }
        }
#if 0
        {

            ref<YElement> collapse_button = n->toElement("collapse_button");
            if (collapse_button != null) {
                fCollapseButton = new YButton(this, actionCollapseTaskbar);
                if (fCollapseButton) {
                    fCollapseButton->setText(">");
                    fCollapseButton->setImage(collapseImage);
                    fCollapseButton->setActionListener(this);
                }
                o = fCollapseButton;
            }
        }
#endif

        {
            ref<YElement> tasks = n->toElement("tasks");
            if (tasks != null) {
                fTasks = new TaskPane(this, this);
                o = fTasks;
            }
        }

        {
            ref<YElement> workspaces = n->toElement("workspaces");
            if (workspaces != null) {
                fWorkspaces = new WorkspacesPane(this);
                o = fWorkspaces;
            }
        }

        {
            ref<YElement> window_tray = n->toElement("window_tray");
            if (window_tray != null) {
                fWindowTray = new TrayPane(this, this);
                o = fWindowTray;
            }
        }
        {
            ref<YElement> desktop_tray = n->toElement("desktop_tray");
            if (desktop_tray != null) {
                char trayatom[64];
                sprintf(trayatom,"_ICEWM_INTTRAY_S%d", xapp->screen());
                fDesktopTray = new YXTray(this, true, trayatom, this);
                fDesktopTray->relayout();
                o = fDesktopTray;
            }
        }
    }
    object_layout->window = o;
    return o;
}

void TaskBar::loadTaskbar(ref<YElement> element, YLayout *layout) {
    ref<YNode> n = element->firstChild();
    while (n != null) {
        ref<YElement> box = n->toElement("box");
        ref<YElement> applet = n->toElement("applet");
        ref<YElement> object;

        YLayout *object_layout = new YLayout();

        if (box != null) {
            object = box;
        }

        if (applet != null) {
            object = applet;
        }

        if (object != null) {
            mstring end = object->getAttribute("end");
            mstring expand = object->getAttribute("expand");
            mstring fill = object->getAttribute("fill");
            mstring vertical = object->getAttribute("vertical");
            mstring spacing = object->getAttribute("spacing");
            mstring visible = object->getAttribute("visible");

            if (end.equals("1") || end.equals("true"))
                object_layout->end = true;
            if (expand.equals("1") || expand.equals("true"))
                object_layout->expand = true;
            if (fill.equals("1") || fill.equals("true"))
                object_layout->fill = true;
            if (vertical.equals("1") || vertical.equals("true"))
                object_layout->vertical = true;
            if (visible.equals("0") || visible.equals("false"))
                object_layout->visible = false;
            if (spacing != null) {
                object_layout->spacing = atoi(cstring(spacing).c_str());
            }
            layout->nested.append(object_layout);
        }
        if (box != null) {
            loadTaskbar(box, object_layout);
        }
        if (applet != null) {
            initApplet(object_layout, applet);
        }

        if (object_layout->window != 0) {
            if (object_layout->visible)
                object_layout->window->show();
        }

        n = n->next();
    }
}

void TaskBar::initApplets() {
    upath configFile = YApplication::findConfigFile(mstring("taskbar2"));

    fLayout = new YLayout();

    msg("applets %s", cstring(configFile.path()).c_str());
    ref<YDocument> configuration = YDocument::loadFile(configFile.path());
    if (configFile != null) {
        if (configuration != null) {
            ref<YNode> m = configuration->firstChild();
            while (m != null) {
                ref<YElement> taskbar = m->toElement("taskbar");
                if (taskbar != null) {
                    loadTaskbar(taskbar, fLayout);
                }
                m = m->next();
            }
        }
    }
}

void TaskBar::layoutSize(YLayout *layout, int &size_w, int &size_h) {
    size_w = 0;
    size_h = 0;

    if (layout->window != 0) {
        YWindow *w = layout->window;
        if (w->visible()) {
            size_w = w->width();
            size_h = w->height();
        }
    } else {
        for (unsigned int i = 0; i < layout->nested.getCount(); i++) {
            YLayout *l = layout->nested[i];

            int w, h;
            layoutSize(l, w, h);
            if (w == 0 && h == 0)
                continue;

            if (layout->vertical) {
                if (l->expand)
                    h = -1;
                if (size_w != -1) {
                    if (w > size_w || w == -1)
                        size_w = w;
                }
                if (size_h != -1) {
                    if (h == -1)
                        size_h = -1;
                    else
                        size_h += h;
                }
            } else {
                if (l->expand)
                    w = -1;
                if (size_h != -1) {
                    if (h > size_h || h == -1)
                        size_h = h;
                }
                if (size_w != -1) {
                    if (w == -1)
                        size_w = -1;
                    else
                        size_w += w;
                }
            }
            if (i > 0) {
                if (layout->vertical) {
                    if (size_h != -1)
                        size_h += layout->spacing;
                } else {
                    if (size_w != -1)
                        size_w += layout->spacing;
                }
            }

        }
    }
    layout->w = size_w;
    layout->h = size_h;
}

void TaskBar::layoutPosition(YLayout *layout, int x, int y, int w, int h) {
    if (layout->window != 0) {
        layout->window->setGeometry(YRect(x, y, w, h));
    } else {
        if (layout->vertical) {
            int ytop = y;
            int ybottom = y + h;

            msg("top-bottom: %d %d", ytop, ybottom);
            for (unsigned int i = 0; i < layout->nested.getCount(); i++) {
                YLayout *l = layout->nested[i];

                if (l->h == -1 || l->expand)
                    continue;

                int wx = x;
                int wy = 0;
                int ww = w;
                int wh = l->h;

                if (l->end) {
                    wy = ybottom - wh;
                    ybottom -= wh + layout->spacing;
                } else {
                    wy = ytop;
                    ytop += wh + layout->spacing;
                }
                layoutPosition(l, wx, wy, ww, wh);
                msg("vbox: %d %d %d %d", wx, wy, ww, wh);
            }

            for (unsigned int i = 0; i < layout->nested.getCount(); i++) {
                YLayout *l = layout->nested[i];

                if (l->h != -1 && !l->expand)
                    continue;

                int wx = x;
                int wy = ytop;
                int ww = w;
                int wh = ybottom - ytop;

                layoutPosition(l, wx, wy, ww, wh);
                msg("vbox expand %d %d %d %d", wx, wy, ww, wh);
            }
        }
        else {
            int xleft = x;
            int xright = x + w;

            msg("left-right: %d %d", xleft, xright);
            for (unsigned int i = 0; i < layout->nested.getCount(); i++) {
                YLayout *l = layout->nested[i];

                if (l->w == -1 || l->expand)
                    continue;

                int wx = 0;
                int wy = y;
                int ww = l->w;
                int wh = h;

                if (l->end) {
                    wx = xright - ww;
                    xright -= ww + layout->spacing;
                } else {
                    wx = xleft;
                    xleft += ww + layout->spacing;
                }
                layoutPosition(l, wx, wy, ww, wh);
                msg("hbox: %d %d %d %d", wx, wy, ww, wh);
            }

            for (unsigned int i = 0; i < layout->nested.getCount(); i++) {
                YLayout *l = layout->nested[i];

                if (l->w != -1 && !l->expand)
                    continue;

                int wx = xleft;
                int wy = y;
                int ww = xright - xleft;
                int wh = h;

                layoutPosition(l, wx, wy, ww, wh);
                msg("hbox expand %d %d %d %d", wx, wy, ww, wh);
            }
        }
    }
}

void TaskBar::trayChanged() {
    relayout();
    //    updateLayout();
}

void TaskBar::relayoutNow() {
#ifdef CONFIG_TRAY
    if (fWindowTray) {
        fWindowTray->relayoutNow();
    }
#endif
    if (fNeedRelayout) {
        updateLocation();
        fNeedRelayout = false;
    }
    if (fTasks)
        fTasks->relayoutNow();
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
    manager->getScreenGeometry(&dx, &dy, &dw, &dh, 0);

    int tx = dx;
    int ty = dy;
    int tw = 0;
    int th = 0;

    layoutSize(fLayout, tw, th);
    msg("size: %d %d", tw, th);

    if (tw == -1)
        tw = dw;
    if (th == -1)
        th = dh;

    msg("size: %d %d", tw, th);
    layoutPosition(fLayout, 0, 0, tw, th);

    if (fTasks)
        fTasks->relayoutNow();

#if 0
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
#endif

    //if (fIsHidden) {
    //    h = 1;
    //    y = taskBarAtTop ? dy : dy + dh - 1;
    //} else {
        ty = taskBarAtTop ? dy : dy + dh - th;
    //}

    int by = taskBarAtTop ? dy : dy + dh - 1;

    fEdgeTrigger->setGeometry(YRect(tx, by, tw, 1));

    ty = taskBarAtTop ? dy : dy + dh - th;

    if (fIsHidden) {
        if (fIsMapped && getFrame())
            getFrame()->wmHide();
        else
            hide();
    } else {
        if (fIsMapped && getFrame()) {
            getFrame()->configureClient(tx, ty, tw, th);
            getFrame()->wmShow();
        } else
            setGeometry(YRect(tx, ty, tw, th));
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
        getFrame()->updateNetWMStrut();
}


void TaskBar::handleCrossing(const XCrossingEvent &crossing) {
    if (crossing.serial != ignore_enternotify_hack && crossing.serial != ignore_enternotify_hack + 1)
    {
        if (crossing.type == EnterNotify /* && crossing.mode != NotifyNormal */) {
            fEdgeTrigger->stopHide();
        } else if (crossing.type == LeaveNotify /* && crossing.mode != NotifyNormal */) {
            if (crossing.detail != NotifyInferior) {
                fEdgeTrigger->startHide();
            }
        }
    }
}


void TaskBar::handleEndPopup(YPopupWindow *popup) {
    if (!hasPopup()) {
        fEdgeTrigger->startHide();
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
        if (button.type == ButtonPress) {
            manager->updateWorkArea();
            if (button.button == 1) {
                if (button.state & xapp->AltMask)
                    lower();
                else if (!(button.state & ControlMask))
                    raise();
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
    } else if (up.button == 3 && count == 1 && IS_BUTTON(up.state, Button3Mask)) {
        contextMenu(up.x_root, up.y_root);
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
    if (fFullscreen && doShow) {
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
        if (fEdgeTrigger)
            fEdgeTrigger->startHide();
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
#if 0
    fIsCollapsed = !fIsCollapsed;
    if (fCollapseButton) {
        fCollapseButton->setText(fIsCollapsed ? "<": ">");
        fCollapseButton->setImage(fIsCollapsed ? expandImage : collapseImage);
    }
#endif

    relayout();
}

void TaskBar::handlePopDown(YPopupWindow */*popup*/) {
}

void TaskBar::configure(const YRect &r, const bool resized) {
    YWindow::configure(r, resized);
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
        fDesktopTray->trayRequestDock(w);
        return true;
    }
    return false;
}

#endif
