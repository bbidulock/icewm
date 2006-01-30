/*
 * IceWM
 *
 * Copyright (C) 1997-2003 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "wmmgr.h"

#include "aaddressbar.h"
#include "atray.h"
#include "aworkspaces.h"
#include "sysdep.h"
#include "wmtaskbar.h"
#include "wmwinlist.h"
#include "objmenu.h"
#include "wmswitch.h"
#include "wmstatus.h"
#include "wmminiicon.h"
#include "wmcontainer.h"
#include "wmframe.h"
#include "wmdialog.h"
#include "wmsession.h"
#include "wmapp.h"
#include "wmaction.h"
#include "wmprog.h"
#include "prefs.h"
#include "yprefs.h"
#include "yrect.h"

XContext frameContext;
XContext clientContext;

YAction *layerActionSet[WinLayerCount];

YWindowManager::YWindowManager(YWindow *parent, Window win):
    YDesktop(parent, win)
{
    fWmState = wmSTARTUP;
    fShuttingDown = false;
    fOtherScreenFocused = false;
    fFocusWin = 0;
    lockFocusCount = 0;
    for (int l(0); l < WinLayerCount; l++) {
        layerActionSet[l] = new YAction();
        fTop[l] = fBottom[l] = 0;
    }
    fFirst = fLast = 0;
    fFirstFocus = fLastFocus = 0;
    fColormapWindow = 0;
    fActiveWorkspace = WinWorkspaceInvalid;
    fLastWorkspace = WinWorkspaceInvalid;
    fArrangeCount = 0;
    fArrangeInfo = 0;
    fWorkAreaMoveWindows = false;
    fWorkArea = 0;
    fWorkAreaCount = 0;
    fFullscreenEnabled = true;
    fFocusedWindow = new YFrameWindow *[workspaceCount()];
    for (int w = 0; w < workspaceCount(); w++)
        fFocusedWindow[w] = 0;

    frameContext = XUniqueContext();
    clientContext = XUniqueContext();

    setStyle(wsManager);
    setPointer(YXApplication::leftPointer);
#ifdef CONFIG_XRANDR
    if (xrandrSupported) {
#if RANDR_MAJOR >= 1
        XRRSelectInput(xapp->display(), handle(), 1);
#else
        XRRScreenChangeSelectInput(xapp->display(), handle(), True);
#endif
    }
#endif

    fTopWin = new YWindow();;
    fTopWin->setStyle(YWindow::wsOverrideRedirect);
    fTopWin->setGeometry(YRect(-1, -1, 1, 1));
    fTopWin->show();
    if (edgeHorzWorkspaceSwitching) {
        fLeftSwitch = new EdgeSwitch(this, -1, false);
        if (fLeftSwitch) {
            fLeftSwitch->setGeometry(YRect(0, 0, 1, height()));
            fLeftSwitch->show();
        }
        fRightSwitch = new EdgeSwitch(this, +1, false);
        if (fRightSwitch) {
            fRightSwitch->setGeometry(YRect(width() - 1, 0, 1, height()));
            fRightSwitch->show();
        }
    } else {
        fLeftSwitch = fRightSwitch = 0;
    }

    if (edgeVertWorkspaceSwitching) {
        fTopSwitch = new EdgeSwitch(this, -1, true);
        if (fTopSwitch) {
            fTopSwitch->setGeometry(YRect(0, 0, width(), 1));
            fTopSwitch->show();
        }
        fBottomSwitch = new EdgeSwitch(this, +1, true);
        if (fBottomSwitch) {
            fBottomSwitch->setGeometry(YRect(0, height() - 1, width(), 1));
            fBottomSwitch->show();
        }
    } else {
        fTopSwitch = fBottomSwitch = 0;
    }
    XSync(xapp->display(), False);

    YWindow::setWindowFocus();
}

YWindowManager::~YWindowManager() {
}

void YWindowManager::grabKeys() {
#ifdef CONFIG_ADDRESSBAR
    ///if (taskBar && taskBar->addressBar())
        GRAB_WMKEY(gKeySysAddressBar);
#endif
///    if (runDlgCommand && runDlgCommand[0])
///        GRAB_WMKEY(gKeySysRun);
    if (quickSwitch) {
        GRAB_WMKEY(gKeySysSwitchNext);
        GRAB_WMKEY(gKeySysSwitchLast);
    }
    GRAB_WMKEY(gKeySysWinNext);
    GRAB_WMKEY(gKeySysWinPrev);
    GRAB_WMKEY(gKeySysDialog);

    GRAB_WMKEY(gKeySysWorkspacePrev);
    GRAB_WMKEY(gKeySysWorkspaceNext);
    GRAB_WMKEY(gKeySysWorkspaceLast);

    GRAB_WMKEY(gKeySysWorkspacePrevTakeWin);
    GRAB_WMKEY(gKeySysWorkspaceNextTakeWin);
    GRAB_WMKEY(gKeySysWorkspaceLastTakeWin);

    GRAB_WMKEY(gKeySysWinMenu);
    GRAB_WMKEY(gKeySysMenu);
    GRAB_WMKEY(gKeySysWindowList);
    GRAB_WMKEY(gKeySysWinListMenu);

    GRAB_WMKEY(gKeySysWorkspace1);
    GRAB_WMKEY(gKeySysWorkspace2);
    GRAB_WMKEY(gKeySysWorkspace3);
    GRAB_WMKEY(gKeySysWorkspace4);
    GRAB_WMKEY(gKeySysWorkspace5);
    GRAB_WMKEY(gKeySysWorkspace6);
    GRAB_WMKEY(gKeySysWorkspace7);
    GRAB_WMKEY(gKeySysWorkspace8);
    GRAB_WMKEY(gKeySysWorkspace9);
    GRAB_WMKEY(gKeySysWorkspace10);
    GRAB_WMKEY(gKeySysWorkspace11);
    GRAB_WMKEY(gKeySysWorkspace12);

    GRAB_WMKEY(gKeySysWorkspace1TakeWin);
    GRAB_WMKEY(gKeySysWorkspace2TakeWin);
    GRAB_WMKEY(gKeySysWorkspace3TakeWin);
    GRAB_WMKEY(gKeySysWorkspace4TakeWin);
    GRAB_WMKEY(gKeySysWorkspace5TakeWin);
    GRAB_WMKEY(gKeySysWorkspace6TakeWin);
    GRAB_WMKEY(gKeySysWorkspace7TakeWin);
    GRAB_WMKEY(gKeySysWorkspace8TakeWin);
    GRAB_WMKEY(gKeySysWorkspace9TakeWin);
    GRAB_WMKEY(gKeySysWorkspace10TakeWin);
    GRAB_WMKEY(gKeySysWorkspace11TakeWin);
    GRAB_WMKEY(gKeySysWorkspace12TakeWin);

    GRAB_WMKEY(gKeySysTileVertical);
    GRAB_WMKEY(gKeySysTileHorizontal);
    GRAB_WMKEY(gKeySysCascade);
    GRAB_WMKEY(gKeySysArrange);
    GRAB_WMKEY(gKeySysUndoArrange);
    GRAB_WMKEY(gKeySysArrangeIcons);
    GRAB_WMKEY(gKeySysMinimizeAll);
    GRAB_WMKEY(gKeySysHideAll);

    GRAB_WMKEY(gKeySysShowDesktop);
#ifdef CONFIG_TASKBAR
    if (taskBar != 0)
        GRAB_WMKEY(gKeySysCollapseTaskBar);
#endif

#ifndef NO_CONFIGURE_MENUS
    {
        KProgram *k = keyProgs;
        while (k) {
            grabVKey(k->key(), k->modifiers());
            k = k->getNext();
        }
    }
#endif
    if (xapp->WinMask && win95keys) {
        if (xapp->Win_L) {
            KeyCode keycode = XKeysymToKeycode(xapp->display(), xapp->Win_L);
            if (keycode != 0)
                XGrabKey(xapp->display(), keycode, AnyModifier, desktop->handle(), False,
                         GrabModeAsync, GrabModeSync);
        }
        if (xapp->Win_R) {
            KeyCode keycode = XKeysymToKeycode(xapp->display(), xapp->Win_R);
            if (keycode != 0)
                XGrabKey(xapp->display(), keycode, AnyModifier, desktop->handle(), False,
                         GrabModeAsync, GrabModeSync);
        }
    }

    if (useMouseWheel) {
        grabButton(4, ControlMask | xapp->AltMask);
        grabButton(5, ControlMask | xapp->AltMask);
        if (xapp->WinMask) {
            grabButton(4, xapp->WinMask);
            grabButton(5, xapp->WinMask);
        }
    }
}

YProxyWindow::YProxyWindow(YWindow *parent): YWindow(parent) {
    setStyle(wsOverrideRedirect);
}

YProxyWindow::~YProxyWindow() {
}

void YProxyWindow::handleButton(const XButtonEvent &/*button*/) {
}

void YWindowManager::setupRootProxy() {
    if (grabRootWindow) {
        rootProxy = new YProxyWindow(0);
        if (rootProxy) {
            rootProxy->setStyle(wsOverrideRedirect);
            XID rid = rootProxy->handle();

            XChangeProperty(xapp->display(), manager->handle(),
                            _XA_WIN_DESKTOP_BUTTON_PROXY, XA_CARDINAL, 32,
                            PropModeReplace, (unsigned char *)&rid, 1);
            XChangeProperty(xapp->display(), rootProxy->handle(),
                            _XA_WIN_DESKTOP_BUTTON_PROXY, XA_CARDINAL, 32,
                            PropModeReplace, (unsigned char *)&rid, 1);
        }
    }
}

bool YWindowManager::handleWMKey(const XKeyEvent &key, KeySym k, unsigned int /*m*/, unsigned int vm) {
    YFrameWindow *frame = getFocus();

    if (quickSwitch && switchWindow) {
        if (IS_WMKEY(k, vm, gKeySysSwitchNext)) {
            XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
            switchWindow->begin(1, key.state);
            return true;
        } else if (IS_WMKEY(k, vm, gKeySysSwitchLast)) {
            XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
            switchWindow->begin(0, key.state);
            return true;
        }
    }
    if (IS_WMKEY(k, vm, gKeySysWinNext)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        if (frame) frame->wmNextWindow();
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWinPrev)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        if (frame) frame->wmPrevWindow();
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWinMenu)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        if (frame) frame->popupSystemMenu(this);
        return true;
