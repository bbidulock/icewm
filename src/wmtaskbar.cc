/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
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

#include "aapm.h"

#include "intl.h"

YColor *taskBarBg(NULL);

YTimer *TaskBarApp::fRaiseTimer(NULL);
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
#ifndef START_PIXMAP
#define START_PIXMAP "icewm.xpm"
/*
#define START_PIXMAP "linux.xpm"
#define START_PIXMAP "debian.xpm"
#define START_PIXMAP "bsd-daemon.xpm"
#define START_PIXMAP "start.xpm"
#define START_PIXMAP "xfree86os2.xpm"
*/
#endif
    YResourcePaths const paths;

    char const * base("taskbar/");
    YResourcePaths subdirs(paths, base);

    icewmImage = subdirs.loadImage(base, START_PIXMAP);
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
    unsigned int ht = 26;
    fIsMapped = false;
    fIsHidden = taskBarAutoHide;
    fMenuShown = false;

    if (taskBarBg == 0) {
        taskBarBg = new YColor(clrDefaultTaskBar);
    }

    initPixmaps();

#if 1
    setWindowTitle(_("Task Bar"));
    setIconTitle(_("Task Bar"));
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
                        atoms.wmState, atoms.wmState,
                        32, PropModeReplace,
                        (unsigned char *)arg, 2);
    }
    setPointer(YApplication::leftPointer);
    setDND(true);

    fAutoHideTimer = new YTimer(autoHideDelay);
    if (fAutoHideTimer) {
        fAutoHideTimer->timerListener(this);
    }

    fTaskBarMenu = new YMenu();
    if (fTaskBarMenu) {
        fTaskBarMenu->actionListener(this);
        fTaskBarMenu->addItem(_("Tile _Vertically"), -2, KEY_NAME(gKeySysTileVertical), actionTileVertical);
        fTaskBarMenu->addItem(_("T_ile Horizontally"), -2, KEY_NAME(gKeySysTileHorizontal), actionTileHorizontal);
        fTaskBarMenu->addItem(_("Ca_scade"), -2, KEY_NAME(gKeySysCascade), actionCascade);
        fTaskBarMenu->addItem(_("_Arrange"), -2, KEY_NAME(gKeySysArrange), actionArrange);
        fTaskBarMenu->addItem(_("_Minimize All"), -2, KEY_NAME(gKeySysMinimizeAll), actionMinimizeAll);
        fTaskBarMenu->addItem(_("_Hide All"), -2, KEY_NAME(gKeySysHideAll), actionHideAll);
        fTaskBarMenu->addItem(_("_Undo"), -2, KEY_NAME(gKeySysUndoArrange), actionUndoArrange);
        if (minimizeToDesktop)
            fTaskBarMenu->addItem(_("Arrange _Icons"), -2, KEY_NAME(gKeySysArrangeIcons), actionArrangeIcons)->setEnabled(false);
        fTaskBarMenu->addSeparator();
#ifdef CONFIG_WINMENU
        fTaskBarMenu->addItem(_("_Windows"), -2, actionWindowList, windowListMenu);
#endif
        fTaskBarMenu->addSeparator();
        fTaskBarMenu->addItem(_("_Refresh"), -2, 0, actionRefresh);

#ifndef LITE
#if 0
        YMenu *helpMenu; // !!!

        helpMenu = new YMenu();
        helpMenu->addItem(_("_License"), -2, "", actionLicense);
        helpMenu->addSeparator();
        helpMenu->addItem(_("_About"), -2, "", actionAbout);
#endif

        fTaskBarMenu->addItem(_("_About"), -2, actionAbout, 0);
#endif
	if (logoutMenu) {
	    fTaskBarMenu->addSeparator();
	    fTaskBarMenu->addItem(_("_Logout..."), -2, actionLogout, logoutMenu);
        }
    }

    fAddressBar = 0;

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
	unsigned cnt(strTokens(netDevice));

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
        if (fClock->height() + ADD1 > ht) ht = fClock->height() + ADD1;
    } else
        fClock = 0;
#ifdef CONFIG_APPLET_APM
    if (taskBarShowApm && access("/proc/apm", 0) == 0) {
        fApm = new YApm(this);
        if (fApm->height() + ADD1 > ht) ht = fApm->height() + ADD1;
    } else
        fApm = 0;
#endif

