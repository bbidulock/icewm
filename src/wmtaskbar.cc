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
#include "wmtaskbar.h"

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

#include "aapm.h"

#include "intl.h"

YColor *taskBarBg(NULL);

#ifdef CONFIG_TRAY
YTimer *TrayApp::fRaiseTimer(NULL);
#endif
YTimer *WorkspaceButton::fRaiseTimer(NULL);

TaskBar *taskBar(NULL);

YIcon::Image *icewmImage(NULL);
YIcon::Image *windowsImage(NULL);
YPixmap *taskbackPixmap(NULL);

#ifdef CONFIG_GRADIENTS
YPixbuf *taskbackPixbuf(NULL);
YPixbuf *taskbuttonPixbuf(NULL);
YPixbuf *taskbuttonactivePixbuf(NULL);
YPixbuf *taskbuttonminimizedPixbuf(NULL);
#endif

static void initPixmaps() {
#ifndef ICEWM_PIXMAP
#define ICEWM_PIXMAP "icewm.xpm"
#endif

#ifndef START_PIXMAP
#define START_PIXMAP "linux.xpm"
/*
#define START_PIXMAP "debian.xpm"
#define START_PIXMAP "bsd-daemon.xpm"
#define START_PIXMAP "start.xpm"
#define START_PIXMAP "xfree86os2.xpm"
*/
#endif
    YResourcePaths const paths;

    char const * base("taskbar/");
    YResourcePaths themedirs(paths, base, true);
    YResourcePaths subdirs(paths, base);

    if (NULL == (icewmImage = themedirs.loadImage(base, ICEWM_PIXMAP)) &&
        NULL == (icewmImage = themedirs.loadImage(base, START_PIXMAP)))
        icewmImage = subdirs.loadImage(base, ICEWM_PIXMAP);

    windowsImage = subdirs.loadImage(base, "windows.xpm");

#ifdef CONFIG_GRADIENTS
    if (!taskbackPixbuf)
	taskbackPixmap = subdirs.loadPixmap(base, "taskbarbg.xpm");
    if (!taskbuttonPixbuf)
	taskbuttonPixmap = subdirs.loadPixmap(base, "taskbuttonbg.xpm");
    if (!taskbuttonactivePixbuf)
	taskbuttonactivePixmap = subdirs.loadPixmap(base, "taskbuttonactive.xpm");
    if (!taskbuttonminimizedPixbuf)
	taskbuttonminimizedPixmap = subdirs.loadPixmap(base, "taskbuttonminimized.xpm");
#else
    taskbackPixmap = subdirs.loadPixmap(base, "taskbarbg.xpm");
    taskbuttonPixmap = subdirs.loadPixmap(base, "taskbuttonbg.xpm");
    taskbuttonactivePixmap = subdirs.loadPixmap(base, "taskbuttonactive.xpm");
    taskbuttonminimizedPixmap = subdirs.loadPixmap(base, "taskbuttonminimized.xpm");
#endif    

#ifdef CONFIG_APPLET_MAILBOX
    base = "mailbox/";
    subdirs.init(paths, base);
    mailPixmap = subdirs.loadPixmap(base, "mail.xpm");
    noMailPixmap = subdirs.loadPixmap(base, "nomail.xpm");
    errMailPixmap = subdirs.loadPixmap(base, "errmail.xpm");
    unreadMailPixmap = subdirs.loadPixmap(base, "unreadmail.xpm");
    newMailPixmap = subdirs.loadPixmap(base, "newmail.xpm");
#endif

#ifdef CONFIG_APPLET_CLOCK
    base = "ledclock/";
    subdirs.init(paths, base);
    PixNum[0] = subdirs.loadPixmap(base, "n0.xpm");
    PixNum[1] = subdirs.loadPixmap(base, "n1.xpm");
    PixNum[2] = subdirs.loadPixmap(base, "n2.xpm");
    PixNum[3] = subdirs.loadPixmap(base, "n3.xpm");
    PixNum[4] = subdirs.loadPixmap(base, "n4.xpm");
    PixNum[5] = subdirs.loadPixmap(base, "n5.xpm");
    PixNum[6] = subdirs.loadPixmap(base, "n6.xpm");
    PixNum[7] = subdirs.loadPixmap(base, "n7.xpm");
    PixNum[8] = subdirs.loadPixmap(base, "n8.xpm");
    PixNum[9] = subdirs.loadPixmap(base, "n9.xpm");
    PixSpace = subdirs.loadPixmap(base, "space.xpm");
    PixColon = subdirs.loadPixmap(base, "colon.xpm");
    PixSlash = subdirs.loadPixmap(base, "slash.xpm");
    PixDot = subdirs.loadPixmap(base, "dot.xpm");
    PixA = subdirs.loadPixmap(base, "a.xpm");
    PixP = subdirs.loadPixmap(base, "p.xpm");
    PixM = subdirs.loadPixmap(base, "m.xpm");
#endif
}