#ifndef LITE
    } else if (IS_WMKEY(k, vm, gKeySysDialog)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        if (ctrlAltDelete) ctrlAltDelete->activate();
        return true;
#endif
#ifdef CONFIG_WINMENU
    } else if (IS_WMKEY(k, vm, gKeySysWinListMenu)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        popupWindowListMenu(this);
        return true;
#endif
    } else if (IS_WMKEY(k, vm, gKeySysMenu)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        popupStartMenu(this);
        return true;
#ifdef CONFIG_WINLIST
    } else if (IS_WMKEY(k, vm, gKeySysWindowList)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        if (windowList) windowList->showFocused(-1, -1);
        return true;
#endif
    } else if (IS_WMKEY(k, vm, gKeySysWorkspacePrev)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToPrevWorkspace(false);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspaceNext)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToNextWorkspace(false);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspaceLast)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToLastWorkspace(false);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspacePrevTakeWin)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToPrevWorkspace(true);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspaceNextTakeWin)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToNextWorkspace(true);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspaceLastTakeWin)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToLastWorkspace(true);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspace1)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(0, false);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspace2)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(1, false);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspace3)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(2, false);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspace4)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(3, false);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspace5)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(4, false);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspace6)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(5, false);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspace7)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(6, false);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspace8)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(7, false);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspace9)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(8, false);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspace10)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(9, false);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspace11)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(10, false);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspace12)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(11, false);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspace1TakeWin)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(0, true);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspace2TakeWin)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(1, true);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspace3TakeWin)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(2, true);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspace4TakeWin)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(3, true);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspace5TakeWin)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(4, true);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspace6TakeWin)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(5, true);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspace7TakeWin)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(6, true);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspace8TakeWin)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(7, true);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspace9TakeWin)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(8, true);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspace10TakeWin)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(9, true);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspace11TakeWin)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(10, true);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWorkspace12TakeWin)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(11, true);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysTileVertical)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        wmapp->actionPerformed(actionTileVertical, 0);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysTileHorizontal)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        wmapp->actionPerformed(actionTileHorizontal, 0);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysCascade)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        wmapp->actionPerformed(actionCascade, 0);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysArrange)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        wmapp->actionPerformed(actionArrange, 0);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysUndoArrange)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        wmapp->actionPerformed(actionUndoArrange, 0);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysArrangeIcons)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        wmapp->actionPerformed(actionArrangeIcons, 0);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysMinimizeAll)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        wmapp->actionPerformed(actionMinimizeAll, 0);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysHideAll)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        wmapp->actionPerformed(actionHideAll, 0);
        return true;
#ifdef CONFIG_ADDRESSBAR
    } else if (IS_WMKEY(k, vm, gKeySysAddressBar)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        if (taskBar) {
            taskBar->popOut();
            if (taskBar->addressBar()) {
                taskBar->addressBar()->showNow();
            }
            return true;
        }
#endif
        ///        } else if (IS_WMKEY(k, vm, gKeySysRun)) {
        ///            if (runDlgCommand && runDlgCommand[0])
        ///                app->runCommand(runDlgCommand);
    } else if(IS_WMKEY(k, vm, gKeySysShowDesktop)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        wmapp->actionPerformed(actionShowDesktop, 0);
        return true;
    } else if(IS_WMKEY(k, vm, gKeySysCollapseTaskBar)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
#ifdef CONFIG_TASKBAR
        if (taskBar)
            taskBar->handleCollapseButton();
#endif
        return true;
    } else {
#ifndef NO_CONFIGURE_MENUS
        KProgram *p = keyProgs;
        while (p) {
            //msg("%X=%X %X=%X", k, p->key(), vm, p->modifiers());
            if (p->isKey(k, vm)) {
                XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
                p->open();
                return true;
            }
            p = p->getNext();
        }
#endif
    }
    return false;
}

bool YWindowManager::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        KeySym k = XKeycodeToKeysym(xapp->display(), key.keycode, 0);
        unsigned int m = KEY_MODMASK(key.state);
        unsigned int vm = VMod(m);

        MSG(("down key: %d, mod: %d", k, m));
        bool handled = handleWMKey(key, k, m, vm);
        if (xapp->WinMask && win95keys) {
            if (handled) {
            } else if (k == xapp->Win_L || k == xapp->Win_R) {
                /// !!! needs sync grab
                XAllowEvents(xapp->display(), SyncKeyboard, key.time);
            } else { //if (m & xapp->WinMask) {
                /// !!! needs sync grab
                XAllowEvents(xapp->display(), ReplayKeyboard, key.time);
            }
        }
        return handled;
    } else if (key.type == KeyRelease) {
        KeySym k = XKeycodeToKeysym(xapp->display(), key.keycode, 0);
        unsigned int m = KEY_MODMASK(key.state);

        (void)m;
#ifdef DEBUG
        MSG(("up key: %d, mod: %d", k, m));
#endif
        if (xapp->WinMask && win95keys) {
            if (k == xapp->Win_L || k == xapp->Win_R) {
                /// !!! needs sync grab
                XAllowEvents(xapp->display(), ReplayKeyboard, key.time);
                return true;
            }
        }
        XAllowEvents(xapp->display(), SyncKeyboard, key.time);
    }
    return true;
}

void YWindowManager::handleButton(const XButtonEvent &button) {
    if (rootProxy && button.window == handle() &&
        !(useRootButtons & (1 << (button.button - 1))) &&
       !((button.state & (ControlMask + xapp->AltMask)) == ControlMask + xapp->AltMask))
    {
        if (button.send_event == False) {
            XUngrabPointer(xapp->display(), CurrentTime);
            XSendEvent(xapp->display(),
                       rootProxy->handle(),
                       False,
                       SubstructureNotifyMask,
                       (XEvent *) &button);
        }
        return ;
    }
    YFrameWindow *frame = 0;
    if (useMouseWheel && ((frame = getFocus()) != 0) && button.type == ButtonPress &&
        ((KEY_MODMASK(button.state) == xapp->WinMask && xapp->WinMask) ||
         (KEY_MODMASK(button.state) == ControlMask + xapp->AltMask && xapp->AltMask)))
    {
        if (button.button == 4)
            frame->wmNextWindow();
        else if (button.button == 5)
            frame->wmPrevWindow();
    }
    if (button.type == ButtonPress) do {
#ifndef NO_CONFIGURE_MENUS
        if (button.button + 10 == (unsigned) rootMenuButton) {
            if (rootMenu)
                rootMenu->popup(0, 0, 0, button.x, button.y,
                                YPopupWindow::pfCanFlipVertical |
                                YPopupWindow::pfCanFlipHorizontal |
                                YPopupWindow::pfPopupMenu |
                                YPopupWindow::pfButtonDown);
            break;
        }
#endif
#ifdef CONFIG_WINMENU
        if (button.button + 10 == (unsigned) rootWinMenuButton) {
            popupWindowListMenu(this, button.x, button.y);
            break;
        }
#endif
    } while (0);
    YWindow::handleButton(button);
}

void YWindowManager::handleClick(const XButtonEvent &up, int count) {
    if (count == 1) do {
#ifndef NO_CONFIGURE_MENUS
        if (up.button == (unsigned) rootMenuButton) {
            if (rootMenu)
                rootMenu->popup(0, 0, 0, up.x, up.y,
                                YPopupWindow::pfCanFlipVertical |
                                YPopupWindow::pfCanFlipHorizontal |
                                YPopupWindow::pfPopupMenu);
            break;
        }
#endif
#ifdef CONFIG_WINMENU
        if (up.button == (unsigned) rootWinMenuButton) {
            popupWindowListMenu(this, up.x, up.y);
            break;
        }
#endif
#ifdef CONFIG_WINLIST
        if (up.button == (unsigned) rootWinListButton) {
            if (windowList)
                windowList->showFocused(up.x_root, up.y_root);
            break;
        }
#endif
    } while (0);
}

void YWindowManager::handleConfigureRequest(const XConfigureRequestEvent &configureRequest) {
    YFrameWindow *frame = findFrame(configureRequest.window);

    if (frame) {
        MSG(("root configure request -- frame"));
        frame->configureClient(configureRequest);
    } else {
        MSG(("root configure request -- client"));

        XWindowChanges xwc;
        xwc.x = configureRequest.x;
        xwc.y = configureRequest.y;
        xwc.width = configureRequest.width;
        xwc.height = configureRequest.height;
        xwc.border_width = configureRequest.border_width;
        xwc.stack_mode = configureRequest.detail;
        xwc.sibling = configureRequest.above;
        XConfigureWindow(xapp->display(), configureRequest.window,
                         configureRequest.value_mask, &xwc);
    }
}

void YWindowManager::handleMapRequest(const XMapRequestEvent &mapRequest) {
    mapClient(mapRequest.window);
}

void YWindowManager::handleUnmap(const XUnmapEvent &unmap) {
#if 1
    if (unmap.send_event)
        xapp->handleWindowEvent(unmap.window, *(XEvent *)&unmap);
#endif
}

void YWindowManager::handleDestroyWindow(const XDestroyWindowEvent &destroyWindow) {
    if (destroyWindow.window == handle())
        YWindow::handleDestroyWindow(destroyWindow);
}

void YWindowManager::handleClientMessage(const XClientMessageEvent &message) {
#ifdef WMSPEC_HINTS
    if (message.message_type == _XA_NET_CURRENT_DESKTOP) {
        setWinWorkspace(message.data.l[0]);
        return;
    }
#endif
#ifdef GNOME1_HINTS
    if (message.message_type == _XA_WIN_WORKSPACE) {
        setWinWorkspace(message.data.l[0]);
        return;
    }
#endif
    if (message.message_type == _XA_ICEWM_ACTION) {
        switch (message.data.l[1]) {
        case ICEWM_ACTION_LOGOUT:
            rebootOrShutdown = 0;
            wmapp->doLogout();
            break;
        case ICEWM_ACTION_CANCEL_LOGOUT:
            wmapp->actionPerformed(actionCancelLogout, 0);
            break;
        case ICEWM_ACTION_SHUTDOWN:
            rebootOrShutdown = 2;
            wmapp->doLogout();
            break;
        case ICEWM_ACTION_REBOOT:
            rebootOrShutdown = 1;
            wmapp->doLogout();
            break;
        }
    }
}