#ifdef CONFIG_APPLET_MAILBOX
    fMailBoxStatus = 0;

    if (taskBarShowMailboxStatus) {
	char const * mailboxes(mailBoxPath ? mailBoxPath : getenv("MAIL"));
	unsigned cnt(strTokens(mailboxes));
	
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
        fApplications->actionListener(this);
        fApplications->setImage(icewmImage);
	fApplications->setToolTip(_("Favorite applications"));
        if (fApplications->height() + ADD1 > ht)
            ht = fApplications->height() + ADD1;
    } else
        fApplications = 0;

    fObjectBar = new ObjectBar(this);
    if (fObjectBar) {
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
        fWinList->actionListener(this);
	fWinList->setToolTip(_("Window list menu"));
        if (fWinList->height() + ADD1 > ht) ht = fWinList->height() + ADD1;
    } else
        fWinList = 0;
#endif

    if (taskBarShowWorkspaces && workspaceCount > 0) {
        fWorkspaces = new WorkspacesPane(this);
    } else
        fWorkspaces = 0;

    if (taskBarDoubleHeight) {
        setSize(desktop->width() + 2, 2 * ht + 1);

        updateLocation();

        leftX = 2;
        rightX = width() - 4;
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
            fApm->setPosition(rightX - fApm->width(), BASE1 + (ht - ADD1 - fApm->height()) / 2);
            fApm->show();
            rightX -= fApm->width() + 2;
        }
#endif
#ifdef CONFIG_APPLET_MAILBOX
        if (fMailBoxStatus)
	    for (MailBoxStatus ** m(fMailBoxStatus); *m; ++m) {
		(*m)->setPosition(rightX - (*m)->width() - 1,
                                  BASE2 + (ht - ADD2 - (*m)->height()) / 2);

		(*m)->show();
		rightX -= (*m)->width() + 2;
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
	    for (NetStatus ** m(fNetStatus); *m; ++m) {
		rightX -= 2;

		(*m)->setPosition(rightX - (*m)->width() - 1,
				  BASE1 + (ht - ADD1 - (*m)->height()) / 2);

		// don't do a show() here because PPPStatus takes care of it
		rightX -= (*m)->width() + 2;
	    }
#endif

        if (fApplications) {
            leftX += 2;
            fApplications->setPosition(leftX,
                                       BASE1 + (ht - ADD1 - fApplications->height()) / 2);
            fApplications->show();
            leftX += fApplications->width();
        }
        if (fWinList) {
            fWinList->setPosition(leftX,
                                  BASE1 + (ht - ADD1 - fWinList->height()) / 2);
            fWinList->show();
            leftX += fWinList->width() + 2;
        }
#ifndef NO_CONFIGURE_MENUS
        if (fObjectBar) {
            leftX += 2;
            fObjectBar->setPosition(leftX,
                                    BASE1 + (ht - ADD1 - fObjectBar->height()) / 2);
            fObjectBar->show();
            leftX += fObjectBar->width() + 2;
        }
#endif

        if (showAddressBar) {
#ifdef CONFIG_ADDRESSBAR
            fAddressBar = new AddressBar(this);
            if (fAddressBar) {
                leftX += 2;
                fAddressBar->setGeometry(leftX,
                                         BASE1 + (ht - ADD1 - fAddressBar->height()) / 2,
                                         rightX - leftX - 4,
                                         fAddressBar->height());

                fAddressBar->show();
            }
#endif
        }

        leftX = 2;
        rightX = width() - 4;

        if (fWorkspaces) {
            leftX += 2;
            fWorkspaces->setPosition(leftX, BASE2 + ht);
            leftX += 2 + fWorkspaces->width();
            fWorkspaces->show();
        }
        leftX += 2;
    } else {
        setSize(desktop->width() + 2, ht + 1);

        updateLocation();

        leftX = 2;
        rightX = width() - 4;
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
	    for (MailBoxStatus ** m(fMailBoxStatus); *m; ++m) {
		(*m)->setPosition(rightX - (*m)->width() - 1,
				  BASE2 + (ht - ADD2 - (*m)->height()) / 2);

		(*m)->show();
		rightX -= (*m)->width() + 2;
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
	    for (NetStatus ** m(fNetStatus); *m; ++m) {
		rightX -= 2;

		(*m)->setPosition(rightX - (*m)->width() - 1,
				  BASE1 + (ht - ADD1 - (*m)->height()) / 2);

		// don't do a show() here because PPPStatus takes care of it
		rightX -= (*m)->width() + 2;
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
            leftX += 2;
            fApplications->setPosition(leftX,
                                       BASE1 + (ht - ADD1 - fApplications->height()) / 2);
            fApplications->show();
            leftX += fApplications->width();
        }
        if (fWinList) {
            fWinList->setPosition(leftX,
                                  BASE1 + (ht - ADD1 - fWinList->height()) / 2);
            fWinList->show();
            leftX += fWinList->width() + 2;
        }
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
            fWorkspaces->setPosition(leftX, BASE2);
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
		rightX-= 2;

            fTray->setGeometry(rightX, y, w, h);
            fTray->show();
            rightX -= 2;
        }
    } else
	fTray = 0;

