/*
 * IceWM
 *
 * Copyright (C) 1997,98 Marko Macek
 *
 * TaskBar
 */

#include "config.h"

#ifdef CONFIG_TASKBAR
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
#include "objmenu.h"
#include "atasks.h"
#include "aworkspaces.h"

#include "aapm.h"

#include "intl.h"

YColor *taskBarBg = 0;

YTimer *TaskBarApp::fRaiseTimer = 0;
YTimer *WorkspaceButton::fRaiseTimer = 0;

TaskBar *taskBar = 0;

YPixmap *startPixmap = 0;
YPixmap *windowsPixmap = 0;
YPixmap *taskbackPixmap = 0;

static void initPixmaps() {
    static const char *home = getenv("HOME");
    const char *base = 0;
    static char themeSubdir[256];

    strcpy(themeSubdir, themeName);
    { char *p = strchr(themeSubdir, '/'); if (p) *p = 0; }

    static const char *themeDir = themeSubdir;
    pathelem paths[] = {
        { &home, "/.icewm/themes/", &themeDir },
        { &home, "/.icewm/", 0,},
        { &configDir, "/themes/", &themeDir },
        { &configDir, "/", 0 },
        { &libDir, "/themes/", &themeDir },
        { &libDir, "/", 0 },
        { 0, 0, 0 }
    };
    pathelem cpaths[sizeof(paths)/sizeof(paths[0])];
    verifyPaths(paths, 0);

    base = "taskbar/";
    memcpy(cpaths, paths, sizeof(cpaths));
    verifyPaths(cpaths, base);

/** Use Linux 2.0 Penguin as start button */
#ifndef START_PIXMAP
#define START_PIXMAP "linux.xpm"
//#define START_PIXMAP "debian.xpm"
//#define START_PIXMAP "bsd-daemon.xpm"
//#define START_PIXMAP "start.xpm"
//#define START_PIXMAP "xfree86os2.xpm"
#endif

    loadPixmap(cpaths, base, START_PIXMAP, &startPixmap);
    loadPixmap(cpaths, base, "windows.xpm", &windowsPixmap);
    loadPixmap(cpaths, base, "taskbarbg.xpm", &taskbackPixmap);
    loadPixmap(cpaths, base, "taskbuttonbg.xpm", &taskbuttonPixmap);
    loadPixmap(cpaths, base, "taskbuttonactive.xpm", &taskbuttonactivePixmap);
    loadPixmap(cpaths, base, "taskbuttonminimized.xpm", &taskbuttonminimizedPixmap);

#ifdef CONFIG_MAILBOX
    base = "mailbox/";
    memcpy(cpaths, paths, sizeof(cpaths));
    verifyPaths(cpaths, base);
    loadPixmap(cpaths, base, "mail.xpm", &mailPixmap);
    loadPixmap(cpaths, base, "nomail.xpm", &noMailPixmap);
    loadPixmap(cpaths, base, "errmail.xpm", &errMailPixmap);
    loadPixmap(cpaths, base, "unreadmail.xpm", &unreadMailPixmap);
    loadPixmap(cpaths, base, "newmail.xpm", &newMailPixmap);
#endif

#ifdef CONFIG_CLOCK
    base = "ledclock/";
    memcpy(cpaths, paths, sizeof(cpaths));
    verifyPaths(cpaths, base);
    loadPixmap(cpaths, base, "n0.xpm", &PixNum[0]);
    loadPixmap(cpaths, base, "n1.xpm", &PixNum[1]);
    loadPixmap(cpaths, base, "n2.xpm", &PixNum[2]);
    loadPixmap(cpaths, base, "n3.xpm", &PixNum[3]);
    loadPixmap(cpaths, base, "n4.xpm", &PixNum[4]);
    loadPixmap(cpaths, base, "n5.xpm", &PixNum[5]);
    loadPixmap(cpaths, base, "n6.xpm", &PixNum[6]);
    loadPixmap(cpaths, base, "n7.xpm", &PixNum[7]);
    loadPixmap(cpaths, base, "n8.xpm", &PixNum[8]);
    loadPixmap(cpaths, base, "n9.xpm", &PixNum[9]);
    loadPixmap(cpaths, base, "space.xpm", &PixSpace);
    loadPixmap(cpaths, base, "colon.xpm", &PixColon);
    loadPixmap(cpaths, base, "slash.xpm", &PixSlash);
    loadPixmap(cpaths, base, "dot.xpm", &PixDot);
    loadPixmap(cpaths, base, "a.xpm", &PixA);
    loadPixmap(cpaths, base, "p.xpm", &PixP);
    loadPixmap(cpaths, base, "m.xpm", &PixM);
#endif
}