void YWindowManager::handleFocus(const XFocusChangeEvent &focus) {
    MSG(("window=0x%lX: %s mode=%d, detail=%d",
         focus.window,
         (focus.type == FocusIn) ? "focusIn" : "focusOut",
         focus.mode,
         focus.detail));
    if (focus.mode == NotifyNormal) {
        if (focus.type == FocusIn) {
            if (focus.detail != NotifyInferior) {
                fOtherScreenFocused = false;
            }
            if (focus.detail == NotifyDetailNone) {
                if (clickFocus || !strongPointerFocus)
                    focusLastWindow();
            }
        } else {
            if (focus.detail != NotifyInferior) {
                fOtherScreenFocused = true;
                switchFocusFrom(fFocusWin);
            }
        }
    }
}

Window YWindowManager::findWindow(const char *resource) {
    char *wmInstance = 0, *wmClass = 0;

    char const * dot(resource ? strchr(resource, '.') : 0);

    if (dot) {
        wmInstance = (dot != resource ? newstr(resource, dot - resource) : 0);
        wmClass = newstr(dot + 1);
    } else if (resource)
        wmInstance = newstr(resource);

    Window win = findWindow(desktop->handle(), wmInstance, wmClass);

    delete[] wmClass;
    delete[] wmInstance;

    return win;
}

Window YWindowManager::findWindow(Window root, char const * wmInstance,
                                  char const * wmClass) {
    Window firstMatch = None;
    Window parent, *clients;
    unsigned nClients;

    XQueryTree(xapp->display(), root, &root, &parent, &clients, &nClients);

    if (clients) {
        unsigned n;

        for (n = 0; !firstMatch && n < nClients; ++n) {
            XClassHint wmclass;

            if (XGetClassHint(xapp->display(), clients[n], &wmclass)) {
                if ((wmInstance == NULL ||
                    strcmp(wmInstance, wmclass.res_name) == 0) &&
                    (wmClass == NULL ||
                    strcmp(wmClass, wmclass.res_class) == 0))
                    firstMatch = clients[n];

                XFree(wmclass.res_name);
                XFree(wmclass.res_class);
            }

            if (!firstMatch)
                firstMatch = findWindow(clients[n], wmInstance, wmClass);
        }
        XFree(clients);
    }
    return firstMatch;
}

YFrameWindow *YWindowManager::findFrame(Window win) {
    YFrameWindow *frame;

    if (XFindContext(xapp->display(), win,
                     frameContext, (XPointer *)&frame) == 0)
        return frame;
    else
        return 0;
}

YFrameClient *YWindowManager::findClient(Window win) {
    YFrameClient *client;

    if (XFindContext(xapp->display(), win,
                     clientContext, (XPointer *)&client) == 0)
        return client;
    else
        return 0;
}

