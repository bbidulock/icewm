/*
 * IceWM
 *
 * Copyright (C) 1997,98 Marko Macek
 *
 * TaskBar
 */

#include "config.h"

#include "yfull.h"
#include "wmtaskbar.h"
#include "yresource.h"

#include "ymenuitem.h"
#include "wmprog.h"
#include "sysdep.h"
#include "wmwinlist.h"

#include "ypaint.h"
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

#include "default.h"
#include "MwmUtil.h"
#include "yapp.h"

YColor *taskBarBg = 0;

//TaskBar *taskBar = 0;

YPixmap *startPixmap = 0;
YPixmap *windowsPixmap = 0;
YPixmap *taskbackPixmap = 0;

YMenu *logoutMenu = 0;

// !!! needs to go away
YPixmap *taskbuttonPixmap = 0;
YPixmap *taskbuttonactivePixmap = 0;
YPixmap *taskbuttonminimizedPixmap = 0;

static void initMenus() {
    logoutMenu = new YMenu();
    PRECONDITION(logoutMenu != 0);
    logoutMenu->setShared(true); /// !!! get rid of this (refcount objects)
    logoutMenu->addItem("Logout", 0, "", actionLogout)->setChecked(true);
    logoutMenu->addItem("Cancel logout", 0, "", actionCancelLogout)->setEnabled(false);
    logoutMenu->addSeparator();

    {
#if 1 // !!! ??? hack
        const char *configArg = 0;
#endif
        const char *c = configArg ? "-c" : 0;
        char **args = (char **)MALLOC(4 * sizeof(char*));
        args[0] = newstr("icewm"EXEEXT);
        args[1] = (char *)c; //!!!
        args[2] = (char *)configArg;
        args[3] = 0;
        DProgram *re_icewm = DProgram::newProgram("Restart icewm", 0, true, "icewm"EXEEXT, args); //!!!
        if (re_icewm)
            logoutMenu->add(new DObjectMenuItem(re_icewm));
    }
    {
        DProgram *re_xterm = DProgram::newProgram("Restart xterm", 0, true, "xterm", 0);
        if (re_xterm)
            logoutMenu->add(new DObjectMenuItem(re_xterm));
    }
}

static void initPixmaps() {
/** Use Linux 2.0 Penguin as start button */
#ifndef START_PIXMAP
#define START_PIXMAP "linux.xpm"
//#define START_PIXMAP "debian.xpm"
//#define START_PIXMAP "bsd-daemon.xpm"
//#define START_PIXMAP "start.xpm"
//#define START_PIXMAP "xfree86os2.xpm"
#endif
    YResourcePath *rp = app->getResourcePath("taskbar/");
    if (rp) {
        startPixmap = app->loadResourcePixmap(rp, START_PIXMAP);
        windowsPixmap = app->loadResourcePixmap(rp, "windows.xpm");
        taskbackPixmap = app->loadResourcePixmap(rp, "taskbarbg.xpm");
        taskbuttonPixmap = app->loadResourcePixmap(rp, "taskbuttonbg.xpm");
        taskbuttonactivePixmap = app->loadResourcePixmap(rp, "taskbuttonactive.xpm");
        taskbuttonminimizedPixmap = app->loadResourcePixmap(rp, "taskbuttonminimized.xpm");
        delete rp;
    }
}