TaskBar::TaskBar(YWindow *aParent):
#if 1
    YFrameClient(aParent, 0) INIT_GRADIENT(fGradient, NULL)
#else
    YWindow(aParent) INIT_GRADIENT(fGradient, NULL)
#endif
{
    int ht = 25;
    fIsMapped = false;
    fIsHidden = taskBarAutoHide;
    fMenuShown = false;
    fAddressBar = 0;

    if (taskBarBg == 0) {
        taskBarBg = new YColor(clrDefaultTaskBar);
    }

    initPixmaps();
    setSize(1, ht);

#if 1
    setWindowTitle(_("Task Bar"));
    setIconTitle(_("Task Bar"));
    setClassHint("icewm", "TaskBar");
    setWinStateHint(WinStateAllWorkspaces, WinStateAllWorkspaces);
    //!!!setWinStateHint(WinStateDockHorizontal, WinStateDockHorizontal);
    setWinHintsHint(WinHintsSkipFocus |
    		    WinHintsSkipWindowMenu |
    		    WinHintsSkipTaskBar |
		    (taskBarAutoHide ? 0 : WinHintsDoNotCover));
    
    setWinWorkspaceHint(0);
    setWinLayerHint(taskBarAutoHide ? WinLayerAboveDock :
		    taskBarKeepBelow ? WinLayerBelow : WinLayerDock);

    {
        XWMHints wmh;

        memset(&wmh, 0, sizeof(wmh));
        wmh.flags = InputHint;
        wmh.input = False;
        //wmh.

        XSetWMHints(app->display(), handle(), &wmh);
        getWMHints();
    }
    { 
        long wk[4] = { 0, 0, 0, 0 };

        XChangeProperty(app->display(),
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
        mwm.decorations =
            MWM_DECOR_BORDER /*|MWM_DECOR_RESIZEH*/;

        setMwmHints(mwm);
    }
#else
    setStyle(wsOverrideRedirect);
#endif
    {
        long arg[2];
        arg[0] = NormalState;
        arg[1] = 0;
        XChangeProperty(app->display(), handle(),
                        _XA_WM_STATE, _XA_WM_STATE,
                        32, PropModeReplace,
                        (unsigned char *)arg, 2);
    }
    setPointer(YApplication::leftPointer);
    setDND(true);

    fAutoHideTimer = new YTimer(autoHideDelay);
    if (fAutoHideTimer) {
        fAutoHideTimer->setTimerListener(this);
    }

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
        taskBarMenu->addItem(_("_Refresh"), -2, 0, actionRefresh);

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
	    taskBarMenu->addItem(_("_Logout..."), -2, actionLogout, logoutMenu);
        }
    }


#ifdef CONFIG_APPLET_CPU_STATUS
#if (defined(linux) || defined(HAVE_KSTAT_H))
    if (taskBarShowCPUStatus)
        fCPUStatus = new CPUStatus(this);
    else
        fCPUStatus = 0;
#endif
#endif