#ifndef LITE
void YWindowManager::setFocus(YFrameWindow *f, bool canWarp) {
#else
void YWindowManager::setFocus(YFrameWindow *f, bool /*canWarp*/) {
#endif
//    updateFullscreenLayerEnable(false);

    YFrameClient *c = f ? f->client() : 0;
    Window w = None;

    if (focusLocked())
        return;
    MSG(("SET FOCUS f=%lX", f));

    if (f == 0) {
        YFrameWindow *ff = getFocus();
        if (ff) switchFocusFrom(ff);
    }

    bool input = f ? f->getInputFocusHint() : false;
#if 0
    XWMHints *hints = c ? c->hints() : 0;

    if (!f || !(f->frameOptions() & YFrameWindow::foIgnoreNoFocusHint)) {
        if (hints && (hints->flags & InputHint) && !hints->input)
            input = false;
    }
    if (f && (f->frameOptions() & YFrameWindow::foDoNotFocus))
        input = false;
#endif

    if (f && f->visible()) {
        if (c && c->visible() && !(f->isRollup() || f->isIconic()))
            w = c->handle();
        else
            w = f->handle();

        if (input)
            switchFocusTo(f);
        f->setWmUrgency(false);
    }
#ifdef DEBUG
    if (w == desktop->handle()) {
        MSG(("%lX Focus 0x%lX desktop",
             xapp->getEventTime("focus1"), w));
    } else if (f && w == f->handle()) {
        MSG(("%lX Focus 0x%lX frame %s",
             xapp->getEventTime("focus1"), w, f->getTitle()));
    } else if (f && c && w == c->handle()) {
        MSG(("%lX Focus 0x%lX client %s",
             xapp->getEventTime("focus1"), w, f->getTitle()));
    } else {
        MSG(("%lX Focus 0x%lX",
             xapp->getEventTime("focus1"), w));
    }
#endif

    if (w != None) {// input || w == desktop->handle()) {
        XSetInputFocus(xapp->display(), w, RevertToNone, xapp->getEventTime("setFocus"));
    } else {
        XSetInputFocus(xapp->display(), fTopWin->handle(), RevertToNone, xapp->getEventTime("setFocus"));
    }

    if (c && w == c->handle() && c->protocols() & YFrameClient::wpTakeFocus) {
        c->sendTakeFocus();
    }

    if (!pointerColormap)
        setColormapWindow(f);

#ifndef LITE
    /// !!! /* warp pointer sucks */
    if (f &&
        canWarp &&
        !clickFocus &&
        warpPointer &&
        wmState() == wmRUNNING)
    {
        XWarpPointer(xapp->display(), None, handle(), 0, 0, 0, 0,
                     f->x() + f->borderX(), f->y() + f->borderY() + f->titleY());
    }

#endif
    MSG(("SET FOCUS END"));
    updateFullscreenLayer();
}

/// TODO lose this function
void YWindowManager::loseFocus(YFrameWindow *window) {
    (void)window;
    PRECONDITION(window != 0);
    focusLastWindow();
}

void YWindowManager::activate(YFrameWindow *window, bool raise, bool canWarp) {
    if (window) {
        if (raise)
            window->wmRaise();
        window->activateWindow(canWarp);
    }
}

void YWindowManager::setTop(long layer, YFrameWindow *top) {
    if (true || !clientMouseActions) // some programs are buggy
        if (fTop[layer]) {
            if (raiseOnClickClient)
                fTop[layer]->container()->grabButtons();
        }
    fTop[layer] = top;
    if (true || !clientMouseActions) // some programs are buggy
        if (fTop[layer]) {
            if (raiseOnClickClient &&
                !(focusOnClickClient && !fTop[layer]->focused()))
                fTop[layer]->container()->releaseButtons();
        }
}

void YWindowManager::installColormap(Colormap cmap) {
    if (xapp->hasColormap()) {
        //MSG(("installing colormap 0x%lX", cmap));
        if (xapp->grabWindow() == 0) {
            if (cmap == None) {
                XInstallColormap(xapp->display(), xapp->colormap());
            } else {
                XInstallColormap(xapp->display(), cmap);
            }
        }
    }
}

void YWindowManager::setColormapWindow(YFrameWindow *frame) {
    if (fColormapWindow != frame) {
        fColormapWindow = frame;

        if (colormapWindow() && colormapWindow()->client())
            installColormap(colormapWindow()->client()->colormap());
        else
            installColormap(None);
    }
}

void YWindowManager::manageClients() {
    unsigned int clientCount;
    Window winRoot, winParent, *winClients;

    manager->fWmState = YWindowManager::wmSTARTUP;
    XGrabServer(xapp->display());
    XSync(xapp->display(), False);
    XQueryTree(xapp->display(), handle(),
               &winRoot, &winParent, &winClients, &clientCount);

    if (winClients)
        for (unsigned int i = 0; i < clientCount; i++)
            if (findClient(winClients[i]) == 0)
                manageClient(winClients[i]);

    XUngrabServer(xapp->display());
    if (winClients)
        XFree(winClients);
    updateWorkArea();
    fWmState = wmRUNNING;
    focusTopWindow();
}

void YWindowManager::unmanageClients() {
    Window w;

    manager->fWmState = YWindowManager::wmSHUTDOWN;
#ifdef CONFIG_TASKBAR
    if (taskBar)
        taskBar->detachTray();
#endif
    setFocus(0);
    XGrabServer(xapp->display());
    for (unsigned int l = 0; l < WinLayerCount; l++) {
        while (bottom(l)) {
            w = bottom(l)->client()->handle();
            unmanageClient(w, true);
        }
    }
    XSetInputFocus(xapp->display(), PointerRoot, RevertToNone, CurrentTime);
    XSync(xapp->display(), False);
    XUngrabServer(xapp->display());
}

int addco(int *v, int &n, int c) {
    int l, r, m;

    /* find */
    l = 0;
    r = n;
    while (l < r) {
        m = (l + r) / 2;
        if (v[m] == c)
            return 0;
        else if (v[m] > c)
            r = m;
        else
            l = m + 1;
    }
    /* insert if not found */
    memmove(v + l + 1, v + l, (n - l) * sizeof(int));
    v[l] = c;
    n++;
    return 1;
}

int YWindowManager::calcCoverage(bool down, YFrameWindow *frame, int x, int y, int w, int h) {
    int cover = 0;
    int factor = down ? 2 : 1; // try harder not to cover top windows

    for (YFrameWindow * f = frame; f ; f = (down ? f->next() : f->prev())) {
        if (f == frame || f->isMinimized() || f->isHidden() || !f->isManaged())
            continue;

        if (!f->isSticky() && f->getWorkspace() != frame->getWorkspace())
            continue;

        cover +=
            intersection(f->x(), f->x() + f->width(), x, x + w) *
            intersection(f->y(), f->y() + f->height(), y, y + h) * factor;

        if (factor > 1)
            factor /= 2;
    }
    //msg("coverage %d %d %d %d = %d", x, y, w, h, cover);
    return cover;
}

void YWindowManager::tryCover(bool down, YFrameWindow *frame, int x, int y, int w, int h,
                              int &px, int &py, int &cover, int xiscreen)
{
    int ncover;

    int mx, my, Mx, My;
    manager->getWorkArea(frame, &mx, &my, &Mx, &My, xiscreen);

    if (x < mx)
        return ;
    if (y < my)
        return ;
    if (x + w > Mx)
        return ;
    if (y + h > My)
        return ;

    ncover = calcCoverage(down, frame, x, y, w, h);
    if (ncover < cover) {
        //msg("min: %d %d %d", ncover, x, y);
        px = x;
        py = y;
        cover = ncover;
    }
}

bool YWindowManager::getSmartPlace(bool down, YFrameWindow *frame, int &x, int &y, int w, int h, int xiscreen) {
    int mx, my, Mx, My;
    manager->getWorkArea(frame, &mx, &my, &Mx, &My, xiscreen);

    x = mx;
    y = my;
    int cover, px, py;
    int *xcoord, *ycoord;
    int xcount, ycount;
    int n = 0;
    YFrameWindow *f = 0;

    for (f = frame; f; f = (down ? f->next() : f->prev()))
        n++;
    n = (n + 1) * 2;
    xcoord = new int[n];
    if (xcoord == 0)
        return false;
    ycoord = new int[n];
    if (ycoord == 0)
        return false;

    xcount = ycount = 0;
    addco(xcoord, xcount, mx);
    addco(ycoord, ycount, my);
    for (f = frame; f; f = (down ? f->next() : f->prev())) {
        if (f == frame || f->isMinimized() || f->isHidden() || !f->isManaged() || f->isMaximized())
            continue;

        if (!f->isSticky() && f->getWorkspace() != frame->getWorkspace())
            continue;

        addco(xcoord, xcount, f->x());
        addco(xcoord, xcount, f->x() + f->width());
        addco(ycoord, ycount, f->y());
        addco(ycoord, ycount, f->y() + f->height());
    }
    addco(xcoord, xcount, Mx);
    addco(ycoord, ycount, My);
    assert(xcount <= n);
    assert(ycount <= n);

    int xn = 0, yn = 0;
    px = x; py = y;
    cover = calcCoverage(down, frame, x, y, w, h);
    while (1) {
        x = xcoord[xn];
        y = ycoord[yn];

        tryCover(down, frame, x - w, y - h, w, h, px, py, cover, xiscreen);
        tryCover(down, frame, x - w, y    , w, h, px, py, cover, xiscreen);
        tryCover(down, frame, x    , y - h, w, h, px, py, cover, xiscreen);
        tryCover(down, frame, x    , y    , w, h, px, py, cover, xiscreen);

        if (cover == 0)
            break;

        xn++;
        if (xn >= xcount) {
            xn = 0;
            yn++;
            if (yn >= ycount)
                break;
        }
    }
    x = px;
    y = py;
    delete [] xcoord;
    delete [] ycoord;
    return true;
}

void YWindowManager::smartPlace(YFrameWindow **w, int count) {
    saveArrange(w, count);

    if (count == 0)
        return;

#ifndef XINERAMA
    int s = 0;
#else
    int n = xiHeads ? xiHeads : 1; /// fix xiHeads, xiInfo init
    for (int s = 0; s < n; s++)
#endif
    {
        for (int i = 0; i < count; i++) {
            YFrameWindow *f = w[i];
            int x = f->x();
            int y = f->y();
            if (s != f->getScreen())
                continue;
            if (getSmartPlace(false, f, x, y, f->width(), f->height(), s)) {
                f->setNormalPositionOuter(x, y);
            }
        }
    }
}

void YWindowManager::getCascadePlace(YFrameWindow *frame, int &lastX, int &lastY, int &x, int &y, int w, int h) {
    int mx, my, Mx, My;
    manager->getWorkArea(frame, &mx, &my, &Mx, &My);

    /// !!! make auto placement cleaner and (optionally) smarter
    if (lastX < mx) lastX = mx;
    if (lastY < my) lastY = my;

    x = lastX;
    y = lastY;

    lastX += wsTitleBar;
    lastY += wsTitleBar;
    if (int(y + h) >= My) {
        y = my;
        lastY = wsTitleBar;
    }
    if (int(x + w) >= Mx) {
        x = mx;
        lastX = wsTitleBar;
    }
}

void YWindowManager::cascadePlace(YFrameWindow **w, int count) {
    saveArrange(w, count);

    if (count == 0)
        return;

    int mx, my, Mx, My;
    manager->getWorkArea(0, &mx, &my, &Mx, &My);

    int lx = mx;
    int ly = my;
    for (int i = count; i > 0; i--) {
        YFrameWindow *f = w[i - 1];
        int x;
        int y;

        getCascadePlace(f, lx, ly, x, y, f->width(), f->height());
        f->setNormalPositionOuter(x, y);
    }
}

void YWindowManager::setWindows(YFrameWindow **w, int count, YAction *action) {
    saveArrange(w, count);

    if (count == 0)
        return;

    for (int i = 0; i < count; ++i) {
        YFrameWindow *f = w[i];
        if (action == actionHideAll) {
            if (!f->isHidden())
                f->wmHide();
        } else if (action == actionMinimizeAll) {
            if (!f->isMinimized())
                f->wmMinimize();
        }
    }
}

void YWindowManager::getNewPosition(YFrameWindow *frame, int &x, int &y, int w, int h, int xiscreen) {
    if (centerTransientsOnOwner && frame->owner() != 0) {
        x = frame->owner()->x() + frame->owner()->width() / 2 - w / 2;
        y = frame->owner()->y() + frame->owner()->width() / 2 - h / 2;
    } else if (smartPlacement) {
        getSmartPlace(true, frame, x, y, w, h, xiscreen);
    } else {

        static int lastX = 0;
        static int lastY = 0;

        getCascadePlace(frame, lastX, lastY, x, y, w, h);
    }
}

void YWindowManager::placeWindow(YFrameWindow *frame,
                                 int x, int y,
                                 int cw, int ch,
                                 bool newClient, bool &
#ifdef CONFIG_SESSION
                                 canActivate
#endif
                                )
{
    YFrameClient *client = frame->client();

    int frameWidth = 2 * frame->borderXN();
    int frameHeight = 2 * frame->borderYN() + frame->titleYN();
    int posWidth = cw + frameWidth;
    int posHeight = ch + frameHeight;
    int posX = x;
    int posY = y;

#ifdef CONFIG_SESSION
    if (smapp->haveSessionManager() && findWindowInfo(frame)) {
        if (frame->getWorkspace() != manager->activeWorkspace())
            canActivate = false;
        return ;
    }
#ifndef NO_WINDOW_OPTIONS
    else
#endif
#endif

#ifndef NO_WINDOW_OPTIONS
    if (newClient) {
        WindowOption wo(0);
        frame->getWindowOptions(wo, true);

        //msg("positioning %d %d %d %d %X", wo.gx, wo.gy, wo.gw, wo.gh, wo.gflags);
        if (wo.gh != 0 && wo.gw != 0) {
            if ((wo.gflags & (WidthValue | HeightValue)) ==
                (WidthValue | HeightValue))
            {
                posWidth = wo.gw + frameWidth;
                posHeight = wo.gh + frameHeight;
            }
        }

        if ((wo.gflags & (XValue | YValue)) == (XValue | YValue)) {
            int wox = wo.gx;
            int woy = wo.gy;

            if (wo.gflags & XNegative)
                wox = desktop->width() - frame->width() - wox;
            if (wo.gflags & YNegative)
                woy = desktop->height() - frame->height() - woy;
            posX = wox;
            posY = woy;
            goto setGeo; /// FIX
        }
    }
#endif

    if (newClient && client->adopted() && client->sizeHints() &&
        (!(client->sizeHints()->flags & (USPosition | PPosition)) ||
         ((client->sizeHints()->flags & PPosition)
#ifndef NO_WINDOW_OPTIONS
          && (frame->frameOptions() & YFrameWindow::foIgnorePosition)
#endif
         )))
    {
        int xiscreen = 0;
        if (frame->owner())
            xiscreen = frame->owner()->getScreen();
        if (fFocusWin)
            xiscreen = fFocusWin->getScreen();
        getNewPosition(frame, x, y, posWidth, posHeight, xiscreen);
        posX = x;
        posY = y;
        newClient = false;
    } else {
        if (client->sizeHints() &&
            (client->sizeHints()->flags & PWinGravity) &&
            client->sizeHints()->win_gravity == StaticGravity)
        {
            posX -= frame->borderXN();
            posY -= frame->borderYN() + frame->titleYN();
        } else {
            int gx, gy;

            client->gravityOffsets(gx, gy);

            if (gx > 0)
                posX -= 2 * frame->borderXN() - 1 - client->getBorder();
            if (gy > 0)
                posY -= 2 * frame->borderYN() + frame->titleYN() - 1 - client->getBorder();
        }
    }

setGeo:
    MSG(("mapping geometry 1 (%d:%d %dx%d)", posX, posY, posWidth, posHeight));
    frame->setNormalGeometryOuter(posX, posY, posWidth, posHeight);

}

YFrameWindow *YWindowManager::manageClient(Window win, bool mapClient) {
    YFrameWindow *frame(NULL);
    YFrameClient *client(NULL);
    int cx = 0;
    int cy = 0;
    int cw = 1;
    int ch = 1;
    bool canManualPlace = false;
    bool canActivate = true;


    MSG(("managing window 0x%lX", win));
    frame = findFrame(win);
    PRECONDITION(frame == 0);

    XGrabServer(xapp->display());
#if 0
    XSync(xapp->display(), False);
    {
        XEvent xev;
        if (XCheckTypedWindowEvent(xapp->display(), win, DestroyNotify, &xev))
            goto end;
    }
#endif

    client = findClient(win);
    if (client == 0) {
        XWindowAttributes attributes;

        if (!XGetWindowAttributes(xapp->display(), win, &attributes))
            goto end;

        if (attributes.override_redirect)
            goto end;

        ///!!! is this correct ?
        if (!mapClient && attributes.map_state == IsUnmapped)
            goto end;

        client = new YFrameClient(0, 0, win);
        if (client == 0)
            goto end;

        if (client->isKdeTrayWindow()) {
#ifdef CONFIG_TASKBAR
#ifdef CONFIG_TRAY
            if (taskBar && taskBar->trayPane()) {
                if (taskBar->netwmTray()->kdeRequestDock(win)) {
                    delete client;
                    goto end;
                }
            }
#endif
#endif
        }

        client->setBorder(attributes.border_width);
        client->setColormap(attributes.colormap);
    }

    MSG(("initial geometry 1 (%d:%d %dx%d)",
         client->x(), client->y(), client->width(), client->height()));

    cx = client->x();
    cy = client->y();
    cw = client->width();
    ch = client->height();

    if (client->visible() && wmState() == wmSTARTUP)
        mapClient = true;

    manager->updateFullscreenLayerEnable(false);

    frame = new YFrameWindow(0);
    if (frame == 0) {
        delete client;
        goto end;
    }
    MSG(("initial geometry 2 (%d:%d %dx%d)",
         client->x(), client->y(), client->width(), client->height()));

    frame->doManage(client);
    MSG(("initial geometry 3 (%d:%d %dx%d)",
         client->x(), client->y(), client->width(), client->height()));

    placeWindow(frame, cx, cy, cw, ch, (wmState() != wmSTARTUP), canActivate);

    if ((limitSize || limitPosition) &&
        (wmState() != wmSTARTUP) &&
        !frame->affectsWorkArea())
    {
        int posX(frame->x() + frame->borderXN()),
            posY(frame->y() + frame->borderYN()),
            posWidth(frame->width() - 2 * frame->borderXN()),
            posHeight(frame->height() - 2 * frame->borderYN());

        MSG(("mapping geometry 2 (%d:%d %dx%d)", posX, posY, posWidth, posHeight));

        if (limitSize) {
            int Mw, Mh;
            manager->getWorkAreaSize(frame, &Mw, &Mh);

            posWidth = min(posWidth, Mw);
            posHeight = min(posHeight, Mh);

/// TODO #warning "cleanup the constrainSize code, there is some duplication"
            posHeight -= frame->titleYN();
            frame->client()->constrainSize(posWidth, posHeight, 0);
            posHeight += frame->titleYN();
        }

        if (limitPosition &&
            !(client->sizeHints() &&
              (client->sizeHints()->flags & USPosition)))
        {
            int mx, my, Mx, My;
            manager->getWorkArea(frame, &mx, &my, &Mx, &My);

            posX = clamp(posX, mx, Mx - posWidth);
            posY = clamp(posY, my, My - posHeight);
        }
        posHeight -= frame->titleYN();

        MSG(("mapping geometry 3 (%d:%d %dx%d)", posX, posY, posWidth, posHeight));
        frame->setNormalGeometryInner(posX, posY, posWidth, posHeight);
    }

    if (!mapClient) {
        /// !!! fix (new internal state)
        frame->setState(WinStateHidden, WinStateHidden);
    }
    if (frame->frameOptions() & YFrameWindow::foMinimized) {
        frame->setState(WinStateMinimized, WinStateMinimized);
    }
    frame->setManaged(true);

    if (canActivate && manualPlacement && wmState() == wmRUNNING &&
#ifdef CONFIG_WINLIST
        client != windowList &&
#endif
        !frame->owner() &&
        (!client->sizeHints() ||
         !(client->sizeHints()->flags & (USPosition | PPosition))))
        canManualPlace = true;

    if (mapClient) {
        if (!(frame->getState() & (WinStateHidden | WinStateMinimized | WinStateFullscreen)))
        {
            if (canManualPlace && !opaqueMove)
                frame->manualPlace();
        }
    }
    frame->updateState();
    frame->updateProperties();
#ifdef CONFIG_TASKBAR
    frame->updateTaskBar();
#endif
    if (frame->affectsWorkArea())
        updateWorkArea();
    if (mapClient) {
        if (!(frame->getState() & (WinStateHidden | WinStateMinimized)))
        {
            if (wmState() == wmRUNNING && canActivate)
                frame->focusOnMap();
            if (canManualPlace && opaqueMove)
                frame->wmMove();
        }
    }
#if 1
    if (wmState() == wmRUNNING) {
#ifndef NO_WINDOW_OPTIONS
        if (frame->frameOptions() & (YFrameWindow::foMaximizedVert | YFrameWindow::foMaximizedHorz))
            frame->setState(
                WinStateMaximizedVert | WinStateMaximizedHoriz,
                ((frame->frameOptions() & YFrameWindow::foMaximizedVert) ? WinStateMaximizedVert : 0) |
                ((frame->frameOptions() & YFrameWindow::foMaximizedHorz) ? WinStateMaximizedHoriz : 0));
#endif
    }
#endif
    manager->updateFullscreenLayerEnable(true);
end:
    XUngrabServer(xapp->display());
    return frame;
}

YFrameWindow *YWindowManager::mapClient(Window win) {
    YFrameWindow *frame = findFrame(win);

    MSG(("mapping window 0x%lX", win));
    if (frame == 0)
        return manageClient(win, true);
    else {
        frame->setState(WinStateMinimized | WinStateHidden, 0);
        if (clickFocus || !strongPointerFocus)
            frame->activate(true);/// !!! is this ok
    }

    return frame;
}

void YWindowManager::unmanageClient(Window win, bool mapClient,
                                    bool reparent) {
    YFrameWindow *frame = findFrame(win);

    MSG(("unmanaging window 0x%lX", win));

    if (frame) {
        YFrameClient *client = frame->client();

        // !!! cleanup
        client->hide();

        frame->hide();
        frame->unmanage(reparent);
        delete frame;

        if (mapClient)
            client->show();
        delete client;
    } else {
        MSG(("unmanage: unknown window: 0x%lX", win));
    }
}

void YWindowManager::destroyedClient(Window win) {
    YFrameWindow *frame = findFrame(win);

    if (frame)
        delete frame;
    else
        MSG(("destroyed: unknown window: 0x%lX", win));
}

void YWindowManager::focusTopWindow() {
    if (wmState() != wmRUNNING)
        return ;
    if (!clickFocus && strongPointerFocus) {
        XSetInputFocus(xapp->display(), PointerRoot, RevertToNone, CurrentTime);
        return ;
    }
    if (!focusTop(topLayer(WinLayerNormal)))
        focusTop(topLayer());
}

bool YWindowManager::focusTop(YFrameWindow *f) {
    if (!f)
        return false;

    f = f->findWindow(YFrameWindow::fwfVisible |
                      YFrameWindow::fwfFocusable |
                      YFrameWindow::fwfWorkspace |
                      YFrameWindow::fwfSame |
                      YFrameWindow::fwfLayers |
                      YFrameWindow::fwfCycle);
    //msg("found focus %lX", f);
    if (!f) {
        setFocus(0);
        return false;
    }
    setFocus(f);
    return true;
}

YFrameWindow *YWindowManager::getLastFocus(long workspace) {
    if (workspace == -1)
        workspace = activeWorkspace();

    YFrameWindow *toFocus = 0;

    toFocus = fFocusedWindow[workspace];
    if (toFocus != 0) {
        if (toFocus->isMinimized() ||
            toFocus->isHidden() ||
            !toFocus->isFocusable(true) ||
            !toFocus->visibleOn(workspace))
            toFocus = 0;
    }

    if (toFocus == 0) {
        for (int pass = 0; pass < 2; pass++) {
            for (YFrameWindow *w = lastFocusFrame();
                 w;
                 w = w->prevFocus())
            {
#if 1
                if ((w->client() && !w->client()->adopted()))
                    continue;
#endif
                if (w->isMinimized())
                    continue;
                if (w->isHidden())
                    continue;
                if (!w->isFocusable(true))
                    continue;
                if (!w->visibleOn(workspace))
                    continue;
                if (!w->isSticky() || pass == 1) {
                    toFocus = w;
                    goto gotit;
                }
            }
        }
    }
gotit:
#if 0
    YFrameWindow *f = toFocus;
    YWindow *c = f ? f->client() : 0;
    Window w = c ? c->handle() : 0;
    if (w == desktop->handle()) {
        msg("%lX Focus 0x%lX desktop",
            app->getEventTime(), w);
    } else if (f && w == f->handle()) {
        msg("%lX Focus 0x%lX frame %s",
            app->getEventTime(), w, f->getTitle());
    } else if (f && c && w == c->handle()) {
        msg("%lX Focus 0x%lX client %s",
            app->getEventTime(), w, f->getTitle());
    } else {
        msg("%lX Focus 0x%lX",
            app->getEventTime(), w);
    }
#endif
    return toFocus;
}

void YWindowManager::focusLastWindow() {
    if (wmState() != wmRUNNING)
        return ;
    if (!clickFocus && strongPointerFocus) {
        XSetInputFocus(xapp->display(), PointerRoot, RevertToNone, CurrentTime);
        return ;
    }

/// TODO #warning "per workspace?"
    YFrameWindow *toFocus = getLastFocus();

    if (toFocus == 0) {
        focusTopWindow();
    } else {
        if (raiseOnFocus)
            toFocus->wmRaise();
        setFocus(toFocus);
    }
}


YFrameWindow *YWindowManager::topLayer(long layer) {
    for (long l = layer; l >= 0; l--)
        if (fTop[l]) return fTop[l];

    return 0;
}

YFrameWindow *YWindowManager::bottomLayer(long layer) {
    for (long l = layer; l < WinLayerCount; l++)
        if (fBottom[l]) return fBottom[l];

    return 0;
}

void YWindowManager::updateFullscreenLayerEnable(bool enable) {
    fFullscreenEnabled = enable;
    updateFullscreenLayer();
}

void YWindowManager::updateFullscreenLayer() { /// HACK !!!
    YFrameWindow *w = topLayer();
    while (w) {
        if (w->getActiveLayer() == WinLayerFullscreen ||
            w->isFullscreen())
            w->updateLayer();
        w = w->nextLayer();
    }
}

void YWindowManager::restackWindows(YFrameWindow *win) {
    int count = 0;
    YFrameWindow *f;
    YPopupWindow *p;
    long ll;

    for (f = win; f; f = f->prev())
        count++;

    for (ll = win->getActiveLayer() + 1; ll < WinLayerCount; ll++) {
        f = bottom(ll);
        for (; f; f = f->prev())
            count++;
    }

#ifndef LITE
    if (statusMoveSize && statusMoveSize->visible())
        count++;
#endif

    p = xapp->popup();
    while (p) {
        count++;
        p = p->prevPopup();
    }
#ifndef LITE
    if (ctrlAltDelete && ctrlAltDelete->visible())
        count++;
#endif

    if (fLeftSwitch && fLeftSwitch->visible())
        count++;

    if (fRightSwitch && fRightSwitch->visible())
        count++;

    if (fTopSwitch && fTopSwitch->visible())
        count++;

    if (fBottomSwitch && fBottomSwitch->visible())
        count++;

    if (count == 0)
        return ;

    count++;

    Window *w = new Window[count];
    if (w == 0)
        return ;

    int i = 0;

    w[i++] = fTopWin->handle();

    p = xapp->popup();
    while (p) {
        w[i++] = p->handle();
        p = p->prevPopup();
    }

    if (fLeftSwitch && fLeftSwitch->visible())
        w[i++] = fLeftSwitch->handle();

    if (fRightSwitch && fRightSwitch->visible())
        w[i++] = fRightSwitch->handle();

    if (fTopSwitch && fTopSwitch->visible())
        w[i++] = fTopSwitch->handle();

    if (fBottomSwitch && fBottomSwitch->visible())
        w[i++] = fBottomSwitch->handle();

#ifndef LITE
    if (ctrlAltDelete && ctrlAltDelete->visible())
        w[i++] = ctrlAltDelete->handle();
#endif

#ifndef LITE
    if (statusMoveSize->visible())
        w[i++] = statusMoveSize->handle();
#endif

    for (ll = WinLayerCount - 1; ll > win->getActiveLayer(); ll--) {
        for (f = top(ll); f; f = f->next())
            w[i++] = f->handle();
    }
    for (f = top(win->getActiveLayer()); f; f = f->next()) {
        w[i++] = f->handle();
        if (f == win)
            break;
    }

    if (count > 0) {
#if 0
        /* remove this code if ok !!! must determine correct top window */
#if 1
        XRaiseWindow(xapp->display(), w[0]);
#else
        if (win->next()) {
            XWindowChanges xwc;

            xwc.sibling = win->next()->handle();
            xwc.stack_mode = Above;
            XConfigureWindow(xapp->display(), w[0], CWSibling | CWStackMode, &xwc);
        }
#endif
        if (count > 1)
#endif
        XRestackWindows(xapp->display(), w, count);
    }
    if (i != count) {
        MSG(("i=%d, count=%d", i, count));
    }
    PRECONDITION(i == count);
    delete[] w;
}

void YWindowManager::getWorkArea(const YFrameWindow *frame,
                                 int *mx, int *my, int *Mx, int *My, int xiscreen) const
{
    bool whole = false;
    long ws = -1;

    if (frame) {
        if (!frame->inWorkArea())
            whole = true;

        ws = frame->getWorkspace();
        if (frame->isSticky())
            ws = activeWorkspace();

        if (ws < 0 || ws >= fWorkAreaCount)
            whole = true;
    } else
        whole = true;


    if (whole) {
        *mx = 0;
        *my = 0;
        *Mx = width();
        *My = height();
    } else {

/// TODO #warning "rewrite workarea determine code (per workspace)"
#if 1
        *mx = fWorkArea[ws].fMinX;
        *my = fWorkArea[ws].fMinY;
        *Mx = fWorkArea[ws].fMaxX;
        *My = fWorkArea[ws].fMaxY;
#endif
    }

    if (xiscreen != -1) {
        int dx, dy, dw, dh;
        manager->getScreenGeometry(&dx, &dy, &dw, &dh, xiscreen);

        if (*mx < dx)
            *mx = dx;
        if (*my < dy)
            *my = dy;
        if (*Mx > dx + dw)
            *Mx = dx + dw;
        if (*My > dy + dh)
            *My = dy + dh;
    }
}

void YWindowManager::getWorkAreaSize(const YFrameWindow *frame, int *Mw,int *Mh) {
    int mx, my, Mx, My;
    manager->getWorkArea(frame, &mx, &my, &Mx, &My);
    *Mw = Mx - mx;
    *Mh = My - my;
}

void YWindowManager::updateArea(long workspace, int l, int t, int r, int b) {
    if (workspace >= 0 && workspace <= fWorkAreaCount) {
        struct WorkAreaRect *wa = fWorkArea + workspace;

        if (l > wa->fMinX) wa->fMinX = l;
        if (t > wa->fMinY) wa->fMinY = t;
        if (r < wa->fMaxX) wa->fMaxX = r;
        if (b < wa->fMaxY) wa->fMaxY = b;
    } else if (workspace == -1) {
        for (int ws = 0; ws < fWorkAreaCount; ws++) {
            struct WorkAreaRect *wa = fWorkArea + ws;

            if (l > wa->fMinX) wa->fMinX = l;
            if (t > wa->fMinY) wa->fMinY = t;
            if (r < wa->fMaxX) wa->fMaxX = r;
            if (b < wa->fMaxY) wa->fMaxY = b;
        }
    }
}

void YWindowManager::updateWorkArea() {
    int fOldWorkAreaCount = fWorkAreaCount;
    struct WorkAreaRect *fOldWorkArea = fWorkArea;

    fWorkAreaCount = 0;
    fWorkArea = 0;

    fWorkArea = new struct WorkAreaRect[::workspaceCount];
    if (fWorkArea == 0)
        return;
    fWorkAreaCount = ::workspaceCount;

    for (int i = 0; i < fWorkAreaCount; i++) {
        fWorkArea[i].fMinX = 0;
        fWorkArea[i].fMinY = 0;
        fWorkArea[i].fMaxX = width();
        fWorkArea[i].fMaxY = height();
    }

    for (YFrameWindow *w = topLayer();
         w;
         w = w->nextLayer())
    {
        if (w->isHidden() ||
            w->isRollup() ||
            w->isIconic() ||
            w->isMinimized())
            continue;

        long ws = w->getWorkspace();
        if (w->isSticky())
            ws = -1;

        {
            int l = w->strutLeft();
            int t = w->strutTop();
            int r = width() - w->strutRight();
            int b = height() - w->strutBottom();

            updateArea(ws, l, t, r, b);
        }

        if (w->doNotCover() ||
            limitByDockLayer && w->getActiveLayer() == WinLayerDock)
        {
            int midX = width() / 4;
            int midY = height() / 4;
            bool const isHoriz(w->width() > w->height());

            int l = 0;
            int t = 0;
            int r = width();
            int b = height();

            if (isHoriz) {
                if (w->y() + int(w->height()) < midY) {
                    t = w->y() + w->height();
                } else if (w->y() > height() - midY) {
                    b = w->y();
                }
            } else {
                if (w->x() + int(w->width()) < midX)
                    l = w->x() + w->width();
                else if (w->x() > width() - midX)
                    r = w->x();
            }
            updateArea(ws, l, t, r, b);
        }
    }

    announceWorkArea();
    if (fWorkAreaMoveWindows) {
        for (long ws = 0; ws < fWorkAreaCount; ws++) {
            if (ws >= fOldWorkAreaCount)
                break;

            int const deltaX = fWorkArea[ws].fMinX - fOldWorkArea[ws].fMinX;
            int const deltaY = fWorkArea[ws].fMinY - fOldWorkArea[ws].fMinY;

            if (deltaX != 0 || deltaY != 0)
                relocateWindows(ws, deltaX, deltaY);
        }
    }
    if (fOldWorkArea) {
        delete [] fOldWorkArea;
    }
    resizeWindows();
}

void YWindowManager::announceWorkArea() {
    int nw = workspaceCount();
#ifdef WMSPEC_HINTS
    long *area = new long[nw * 4];

    if (!area)
        return;

    for (int ws = 0; ws < nw; ws++) {
        area[ws * 4    ] = fWorkArea[ws].fMinX;
        area[ws * 4 + 1] = fWorkArea[ws].fMinY;
        area[ws * 4 + 2] = fWorkArea[ws].fMaxX - fWorkArea[ws].fMinX;
        area[ws * 4 + 3] = fWorkArea[ws].fMaxY - fWorkArea[ws].fMinY;
    }

    XChangeProperty(xapp->display(), handle(),
                    _XA_NET_WORKAREA,
                    XA_CARDINAL,
                    32, PropModeReplace,
                    (unsigned char *)area, nw * 4);
    delete [] area;
#endif
#ifdef GNOME1_HINTS
    if (fActiveWorkspace != -1) {
        long area[4];
        int cw = fActiveWorkspace;
        area[0] = fWorkArea[cw].fMinX;
        area[1] = fWorkArea[cw].fMinY;
        area[2] = fWorkArea[cw].fMaxX;
        area[3] = fWorkArea[cw].fMaxY;

        XChangeProperty(xapp->display(), handle(),
                        _XA_WIN_WORKAREA,
                        XA_CARDINAL,
                        32, PropModeReplace,
                        (unsigned char *)area, 4);
    }
#endif
}

void YWindowManager::relocateWindows(long workspace, int dx, int dy) {
/// TODO #warning "needs a rewrite (save old work area) for each workspace"
#if 1
    for (YFrameWindow * f = topLayer(); f; f = f->nextLayer())
        if (f->inWorkArea()) {
            if (f->getWorkspace() == workspace ||
                (f->isSticky() && workspace == activeWorkspace()))
            {
                f->setNormalPositionOuter(f->x() + dx, f->y() + dy);
            }
        }
#endif
}

void YWindowManager::resizeWindows() {
    for (YFrameWindow * f = topLayer(); f; f = f->nextLayer()) {
        if (f->inWorkArea()) {
            if (f->isMaximized())
                f->updateDerivedSize(0);
                f->updateLayout();
        }
    }
}

void YWindowManager::activateWorkspace(long workspace) {
    if (workspace != fActiveWorkspace) {
        YFrameWindow *toFocus = getLastFocus(workspace);

        lockFocus();
///        XSetInputFocus(app->display(), desktop->handle(), RevertToNone, CurrentTime);

#ifdef CONFIG_TASKBAR
        if (taskBar && taskBar->workspacesPane() &&
            fActiveWorkspace != (long)WinWorkspaceInvalid) {
            if (taskBar->workspacesPane()->workspaceButton(fActiveWorkspace))
            {
                taskBar->workspacesPane()->workspaceButton(fActiveWorkspace)->setPressed(0);
            }
        }
#endif
        fLastWorkspace = fActiveWorkspace;
        fActiveWorkspace = workspace;
#ifdef CONFIG_TASKBAR
        if (taskBar && taskBar->workspacesPane() &&
            taskBar->workspacesPane()->workspaceButton(fActiveWorkspace))
        {
            taskBar->workspacesPane()->workspaceButton(fActiveWorkspace)->setPressed(1);
        }
#endif

        long ws = fActiveWorkspace;
#ifdef GNOME1_HINTS

        XChangeProperty(xapp->display(), handle(),
                        _XA_WIN_WORKSPACE,
                        XA_CARDINAL,
                        32, PropModeReplace,
                        (unsigned char *)&ws, 1);
#endif
#ifdef WMSPEC_HINTS

        XChangeProperty(xapp->display(), handle(),
                        _XA_NET_CURRENT_DESKTOP,
                        XA_CARDINAL,
                        32, PropModeReplace,
                        (unsigned char *)&ws, 1);
#endif

#if 1 // not needed when we drop support for GNOME hints
        updateWorkArea();
#endif
        resizeWindows();

        YFrameWindow *w;

        for (w = bottomLayer(); w; w = w->prevLayer())
            if (!w->visibleNow()) {
                w->updateState();
#ifdef CONFIG_TASKBAR
                w->updateTaskBar();
#endif
            }

        for (w = topLayer(); w; w = w->nextLayer())
            if (w->visibleNow()) {
                w->updateState();
#ifdef CONFIG_TASKBAR
                w->updateTaskBar();
#endif
            }
        unlockFocus();
        setFocus(toFocus);
        resetColormap(true);

#ifdef CONFIG_TASKBAR
        if (taskBar) taskBar->relayoutNow();
#endif

#ifndef LITE
        if (workspaceSwitchStatus
#ifdef CONFIG_TASKBAR
            && (!showTaskBar || !taskBarShowWorkspaces || taskBarAutoHide)
#endif
           )
            statusWorkspace->begin(workspace);
#endif
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWorkspaceChange);
#endif
        updateWorkArea();
    }
}