TaskBar::TaskBar(YWindow *aParent):
#if 1
YFrameClient(aParent, 0)
#else
YWindow(aParent)
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
    setWinWorkspaceHint(0);
    if (taskBarAutoHide)
        setWinLayerHint(WinLayerAboveDock);
    else
        setWinLayerHint(WinLayerDock);
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
                        _XA_WM_STATE, _XA_WM_STATE,
                        32, PropModeReplace,
                        (unsigned char *)arg, 2);
    }
    setPointer(leftPointer);
    setDND(true);

    fAutoHideTimer = new YTimer(autoHideDelay);
    if (fAutoHideTimer) {
        fAutoHideTimer->setTimerListener(this);
    }

    taskBarMenu = new YMenu();
    if (taskBarMenu) {
        taskBarMenu->setActionListener(this);
        taskBarMenu->addItem(_("Tile Vertically"), 5, "", actionTileVertical);
        taskBarMenu->addItem(_("Tile Horizontally"), 1, "", actionTileHorizontal);
        taskBarMenu->addItem(_("Cascade"), 1, "", actionCascade);
        taskBarMenu->addItem(_("Arrange"), 1, "", actionArrange);
        taskBarMenu->addItem(_("Minimize All"), 0, "", actionMinimizeAll);
        taskBarMenu->addItem(_("Hide All"), 0, "", actionHideAll);
        taskBarMenu->addItem(_("Undo"), 1, "", actionUndoArrange);
        if (minimizeToDesktop)
            taskBarMenu->addItem(_("Arrange Icons"), 8, "", actionArrangeIcons)->setEnabled(false);
        taskBarMenu->addSeparator();
#ifdef CONFIG_WINMENU
        taskBarMenu->addItem(_("Windows"), 0, actionWindowList, windowListMenu);
#endif
        taskBarMenu->addSeparator();
        taskBarMenu->addItem(_("Refresh"), 0, "", actionRefresh);

#ifndef LITE
#if 0
        YMenu *helpMenu; // !!!

        helpMenu = new YMenu();
        helpMenu->addItem(_("License"), 0, "", actionLicense);
        helpMenu->addSeparator();
        helpMenu->addItem(_("About"), 0, "", actionAbout);
#endif

        taskBarMenu->addItem(_("About"), 0, actionAbout, 0);
#endif
        taskBarMenu->addSeparator();
        taskBarMenu->addItem(_("Logout..."), 0, actionLogout, logoutMenu);
    }

    fAddressBar = 0;

#ifdef CONFIG_CPUSTATUS
#if (defined(linux) || defined(HAVE_KSTAT_H))
    if (taskBarShowCPUStatus)
        fCPUStatus = new CPUStatus(cpuCommand, this);
    else
        fCPUStatus = 0;
#endif
#endif

#ifdef HAVE_NET_STATUS
    if (taskBarShowNetStatus && netDevice)
        fNetStatus = new NetStatus(netCommand, this);
    else
        fNetStatus = 0;
#endif

    if (taskBarShowClock) {
        fClock = new YClock(this);
        if (fClock->height() + ADD1 > ht) ht = fClock->height() + ADD1;
    } else
        fClock = 0;
#ifdef CONFIG_APM
    if (taskBarShowApm && access("/proc/apm", 0) == 0) {
	fApm = new YApm(this);
	if (fApm->height() + ADD1 > ht) ht = fApm->height() + ADD1;
    } else
        fApm = 0;
#endif

#ifdef CONFIG_MAILBOX
    if (taskBarShowMailboxStatus)
        fMailBoxStatus = new MailBoxStatus(mailBoxPath, mailCommand, this);
    else
        fMailBoxStatus = 0;