#ifdef HAVE_NET_STATUS
    fNetStatus = 0;

    if (taskBarShowNetStatus && netDevice) {
	unsigned cnt(strtoken(netDevice));

	if (cnt) {
	    fNetStatus = new NetStatus*[cnt + 1];
	    fNetStatus[cnt--] = NULL;

	    for (char const * s(netDevice + strspn(netDevice, " \t"));
		 *s != '\0'; s = strnxt(s)) {
		 char const * netdev(newstr(s, " \t"));
		 fNetStatus[cnt--] = new NetStatus(netdev, this);
		 delete[] netdev;
	     }
	}
    }
#endif

    if (taskBarShowClock) {
        fClock = new YClock(this);
        if (fClock->height() > ht) ht = fClock->height();
    } else
        fClock = 0;
#ifdef CONFIG_APPLET_APM
    if (taskBarShowApm && (access("/proc/apm", 0) == 0 ||
                           access("/proc/acpi", 0) == 0))
    {
        fApm = new YApm(this);
        if (fApm->height() > ht) ht = fApm->height();
    } else
        fApm = 0;
#endif

#ifdef CONFIG_APPLET_MAILBOX
    fMailBoxStatus = 0;

    if (taskBarShowMailboxStatus) {
	char const * mailboxes(mailBoxPath ? mailBoxPath : getenv("MAIL"));
	unsigned cnt(strtoken(mailboxes));
	
	if (cnt) {
	    fMailBoxStatus = new MailBoxStatus*[cnt + 1];
            fMailBoxStatus[cnt--] = NULL;

	    for (char const * s(mailboxes + strspn(mailboxes, " \t"));
		 *s != '\0'; s = strnxt(s)) {
		char * mailbox(newstr(s, " \t"));
		fMailBoxStatus[cnt--] = new MailBoxStatus(mailbox, this);
		delete[] mailbox;
	    }
	} else if (getenv("MAIL")) {
	    fMailBoxStatus = new MailBoxStatus*[2];
            fMailBoxStatus[0] = new MailBoxStatus(getenv("MAIL"), this);
            fMailBoxStatus[1] = NULL;
	} else if (getlogin()) {
	    char * mbox = strJoin("/var/spool/mail/", getlogin(), NULL);

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
        fApplications->setImage(icewmImage);
	fApplications->setToolTip(_("Favorite applications"));
        if (fApplications->height() > ht)
            ht = fApplications->height();
    } else
        fApplications = 0;

    fObjectBar = new ObjectBar(this);
    if (fObjectBar) {
        fObjectBar->setSize(1, ht);
        char *t = app->findConfigFile("toolbar");
        if (t) {
            loadMenus(t, fObjectBar);
            delete [] t;
        }
    }
#endif

#ifdef CONFIG_WINMENU
    if (taskBarShowWindowListMenu) {
        fWinList = new ObjectButton(this, windowListMenu);
        fWinList->setImage(windowsImage);
        fWinList->setActionListener(this);
	fWinList->setToolTip(_("Window list menu"));
        if (fWinList->height() > ht) ht = fWinList->height();
    } else
        fWinList = 0;
#endif

    if (taskBarShowWorkspaces && workspaceCount > 0) {
        fWorkspaces = new WorkspacesPane(this);
    } else
        fWorkspaces = 0;

    if (taskBarDoubleHeight) {
        {
            int dx, dy, dw, dh;
            manager->getScreenGeometry(&dx, &dy, &dw, &dh);
            setSize(dw + 2, 2 * ht + 2 * BASE1);
        }

        updateLocation();

        leftX = 0;
        rightX = width() - 1;
#ifdef CONFIG_APPLET_CLOCK
        if (fClock) {
            fClock->setPosition(rightX - fClock->width(),
                                BASE1 + (ht - ADD1 - fClock->height()) / 2);
            fClock->show();
            rightX -= fClock->width() + 2;
        }
#endif
#ifdef CONFIG_APPLET_APM
        if (fApm) {
            rightX -= 2;
            fApm->setPosition(rightX - fApm->width(),
                              BASE1 + (ht - ADD1 - fApm->height()) / 2);
            fApm->show();
            rightX -= fApm->width() + 2;
        }
#endif
#ifdef CONFIG_APPLET_MAILBOX
        if (fMailBoxStatus)
	    for (MailBoxStatus ** mbox(fMailBoxStatus); *mbox; ++mbox) {
		(*mbox)->setPosition(rightX - (*mbox)->width() - 1,
                                  BASE2 + (ht - ADD2 - (*mbox)->height()) / 2);

		(*mbox)->show();
		rightX -= (*mbox)->width() + 2;
	    }
#endif
#ifdef CONFIG_APPLET_CPU_STATUS
#if (defined(linux) || defined(HAVE_KSTAT_H))
        if (fCPUStatus) {
            fCPUStatus->setPosition(rightX - fCPUStatus->width() - 1,
                                    BASE1 + (ht - ADD1 - fCPUStatus->height()) / 2);
            fCPUStatus->show();
            rightX -= fCPUStatus->width() + 2;
        }
#endif
#endif

#ifdef HAVE_NET_STATUS
        if (fNetStatus)
	    for (NetStatus ** netstat(fNetStatus); *netstat; ++netstat) {
		rightX -= 2;

		(*netstat)->setPosition(rightX - (*netstat)->width() - 1,
				  BASE1 + (ht - ADD1 - (*netstat)->height()) / 2);

		// don't do a show() here because PPPStatus takes care of it
		rightX -= (*netstat)->width() + 2;
	    }
#endif

        if (fApplications) {
            fApplications->setPosition(leftX,
                                       BASE1 + (ht - ADD1 - fApplications->height()) / 2);
            fApplications->show();
            leftX += fApplications->width();
        }
#ifdef CONFIG_WINMENU
        if (fWinList) {
            fWinList->setPosition(leftX,
                                  BASE1 + (ht - ADD1 - fWinList->height()) / 2);
            fWinList->show();
            leftX += fWinList->width() + 2;
        }
#endif
#ifndef NO_CONFIGURE_MENUS
        if (fObjectBar) {
            leftX += 2;
            fObjectBar->setPosition(leftX,
                                    BASE1 + (ht - ADD1 - fObjectBar->height()) / 2);
            fObjectBar->show();
            leftX += fObjectBar->width() + 2;
        }
#endif

#ifdef CONFIG_ADDRESSBAR
        {
            fAddressBar = new AddressBar(this);
            if (fAddressBar) {
                if (1) {
                    leftX += 2;
                    fAddressBar->setGeometry(
                        YRect(leftX,
                              BASE1 + (ht - ADD1 - fAddressBar->height()) / 2,
                              rightX - leftX - 4,
                              fAddressBar->height()));

                    fAddressBar->show();
                } else {
                    //fAddressBar->setGeometry(2, 2, width() - 4, height() - 4);
                }
            }
#endif
        }

        leftX = 0;
        rightX = width() - 1;

        if (fWorkspaces) {
            leftX += 2;
            fWorkspaces->setPosition(leftX, height() - fWorkspaces->height() - BASE1);
            leftX += 2 + fWorkspaces->width();
            fWorkspaces->show();
        }
        leftX += 4;
    } else {
        {
            int dx, dy, dw, dh;
            manager->getScreenGeometry(&dx, &dy, &dw, &dh);
            setSize(dw, ht + 2 * BASE1);
        }

        updateLocation();

        leftX = 0;
        rightX = width() - 1;
#ifdef CONFIG_APPLET_CLOCK
        if (fClock) {
            fClock->setPosition(rightX - fClock->width(),
                                BASE1 + (ht - ADD1 - fClock->height()) / 2);
            fClock->show();
            rightX -= fClock->width() + 2;
        }
#endif
#ifdef CONFIG_APPLET_MAILBOX
        if (fMailBoxStatus)
	    for (MailBoxStatus ** mbox(fMailBoxStatus); *mbox; ++mbox) {
		(*mbox)->setPosition(rightX - (*mbox)->width() - 1,
				  BASE2 + (ht - ADD2 - (*mbox)->height()) / 2);

		(*mbox)->show();
		rightX -= (*mbox)->width() + 2;
            }
#endif
#ifdef CONFIG_APPLET_CPU_STATUS
#if (defined(linux) || defined(HAVE_KSTAT_H))
        if (fCPUStatus) {
            fCPUStatus->setPosition(rightX - fCPUStatus->width() - 1,
                                    BASE1 + (ht - ADD1 - fCPUStatus->height()) / 2);
            fCPUStatus->show();
            rightX -= fCPUStatus->width() + 2;
        }
#endif
#endif
#ifdef HAVE_NET_STATUS
        if (fNetStatus)
	    for (NetStatus ** netstat(fNetStatus); *netstat; ++netstat) {
		rightX -= 2;

		(*netstat)->setPosition(rightX - (*netstat)->width() - 1,
				  BASE1 + (ht - ADD1 - (*netstat)->height()) / 2);

		// don't do a show() here because PPPStatus takes care of it
		rightX -= (*netstat)->width() + 2;
            }
#endif
#ifdef CONFIG_APPLET_APM
        if (fApm) {
            rightX -= 2;
            fApm->setPosition(rightX - fApm->width(), BASE1 + (ht - ADD1 - fApm->height()) / 2);
            fApm->show();
            rightX -= fApm->width() + 2;
        }
#endif
        if (fApplications) {
            fApplications->setPosition(leftX,
                                       BASE1 + (ht - ADD1 - fApplications->height()) / 2);
            fApplications->show();
            leftX += fApplications->width();
        }
#ifdef CONFIG_WINMENU
        if (fWinList) {
            fWinList->setPosition(leftX,
                                  BASE1 + (ht - ADD1 - fWinList->height()) / 2);
            fWinList->show();
            leftX += fWinList->width() + 2;
        }
#endif
#ifndef NO_CONFIGURE_MENUS
        if (fObjectBar) {
            leftX += 2;
            fObjectBar->setPosition(leftX,
                                    BASE1 + (ht - ADD1 - fObjectBar->height()) / 2);
            fObjectBar->show();
            leftX += fObjectBar->width() + 2;
        }
#endif

        if (fWorkspaces) {
            leftX += 2;
            fWorkspaces->setPosition(leftX, BASE1);
            leftX += 2 + fWorkspaces->width();
            fWorkspaces->show();
        }
        leftX += 2;
    }

#ifdef CONFIG_TRAY
    if (taskBarShowTray) {
        fTray = new TrayPane(this);

        if (fTray) {
            int trayWidth(fTray->getRequiredWidth());
            int w((rightX - leftX ) / 2);
            if (trayWidth > w)
		trayWidth = w;
	    else
		w = trayWidth;

            rightX-= w;

            int h((int) height() - ADD2 - ((wmLook == lookMetal) ? 0 : 1));
            int y(BASE2 + ((int) height() - ADD2 - 1 - h) / 2);

            if (taskBarDoubleHeight) {
                h = h / 2 - 1;
                y = 3 * height() / 4 - h / 2;
            } else if (trayDrawBevel)
		rightX -= 2;

            fTray->setGeometry(YRect(rightX, y, w, h));
            fTray->show();
            rightX -= 2;
        }
    } else
	fTray = 0;

#endif
    if (fAddressBar == 0) {
        fAddressBar = new AddressBar(this);
        if (fAddressBar) {
            fAddressBar->setGeometry(YRect(leftX, 0, rightX - leftX, height()));
        }
    }

    if (taskBarShowWindows) {
        fTasks = new TaskPane(this);
        if (fTasks) {
            int h = height();

            if (taskBarDoubleHeight) {
                h = ht; //  / 2 - 1;
            } else {
                h = ht;
            }
            int y = height() - h - BASE1;
            fTasks->setGeometry(YRect(leftX, y, rightX - leftX, h));
            fTasks->show();
        }
    } else {
        fTasks = 0;
#ifdef CONFIG_ADDRESSBAR
        if (fAddressBar) {
            if (showAddressBar) {
                leftX += 2;
                fAddressBar->setGeometry(
                    YRect(leftX,
                          BASE1 + (ht - ADD1 - fAddressBar->height()) / 2,
                          rightX - leftX - 4,
                          fAddressBar->height()));
                fAddressBar->show();
            }
        }
#endif
    }
    getPropertiesList();
    fIsMapped = true;
}