void YWindowManager::setWinWorkspace(long workspace) {
    if (workspace >= workspaceCount() || workspace < 0) {
        MSG(("invalid workspace switch %ld", (long)workspace));
        return;
    }
    activateWorkspace(workspace);
}

void YWindowManager::wmCloseSession() { // ----------------- shutdow started ---
    for (YFrameWindow * f(topLayer()); f; f = f->nextLayer())
        if (f->client()->adopted()) // not to ourselves?
            f->wmClose();
}

void YWindowManager::getIconPosition(YFrameWindow *frame, int *iconX, int *iconY) {
    static int row, col;
    static bool init = false;
    MiniIcon *iw = frame->getMiniIcon();

    int mrow, mcol, Mrow, Mcol; /* Minimum and maximum for rows and columns */
    int width, height; /* column width and row height */
    int drow, dcol; /* row and column directions */
    int *iconRow, *iconCol;

    if (miniIconsPlaceHorizontal) {
	manager->getWorkArea(frame, &mcol, &mrow, &Mcol, &Mrow);
	width = iw->width();
	height = iw->height();
	drow = (int)miniIconsBottomToTop * -2 + 1;
	dcol = (int)miniIconsRightToLeft * -2 + 1;
	iconRow = iconY;
	iconCol = iconX;
    } else {
	manager->getWorkArea(frame, &mrow, &mcol, &Mrow, &Mcol);
	width = iw->height();
	height = iw->width();
	drow = (int)miniIconsRightToLeft * -2 + 1;
	dcol = (int)miniIconsBottomToTop * -2 + 1;
	iconRow = iconX;
	iconCol = iconY;
    }

    /* Calculate start row and start column */
    int srow = (drow > 0) ? mrow : (Mrow - height);
    int scol = (dcol > 0) ? mcol : (Mcol - width);

    if (!init) {
	row = srow;
	col = scol;
	init = true;
    }

    /* Return values */
    *iconRow = row;
    *iconCol = col;

    /* Set row and column to new position */
    col += width * dcol;

    int w2 = width / 2;
    if (col >= Mcol - w2 || col < mcol - w2) {
        row += height * drow;
        col = scol;
	int h2 = height / 2;
        if (row >= Mrow - h2 || row < mrow - h2)
	    init = false;
    }
}