#endif
    if (taskBarShowWindows) {
        fTasks = new TaskPane(this);
        if (fTasks) {
            int h((int) height() - ADD2 - ((wmLook == lookMetal) ? 0 : 1));
            int y(BASE2 + ((int) height() - ADD2 - 1 - h) / 2);
            if (taskBarDoubleHeight) {
                h = h / 2 - 1;
                y += height() / 2 - 1;
            }
            fTasks->setGeometry(leftX, y, rightX - leftX, h);
            fTasks->show();
        }
    } else {
        fTasks = 0;
#ifdef CONFIG_ADDRESSBAR
        if (showAddressBar && fAddressBar == 0) {
            fAddressBar = new AddressBar(this);
            if (fAddressBar) {
                leftX += 2;
                fAddressBar->setGeometry(leftX,
                                         BASE1 + (ht - ADD1 - fAddressBar->height()) / 2,
                                         rightX - leftX - 4,
                                         fAddressBar->height());

                fAddressBar->show();
            }
        }
#endif
    }

    fIsMapped = true;
}

TaskBar::~TaskBar() {
    if (fAutoHideTimer) {
        fAutoHideTimer->stop();
        fAutoHideTimer->timerListener(NULL);
        delete fAutoHideTimer;
    }
#ifdef CONFIG_APPLET_CLOCK
    delete fClock; fClock = 0;
#endif
#ifdef CONFIG_APPLET_MAILBOX
    for (MailBoxStatus ** m(fMailBoxStatus); m && *m; ++m) delete *m;
    delete[] fMailBoxStatus; fMailBoxStatus = 0;
#endif
    delete fApplications; fApplications = 0;
    delete fWinList; fWinList = 0;
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
    taskBar = 0;
}

void TaskBar::updateLocation() {
    int x = -1;
    int y = 0;
    int h = height() - 1;

    if (fIsHidden)
        y = taskBarAtTop ? -h : int(desktop->height() - 1);
    else
        y = taskBarAtTop ? -1 : int(desktop->height() - h);

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
                        atoms.mwmHints, atoms.mwmHints, 32, PropModeReplace,
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
            fAutoHideTimer->start();
    } else if (crossing.type == LeaveNotify /* && crossing.mode != NotifyNormal */) {
        if (crossing.detail != NotifyInferior) {
            fIsHidden = taskBarAutoHide;
            if (taskBarAutoHide && fAutoHideTimer)
                fAutoHideTimer->start();
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

void TaskBar::paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
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
    fTaskBarMenu->popup(0, 0, x_root, y_root, -1, -1,
                        YPopupWindow::pfCanFlipVertical |
                        YPopupWindow::pfCanFlipHorizontal |
                        YPopupWindow::pfPopupMenu);
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
    if (fWinList) {
        if (fIsHidden == true)
            popOut();
        fWinList->popupMenu();
    }
}

void TaskBar::handleDNDEnter() {
    fIsHidden = false;
    if (taskBarAutoHide && fAutoHideTimer)
        fAutoHideTimer->start();
}

void TaskBar::handleDNDLeave() {
    fIsHidden = taskBarAutoHide;
    if (taskBarAutoHide && fAutoHideTimer)
        fAutoHideTimer->start();
}

void TaskBar::popOut() {
    if (taskBarAutoHide) {
        fIsHidden = false;
        updateLocation();
        fIsHidden = taskBarAutoHide;
        if (taskBarAutoHide && fAutoHideTimer)
            fAutoHideTimer->start();
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