TaskBar::~TaskBar() {
    if (fAutoHideTimer) {
        fAutoHideTimer->stopTimer();
        fAutoHideTimer->setTimerListener(0);
        delete fAutoHideTimer; fAutoHideTimer = 0;
    }
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
    delete taskbackPixmap;
    delete taskbuttonPixmap;
    delete taskbuttonactivePixmap;
    delete taskbuttonminimizedPixmap;
#ifdef CONFIG_GRADIENT
    delete taskbackPixbuf;
    delete taskbuttonPixbuf;
    delete taskbuttonactivePixbuf;
    delete taskbuttonminimizedPixbuf;
    delete fGradient;
#endif
    delete icewmImage;
    delete windowsImage;
#ifdef CONFIG_APPLET_MAILBOX
    delete mailPixmap;
    delete noMailPixmap;
    delete errMailPixmap;
    delete unreadMailPixmap;
    delete newMailPixmap;
#endif
#ifdef CONFIG_APPLET_CLOCK
    delete PixSpace;
    delete PixSlash;
    delete PixDot;
    delete PixA;
    delete PixP;
    delete PixM;
    delete PixColon;
    for (int n = 0; n < 10; n++) delete PixNum[n];
#endif
#ifdef CONFIG_APPLET_APM
    delete fApm; fApm = 0;
#endif
#ifdef HAVE_NET_STATUS
    delete [] fNetStatus;
#endif
    taskBar = 0;
}