int YWindowManager::windowCount(long workspace) {
    int count = 0;

    for (int layer = 0 ; layer <= WinLayerCount; layer++) {
        for (YFrameWindow *frame = top(layer); frame; frame = frame->next()) {
            if (!frame->visibleOn(workspace))
                continue;
#ifndef NO_WINDOW_OPTIONS
            if (frame->frameOptions() & YFrameWindow::foIgnoreWinList)
                continue;
#endif
            if (workspace != activeWorkspace() &&
                frame->visibleOn(activeWorkspace()))
                continue;
            count++;
        }
    }
    return count;
}

void YWindowManager::resetColormap(bool active) {
    if (active) {
        if (manager->colormapWindow() && manager->colormapWindow()->client())
            manager->installColormap(manager->colormapWindow()->client()->colormap());
    } else {
        manager->installColormap(None);
    }
}

void YWindowManager::handleProperty(const XPropertyEvent &property) {
#ifndef NO_WINDOW_OPTIONS
    if (property.atom == XA_IcewmWinOptHint) {
        Atom type;
        int format;
        unsigned long nitems, lbytes;
        unsigned char *propdata;

        if (XGetWindowProperty(xapp->display(), handle(),
                               XA_IcewmWinOptHint, 0, 8192, True, XA_IcewmWinOptHint,
                               &type, &format, &nitems, &lbytes,
                               &propdata) == Success && propdata)
        {
            char *p = (char *)propdata;
            char *e = (char *)propdata + nitems;

            while (p < e) {
                char *clsin;
                char *option;
                char *arg;

                clsin = p;
                while (p < e && *p) p++;
                if (p == e)
                    break;
                p++;

                option = p;
                while (p < e && *p) p++;
                if (p == e)
                    break;
                p++;

                arg = p;
                while (p < e && *p) p++;
                if (p == e)
                    break;
                p++;

                if (p != e)
                    break;

                hintOptions->setWinOption(clsin, option, arg);
            }
            XFree(propdata);
        }
    }
#endif
}