#endif
#ifndef NO_CONFIGURE_MENUS
    if (taskBarShowStartMenu) {
        fApplications = new YButton(this, 0, rootMenu);
        fApplications->setActionListener(this);
        fApplications->setPixmap(startPixmap);
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
        fWinList = new YButton(this, 0, windowListMenu);
        fWinList->setPixmap(windowsPixmap);
        fWinList->setActionListener(this);
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
#ifdef CONFIG_CLOCK
        if (fClock) {
            fClock->setPosition(rightX - fClock->width(),
                                BASE1 + (ht - ADD1 - fClock->height()) / 2);
            fClock->show();
            rightX -= fClock->width() + 2;
        }
#endif
#ifdef CONFIG_APM
        if (fApm) {
            rightX -= 2;
            fApm->setPosition(rightX - fApm->width(), BASE1 + (ht - ADD1 - fApm->height()) / 2);
	    fApm->show();
	    rightX -= fApm->width() + 2;
	}    
#endif
#ifdef CONFIG_MAILBOX
        if (fMailBoxStatus) {
            fMailBoxStatus->setPosition(rightX - fMailBoxStatus->width() - 1,
                                        BASE2 + (ht - ADD2 - fMailBoxStatus->height()) / 2);
            fMailBoxStatus->show();
            rightX -= fMailBoxStatus->width() + 2;
        }
#endif
#ifdef CONFIG_CPUSTATUS
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
        if (fNetStatus) {
            rightX -= 2;
            fNetStatus->setPosition(rightX - fNetStatus->width() - 1,
                        BASE1 + (ht - ADD1 - fNetStatus->height()) / 2);
            // don't do a show() here because PPPStatus takes care of it
            rightX -= fNetStatus->width() + 2;
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
#ifdef CONFIG_CLOCK
        if (fClock) {
            fClock->setPosition(rightX - fClock->width(),
                                BASE1 + (ht - ADD1 - fClock->height()) / 2);
            fClock->show();
            rightX -= fClock->width() + 2;
        }
#endif
#ifdef CONFIG_MAILBOX
        if (fMailBoxStatus) {
            fMailBoxStatus->setPosition(rightX - fMailBoxStatus->width() - 1,
                                        BASE2 + (ht - ADD2 - fMailBoxStatus->height()) / 2);
            fMailBoxStatus->show();
            rightX -= fMailBoxStatus->width() + 2;
        }
#endif
#ifdef CONFIG_CPUSTATUS
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
        if (fNetStatus) {
            rightX -= 2;
            fNetStatus->setPosition(rightX - fNetStatus->width() - 1,
                        BASE1 + (ht - ADD1 - fNetStatus->height()) / 2);
            // don't do a show() here because PPPStatus takes care of it
            rightX -= fNetStatus->width() + 2;
        }
#endif
#ifdef CONFIG_APM
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

    if (taskBarShowWindows) {
        fTasks = new TaskPane(this);
        if (fTasks) {
            int w = rightX - leftX;
            int x = leftX;
            int h = height() - ADD2 - ((wmLook == lookMetal) ? 0 : 1);
            int y = BASE2 + (height() - ADD2 - 1 - h) / 2;
            if (taskBarDoubleHeight) {
                h /= 2; h--;
                y += height() / 2; y--;
            }
            fTasks->setGeometry(x, y, w, h);
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
        fAutoHideTimer->stopTimer();
        fAutoHideTimer->setTimerListener(0);
        delete fAutoHideTimer; fAutoHideTimer = 0;
    }
#ifdef CONFIG_CLOCK
    delete fClock; fClock = 0;
#endif
#ifdef CONFIG_MAILBOX
    delete fMailBoxStatus; fMailBoxStatus = 0;
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
    delete startPixmap;
    delete windowsPixmap;
#ifdef CONFIG_MAILBOX
    delete mailPixmap;
    delete noMailPixmap;
    delete errMailPixmap;
    delete unreadMailPixmap;
    delete newMailPixmap;
#endif
#ifdef CONFIG_CLOCK
    delete PixSpace;
    delete PixSlash;
    delete PixDot;
    delete PixA;
    delete PixP;
    delete PixM;
    delete PixColon;
    for (int n = 0; n < 10; n++) delete PixNum[n];
#endif
#ifdef CONFIG_APM
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

void TaskBar::paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
    g.setColor(taskBarBg);
    //g.draw3DRect(0, 0, width() - 1, height() - 1, true);
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
    if (fWinList) {
        if (fIsHidden == true)
            popOut();
        fWinList->popupMenu();
    }
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
            if (taskBarAutoHide)
                getFrame()->setLayer(WinLayerAboveDock);
            else
                getFrame()->setLayer(WinLayerDock);
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