void TaskBar::updateLocation() {
    int x = 0;
    int y = 0;
    int h = height();

    { 
        long wk[4] = { 0, 0, 0, 0 };
        if (!taskBarAutoHide) {
            if (taskBarAtTop)
                wk[2] = height();
            else
                wk[3] = height();
        }

        MSG(("SET NET WM STRUT"));
     
        XChangeProperty(app->display(),
                        handle(),
                        _XA_NET_WM_STRUT,
                        XA_CARDINAL,
                        32, PropModeReplace,
                        (unsigned char *)&wk, 4);
        if (getFrame())
            getFrame()->updateNetWMStrut();

    }
    {
        int dx, dy, dw, dh;
        manager->getScreenGeometry(&dx, &dy, &dw, &dh);

        if (fIsHidden)
            y = taskBarAtTop ? dy -h + 1 : int(dh - 1);
        else
            y = taskBarAtTop ? dy -1 : int(dh - h + 1);
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
        if (fIsHidden)
            mwm.decorations = 0;
        else
            mwm.decorations =
                MWM_DECOR_BORDER /*|
                MWM_DECOR_RESIZEH*/;

        XChangeProperty(app->display(), handle(),
                        _XATOM_MWM_HINTS, _XATOM_MWM_HINTS,
                        32, PropModeReplace,
                        (unsigned char *)&mwm, sizeof(mwm)/sizeof(long)); /// !!! ???????
        getMwmHints();
        if (getFrame())
            getFrame()->updateMwmHints();
    }
    /// !!! fix
#if 1
    if (fIsMapped && getFrame())
        getFrame()->configureClient(x, y, width(), height());
    else
#endif
        setPosition(x, y);
}