void YWindowManager::updateClientList() {
#if defined(GNOME1_HINTS) || defined(WMSPEC_HINTS)
    int count = 0;
    XID *ids = 0;

    for (YFrameWindow *frame = topLayer(); frame; frame = frame->nextLayer()) {
        if (frame->client() && frame->client()->adopted())
            count++;
    }

    if ((ids = new XID[count]) != 0) {
        int w = 0;
        for (YFrameWindow *frame2 = topLayer(); frame2; frame2 = frame2->nextLayer()) {
            if (frame2->client() && frame2->client()->adopted())
                ids[count - 1 - w++] = frame2->client()->handle();
        }
        PRECONDITION(w == count);
    }
#ifdef GNOME1_HINTS
    XChangeProperty(xapp->display(), desktop->handle(),
                    _XA_WIN_CLIENT_LIST,
                    XA_CARDINAL,
                    32, PropModeReplace,
                    (unsigned char *)ids, count);
#endif
#ifdef WMSPEC_HINTS
    XChangeProperty(xapp->display(), desktop->handle(),
                    _XA_NET_CLIENT_LIST_STACKING,
                    XA_WINDOW,
                    32, PropModeReplace,
                    (unsigned char *)ids, count);

    if (ids) {
        int w = 0;
        for (YFrameWindow *frame2 = firstFrame(); frame2; frame2 = frame2->nextCreated()) {
            if (frame2->client() && frame2->client()->adopted())
                ids[w++] = frame2->client()->handle();
        }
        PRECONDITION(w == count);
    }

    XChangeProperty(xapp->display(), desktop->handle(),
                    _XA_NET_CLIENT_LIST,
                    XA_WINDOW,
                    32, PropModeReplace,
                    (unsigned char *)ids, count);
#endif
    delete [] ids;
#endif
    checkLogout();
}

void YWindowManager::checkLogout() {
    if (fShuttingDown && !haveClients()) {
        if (rebootOrShutdown == 1 && rebootCommand && rebootCommand[0]) {
            msg("reboot... (%s)", rebootCommand);
            system(rebootCommand);
        } else if (rebootOrShutdown == 2 && shutdownCommand && shutdownCommand[0]) {
            msg("shutdown ... (%s)", shutdownCommand);
            system(shutdownCommand);
        } else
            app->exit(0);
    }
}

void YWindowManager::removeClientFrame(YFrameWindow *frame) {
    if (fArrangeInfo) {
        for (int i = 0; i < fArrangeCount; i++)
            if (fArrangeInfo[i].frame == frame)
                fArrangeInfo[i].frame = 0;
    }
    for (int w = 0; w < workspaceCount(); w++) {
        if (fFocusedWindow[w] == frame) {
            fFocusedWindow[w] = frame->nextLayer();
        }
    }
    if (frame == getFocus())
        manager->loseFocus(frame);
    if (frame == getFocus())
        setFocus(0);
    if (colormapWindow() == frame)
        setColormapWindow(getFocus());
    if (frame->affectsWorkArea())
        updateWorkArea();
}

void YWindowManager::notifyFocus(YFrameWindow *frame) {
    long wnd = frame ? frame->client()->handle() : None;
    XChangeProperty(xapp->display(), handle(),
                    _XA_NET_ACTIVE_WINDOW,
                    XA_WINDOW,
                    32, PropModeReplace,
                    (unsigned char *)&wnd, 1);

}

void YWindowManager::switchFocusTo(YFrameWindow *frame, bool reorderFocus) {

    if (frame != fFocusWin) {
        if (fFocusWin)
            fFocusWin->loseWinFocus();
        fFocusWin = frame;
        ///msg("setting %lX", fFocusWin);
        if (fFocusWin)
            fFocusWin->setWinFocus();

        fFocusedWindow[activeWorkspace()] = frame;
    }
    if (frame && frame->nextFocus() && reorderFocus) {
        frame->removeFocusFrame();
        frame->insertFocusFrame(true);
    }
    notifyFocus(frame);
}

void YWindowManager::switchFocusFrom(YFrameWindow *frame) {
    if (frame == fFocusWin) {
        if (fFocusWin) {
            ///msg("losing %lX", fFocusWin);
            fFocusWin->loseWinFocus();
        }
        fFocusWin = 0;
    }
}

#ifdef CONFIG_WINMENU
void YWindowManager::popupWindowListMenu(YWindow *owner, int x, int y) {
    windowListMenu->popup(owner, 0, 0, x, y,
                          YPopupWindow::pfCanFlipVertical |
                          YPopupWindow::pfCanFlipHorizontal |
                          YPopupWindow::pfPopupMenu);
}
#endif

void YWindowManager::popupStartMenu(YWindow *owner) { // !! fix
#ifdef CONFIG_TASKBAR
    if (showTaskBar && taskBar && taskBarShowStartMenu)
        taskBar->popupStartMenu();
    else
#endif
    {
#ifndef NO_CONFIGURE_MENUS
        rootMenu->popup(owner, 0, 0, 0, 0,
                        YPopupWindow::pfCanFlipVertical |
                        YPopupWindow::pfCanFlipHorizontal |
                        YPopupWindow::pfPopupMenu);
#endif
    }
}