TaskBar::TaskBar(DesktopInfo *desktopinfo, YWindow *aParent):
YWindow(aParent), fAutoHideTimer(this, autoHideDelay)
{
    unsigned int ht = 26;
    fIsMapped = false;
    fIsHidden = taskBarAutoHide;
    fMenuShown = false;
    startMenu = 0;

    if (taskBarBg == 0) {
        taskBarBg = new YColor(clrDefaultTaskBar);
    }

    initPixmaps();

    initMenus();
#if 0
    setWindowTitle("Task Bar");
    setIconTitle("Task Bar");
#ifdef GNOME1_HINTS
    setWinStateHint(WinStateAllWorkspaces, WinStateAllWorkspaces);
    //!!!setWinStateHint(WinStateDockHorizontal, WinStateDockHorizontal);
    setWinWorkspaceHint(0);
    if (taskBarAutoHide)
        setWinLayerHint(WinLayerAboveDock);
    else
        setWinLayerHint(WinLayerDock);
#endif
#endif
    {
        XWMHints wmh;

        memset(&wmh, 0, sizeof(wmh));
        wmh.flags = InputHint;
        wmh.input = False;

        XSetWMHints(app->display(), handle(), &wmh);
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

        XChangeProperty(app->display(), handle(),
                        _XATOM_MWM_HINTS, _XATOM_MWM_HINTS,
                        32, PropModeReplace,
                        (const unsigned char *)&mwm, sizeof(mwm)/sizeof(long)); ///!!! ???
    }
    {
        long arg[2];
        arg[0] = NormalState;
        arg[1] = 0;
        XChangeProperty(app->display(), handle(),
                        _XA_WM_STATE, _XA_WM_STATE,
                        32, PropModeReplace,
                        (unsigned char *)arg, 2);
    }
    setDND(true);

#if 0
    fAutoHideTimer = new YTimer(autoHideDelay);
    if (fAutoHideTimer) {
        fAutoHideTimer->setTimerListener(this);
    }
#endif

    taskBarMenu = new YMenu();
    if (taskBarMenu) {
        taskBarMenu->setActionListener(this);
        taskBarMenu->addItem("Tile Vertically", 5, "", actionTileVertical);
        taskBarMenu->addItem("Tile Horizontally", 1, "", actionTileHorizontal);
        taskBarMenu->addItem("Cascade", 1, "", actionCascade);
        taskBarMenu->addItem("Arrange", 1, "", actionArrange);
        taskBarMenu->addItem("Minimize All", 0, "", actionMinimizeAll);
        taskBarMenu->addItem("Hide All", 0, "", actionHideAll);
        taskBarMenu->addItem("Undo", 1, "", actionUndoArrange);
        if (minimizeToDesktop)
            taskBarMenu->addItem("Arrange Icons", 8, "", actionArrangeIcons)->setEnabled(false);
        taskBarMenu->addSeparator();
        taskBarMenu->addItem("Windows", 0, actionWindowList, windowListMenu);
        taskBarMenu->addSeparator();
        taskBarMenu->addItem("Refresh", 0, "", actionRefresh);

        YMenu *helpMenu; // !!!

        helpMenu = new YMenu();
        helpMenu->addItem("License", 0, "", actionLicense);
        helpMenu->addSeparator();
        helpMenu->addItem("About", 0, "", actionAbout);

        taskBarMenu->addItem("About", 0, actionAbout, 0);
        taskBarMenu->addSeparator();
        taskBarMenu->addItem("Logout...", 0, actionLogout, logoutMenu);
    }

    startMenu = new StartMenu("menu");
    startMenu->setActionListener(this);

    fAddressBar = 0;

#if (defined(linux) || defined(HAVE_KSTAT_H))
    if (taskBarShowCPUStatus)
        fCPUStatus = new CPUStatus(cpuCommand, this);
    else
        fCPUStatus = 0;
#endif

    if (taskBarShowNetStatus && netDevice)
        fNetStatus = new NetStatus(netCommand, this);
    else
        fNetStatus = 0;

    if (taskBarShowClock) {
        fClock = new YClock(this);
        if (fClock->height() + ADD1 > ht) ht = fClock->height() + ADD1;
    } else
        fClock = 0;
    if (taskBarShowApm && access("/proc/apm", 0) == 0) {
	fApm = new YApm(this);
	if (fApm->height() + ADD1 > ht) ht = fApm->height() + ADD1;
    } else
        fApm = 0;

    if (taskBarShowMailboxStatus)
        fMailBoxStatus = new MailBoxStatus(mailBoxPath, mailCommand, this);
    else
        fMailBoxStatus = 0;

    if (taskBarShowStartMenu) {
        fApplications = new YButton(this, 0, startMenu);
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

#if 0
    if (taskBarShowWindowListMenu) {
        fWinList = new YButton(this, 0, windowListMenu);
        fWinList->setPixmap(windowsPixmap);
        fWinList->setActionListener(this);
        if (fWinList->height() + ADD1 > ht) ht = fWinList->height() + ADD1;
    } else
#endif
        fWinList = 0;

    if (taskBarShowWorkspaces) {
        fWorkspaces = new WorkspacesPane(this);
    } else
        fWorkspaces = 0;

    if (taskBarDoubleHeight) {
        setSize(desktop->width() *4/5 + 2, 2 * ht + 1);

        updateLocation();

        leftX = 2;
        rightX = width() - 4;
        if (fClock) {
            fClock->setPosition(rightX - fClock->width(),
                                BASE1 + (ht - ADD1 - fClock->height()) / 2);
            fClock->show();
            rightX -= fClock->width() + 2;
        }
        if (fApm) {
            rightX -= 2;
            fApm->setPosition(rightX - fApm->width(), BASE1 + (ht - ADD1 - fApm->height()) / 2);
	    fApm->show();
	    rightX -= fApm->width() + 2;
	}    
        if (fMailBoxStatus) {
            fMailBoxStatus->setPosition(rightX - fMailBoxStatus->width() - 1,
                                        BASE2 + (ht - ADD2 - fMailBoxStatus->height()) / 2);
            fMailBoxStatus->show();
            rightX -= fMailBoxStatus->width() + 2;
        }
#if (defined(linux) || defined(HAVE_KSTAT_H))
        if (fCPUStatus) {
            fCPUStatus->setPosition(rightX - fCPUStatus->width() - 1,
                                    BASE1 + (ht - ADD1 - fCPUStatus->height()) / 2);
            fCPUStatus->show();
            rightX -= fCPUStatus->width() + 2;
        }
#endif

        if (fNetStatus) {
            rightX -= 2;
            fNetStatus->setPosition(rightX - fNetStatus->width() - 1,
                        BASE1 + (ht - ADD1 - fNetStatus->height()) / 2);
            // don't do a show() here because PPPStatus takes care of it
            rightX -= fNetStatus->width() + 2;
        }

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
        if (fObjectBar) {
            leftX += 2;
            fObjectBar->setPosition(leftX,
                                    BASE1 + (ht - ADD1 - fObjectBar->height()) / 2);
            fObjectBar->show();
            leftX += fObjectBar->width() + 2;
        }
        fAddressBar = new AddressBar(this);
        if (fAddressBar) {
            leftX += 2;
            fAddressBar->setGeometry(leftX,
                                     BASE1 + (ht - ADD1 - fAddressBar->height()) / 2,
                                     rightX - leftX - 4,
                                     fAddressBar->height());

            fAddressBar->show();
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
        if (fClock) {
            fClock->setPosition(rightX - fClock->width(),
                                BASE1 + (ht - ADD1 - fClock->height()) / 2);
            fClock->show();
            rightX -= fClock->width() + 2;
        }
        if (fMailBoxStatus) {
            fMailBoxStatus->setPosition(rightX - fMailBoxStatus->width() - 1,
                                        BASE2 + (ht - ADD2 - fMailBoxStatus->height()) / 2);
            fMailBoxStatus->show();
            rightX -= fMailBoxStatus->width() + 2;
        }
#if (defined(linux) || defined(HAVE_KSTAT_H))
        if (fCPUStatus) {
            fCPUStatus->setPosition(rightX - fCPUStatus->width() - 1,
                                    BASE1 + (ht - ADD1 - fCPUStatus->height()) / 2);
            fCPUStatus->show();
            rightX -= fCPUStatus->width() + 2;
        }
#endif
        if (fNetStatus) {
            rightX -= 2;
            fNetStatus->setPosition(rightX - fNetStatus->width() - 1,
                        BASE1 + (ht - ADD1 - fNetStatus->height()) / 2);
            // don't do a show() here because PPPStatus takes care of it
            rightX -= fNetStatus->width() + 2;
        }
        if (fApm) {
            rightX -= 2;
            fApm->setPosition(rightX - fApm->width(), BASE1 + (ht - ADD1 - fApm->height()) / 2);
            fApm->show();
            rightX -= fApm->width() + 2;
        }

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
        if (fObjectBar) {
            leftX += 2;
            fObjectBar->setPosition(leftX,
                                    BASE1 + (ht - ADD1 - fObjectBar->height()) / 2);
            fObjectBar->show();
            leftX += fObjectBar->width() + 2;
        }
        if (fWorkspaces) {
            leftX += 2;
            fWorkspaces->setPosition(leftX, BASE2);
            leftX += 2 + fWorkspaces->width();
            fWorkspaces->show();
        }
        leftX += 2;
    }
    if (taskBarShowWindows) {
        fTasks = new TaskPane(desktopinfo, this);
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
            printf("%d %d %d %d\n", x, y, w, h);
            fTasks->show();
        }
    } else
    {
        fTasks = 0;
        if (fAddressBar == 0) {
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
    }

    fIsMapped = true;
}

TaskBar::~TaskBar() {
#if 0
    if (fAutoHideTimer) {
        fAutoHideTimer->stopTimer();
        fAutoHideTimer->setTimerListener(0);
        delete fAutoHideTimer; fAutoHideTimer = 0;
    }
#endif
    delete fClock; fClock = 0;
    delete fMailBoxStatus; fMailBoxStatus = 0;
    delete fApm; fApm = 0;
    delete fObjectBar; fObjectBar = 0;
    delete fApplications; fApplications = 0;
    delete fWinList; fWinList = 0;
    delete fWorkspaces;
    delete taskbackPixmap;
    delete taskbuttonPixmap;
    delete taskbuttonactivePixmap;
    delete taskbuttonminimizedPixmap;
    delete startPixmap;
    delete windowsPixmap;
}

void TaskBar::updateLocation() {
    int x = -1;
    int y = 0;
    int h = height() - 1;

    if (fIsHidden)
        y = taskBarAtTop ? -h : int(desktop->height() - 1);
    else
        y = taskBarAtTop ? -1 : int(desktop->height() - h);

#if 0
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
    if (fIsMapped && getFrame())
        getFrame()->configureClient(x, y, width(), height());
    else
#endif
        setPosition(x, y);
}

void TaskBar::handleCrossing(const XCrossingEvent &crossing) {
    if (crossing.type == EnterNotify /* && crossing.mode != NotifyNormal */) {
        fIsHidden = false;
        if (taskBarAutoHide)
            fAutoHideTimer.startTimer();
    } else if (crossing.type == LeaveNotify /* && crossing.mode != NotifyNormal */) {
        if (crossing.detail != NotifyInferior) {
            fIsHidden = taskBarAutoHide;
            if (taskBarAutoHide)
                fAutoHideTimer.startTimer();
        }
    }
}

bool TaskBar::handleTimer(YTimer *t) {
    if (t == &fAutoHideTimer) {
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

bool TaskBar::handleKeyEvent(const XKeyEvent &key) {
    return YWindow::handleKeyEvent(key);
}

void TaskBar::handleButton(const XButtonEvent &button) {
    if ((button.type == ButtonRelease) &&
        (button.button == 1 || button.button == 3) &&
        (BUTTON_MODMASK(button.state) == Button1Mask + Button3Mask))
    {
#if 0
        fRoot->showWindowList(button.x_root, button.y_root);
#endif
    } else
    {
#if 0
        if (button.type == ButtonPress) {
            fRoot->updateWorkArea();
            if (button.button == 1) {
                if (button.state & app->AltMask)
                    lower();
                else if (!(button.state & ControlMask))
                    raise();
            }
        }
#endif
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
#if 0
        fRoot->showWindowList(up.x_root, up.y_root);
#endif
    } else if (up.button == 3 && count == 1 && IS_BUTTON(up.state, Button3Mask)) {
        contextMenu(up.x_root, up.y_root);
    }
}

void TaskBar::handleDrag(const XButtonEvent &/*down*/, const XMotionEvent &motion) {
    int newPosition = 0;

    if (motion.y_root < int(desktop->height() / 2))
        newPosition = 1;

    if (taskBarAtTop != newPosition) {
        taskBarAtTop = newPosition;
        //setPosition(x(), taskBarAtTop ? -1 : int(manager->height() - height() + 1));
#if 0
        fRoot->setWorkAreaMoveWindows(true);
#endif
        updateLocation();
        //manager->updateWorkArea();
#if 0
        fRoot->setWorkAreaMoveWindows(false);
#endif
    }
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

void TaskBar::handleDNDEnter(int /*nTypes*/, Atom */*types*/) {
    fIsHidden = false;
    if (taskBarAutoHide)
        fAutoHideTimer.startTimer();
}

void TaskBar::handleDNDLeave() {
    fIsHidden = taskBarAutoHide;
    if (taskBarAutoHide)
        fAutoHideTimer.startTimer();
}

void TaskBar::popOut() {
    if (taskBarAutoHide) {
        fIsHidden = false;
        updateLocation();
        fIsHidden = taskBarAutoHide;
        if (taskBarAutoHide)
            fAutoHideTimer.startTimer();
    }
}

void TaskBar::showBar(bool visible) {
    if (visible) {
        show();
#if 0
        if (getFrame() == 0)
            desktop->manageWindow(this, false);
        if (getFrame() != 0) {
            if (taskBarAutoHide)
                getFrame()->setLayer(WinLayerAboveDock);
            else
                getFrame()->setLayer(WinLayerDock);
            getFrame()->setState(WinStateAllWorkspaces, WinStateAllWorkspaces);
            getFrame()->activate(true);
            updateLocation();
        }
#endif
    }
}

void TaskBar::actionPerformed(YAction *action, unsigned int modifiers) {
#if 0
    fRoot->actionPerformed(action, modifiers);
#endif
}

void TaskBar::handlePopDown(YPopupWindow *popup) {
}