void TaskBar::handleCrossing(const XCrossingEvent &crossing) {
    if (crossing.type == EnterNotify /* && crossing.mode != NotifyNormal */) {
        fIsHidden = false;
        if (taskBarAutoHide && fAutoHideTimer)
            fAutoHideTimer->startTimer();
    } else if (crossing.type == LeaveNotify /* && crossing.mode != NotifyNormal */) {
        if (crossing.detail != NotifyInferior) {
            fIsHidden = taskBarAutoHide;
            if (taskBarAutoHide && fAutoHideTimer)
                fAutoHideTimer->startTimer();
        }
    }
}

bool TaskBar::handleTimer(YTimer *t) {
    if (t == fAutoHideTimer) {
        if (app->popup())
            fIsHidden = false;
        updateLocation();
    }
    return false;
}

void TaskBar::paint(Graphics &g, const YRect &/*r*/) {
#ifdef CONFIG_GRADIENTS
    if (taskbackPixbuf && !(fGradient &&
    			    fGradient->width() == width() &&
			    fGradient->height() == height())) {
	delete fGradient;
	fGradient = new YPixbuf(*taskbackPixbuf, width(), height());
    }
#endif

    g.setColor(taskBarBg);
    //g.draw3DRect(0, 0, width() - 1, height() - 1, true);

#ifdef CONFIG_GRADIENTS
    if (fGradient)
        g.copyPixbuf(*fGradient, 0, 0, width(), height(), 0, 0);
    else 
#endif    
    if (taskbackPixmap)
        g.fillPixmap(taskbackPixmap, 0, 0, width(), height());
    else
        g.fillRect(0, 0, width(), height());
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
            if (button.state & app->AltMask)
                lower();
            else if (!(button.state & ControlMask))
                raise();
        }
    }
    YWindow::handleButton(button);
}