#ifdef CONFIG_WINMENU
void YWindowManager::popupWindowListMenu(YWindow *owner) {
#ifdef CONFIG_TASKBAR
    if (showTaskBar && taskBar && taskBarShowWindowListMenu)
        taskBar->popupWindowListMenu();
    else
#endif
        popupWindowListMenu(owner, 0, 0);
}
#endif

void YWindowManager::switchToWorkspace(long nw, bool takeCurrent) {
    if (nw >= 0 && nw < workspaceCount()) {
        YFrameWindow *frame = getFocus();
        if (takeCurrent && frame && !frame->isSticky()) {
            lockFocus();
            frame->wmOccupyAll();
            frame->wmRaise();
            activateWorkspace(nw);
            frame->wmOccupyOnlyWorkspace(nw);
            unlockFocus();
            frame->wmRaise();
            setFocus(frame);
        } else {
            activateWorkspace(nw);
        }
    }
}

void YWindowManager::switchToPrevWorkspace(bool takeCurrent) {
    long nw = (activeWorkspace() + workspaceCount() - 1) % workspaceCount();

    switchToWorkspace(nw, takeCurrent);
}

void YWindowManager::switchToNextWorkspace(bool takeCurrent) {
    long nw = (activeWorkspace() + 1) % workspaceCount();

    switchToWorkspace(nw, takeCurrent);
}

void YWindowManager::switchToLastWorkspace(bool takeCurrent) {
    switchToWorkspace(lastWorkspace(), takeCurrent);
}

void YWindowManager::tilePlace(YFrameWindow *w, int tx, int ty, int tw, int th) {
    w->setState(WinStateMinimized |
                WinStateRollup |
                WinStateMaximizedVert |
                WinStateMaximizedHoriz |
                WinStateHidden, 0);
    tw -= 2 * w->borderXN();
    th -= 2 * w->borderYN() + w->titleYN();
    w->client()->constrainSize(tw, th, ///WinLayerNormal,
                               0);
    tw += 2 * w->borderXN();
    th += 2 * w->borderYN() + w->titleYN();
    w->setNormalGeometryOuter(tx, ty, tw, th);
}

void YWindowManager::tileWindows(YFrameWindow **w, int count, bool vertical) {
    saveArrange(w, count);

    if (count == 0)
        return ;

    int curWin = 0;
    int cols = 1;

    while (cols * cols <= count)
        cols++;
    cols--;

    int areaX, areaY, areaW, areaH;

    int mx, my, Mx, My;
    manager->getWorkArea(w[0], &mx, &my, &Mx, &My);

    if (vertical) { // swap meaning of rows/cols
        areaY = mx;
        areaX = my;
        areaH = Mx - mx;
        areaW = My - my;
    } else {
        areaX = mx;
        areaY = my;
        areaW = Mx - mx;
        areaH = My - my;
    }

    int normalRows = count / cols;
    int normalWidth = areaW / cols;
    int windowX = areaX;

    for (int col = 0; col < cols; col++) {
        int rows = normalRows;
        int windowWidth = normalWidth;
        int windowY = areaY;

        if (col >= (cols * (1 + normalRows) - count))
            rows++;
        if (col >= (cols * (1 + normalWidth) - areaW))
            windowWidth++;

        int normalHeight = areaH / rows;

        for (int row = 0; row < rows; row++) {
            int windowHeight = normalHeight;

            if (row >= (rows * (1 + normalHeight) - areaH))
                windowHeight++;

            if (vertical) // swap meaning of rows/cols
                tilePlace(w[curWin++],
                          windowY, windowX, windowHeight, windowWidth);
            else
                tilePlace(w[curWin++],
                          windowX, windowY, windowWidth, windowHeight);

            windowY += windowHeight;
        }
        windowX += windowWidth;
    }
}

void YWindowManager::getWindowsToArrange(YFrameWindow ***win, int *count, bool sticky, bool skipNonMinimizable) {
    YFrameWindow *w = topLayer(WinLayerNormal);

    *count = 0;
    while (w) {
        if (w->owner() == 0 && // not transient ?
            w->visibleOn(activeWorkspace()) && // visible
            (sticky || !w->isSticky()) && // not on all workspaces
            !w->isRollup() &&
            !w->isMinimized() &&
            !w->isHidden() &&
            (!skipNonMinimizable || w->canMinimize()))
        {
            ++*count;
        }
        w = w->next();
    }
    *win = new YFrameWindow *[*count];
    int n = 0;
    w = topLayer(WinLayerNormal);
    if (*win) {
        while (w) {
            if (w->owner() == 0 && // not transient ?
                w->visibleOn(activeWorkspace()) && // visible
                (sticky || !w->isSticky()) && // not on all workspaces
                !w->isRollup() &&
                !w->isMinimized() &&
                !w->isHidden()&&
            (!skipNonMinimizable || w->canMinimize()))
            {
                (*win)[n] = w;
                n++;
            }

            w = w->next();
        }
    }
    PRECONDITION(n == *count);
}

void YWindowManager::saveArrange(YFrameWindow **w, int count) {
    delete [] fArrangeInfo;
    fArrangeCount = count;
    fArrangeInfo = new WindowPosState[count];
    if (fArrangeInfo) {
        for (int i = 0; i < count; i++) {
            fArrangeInfo[i].x = w[i]->x();
            fArrangeInfo[i].y = w[i]->y();
            fArrangeInfo[i].w = w[i]->width();
            fArrangeInfo[i].h = w[i]->height();
            fArrangeInfo[i].state = w[i]->getState();
            fArrangeInfo[i].frame = w[i];
        }
    }
}
void YWindowManager::undoArrange() {
    if (fArrangeInfo) {
        for (int i = 0; i < fArrangeCount; i++) {
            YFrameWindow *f = fArrangeInfo[i].frame;
            if (f) {
                f->setState(WIN_STATE_ALL, fArrangeInfo[i].state);
                f->setNormalGeometryOuter(fArrangeInfo[i].x,
                                          fArrangeInfo[i].y,
                                          fArrangeInfo[i].w,
                                          fArrangeInfo[i].h);
            }
        }
        delete [] fArrangeInfo; fArrangeInfo = 0;
        fArrangeCount = 0;
        focusTopWindow();
    }
}

bool YWindowManager::haveClients() {
    for (YFrameWindow * f(topLayer()); f ; f = f->nextLayer())
        if (f->canClose() && f->client()->adopted())
            return true;

    return false;
}

void YWindowManager::exitAfterLastClient(bool shuttingDown) {
    fShuttingDown = shuttingDown;
    checkLogout();
}

YTimer *EdgeSwitch::fEdgeSwitchTimer(NULL);

EdgeSwitch::EdgeSwitch(YWindowManager *manager, int delta, bool vertical):
YWindow(manager),
fManager(manager),
fCursor(delta < 0 ? vertical ? YWMApp::scrollUpPointer
                             : YWMApp::scrollLeftPointer
                  : vertical ? YWMApp::scrollDownPointer
                             : YWMApp::scrollRightPointer),
fDelta(delta) {
    setStyle(wsOverrideRedirect | wsInputOnly);
    setPointer(YXApplication::leftPointer);
}

EdgeSwitch::~EdgeSwitch() {
    if (fEdgeSwitchTimer && fEdgeSwitchTimer->getTimerListener() == this) {
        fEdgeSwitchTimer->stopTimer();
        fEdgeSwitchTimer->setTimerListener(NULL);
        delete fEdgeSwitchTimer;
        fEdgeSwitchTimer = NULL;
    }
}

void EdgeSwitch::handleCrossing(const XCrossingEvent &crossing) {
    if (crossing.type == EnterNotify && crossing.mode == NotifyNormal) {
        if (!fEdgeSwitchTimer)
            fEdgeSwitchTimer = new YTimer(edgeSwitchDelay);
        if (fEdgeSwitchTimer) {
            fEdgeSwitchTimer->setTimerListener(this);
            fEdgeSwitchTimer->startTimer();
            setPointer(fCursor);
        }
    } else if (crossing.type == LeaveNotify && crossing.mode == NotifyNormal) {
        if (fEdgeSwitchTimer && fEdgeSwitchTimer->getTimerListener() == this) {
            fEdgeSwitchTimer->stopTimer();
            fEdgeSwitchTimer->setTimerListener(NULL);
            setPointer(YXApplication::leftPointer);
        }
    }
}

bool EdgeSwitch::handleTimer(YTimer *t) {
    if (t != fEdgeSwitchTimer)
        return false;

    if (fDelta == -1)
        fManager->switchToPrevWorkspace(false);
    else
        fManager->switchToNextWorkspace(false);

    if (edgeContWorkspaceSwitching) {
        return true;
    } else {
        setPointer(YXApplication::leftPointer);
        return false;
    }
}

int YWindowManager::getScreen() {
#ifdef XINERAMA
    if (xiHeads == 0)
        return 0;
    if (fFocusWin)
        return fFocusWin->getScreen();
#endif
    return 0;
}

void YWindowManager::doWMAction(long action) {
    XClientMessageEvent xev;
    memset(&xev, 0, sizeof(xev));

    xev.type = ClientMessage;
    xev.window = handle();
    xev.message_type = _XA_ICEWM_ACTION;
    xev.format = 32;
    xev.data.l[0] = CurrentTime;
    xev.data.l[1] = action;

    MSG(("new mask/state: %d/%d", xev.data.l[0], xev.data.l[1]));

    XSendEvent(xapp->display(), handle(), False, SubstructureNotifyMask, (XEvent *) &xev);
}

#ifdef CONFIG_XRANDR
void YWindowManager::handleRRScreenChangeNotify(const XRRScreenChangeNotifyEvent &xrrsc) {
#if RANDR_MAJOR >= 1
    XRRUpdateConfiguration((XEvent *)&xrrsc);
#endif

    if (width() != xrrsc.width ||
        height() != xrrsc.height)
    {

        MSG(("xrandr: %d %d",
             xrrsc.width,
             xrrsc.height));
        setSize(xrrsc.width, xrrsc.height);
        updateXineramaInfo();
        updateWorkArea();
#ifdef CONFIG_TASKBAR
        if (taskBar) {
            taskBar->relayout();
            taskBar->relayoutNow();
            taskBar->updateLocation();
        }
#endif
/// TODO #warning "make something better"
        wmapp->actionPerformed(actionArrange, 0);
    }
}
#endif