void TaskBar::contextMenu(int x_root, int y_root) {
    taskBarMenu->popup(0, 0, x_root, y_root, -1, -1,
                       YPopupWindow::pfCanFlipVertical |
                       YPopupWindow::pfCanFlipHorizontal);
}

void TaskBar::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 1) {
    } else if (up.button == 2) {
        if (windowList)
            windowList->showFocused(up.x_root, up.y_root);
    } else if (up.button == 3 && count == 1 && IS_BUTTON(up.state, Button3Mask)) {
        contextMenu(up.x_root, up.y_root);
    }
}

void TaskBar::handleDrag(const XButtonEvent &/*down*/, const XMotionEvent &motion) {
#ifndef NO_CONFIGURE
    int newPosition = 0;

    if (motion.y_root < int(desktop->height() / 2))
        newPosition = 1;

    if (taskBarAtTop != newPosition) {
        taskBarAtTop = newPosition;
        //setPosition(x(), taskBarAtTop ? -1 : int(manager->height() - height() + 1));
        manager->setWorkAreaMoveWindows(true);
        updateLocation();
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
        if (fIsHidden == true)
            popOut();
        fApplications->popupMenu();
    }
}

void TaskBar::popupWindowListMenu() {
#ifdef CONFIG_WINMENU
    if (fWinList) {
        if (fIsHidden == true)
            popOut();
        fWinList->popupMenu();
    }
#endif
}

void TaskBar::handleDNDEnter() {
    fIsHidden = false;
    if (taskBarAutoHide && fAutoHideTimer)
        fAutoHideTimer->startTimer();
}

void TaskBar::handleDNDLeave() {
    fIsHidden = taskBarAutoHide;
    if (taskBarAutoHide && fAutoHideTimer)
        fAutoHideTimer->startTimer();
}

void TaskBar::popOut() {
    if (taskBarAutoHide) {
        fIsHidden = false;
        updateLocation();
        fIsHidden = taskBarAutoHide;
        if (taskBarAutoHide && fAutoHideTimer)
            fAutoHideTimer->startTimer();
    }
}

void TaskBar::showBar(bool visible) {
    if (visible) {
        if (getFrame() == 0)
            manager->mapClient(handle());
        if (getFrame() != 0) {
	    setWinLayerHint(taskBarAutoHide ? WinLayerAboveDock :
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

void TaskBar::handlePopDown(YPopupWindow */*popup*/) {
}
#endif
