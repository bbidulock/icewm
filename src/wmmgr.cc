/*
 * IceWM
 *
 * Copyright (C) 1997-2013 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "ymenu.h"
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
#include "atasks.h"
#include "yxcontext.h"

YContext<YFrameClient> clientContext("clientContext", false);
YContext<YFrameWindow> frameContext("framesContext", false);

YAction layerActionSet[WinLayerCount];

YWindowManager::YWindowManager(
    IApp *app,
    YActionListener *wmActionListener,
    YSMListener *smListener,
    YWindow *parent,
    Window win)
    : YDesktop(parent, win),
    app(app),
    wmActionListener(wmActionListener),
    smActionListener(smListener)
{
    fWmState = wmSTARTUP;
    fShowingDesktop = false;
    fShuttingDown = false;
    fOtherScreenFocused = false;
    fActiveWindow = (Window) -1;
    fFocusWin = 0;
    lockFocusCount = 0;
    fLayout = (DesktopLayout) {
         _NET_WM_ORIENTATION_HORZ,
         MAXWORKSPACES,
         1,
         _NET_WM_TOPLEFT,
    };

    fColormapWindow = 0;
    fActiveWorkspace = WinWorkspaceInvalid;
    fLastWorkspace = WinWorkspaceInvalid;
    fArrangeCount = 0;
    fArrangeInfo = 0;
    rootProxy = 0;
    fWorkAreaMoveWindows = false;
    fWorkArea = 0;
    fWorkAreaWorkspaceCount = 0;
    fWorkAreaScreenCount = 0;
    fFullscreenEnabled = true;
    for (int w = 0; w < MAXWORKSPACES; w++)
        fFocusedWindow[w] = 0;
    fCreatedUpdated = true;
    fLayeredUpdated = true;

    setStyle(wsManager);
    setPointer(YXApplication::leftPointer);
#ifdef CONFIG_XRANDR
    if (xrandrSupported) {
#if RANDR_MAJOR >= 1
        XRRSelectInput(xapp->display(), handle(),
                       RRScreenChangeNotifyMask
#if RANDR_MINOR >= 2
                       | RRCrtcChangeNotifyMask
                       | RROutputChangeNotifyMask
                       | RROutputPropertyNotifyMask
#endif
                      );
#else
        XRRScreenChangeSelectInput(xapp->display(), handle(), True);
#endif
    }
#endif

    fTopWin = new YWindow();
    fTopWin->setStyle(YWindow::wsOverrideRedirect);
    fTopWin->setGeometry(YRect(-1, -1, 1, 1));
    fTopWin->setTitle("IceTopWin");
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

    YWindow::setWindowFocus();
}

YWindowManager::~YWindowManager() {
    if (fWorkArea) {
        for (int i = 0; i < fWorkAreaWorkspaceCount; i++)
            delete [] fWorkArea[i];
        delete [] fWorkArea;
    }
    delete fBottomSwitch;
    delete fTopSwitch;
    delete fRightSwitch;
    delete fLeftSwitch;
    delete fTopWin;
    delete rootProxy;
}

void YWindowManager::grabKeys() {
    XUngrabKey(xapp->display(), AnyKey, AnyModifier, handle());

    ///if (taskBar && taskBar->addressBar())
        GRAB_WMKEY(gKeySysAddressBar);
    if (quickSwitch) {
        GRAB_WMKEY(gKeySysSwitchNext);
        GRAB_WMKEY(gKeySysSwitchLast);
        GRAB_WMKEY(gKeySysSwitchClass);
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
    if (taskBar != 0) {
        GRAB_WMKEY(gKeySysCollapseTaskBar);
        GRAB_WMKEY(gKeyTaskBarSwitchNext);
        GRAB_WMKEY(gKeyTaskBarSwitchPrev);
        GRAB_WMKEY(gKeyTaskBarMoveNext);
        GRAB_WMKEY(gKeyTaskBarMovePrev);
    }

    {
        YObjectArray<KProgram>::IterType k = keyProgs.iterator();
        while (++k) {
            grabVKey(k->key(), k->modifiers());
        }
    }
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
    {
        YFrameWindow *ff = topLayer();
        while (ff != 0) {
            ff->grabKeys();
            ff = ff->nextLayer();
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
            rootProxy->setTitle("IceRootProxy");
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

    YObjectArray<KProgram>::IterType p = keyProgs.iterator();
    while (++p) {
        if (!p->isKey(k, vm)) continue;

        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        p->open(key.state);
        return true;
    }

    if (wmapp->getSwitchWindow() != 0) {
        if (IS_WMKEY(k, vm, gKeySysSwitchNext)) {
            XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
            wmapp->getSwitchWindow()->begin(true, key.state);
            return true;
        } else if (IS_WMKEY(k, vm, gKeySysSwitchLast)) {
            XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
            wmapp->getSwitchWindow()->begin(false, key.state);
            return true;
        } else if (gKeySysSwitchClass.eq(k, vm)) {
            char *prop = frame && frame->client()->adopted()
                       ? frame->client()->classHint()->resource() : 0;
            XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
            wmapp->getSwitchWindow()->begin(true, key.state, prop);
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
    } else if (IS_WMKEY(k, vm, gKeySysDialog)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        if (wmapp->getCtrlAltDelete()) {
            wmapp->getCtrlAltDelete()->activate();
        }
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWinListMenu)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        popupWindowListMenu(this);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysMenu)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        popupStartMenu(this);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysWindowList)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        wmActionListener->actionPerformed(actionWindowList, 0);
        return true;
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
        wmActionListener->actionPerformed(actionTileVertical, 0);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysTileHorizontal)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        wmActionListener->actionPerformed(actionTileHorizontal, 0);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysCascade)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        wmActionListener->actionPerformed(actionCascade, 0);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysArrange)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        wmActionListener->actionPerformed(actionArrange, 0);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysUndoArrange)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        wmActionListener->actionPerformed(actionUndoArrange, 0);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysArrangeIcons)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        wmActionListener->actionPerformed(actionArrangeIcons, 0);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysMinimizeAll)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        wmActionListener->actionPerformed(actionMinimizeAll, 0);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysHideAll)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        wmActionListener->actionPerformed(actionHideAll, 0);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysAddressBar)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        if (taskBar) {
            taskBar->showAddressBar();
            return true;
        }
    } else if (IS_WMKEY(k, vm, gKeySysShowDesktop)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        wmActionListener->actionPerformed(actionShowDesktop, 0);
        return true;
    } else if (IS_WMKEY(k, vm, gKeySysCollapseTaskBar)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        if (taskBar)
            taskBar->handleCollapseButton();
        return true;

    } else if (IS_WMKEY(k, vm, gKeyTaskBarSwitchPrev)) {
        if (taskBar)
            taskBar->switchToPrev();
        return true;
    } else if (IS_WMKEY(k, vm, gKeyTaskBarSwitchNext)) {
        if (taskBar)
            taskBar->switchToNext();
        return true;
    } else if (IS_WMKEY(k, vm, gKeyTaskBarMovePrev)) {
        if (taskBar)
            taskBar->movePrev();
        return true;
    } else if (IS_WMKEY(k, vm, gKeyTaskBarMoveNext)) {
        if (taskBar)
            taskBar->moveNext();
        return true;
    }
    return false;
}

bool YWindowManager::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        KeySym k = keyCodeToKeySym(key.keycode);
        unsigned int m = KEY_MODMASK(key.state);
        unsigned int vm = VMod(m);

        MSG(("down key: %lu, mod: %d", k, m));
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
        KeySym k = keyCodeToKeySym(key.keycode);
        unsigned int m = KEY_MODMASK(key.state);

        (void)m;
#ifdef DEBUG
        MSG(("up key: %lu, mod: %d", k, m));
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
        if (button.send_event == False)
            XUngrabPointer(xapp->display(), CurrentTime);
        XSendEvent(xapp->display(),
                   rootProxy->handle(),
                   False,
                   SubstructureNotifyMask,
                   (XEvent *) &button);
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
        if (button.button + 10 == (unsigned) rootMenuButton) {
            if (rootMenu)
                rootMenu->popup(0, 0, 0, button.x, button.y,
                                YPopupWindow::pfCanFlipVertical |
                                YPopupWindow::pfCanFlipHorizontal |
                                YPopupWindow::pfPopupMenu |
                                YPopupWindow::pfButtonDown);
            break;
        }
        if (button.button + 10 == (unsigned) rootWinMenuButton) {
            popupWindowListMenu(this, button.x, button.y);
            break;
        }
    } while (0);
    YWindow::handleButton(button);
}

void YWindowManager::handleClick(const XButtonEvent &up, int count) {
    if (count == 1) do {
        if (up.button == (unsigned) rootMenuButton) {
            if (rootMenu)
                rootMenu->popup(0, 0, 0, up.x, up.y,
                                YPopupWindow::pfCanFlipVertical |
                                YPopupWindow::pfCanFlipHorizontal |
                                YPopupWindow::pfPopupMenu);
            break;
        }
        if (up.button == (unsigned) rootWinMenuButton) {
            popupWindowListMenu(this, up.x, up.y);
            break;
        }
        if (up.button == (unsigned) rootWinListButton) {
            if (windowList)
                windowList->showFocused(up.x_root, up.y_root);
            break;
        }
    } while (0);
}

void YWindowManager::handleConfigure(const XConfigureEvent &configure) {
    if (configure.window == handle()) {
#ifdef CONFIG_XRANDR
        UpdateScreenSize((XEvent *)&configure);
#endif
    }
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

void YWindowManager::handleUnmapNotify(const XUnmapEvent &unmap) {
#if 1
    if (unmap.send_event) {
        if (unmap.window != handle() && handle() != 0) {
            xapp->handleWindowEvent(unmap.window, *(XEvent *)&unmap);
        } else {
            MSG(("unhandled root window unmap: %lX %lX", (long)unmap.window, (long)handle()));
        }
    }
#endif
}

void YWindowManager::handleDestroyWindow(const XDestroyWindowEvent &destroyWindow) {
    if (destroyWindow.window == handle())
        YWindow::handleDestroyWindow(destroyWindow);
}

void YWindowManager::handleClientMessage(const XClientMessageEvent &message) {
    if (message.message_type == _XA_NET_CURRENT_DESKTOP) {
        MSG(("ClientMessage: _NET_CURRENT_DESKTOP => %ld", message.data.l[0]));
        setWinWorkspace(message.data.l[0]);
        return;
    }
    if (message.message_type == _XA_NET_NUMBER_OF_DESKTOPS) {
        MSG(("ClientMessage: _NET_NUMBER_OF_DESKTOPS => %ld", message.data.l[0]));
        long ws = message.data.l[0];
        if (ws >= 1 && ws <= MAXWORKSPACES) {
            if (ws > workspaceCount()) {
                int more = ws - workspaceCount();
                for (int i=0; i<more; i++)
                    appendNewWorkspace();
            } else
            if (ws < workspaceCount()) {
                int less = workspaceCount() - ws;
                for (int i=0; i<less; i++)
                    removeLastWorkspace();
            }
        }
        return;
    }
    if (message.message_type == _XA_NET_SHOWING_DESKTOP) {
        MSG(("ClientMessage: _NET_SHOWING_DESKTOP => %ld", message.data.l[0]));
        if (message.data.l[0] == 0 && fShowingDesktop) {
            undoArrange();
            setShowingDesktop(false);
        } else
        if (message.data.l[0] != 0 && !fShowingDesktop) {
            YFrameWindow **w = 0;
            int count = 0;
            getWindowsToArrange(&w, &count, true, true);
            if (w && count > 0)
                setWindows(w, count, actionMinimizeAll);
            setShowingDesktop(true);
            delete [] w;
        }
        return;
    }
    if (message.message_type == _XA_NET_REQUEST_FRAME_EXTENTS) {
        Window win = message.window;
        XWindowAttributes attributes;
        if (!XGetWindowAttributes(xapp->display(), win, &attributes))
            return;

        if (attributes.map_state != IsUnmapped)
            return;

        if (attributes.override_redirect)
            return;

        YFrameClient* client = new YFrameClient(desktop, 0);
        client->setSize(attributes.width, attributes.height);
        client->setPosition(attributes.x, attributes.y);
        client->setBorder(attributes.border_width);
        int count(0);
        Atom* atoms = XListProperties(xapp->display(), win, &count);
        for (int i = 0; i < count; ++i) {
            Atom type(None);
            int format(0);
            unsigned long nitems(0);
            unsigned long after(0);
            unsigned char* prop(0);
            int p = XGetWindowProperty(xapp->display(), win, atoms[i],
                                       0, 100*1000, False, AnyPropertyType,
                                       &type, &format, &nitems,
                                       &after, &prop);
            if (p == Success && type && format && nitems) {
                XChangeProperty(xapp->display(), client->handle(),
                                atoms[i], type, format, PropModeReplace,
                                prop, nitems);
            }
            if (prop && p == Success)
                XFree(prop);
        }
        if (atoms)
            XFree(atoms);

        YFrameWindow* frame = new YFrameWindow(wmActionListener);
        bool activate = false;
        bool focus = false;
        frame->doManage(client, activate, focus);
        long bx = frame->borderX();
        long by = frame->borderY();
        long ty = frame->titleY();
        delete frame;

        long extents[] = { bx, bx, by + ty, by, };
        XChangeProperty(xapp->display(), win,
                        _XA_NET_FRAME_EXTENTS, XA_CARDINAL,
                        32, PropModeReplace,
                        (unsigned char *) extents, int ACOUNT(extents));

        return;
    }
    if (message.message_type == _XA_WM_PROTOCOLS &&
        message.data.l[0] == (long) _XA_NET_WM_PING) {
        YFrameWindow* frame = findFrame(message.data.l[2]);
        if (frame) {
            frame->client()->recvPing(message);
        }
        return;
    }
    if (message.message_type == _XA_WIN_WORKSPACE) {
        MSG(("ClientMessage: _WIN_WORKSPACE => %ld", message.data.l[0]));
        setWinWorkspace(message.data.l[0]);
        return;
    }
    if (message.message_type == _XA_ICEWM_ACTION) {
        MSG(("ClientMessage: _ICEWM_ACTION => %ld", message.data.l[1]));
        WMAction action = WMAction(message.data.l[1]);
        switch (action) {
        case ICEWM_ACTION_LOGOUT:
        case ICEWM_ACTION_CANCEL_LOGOUT:
        case ICEWM_ACTION_SHUTDOWN:
        case ICEWM_ACTION_SUSPEND:
        case ICEWM_ACTION_REBOOT:
        case ICEWM_ACTION_RESTARTWM:
        case ICEWM_ACTION_WINDOWLIST:
        case ICEWM_ACTION_ABOUT:
            smActionListener->handleSMAction(action);
            break;
        }
    }
}

void YWindowManager::handleFocus(const XFocusChangeEvent &focus) {
    DBG logFocus((const union _XEvent&) focus);
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
    if (isEmpty(resource))
        return None;

    for (YFrameIter iter = focusedReverseIterator(); ++iter; ) {
        YFrameClient* cli(iter->client());
        if (cli && cli->adopted() && !cli->destroyed()) {
            if (cli->classHint()->match(resource)) {
                return cli->handle();
            }
        }
    }

    Window match = None, root = desktop->handle(), parent;
    xsmart<Window> clients;
    unsigned count = 0;
    XQueryTree(xapp->display(), root, &root, &parent, &clients, &count);

    for (unsigned i = 0; match == None && i < count; ++i) {
        YWindow* ywin = windowContext.find(clients[i]);
        if (0 == ywin) {
            match = matchWindow(clients[i], resource);
        }
    }

    return match;
}

Window YWindowManager::findWindow(Window win, char const* resource,
                                  int maxdepth) {
    if (isEmpty(resource))
        return None;

    Window match = None, parent, root;
    xsmart<Window> clients;
    unsigned count = 0;

    XQueryTree(xapp->display(), win, &root, &parent, &clients, &count);

    for (unsigned i = 0; match == None && i < count; ++i) {
        if (matchWindow(clients[i], resource))
            match = clients[i];
    }
    if (maxdepth) {
        for (unsigned i = 0; match == None && i < count; ++i) {
            match = findWindow(clients[i], resource, maxdepth - 1);
        }
    }

    return match;
}

bool YWindowManager::matchWindow(Window win, char const* resource) {
    ClassHint hint;
    return XGetClassHint(xapp->display(), handle(), &hint)
        && hint.match(resource);
}

YFrameWindow *YWindowManager::findFrame(Window win) {
    return frameContext.find(win);
}

YFrameClient *YWindowManager::findClient(Window win) {
    return clientContext.find(win);
}

void YWindowManager::setFocus(YFrameWindow *f, bool canWarp) {
//    updateFullscreenLayerEnable(false);

    YFrameClient *c = f ? f->client() : 0;
    Window w = None;

    if (focusLocked())
        return;
    MSG(("SET FOCUS f=%p", f));

    if (f == 0) {
        YFrameWindow *ff = getFocus();
        if (ff) switchFocusFrom(ff);
    }

    if (f && f->visible()) {
        if (c && c->visible() && !(f->isRollup() || f->isIconic()))
            w = c->handle();
        else
            w = f->handle();

        if (f->getInputFocusHint())
            switchFocusTo(f);

        f->setWmUrgency(false);
    }
#ifdef DEBUG
    if (w == desktop->handle()) {
        MSG(("%lX Focus 0x%lX desktop",
             xapp->getEventTime("focus1"), w));
    } else if (f && w == f->handle()) {
        MSG(("%lX Focus 0x%lX frame %s",
             xapp->getEventTime("focus1"), w, cstring(f->getTitle()).c_str()));
    } else if (f && c && w == c->handle()) {
        MSG(("%lX Focus 0x%lX client %s",
             xapp->getEventTime("focus1"), w, cstring(f->getTitle()).c_str()));
    } else {
        MSG(("%lX Focus 0x%lX",
             xapp->getEventTime("focus1"), w));
    }
#endif

    bool focusUnset(fFocusWin == 0);
    if (w != None) {
        if (f->getInputFocusHint()) {
            XSetInputFocus(xapp->display(), w, RevertToNone,
                           xapp->getEventTime("setFocus"));
            focusUnset = false;
        }
    }
    if (w == None || focusUnset) {
        XSetInputFocus(xapp->display(), fTopWin->handle(), RevertToNone,
                       xapp->getEventTime("setFocus"));
    }

    if (c &&
        w == c->handle() &&
        ((c->protocols() & YFrameClient::wpTakeFocus) ||
         (f->frameOptions() & YFrameWindow::foAppTakesFocus)))
    {
        c->sendTakeFocus();
    }

    if (!pointerColormap)
        setColormapWindow(f);

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

    MSG(("SET FOCUS END"));
    updateFullscreenLayer();
}

/// TODO lose this function
void YWindowManager::loseFocus(YFrameWindow *window) {
    (void)window;
    PRECONDITION(window != 0);
    focusLastWindow();
}

YFrameWindow *YWindowManager::top(long layer) const {
    PRECONDITION(inrange(layer, 0L, WinLayerCount - 1L));
    return fLayers[layer].front();
}

void YWindowManager::setTop(long layer, YFrameWindow *top) {
    if (true || !clientMouseActions) // some programs are buggy
        if (fLayers[layer]) {
            if (raiseOnClickClient)
                fLayers[layer].front()->container()->grabButtons();
        }
    fLayers[layer].prepend(top);
    fLayeredUpdated = true;
    if (true || !clientMouseActions) // some programs are buggy
        if (fLayers[layer]) {
            if (raiseOnClickClient &&
                !(focusOnClickClient && !fLayers[layer].front()->focused()))
                fLayers[layer].front()->container()->releaseButtons();
        }
}

YFrameWindow *YWindowManager::bottom(long layer) const {
    return fLayers[layer].back();
}

void YWindowManager::setBottom(long layer, YFrameWindow *bottom) {
    fLayers[layer].append(bottom);
    fLayeredUpdated = true;
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
    Window winRoot, winParent, *winClients(NULL);

    manager->fWmState = YWindowManager::wmSTARTUP;
    XGrabServer(xapp->display());
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
    if (taskBar)
        taskBar->detachDesktopTray();
    setFocus(0);
    XGrabServer(xapp->display());
    for (unsigned int l = 0; l < WinLayerCount; l++) {
        while (bottom(l)) {
            w = bottom(l)->client()->handle();
            unmanageClient(w, true);
        }
    }
    XSetInputFocus(xapp->display(), PointerRoot, RevertToNone, CurrentTime);
    XUngrabServer(xapp->display());
    XSync(xapp->display(), True);
}

static int addco(int *v, int &n, int c) {
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

int YWindowManager::calcCoverage(bool down, YFrameWindow *frame1, int x, int y, int w, int h) {
    int cover = 0;
    int factor = down ? 2 : 1; // try harder not to cover top windows
    const YRect rect(x, y, w, h);

    YFrameWindow *frame = 0;

    if (down) {
        frame = top(frame1->getActiveLayer());
    } else {
        frame = frame1;
    }
    for (YFrameWindow * f = frame; f ; f = (down ? f->next() : f->prev())) {
        if (f == frame1 || f->isMinimized() || f->isHidden() || !f->isManaged())
            continue;

        if (!f->isAllWorkspaces() && f->getWorkspace() != frame1->getWorkspace())
            continue;

        cover += rect.overlap(f->geometry()) * factor;

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

bool YWindowManager::getSmartPlace(bool down, YFrameWindow *frame1, int &x, int &y, int w, int h, int xiscreen) {
    int mx, my, Mx, My;
    manager->getWorkArea(frame1, &mx, &my, &Mx, &My, xiscreen);

    x = mx;
    y = my;
    int cover, px, py;
    int *xcoord, *ycoord;
    int xcount, ycount;
    int n = 0;
    YFrameWindow *frame = 0;

    if (down) {
        frame = top(frame1->getActiveLayer());
    } else {
        frame = frame1;
    }
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
        if (f == frame1 || f->isMinimized() || f->isHidden() || !f->isManaged() || f->isMaximized())
            continue;

        if (!f->isAllWorkspaces() && f->getWorkspace() != frame1->getWorkspace())
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
    cover = calcCoverage(down, frame1, x, y, w, h);
    while (1) {
        x = xcoord[xn];
        y = ycoord[yn];

        tryCover(down, frame1, x - w, y - h, w, h, px, py, cover, xiscreen);
        tryCover(down, frame1, x - w, y    , w, h, px, py, cover, xiscreen);
        tryCover(down, frame1, x    , y - h, w, h, px, py, cover, xiscreen);
        tryCover(down, frame1, x    , y    , w, h, px, py, cover, xiscreen);

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

    int n = xiInfo.getCount();
    for (int s = 0; s < n; s++)
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

void YWindowManager::setWindows(YFrameWindow **w, int count, YAction action) {
    saveArrange(w, count);

    if (count == 0)
        return;

    lockFocus();
    for (int i = 0; i < count; ++i) {
        YFrameWindow *f = w[i];
        if (action == actionHideAll) {
            if (!f->isHidden())
                f->setState(WinStateHidden, WinStateHidden);
        } else if (action == actionMinimizeAll) {
            if (!f->isMinimized())
                f->setState(WinStateMinimized, WinStateMinimized);
        }
    }
    unlockFocus();
    focusTopWindow();
}

void YWindowManager::getNewPosition(YFrameWindow *frame, int &x, int &y, int w, int h, int xiscreen) {
    if (centerTransientsOnOwner && frame->owner() != 0) {
        x = frame->owner()->x() + frame->owner()->width() / 2 - w / 2;
        y = frame->owner()->y() + frame->owner()->height() / 2 - h / 2;
    } else if (smartPlacement) {
        getSmartPlace(true, frame, x, y, w, h, xiscreen);
    } else {

        static int lastX = 0;
        static int lastY = 0;

        getCascadePlace(frame, lastX, lastY, x, y, w, h);
    }
    if (centerLarge) {
        int mx, my, Mx, My;
        manager->getWorkArea(frame, &mx, &my, &Mx, &My);
        if (w > (Mx - mx) / 2 && h > (My - my) / 2) {
            x = (mx + Mx - w) / 2;   /* = mx + (Mx - mx - w) / 2 */
            if (x < mx) x = mx;
            y = (my + My - h) / 2;   /* = my + (My - my - h) / 2 */
            if (y < my) y = my;
        }
    }
}

void YWindowManager::placeWindow(YFrameWindow *frame,
                                 int x, int y,
                                 int cw, int ch,
                                 bool newClient, bool &
#ifdef CONFIG_SESSION
                                 doActivate
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
            doActivate = false;
    }
    else
#endif
    if (newClient) {
        WindowOption wo(null);
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

    if (newClient && client->adopted() && client->sizeHints() &&
        (!(client->sizeHints()->flags & (USPosition | PPosition)) ||
         ((client->sizeHints()->flags & PPosition)
          && (frame->frameOptions() & YFrameWindow::foIgnorePosition)
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
                posX -= 2 * frame->borderXN() - client->getBorder() - 1;
            if (gy > 0)
                posY -= 2 * frame->borderYN() + frame->titleYN() - client->getBorder() - 1;
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
    bool doActivate = (wmState() == YWindowManager::wmRUNNING);
    bool requestFocus = true;

    MSG(("managing window 0x%lX", win));
    PRECONDITION(findFrame(win) == 0);

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
            if (taskBar) {
                if (taskBar->windowTrayRequestDock(win)) {
                    delete client;
                    goto end;
                }
            }
        }

        // temp workaro/und for flashblock problems
        // reverted, causes problems with Qt5
        if (client->isEmbed() && 0) {
            warn("app trying to map XEmbed window 0x%lX, ignoring", client->handle());
            delete client;
            goto end;
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

    XWindowAttributes wa;
    XGetWindowAttributes(xapp->display(), client->handle(), &wa);

    if (wa.depth == 32)
        frame = new YFrameWindow(wmActionListener, 0,
                                 wa.depth,
                                 wa.visual);
    else
        frame = new YFrameWindow(wmActionListener);

    if (frame == 0) {
        delete client;
        goto end;
    }
    MSG(("initial geometry 2 (%d:%d %dx%d)",
         client->x(), client->y(), client->width(), client->height()));

    if (!mapClient) {
        /// !!! fix (new internal state)
        //frame->setState(WinStateHidden, WinStateHidden);
        doActivate = false;
    }

    frame->doManage(client, doActivate, requestFocus);
    MSG(("initial geometry 3 (%d:%d %dx%d)",
         client->x(), client->y(), client->width(), client->height()));

    placeWindow(frame, cx, cy, cw, ch, (wmState() != wmSTARTUP), doActivate);

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

    if (wmState() == YWindowManager::wmRUNNING) {
        if (frame->frameOptions() & YFrameWindow::foAllWorkspaces)
            frame->setAllWorkspaces();
        if (frame->frameOptions() & YFrameWindow::foFullscreen)
            frame->setState(WinStateFullscreen, WinStateFullscreen);
        if (frame->frameOptions() & (YFrameWindow::foMaximizedVert | YFrameWindow::foMaximizedHorz))
            frame->setState(WinStateMaximizedVert | WinStateMaximizedHoriz,
                            ((frame->frameOptions() & YFrameWindow::foMaximizedVert) ? WinStateMaximizedVert : 0) |
                            ((frame->frameOptions() & YFrameWindow::foMaximizedHorz) ? WinStateMaximizedHoriz : 0));
        if (frame->frameOptions() & YFrameWindow::foMinimized) {
            frame->setState(WinStateMinimized, WinStateMinimized);
            doActivate = false;
            requestFocus = false;
        }

        if (frame->frameOptions() & YFrameWindow::foNoFocusOnMap)
            requestFocus = false;
    }

    frame->setManaged(true);

    if (doActivate && manualPlacement && wmState() == wmRUNNING &&
        client != windowList &&
        client != taskBar &&
        !frame->owner() &&
        (!client->sizeHints() ||
         !(client->sizeHints()->flags & (USPosition | PPosition))))
        canManualPlace = true;

    if (doActivate) {
        if (!(frame->getState() & (WinStateHidden | WinStateMinimized | WinStateFullscreen)))
        {
            if (canManualPlace && !opaqueMove)
                frame->manualPlace();
        }
    }
    frame->updateState();
    frame->updateProperties();
    frame->updateTaskBar();
    if (frame->affectsWorkArea())
        updateWorkArea();
    if (wmState() == wmRUNNING) {
        if (doActivate == true) {
            frame->activateWindow(true);
            if (canManualPlace && opaqueMove)
                frame->wmMove();
        } else if (requestFocus) {
            if (mapInactiveOnTop)
                frame->wmRaise();
            frame->setWmUrgency(true);
        }
    }
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

    if (frame) {
        frame->hide();
        delete frame;
    }
    else {
        MSG(("destroyed: unknown window: 0x%lX", win));
    }
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

YFrameWindow *YWindowManager::getFrameUnderMouse(long workspace) {
    YFrameWindow* frame;
    Window root, xwin = None;
    YWindow* ywin;
    xsmart<char> title;
    int ignore;
    unsigned ignore2;
    if (XQueryPointer(xapp->display(), xapp->root(), &root, &xwin,
                      &ignore, &ignore, &ignore, &ignore, &ignore2) &&
        xwin != None &&
        (ywin = windowContext.find(xwin)) != 0 &&
        !ywin->adopted() &&
        ywin->fetchTitle(&title) &&
        strcmp(title, "Frame") == 0 &&
        (frame = (YFrameWindow *) ywin)->isManaged() &&
        frame->visibleOn(workspace) &&
        frame->avoidFocus() == false &&
        frame->client()->destroyed() == false &&
        frame->client()->visible() &&
        frame->client() != taskBar)
    {
        return frame;
    }
    return 0;
}

YFrameWindow *YWindowManager::getLastFocus(bool skipAllWorkspaces, long workspace) {
    if (workspace == -1)
        workspace = activeWorkspace();

    YFrameWindow *toFocus = 0;

    toFocus = fFocusedWindow[workspace];
    if (toFocus != 0) {
        if (toFocus->isMinimized() ||
            toFocus->isHidden() ||
            !toFocus->visibleOn(workspace) ||
            toFocus->client()->destroyed() ||
            toFocus->client() == taskBar ||
            toFocus->avoidFocus())
        {
            toFocus = 0;
        }
    }

    if (toFocus == 0) {
        toFocus = getFrameUnderMouse(workspace);
    }

    if (toFocus == 0) {
        int pass = 0;
        if (!skipAllWorkspaces)
            pass = 1;
        for (; pass < 3; pass++) {
            YFrameIter w = focusedReverseIterator();
            while (++w) {
#if 1
                if ((w->client() && !w->client()->adopted()))
                    continue;
#endif
                if (w->isMinimized())
                    continue;
                if (w->isHidden())
                    continue;
                if (!w->visibleOn(workspace))
                    continue;
                if (w->avoidFocus() || pass == 2)
                    continue;
                if ((w->isAllWorkspaces() && w != fFocusWin) || pass == 1) {
                    if (w->client() != taskBar && toFocus == 0)
                        toFocus = w;
                    continue;
                }
                if (w->client() == taskBar)
                    continue;
                toFocus = w;
                goto gotit;
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
            xapp->getEventTime(0), w);
    } else if (f && w == f->handle()) {
        msg("%lX Focus 0x%lX frame %s",
            xapp->getEventTime(0), w, cstring(f->getTitle()).c_str());
    } else if (f && c && w == c->handle()) {
        msg("%lX Focus 0x%lX client %s",
            xapp->getEventTime(0), w, cstring(f->getTitle()).c_str());
    } else {
        msg("%lX Focus 0x%lX",
            xapp->getEventTime(0), w);
    }
#endif
    return toFocus;
}

void YWindowManager::focusLastWindow() {
    if (wmState() != wmRUNNING)
        return ;
    if (focusLocked())
        return;
    if (!clickFocus && strongPointerFocus) {
        XSetInputFocus(xapp->display(), PointerRoot, RevertToNone, CurrentTime);
        return ;
    }

/// TODO #warning "per workspace?"
    YFrameWindow *toFocus = getLastFocus(false);

    if (toFocus == 0 || toFocus->client() == taskBar) {
        focusTopWindow();
    } else {
        if (raiseOnFocus)
            toFocus->wmRaise();
        setFocus(toFocus);
    }
}


YFrameWindow *YWindowManager::topLayer(long layer) {
    for (long l = layer; l >= 0; l--)
        if (fLayers[l])
            return fLayers[l].front();

    return 0;
}

YFrameWindow *YWindowManager::bottomLayer(long layer) {
    for (long l = layer; l < WinLayerCount; l++)
        if (fLayers[l])
            return fLayers[l].back();

    return 0;
}

void YWindowManager::setAbove(YFrameWindow* frame, YFrameWindow* above) {
    const long layer = frame->getActiveLayer();
    if (above != 0 && layer != above->getActiveLayer()) {
        MSG(("ignore z-order change between layers: win=0x%lX (above: 0x%lX) ",
                    frame->handle(), above->client()->handle()));
        return;
    }

#ifdef DEBUG
    if (debug_z) dumpZorder("before setAbove", frame, above);
#endif
    if (above != frame->next() && above != frame) {
        fLayers[layer].remove(frame);
        fLayeredUpdated = true;
        if (above) {
            fLayers[layer].insertBefore(frame, above);
        } else {
            fLayers[layer].append(frame);
        }
#ifdef DEBUG
        if (debug_z) dumpZorder("after setAbove", frame, above);
#endif
    }
    updateFullscreenLayer();
}

void YWindowManager::setBelow(YFrameWindow* frame, YFrameWindow* below) {
    if (below != 0 &&
        frame->getActiveLayer() != below->getActiveLayer())
    {
        MSG(("ignore z-order change between layers: win=0x%lX (below %ld)",
                   frame->handle(), below->client()->handle()));
        return;
    }
    if (below != frame->prev() && below != frame)
        setAbove(frame, below ? below->next() : 0);
}

void YWindowManager::removeLayeredFrame(YFrameWindow *frame) {
    long layer = frame->getActiveLayer();
    PRECONDITION(inrange(layer, 0L, WinLayerCount - 1L));
    fLayers[layer].remove(frame);
    fLayeredUpdated = true;
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
    if (taskBar)
        taskBar->updateFullscreen(getFocus() && getFocus()->isFullscreen());

    if (taskBar && taskBar->workspacesPane()) {
        taskBar->workspacesPane()->repaint();
    }
}

void YWindowManager::restackWindows(YFrameWindow *) {
    int count = focusedCount();

    count++; // permanent top window

    YArray<Window> w;
    w.setCapacity(9 + count);

    w.append(fTopWin->handle());

    for (YPopupWindow* p = xapp->popup(); p; p = p->prevPopup())
        w.append(p->handle());

    if (taskBar)
        w.append(taskBar->edgeTriggerWindow());

    if (fLeftSwitch && fLeftSwitch->visible())
        w.append(fLeftSwitch->handle());

    if (fRightSwitch && fRightSwitch->visible())
        w.append(fRightSwitch->handle());

    if (fTopSwitch && fTopSwitch->visible())
        w.append(fTopSwitch->handle());

    if (fBottomSwitch && fBottomSwitch->visible())
        w.append(fBottomSwitch->handle());

    if (wmapp->hasCtrlAltDelete()) {
        if (wmapp->getCtrlAltDelete()->visible()) {
            w.append(wmapp->getCtrlAltDelete()->handle());
        }
    }

    if (statusMoveSize->visible())
        w.append(statusMoveSize->handle());

    for (YFrameWindow* f = topLayer(); f; f = f->nextLayer()) {
        w.append(f->handle());
    }

    if (w.getCount() > 1) {
        XRestackWindows(xapp->display(), &*w, w.getCount());
    }
}

void YWindowManager::getWorkArea(YFrameWindow *frame,
                                 int *mx, int *my, int *Mx, int *My, int xiscreen) const
{
    bool whole = false;
    long ws = -1;

    if (frame) {
        if (xiscreen == -1)
            xiscreen = frame->getScreen();

        if (!frame->inWorkArea())
            whole = true;

        ws = frame->getWorkspace();
        if (frame->isAllWorkspaces())
            ws = activeWorkspace();

        if (ws < 0 || ws >= fWorkAreaWorkspaceCount)
            whole = true;
    } else
        whole = true;


    if (whole) {
        *mx = 0;
        *my = 0;
        *Mx = width();
        *My = height();
    } else {
        *mx = fWorkArea[ws][xiscreen].fMinX;
        *my = fWorkArea[ws][xiscreen].fMinY;
        *Mx = fWorkArea[ws][xiscreen].fMaxX;
        *My = fWorkArea[ws][xiscreen].fMaxY;
    }

    if (xiscreen != -1) {
        int dx, dy;
        unsigned dw, dh;
        manager->getScreenGeometry(&dx, &dy, &dw, &dh, xiscreen);

        if (*mx < dx)
            *mx = dx;
        if (*my < dy)
            *my = dy;
        if (*Mx > dx + (int) dw)
            *Mx = dx + (int) dw;
        if (*My > dy + (int) dh)
            *My = dy + (int) dh;
    }
}

void YWindowManager::getWorkAreaSize(YFrameWindow *frame, int *Mw,int *Mh) {
    int mx, my, Mx, My;
    manager->getWorkArea(frame, &mx, &my, &Mx, &My);
    *Mw = Mx - mx;
    *Mh = My - my;
}

void YWindowManager::updateArea(long workspace, int screen_number, int l, int t, int r, int b) {
    if (workspace >= 0 && workspace < fWorkAreaWorkspaceCount) {
        struct WorkAreaRect *wa = &(fWorkArea[workspace][screen_number]);

        if (l > wa->fMinX) wa->fMinX = l;
        if (t > wa->fMinY) wa->fMinY = t;
        if (r < wa->fMaxX) wa->fMaxX = r;
        if (b < wa->fMaxY) wa->fMaxY = b;
    } else if (workspace == -1) {
        for (int ws = 0; ws < fWorkAreaWorkspaceCount; ws++) {
            struct WorkAreaRect *wa = fWorkArea[ws] + screen_number;

            if (l > wa->fMinX) wa->fMinX = l;
            if (t > wa->fMinY) wa->fMinY = t;
            if (r < wa->fMaxX) wa->fMaxX = r;
            if (b < wa->fMaxY) wa->fMaxY = b;
        }
    }
}

void YWindowManager::updateWorkArea() {
    int fOldWorkAreaWorkspaceCount = fWorkAreaWorkspaceCount;
    int fOldWorkAreaScreenCount = fWorkAreaScreenCount;
    struct WorkAreaRect **fOldWorkArea = fWorkArea;

    fWorkAreaWorkspaceCount = 0;
    fWorkAreaScreenCount = 0;
    fWorkArea = 0;

    fWorkArea = new struct WorkAreaRect *[::workspaceCount];
    if (fWorkArea == 0) {
        if (fOldWorkArea) {
            for (int i = 0; i < fOldWorkAreaWorkspaceCount; i++)
                delete [] fOldWorkArea[i];
            delete [] fOldWorkArea;
        }
        return;
    }
    fWorkAreaWorkspaceCount = ::workspaceCount;

    for (int i = 0; i < fWorkAreaWorkspaceCount; i++) {
        fWorkAreaScreenCount = xiInfo.getCount();
        fWorkArea[i] = new struct WorkAreaRect[fWorkAreaScreenCount];
        if (fWorkArea[i] == 0) {
            fWorkArea = 0;
            fWorkAreaWorkspaceCount = 0;
            fWorkAreaScreenCount = 0;
            if (fOldWorkArea) {
                for (int i = 0; i < fOldWorkAreaWorkspaceCount; i++)
                    delete [] fOldWorkArea[i];
                delete [] fOldWorkArea;
            }
            return;
        }

        for (int j = 0; j < fWorkAreaScreenCount; j++) {
            fWorkArea[i][j].fMinX = xiInfo[j].x_org;
            fWorkArea[i][j].fMinY = xiInfo[j].y_org;
            fWorkArea[i][j].fMaxX = xiInfo[j].x_org + xiInfo[j].width;
            fWorkArea[i][j].fMaxY = xiInfo[j].y_org + xiInfo[j].height;
        }
    }
    for (int i = 0; i < fWorkAreaWorkspaceCount; i++) {
        for (int j = 0; j < fWorkAreaScreenCount; j++) {
            MSG(("before: workarea w:%d s:%d %d %d %d %d",
                i, j,
                fWorkArea[i][j].fMinX,
                fWorkArea[i][j].fMinY,
                fWorkArea[i][j].fMaxX,
                fWorkArea[i][j].fMaxY));
        }
    }

    for (YFrameWindow *w = topLayer();
         w;
         w = w->nextLayer())
    {
        if (w->client() == 0 ||
            w->isHidden() ||
            w->isRollup() ||
            w->isIconic() ||
            w->isMinimized())
            continue;

        int ws = w->getWorkspace();
        int s = w->getScreen();
        int sx = xiInfo[s].x_org;
        int sy = xiInfo[s].y_org;
        int sw = xiInfo[s].width;
        int sh = xiInfo[s].height;

        MSG(("workarea window %s: ws:%d s:%d x:%d y:%d w:%d h:%d", cstring(w->getTitle()).c_str(), ws, s, w->x(), w->y(), w->width(), w->height()));
        {
            int l = sx + w->strutLeft();
            int t = sy + w->strutTop();
            int r = sx + sw - w->strutRight();
            int b = sy + sh - w->strutBottom();

            MSG(("strut %d %d %d %d", w->strutLeft(), w->strutTop(), w->strutRight(), w->strutBottom()));
            MSG(("limit %d %d %d %d", l, t, r, b));
            updateArea(ws, s, l, t, r, b);
        }

        if (w->doNotCover() ||
            (limitByDockLayer && w->getActiveLayer() == WinLayerDock))
        {
            int lowX = sx + sw / 4;
            int lowY = sy + sh / 4;
            int hiX = sx + 3 * sw / 4;
            int hiY = sy + 3 * sh / 4;
            bool const isHoriz(w->width() > w->height());

            int l = sx;
            int t = sy;
            int r = sx + sw;
            int b = sy + sh;

            if (isHoriz) {
                if (w->y() + (int) w->height() < lowY) {
                    t = w->y() + w->height();
                } else if (w->y() + (int) height() > hiY) {
                    b = w->y();
                }
            } else {
                if (w->x() + (int) w->width() < lowX) {
                    l = w->x() + w->width();
                } else if (w->x() + (int) width() > hiX) {
                    r = w->x();
                }
            }
            MSG(("dock limit %d %d %d %d", l, t, r, b));
            updateArea(ws, s, l, t, r, b);
        }
    for (int i = 0; i < fWorkAreaWorkspaceCount; i++) {
        for (int j = 0; j < fWorkAreaScreenCount; j++) {
           if (0) {
           MSG(("updated: workarea w:%d s:%d %d %d %d %d",
                i, j,
                fWorkArea[i][j].fMinX,
                fWorkArea[i][j].fMinY,
                fWorkArea[i][j].fMaxX,
                fWorkArea[i][j].fMaxY));
           }
        }
    }
    }
    for (int i = 0; i < fWorkAreaWorkspaceCount; i++) {
        for (int j = 0; j < fWorkAreaScreenCount; j++) {
            MSG(("after: workarea w:%d s:%d %d %d %d %d",
                i, j,
                fWorkArea[i][j].fMinX,
                fWorkArea[i][j].fMinY,
                fWorkArea[i][j].fMaxX,
                fWorkArea[i][j].fMaxY));
        }
    }

    bool changed = false;
    if (fOldWorkArea == 0 ||
        fOldWorkAreaWorkspaceCount != fWorkAreaWorkspaceCount ||
        fOldWorkAreaScreenCount != fWorkAreaScreenCount) {
        changed = true;
    } else {
        for (int ws = 0; ws < fWorkAreaWorkspaceCount; ws++) {
            for (int s = 0; s < fWorkAreaScreenCount; s++) {
                if (fWorkArea[ws][s].fMinX != fOldWorkArea[ws][s].fMinX ||
                    fWorkArea[ws][s].fMinY != fOldWorkArea[ws][s].fMinY ||
                    fWorkArea[ws][s].fMaxX != fOldWorkArea[ws][s].fMaxX ||
                    fWorkArea[ws][s].fMaxY != fOldWorkArea[ws][s].fMaxY) {
                    changed = true;
                    break;
                }
            }
            if (changed)
                break;
        }
    }

    if (changed) {
        announceWorkArea();
        if (fWorkAreaMoveWindows) {
            for (int ws = 0; ws < fWorkAreaWorkspaceCount; ws++) {
                if (ws >= fOldWorkAreaWorkspaceCount)
                    break;

                for (int s = 0; s < fWorkAreaScreenCount; s++) {
                    int const deltaX = fWorkArea[ws][s].fMinX - fOldWorkArea[ws][s].fMinX;
                    int const deltaY = fWorkArea[ws][s].fMinY - fOldWorkArea[ws][s].fMinY;

                    if (deltaX != 0 || deltaY != 0)
                        relocateWindows(ws, s, deltaX, deltaY);
                }
            }
        }
    }
    if (fOldWorkArea) {
        for (int i = 0; i < fOldWorkAreaWorkspaceCount; i++)
            delete [] fOldWorkArea[i];
        delete [] fOldWorkArea;
    }
    resizeWindows();
}

void YWindowManager::announceWorkArea() {
    int nw = workspaceCount();
    long *area = new long[nw * 4];
    bool isCloned = true;

    /*
      NET_WORKAREA behaviour: 0 (single/multimonitor with STRUT information,
                                 like metacity),
                              1 (always full desktop),
                              2 (singlemonitor with STRUT,
                                 multimonitor without STRUT)
    */

    if (!area)
        return;

    if (xiInfo.getCount() > 1 && netWorkAreaBehaviour != 1) {
        for (int i = 0; i < xiInfo.getCount(); i++) {
            if (xiInfo[i].x_org != 0 || xiInfo[i].y_org != 0) {
                isCloned = false;
                break;
            }
        }
    }

    const YRect desktopArea(0, 0, desktop->width(), desktop->height());
    for (int ws = 0; ws < nw; ws++) {
        YRect r(desktopArea);
        if (netWorkAreaBehaviour != 1) {
            r = YRect(fWorkArea[ws][0].fMinX, fWorkArea[ws][0].fMinY,
                      fWorkArea[ws][0].fMaxX - fWorkArea[ws][0].fMinX,
                      fWorkArea[ws][0].fMaxY - fWorkArea[ws][0].fMinY);
        }

        if (xiInfo.getCount() > 1 && ! isCloned && netWorkAreaBehaviour != 1) {
            if (netWorkAreaBehaviour == 0) {
                // STRUTS information is messy and broken for multimonitor,
                // but there is no solution for this problem.
                // So we imitate metacity's behaviour := merge,
                // but limit height of each screen and hope for the best
                for (int i = 1; i < xiInfo.getCount(); i++) {
                    r.unionRect(fWorkArea[ws][i].fMinX, fWorkArea[ws][i].fMinY,
                                fWorkArea[ws][i].fMaxX - fWorkArea[ws][i].fMinX,
                                fWorkArea[ws][0].fMaxY - fWorkArea[ws][0].fMinY);
                }
            } else if (netWorkAreaBehaviour == 2) {
                r = desktopArea;
            }
        }
        area[ws * 4 + 0] = r.x();
        area[ws * 4 + 1] = r.y();
        area[ws * 4 + 2] = r.width();
        area[ws * 4 + 3] = r.height();
    }

    XChangeProperty(xapp->display(), handle(),
                    _XA_NET_WORKAREA,
                    XA_CARDINAL,
                    32, PropModeReplace,
                    (unsigned char *)area, nw * 4);
    delete [] area;

    if (fActiveWorkspace != -1 && fActiveWorkspace < workspaceCount()) {
        long area[4];
        int cw = fActiveWorkspace;
        area[0] = fWorkArea[cw][0].fMinX;
        area[1] = fWorkArea[cw][0].fMinY;
        area[2] = fWorkArea[cw][0].fMaxX;
        area[3] = fWorkArea[cw][0].fMaxY;

        XChangeProperty(xapp->display(), handle(),
                        _XA_WIN_WORKAREA,
                        XA_CARDINAL,
                        32, PropModeReplace,
                        (unsigned char *)area, 4);
    }
}

void YWindowManager::relocateWindows(long workspace, int screen, int dx, int dy) {
/// TODO #warning "needs a rewrite (save old work area) for each workspace"
#if 1
    for (YFrameWindow * f = topLayer(); f; f = f->nextLayer())
        if (f->inWorkArea() && f->getScreen() == screen) {
            if (f->getWorkspace() == workspace ||
                (f->isAllWorkspaces() && workspace == activeWorkspace()))
            {
                f->setNormalPositionOuter(f->x() + dx, f->y() + dy);
            }
        }
#endif
}

void YWindowManager::resizeWindows() {
    for (YFrameWindow * f = topLayer(); f; f = f->nextLayer()) {
        if (f->inWorkArea() && f->client() && !f->client()->destroyed()) {
            if (f->isMaximized())
                f->updateDerivedSize(WinStateMaximizedVert | WinStateMaximizedHoriz);
            f->updateLayout();
        }
    }
}

void YWindowManager::initWorkspaces() {
    long ws = 0;

    if (!readDesktopNames())
        setDesktopNames();
    setDesktopCount();
    setDesktopGeometry();
    setDesktopViewport();
    setShowingDesktop();
    readCurrentDesktop(ws);
    readDesktopLayout();
    activateWorkspace(ws);
}

void YWindowManager::activateWorkspace(long workspace) {
    if (workspace != fActiveWorkspace) {
        lockFocus();
///        XSetInputFocus(app->display(), desktop->handle(), RevertToNone, CurrentTime);

        if (taskBar && fActiveWorkspace != WinWorkspaceInvalid) {
            taskBar->setWorkspaceActive(fActiveWorkspace, 0);
        }
        fLastWorkspace = fActiveWorkspace;
        fActiveWorkspace = workspace;
        if (taskBar) {
            taskBar->setWorkspaceActive(fActiveWorkspace, 1);
        }

        long ws = fActiveWorkspace;

        XChangeProperty(xapp->display(), handle(),
                        _XA_WIN_WORKSPACE,
                        XA_CARDINAL,
                        32, PropModeReplace,
                        (unsigned char *)&ws, 1);
        XChangeProperty(xapp->display(), handle(),
                        _XA_NET_CURRENT_DESKTOP,
                        XA_CARDINAL,
                        32, PropModeReplace,
                        (unsigned char *)&ws, 1);
        ws = 0;

#if 1 // not needed when we drop support for GNOME hints
        updateWorkArea();
#endif
        resizeWindows();

        YFrameWindow *w;

        for (w = topLayer(); w; w = w->nextLayer())
            if (w->visibleNow()) {
                w->updateState();
                w->updateTaskBar();
            }

        for (w = bottomLayer(); w; w = w->prevLayer())
            if (!w->visibleNow()) {
                w->updateState();
                w->updateTaskBar();
            }
        unlockFocus();

        YFrameWindow *toFocus = getLastFocus(true, workspace);
        setFocus(toFocus);
        resetColormap(true);

        if (taskBar) taskBar->relayoutNow();

        if (workspaceSwitchStatus
            && (!showTaskBar || !taskBarShowWorkspaces || taskBarAutoHide
                || (taskBar && taskBar->hidden()))
           )
            statusWorkspace->begin(workspace);
        wmapp->signalGuiEvent(geWorkspaceChange);
        updateWorkArea();
    }
}

void YWindowManager::appendNewWorkspace() {
    if (::workspaceCount >= MAXWORKSPACES)
        return;

    long ws = ::workspaceCount;

    if (workspaceNames[ws] == 0) {
        char s[32];
        snprintf(s, 32, " %ld ", ws);
        workspaceNames[ws] = newstr(s);
    }

    ::workspaceCount++;

    updateWorkspaces(true);
}

void YWindowManager::removeLastWorkspace() {
    if (::workspaceCount <= 1)
        return;

    long ws = ::workspaceCount - 1;

    // switch away from the workspace being removed
    if (fActiveWorkspace == ws)
        activateWorkspace(ws-1);

    // move windows away from the workspace being removed
    bool changed;
    do {
        changed = false;
        for (YFrameWindow *frame = topLayer(); frame; frame = frame->nextLayer())
            if (frame->getWorkspace() == ws) {
                frame->setWorkspace(ws-1);
                changed = true;
                break;
            }
    } while (changed);

    ::workspaceCount--;

    updateWorkspaces(false);
}

void YWindowManager::updateWorkspaces(bool increase) {
    if (increase) {
        setDesktopViewport();
        updateWorkArea();
        setDesktopCount();
    } else {
        setDesktopCount();
        updateWorkArea();
        setDesktopViewport();
    }
    if (taskBar) {
        if (taskBar->workspacesPane()) {
            taskBar->workspacesPane()->updateButtons();
            taskBar->relayout();
            taskBar->relayoutNow();
        }
    }
    if (windowList) {
        windowList->updateWorkspaces();
    }
    updateMoveMenu();
}

bool YWindowManager::readCurrentDesktop(long &workspace) {
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    xsmart<unsigned char> prop;
    workspace = 0;

    r_type = None;
    r_format = 0;
    count = 0;
    bytes_remain = 0;
    prop = 0;
    if (XGetWindowProperty(xapp->display(), handle(),
                           _XA_NET_CURRENT_DESKTOP, 0, 1, False,
                           XA_CARDINAL, &r_type, &r_format,
                           &count, &bytes_remain, &prop) == Success && prop) {
        if (r_type == XA_CARDINAL && r_format == 32 && count == 1) {
            workspace = clamp(prop.extract<long>(), 0L, workspaceCount() - 1L);
            return true;
        }
    }
    r_type = None;
    r_format = 0;
    count = 0;
    bytes_remain = 0;
    prop = 0;
    if (XGetWindowProperty(xapp->display(), handle(),
                           _XA_WIN_WORKSPACE, 0, 1, False,
                           XA_CARDINAL, &r_type, &r_format,
                           &count, &bytes_remain, &prop) == Success && prop) {
        if (r_type == XA_CARDINAL && r_format == 32 && count == 1) {
            workspace = clamp(prop.extract<long>(), 0L, workspaceCount() - 1L);
            return true;
        }
    }
    return false;
}

void YWindowManager::setDesktopGeometry() {
    long data[2];
    data[0] = desktop->width();
    data[1] = desktop->height();
    MSG(("setting: _NET_DESKTOP_GEOMETRY = (%ld,%ld)", data[0], data[1]));
    XChangeProperty(xapp->display(), handle(),
                    _XA_NET_DESKTOP_GEOMETRY, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&data, 2);
}

void YWindowManager::setShowingDesktop() {
    long value = fShowingDesktop ? 1 : 0;
    MSG(("setting: _NET_SHOWING_DESKTOP = %ld", value));
    XChangeProperty(xapp->display(), handle(),
                    _XA_NET_SHOWING_DESKTOP, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&value, 1);
}

void YWindowManager::setShowingDesktop(bool setting) {

    if (fShowingDesktop != setting) {
        fShowingDesktop = setting;
        setShowingDesktop();
    }
}

void YWindowManager::updateTaskBarNames() {
    if (taskBar) {
        if (taskBar->workspacesPane()) {
            taskBar->workspacesPane()->relabelButtons();
            taskBar->relayout();
            taskBar->relayoutNow();
        }
    }
}

void YWindowManager::updateMoveMenu() {
    if (moveMenu) {
        moveMenu->removeAll();
        for (long w = 0; w < ::workspaceCount; w++) {
            char s[128];
            snprintf(s, 128, "%lu. %s", (unsigned long)(w + 1), workspaceNames[w]);
            moveMenu->addItem(s, 0, null, workspaceActionMoveTo[w]);
        }
    }
}

bool YWindowManager::readDesktopLayout() {
    bool success = false;

    Atom type(0);
    int format(0);
    unsigned long count(0), remain(0);
    long *prop(0);
    if (XGetWindowProperty(xapp->display(), handle(),
                           _XA_NET_DESKTOP_LAYOUT, 0L, 4L, False,
                           XA_CARDINAL, &type, &format, &count, &remain,
                           (unsigned char **) &prop) == Success && prop)
    {
        if (type == XA_CARDINAL && format == 32 && count >= 3) {
            int orient = (int) prop[0];
            int cols   = (int) prop[1];
            int rows   = (int) prop[2];
            int corner = (count == 3) ? _NET_WM_TOPLEFT : (int) prop[3];
            if (inrange(orient, 0, 1) &&
                inrange(cols, 0, 100) &&
                inrange(rows, 0, 100) &&
                (rows || cols) &&
                inrange(corner, 0, 3))
            {
                fLayout = (DesktopLayout) { orient, cols, rows, corner, };
                success = true;
            }
        }
        XFree(prop);
    }
    MSG(("read: _NET_DESKTOP_LAYOUT(%d): %s (%d, %lu) { %d, %d, %d, %d }",
        (int) _XA_NET_DESKTOP_LAYOUT, boolstr(success), format, (long unsigned int) count,
        fLayout.orient, fLayout.columns, fLayout.rows, fLayout.corner));

    return success;
}

bool YWindowManager::readDesktopNames() {
    return readNetDesktopNames()
        || readWinDesktopNames();
}

bool YWindowManager::compareDesktopNames(char **strings, int count) {
    bool changed = false;

    // old strings must persist until after the update
    asmart<csmart> oldWorkspaceNames(new csmart[count]);

    for (int i = 0; i < count; i++) {
        if (workspaceNames[i] != 0) {
            MSG(("Workspace %d: '%s' -> '%s'", i, workspaceNames[i], strings[i]));
            if (strcmp(workspaceNames[i], strings[i])) {
                oldWorkspaceNames[i] = workspaceNames[i];
                workspaceNames[i] = newstr(strings[i]);
                changed = true;
            }
        } else {
            MSG(("Workspace %d: (null) -> '%s'", i, strings[i]));
            workspaceNames[i] = newstr(strings[i]);
            changed = true;
        }
    }

    if (changed) {
        updateTaskBarNames();
        updateMoveMenu();
    }

    return changed;
}

bool YWindowManager::readNetDesktopNames() {
    bool success = false;

    MSG(("reading: _NET_DESKTOP_NAMES(%d)",(int)_XA_NET_DESKTOP_NAMES));

    XTextProperty names;
    if (XGetTextProperty(xapp->display(), handle(), &names,
                         _XA_NET_DESKTOP_NAMES) && names.nitems > 0) {
        int count = 0;
        char **strings = 0;
        if (XmbTextPropertyToTextList(xapp->display(), &names,
                                      &strings, &count) == Success) {
            if (count > 0 && strings[count - 1][0] == '\0')
                count--;
            count = min(count, MAXWORKSPACES);
            if (compareDesktopNames(strings, count)) {
                setWinDesktopNames(count);
            }
            XFreeStringList(strings);
            success = true;
        } else {
            MSG(("warning: could not convert strings for _NET_DESKTOP_NAMES"));
        }
        XFree(names.value);
    } else {
        MSG(("warning: could not read _NET_DESKTOP_NAMES"));
    }

    return success;
}

bool YWindowManager::readWinDesktopNames() {
    bool success = false;

    MSG(("reading: _WIN_WORKSPACE_NAMES(%d)",(int)_XA_WIN_WORKSPACE_NAMES));

    XTextProperty names;
    if (XGetTextProperty(xapp->display(), handle(), &names,
                         _XA_WIN_WORKSPACE_NAMES) && names.nitems > 0) {
        int count = 0;
        char **strings = 0;
        if (XmbTextPropertyToTextList(xapp->display(), &names,
                                      &strings, &count) == Success) {
            if (count > 0 && strings[count - 1][0] == '\0')
                count--;
            count = min(count, MAXWORKSPACES);
            if (compareDesktopNames(strings, count)) {
                setNetDesktopNames(count);
            }
            XFreeStringList(strings);
            success = true;
        } else {
            MSG(("warning: could not convert strings for _WIN_WORKSPACE_NAMES"));
        }
        XFree(names.value);
    } else {
        MSG(("warning: could not read _WIN_WORKSPACE_NAMES"));
    }
    return success;
}

void YWindowManager::setWinDesktopNames(long count) {
    MSG(("setting: _WIN_WORKSPACE_NAMES"));
    static char terminator[] = { '\0' };
    char **strings = new char *[count + 1];
    for (long i = 0; i < count; i++) {
        if (workspaceNames[i] != 0)
            strings[i] = workspaceNames[i];
        else
            strings[i] = terminator;
    }
    strings[count] = terminator;
    XTextProperty names;
    if (XmbTextListToTextProperty(xapp->display(), strings,
                                  count + 1, XStdICCTextStyle,
                                  &names) == Success) {
        XSetTextProperty(xapp->display(), handle(), &names,
                         _XA_WIN_WORKSPACE_NAMES);
        XFree(names.value);
    }
    delete[] strings;
}

void YWindowManager::setNetDesktopNames(long count) {
    MSG(("setting: _NET_DESKTOP_NAMES"));
    static char terminator[] = { '\0' };
    char **strings = new char *[count + 1];
    for (long i = 0; i < count; i++) {
        if (workspaceNames[i] != 0)
            strings[i] = workspaceNames[i];
        else
            strings[i] = terminator;
    }
    strings[count] = terminator;
    XTextProperty names;
    if (XmbTextListToTextProperty(xapp->display(), strings,
                                  count + 1, XUTF8StringStyle,
                                  &names) == Success) {
        XSetTextProperty(xapp->display(), handle(), &names,
                         _XA_NET_DESKTOP_NAMES);
        XFree(names.value);
    }
    delete[] strings;
}

void YWindowManager::setDesktopNames(long count) {
    MSG(("setting: %ld desktop names", count));
    setWinDesktopNames(count);
    setNetDesktopNames(count);
}

void YWindowManager::setDesktopNames() {
    setDesktopNames(workspaceCount());
}

void YWindowManager::setDesktopCount() {
    long count = workspaceCount();
    MSG(("setting: _WIN_WORKSPACE_COUNT = %ld",count));
    XChangeProperty(xapp->display(), handle(),
                    _XA_WIN_WORKSPACE_COUNT, XA_CARDINAL,
                    32, PropModeReplace, (unsigned char *)&count, 1);
    MSG(("setting: _NET_NUMBER_OF_DESKTOPS = %ld",count));
    XChangeProperty(xapp->display(), handle(),
                    _XA_NET_NUMBER_OF_DESKTOPS, XA_CARDINAL,
                    32, PropModeReplace, (unsigned char *)&count, 1);
}

void YWindowManager::setDesktopViewport() {
    MSG(("setting: _NET_DESKTOP_VIEWPORT"));
    int n = 2 * workspaceCount();
    long *data = new long[n];
    for (int i = 0; i < n; i++)
        data[i] = 0;
    XChangeProperty(xapp->display(), handle(),
                    _XA_NET_DESKTOP_VIEWPORT, XA_CARDINAL,
                    32, PropModeReplace, (unsigned char *)data, n);
    delete[] data;
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

    for (int layer = 0 ; layer < WinLayerCount; layer++) {
        for (YFrameWindow *frame = top(layer); frame; frame = frame->next()) {
            if (!frame->visibleOn(workspace))
                continue;
            if (frame->frameOptions() & YFrameWindow::foIgnoreWinList)
                continue;
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
    if (property.atom == XA_IcewmWinOptHint) {
        Atom type;
        int format;
        unsigned long nitems, lbytes;
        unsigned char *propdata(0);

        if (XGetWindowProperty(xapp->display(), handle(),
                               XA_IcewmWinOptHint, 0, 8192, True, XA_IcewmWinOptHint,
                               &type, &format, &nitems, &lbytes,
                               &propdata) == Success && propdata)
        {
            for (unsigned i = 0; i + 3 < nitems; ) {
                const char* s[3] = { 0, 0, 0, };
                for (int k = 0; k < 3 && i < nitems; ++k) {
                    s[k] = i + (const char *) propdata;
                    while (i < nitems && propdata[i++]);
                }
                if (s[0] && s[1] && s[2] && propdata[i - 1] == 0) {
                    hintOptions->setWinOption(s[0], s[1], s[2]);
                }
            }
            XFree(propdata);
        }
    }
    else if (property.atom == _XA_NET_DESKTOP_NAMES) {
        MSG(("change: net desktop names"));
        readDesktopNames();
    }
    else if (property.atom == _XA_NET_DESKTOP_LAYOUT) {
        readDesktopLayout();
    }
    else if (property.atom == _XA_WIN_WORKSPACE_NAMES) {
        MSG(("change: win workspace names"));
        readDesktopNames();
    }
}

void YWindowManager::updateClientList() {
    YArray<XID> ids;
    if (fLayeredUpdated || fCreatedUpdated) {
        ids.setCapacity(fCreationOrder.count());
    }

    if (fLayeredUpdated) {
        fLayeredUpdated = false;

        for (int i = 0; i < WinLayerCount; ++i) {
            if (fLayers[i]) {
                YFrameIter frame = fLayers[i].reverseIterator();
                while (++frame) {
                    if (frame->client() && frame->client()->adopted()) {
                        ids.append(frame->client()->handle());
                    }
                }
            }
        }

        XChangeProperty(xapp->display(), desktop->handle(),
                        _XA_WIN_CLIENT_LIST,
                        XA_CARDINAL,
                        32, PropModeReplace,
                        ids.getCount() ? (unsigned char *) &*ids : 0,
                        ids.getCount());
        XChangeProperty(xapp->display(), desktop->handle(),
                        _XA_NET_CLIENT_LIST_STACKING,
                        XA_WINDOW,
                        32, PropModeReplace,
                        ids.getCount() ? (unsigned char *) &*ids : 0,
                        ids.getCount());
    }

    if (fCreatedUpdated) {
        fCreatedUpdated = false;

        ids.shrink(0);
        for (YFrameIter frame = fCreationOrder.iterator(); ++frame; ) {
            if (frame->client() && frame->client()->adopted())
                ids.append(frame->client()->handle());
        }

        XChangeProperty(xapp->display(), desktop->handle(),
                        _XA_NET_CLIENT_LIST,
                        XA_WINDOW,
                        32, PropModeReplace,
                        ids.getCount() ? (unsigned char *) &*ids : 0,
                        ids.getCount());
    }
    checkLogout();
}

void YWindowManager::updateUserTime(const UserTime& userTime) {
    if (userTime.good() && fLastUserTime < userTime)
        fLastUserTime = userTime;
}

void YWindowManager::execAfterFork(const char *command) {
        if (!command || !*command)
                return;
    msg("Running system command in shell: %s", command);
        pid_t pid = fork();
    switch(pid) {
    case -1: /* Failed */
        fail("fork failed");
        return;
    case 0: /* Child */
        execl("/bin/sh", "sh", "-c", command, (char *) 0);
        _exit(99);
    default: /* Parent */
        return;
    }
}

void YWindowManager::checkLogout() {
    if (fShuttingDown && !haveClients()) {
        fShuttingDown = false; /* Only run the command once */

        if (rebootOrShutdown == Reboot && nonempty(rebootCommand)) {
            msg("reboot... (%s)", rebootCommand);
            execAfterFork(rebootCommand);
        } else if (rebootOrShutdown == Shutdown && nonempty(shutdownCommand)) {
            msg("shutdown ... (%s)", shutdownCommand);
            execAfterFork(shutdownCommand);
        } else if (rebootOrShutdown == Logout)
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

void YWindowManager::notifyActive(YFrameWindow *frame) {
    Window wnd = frame ? frame->client()->handle() : None;
    if (wnd != fActiveWindow) {
        XChangeProperty(xapp->display(), handle(),
                        _XA_NET_ACTIVE_WINDOW,
                        XA_WINDOW,
                        32, PropModeReplace,
                        (unsigned char *)&wnd, 1);
        fActiveWindow = wnd;
    }
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
    if (frame && reorderFocus) {
        raiseFocusFrame(frame);
    }
    notifyActive(frame);
    updateClientList();
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

void YWindowManager::popupWindowListMenu(YWindow *owner, int x, int y) {
    windowListMenu->popup(owner, 0, 0, x, y,
                          YPopupWindow::pfCanFlipVertical |
                          YPopupWindow::pfCanFlipHorizontal |
                          YPopupWindow::pfPopupMenu);
}

void YWindowManager::popupStartMenu(YWindow *owner) { // !! fix
    if (showTaskBar && taskBar && taskBarShowStartMenu)
        taskBar->popupStartMenu();
    else
        rootMenu->popup(owner, 0, 0, 0, 0,
                        YPopupWindow::pfCanFlipVertical |
                        YPopupWindow::pfCanFlipHorizontal |
                        YPopupWindow::pfPopupMenu);
}

void YWindowManager::popupWindowListMenu(YWindow *owner) {
    if (showTaskBar && taskBar && taskBarShowWindowListMenu)
        taskBar->popupWindowListMenu();
    else
        popupWindowListMenu(owner, 0, 0);
}

void YWindowManager::switchToWorkspace(long nw, bool takeCurrent) {
    if (nw >= 0 && nw < workspaceCount()) {
        YFrameWindow *frame = getFocus();
        if (takeCurrent && frame && !frame->isAllWorkspaces()) {
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

    if (count <= 0)
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

        int normalHeight = areaH / max(1, rows);

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

void YWindowManager::getWindowsToArrange(YFrameWindow ***win, int *count, bool all, bool skipNonMinimizable) {
    YFrameWindow *w = topLayer(WinLayerNormal);

    *count = 0;
    while (w) {
        if (w->owner() == 0 && // not transient ?
            w->visibleOn(activeWorkspace()) && // visible
            (all || !w->isAllWorkspaces()) && // not on all workspaces
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
                (all || !w->isAllWorkspaces()) && // not on all workspaces
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
    setShowingDesktop(false);
}
void YWindowManager::undoArrange() {
    if (fArrangeInfo) {
        lockFocus();
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
        unlockFocus();
        focusTopWindow();
    }
    setShowingDesktop(false);
}

bool YWindowManager::haveClients() {
    for (YFrameWindow * f(topLayer()); f ; f = f->nextLayer())
        if (f->canClose() && f->client() && f->client()->adopted())
            return true;

    return false;
}

void YWindowManager::exitAfterLastClient(bool shuttingDown) {
    fShuttingDown = shuttingDown;
    checkLogout();
}

lazy<YTimer> EdgeSwitch::fEdgeSwitchTimer;

EdgeSwitch::EdgeSwitch(YWindowManager *manager, int delta, bool vertical):
    YWindow(manager),
    fManager(manager),
    fCursor(delta < 0 ? vertical ? YWMApp::scrollUpPointer
                                 : YWMApp::scrollLeftPointer
                      : vertical ? YWMApp::scrollDownPointer
                                 : YWMApp::scrollRightPointer),
    fDelta(delta),
    fVert(vertical)
{
    setStyle(wsOverrideRedirect | wsInputOnly);
    setPointer(YXApplication::leftPointer);
}

EdgeSwitch::~EdgeSwitch() {
    if (fEdgeSwitchTimer && fEdgeSwitchTimer->getTimerListener() == this) {
        fEdgeSwitchTimer = null;
    }
}

void EdgeSwitch::handleCrossing(const XCrossingEvent &crossing) {
    if (crossing.type == EnterNotify && crossing.mode == NotifyNormal) {
        fEdgeSwitchTimer->setTimer(edgeSwitchDelay, this, true);
        setPointer(fCursor);
    } else if (crossing.type == LeaveNotify && crossing.mode == NotifyNormal) {
        if (fEdgeSwitchTimer && fEdgeSwitchTimer->getTimerListener() == this) {
            fEdgeSwitchTimer = null;
            setPointer(YXApplication::leftPointer);
        }
    }
}

bool EdgeSwitch::handleTimer(YTimer *t) {
    if (t != fEdgeSwitchTimer)
        return false;

    int worksps = (int) fManager->workspaceCount();
    int orient  = fManager->layout().orient;
    int columns = min(fManager->layout().columns, worksps);
    int rows    = min(fManager->layout().rows, worksps);
    int corner  = fManager->layout().corner;

    if (rows == 0)
        rows = (worksps + (columns - 1)) / columns;
    if (columns == 0)
        columns = (worksps + (rows - 1)) / rows;
    if (orient == _NET_WM_ORIENTATION_VERT) {
        columns = (worksps + (rows - 1)) / rows;
    } else {
        rows = (worksps + (columns - 1)) / columns;
    }

    int dx = fVert ? 0 : fDelta;
    int dy = fVert ? fDelta : 0;
    if (corner == _NET_WM_TOPRIGHT || corner == _NET_WM_BOTTOMRIGHT)
        dx = -dx;
    if (corner == _NET_WM_BOTTOMLEFT || corner == _NET_WM_BOTTOMRIGHT)
        dy = -dy;
    if (orient == _NET_WM_ORIENTATION_VERT) {
        swap(rows, columns);
        swap(dx, dy);
    }

    int col = fManager->activeWorkspace() % columns;
    int row = fManager->activeWorkspace() / columns;

    if (dx == 0 && rows == 1) {
        swap(dx, dy);
    }

    int wsp, end(100);
    do {
        if (0 == --end) {
            tlog("inflp");
            goto end;
        }

        col = (col + dx + columns) % columns;
        row = (row + dy + rows) % rows;
        wsp = col + row * columns;
    } while (wsp >= worksps);

    fManager->switchToWorkspace(wsp, false);

    if (warpPointerOnEdgeSwitch) {
        int dest_x = -fDelta * !fVert * (desktop->width() - 5);
        int dest_y = -fDelta * fVert * (desktop->height() - 5);
        XWarpPointer(xapp->display(), None, None, 0, 0, 0, 0,
                     dest_x, dest_y);
    }

    if (edgeContWorkspaceSwitching) {
        return true;
    }

end:
    setPointer(YXApplication::leftPointer);
    return false;
}

int YWindowManager::getScreen() {
    if (fFocusWin)
        return fFocusWin->getScreen();
    return 0;
}

void YWindowManager::doWMAction(WMAction action) {
    XClientMessageEvent xev;
    memset(&xev, 0, sizeof(xev));

    xev.type = ClientMessage;
    xev.window = xapp->root();
    xev.message_type = _XA_ICEWM_ACTION;
    xev.format = 32;
    xev.data.l[0] = CurrentTime;
    xev.data.l[1] = action;

    MSG(("new mask/state: %ld/%ld", xev.data.l[0], xev.data.l[1]));

    XSendEvent(xapp->display(), xapp->root(), False, SubstructureNotifyMask, (XEvent *) &xev);
}

#ifdef CONFIG_XRANDR
void YWindowManager::handleRRScreenChangeNotify(const XRRScreenChangeNotifyEvent &xrrsc) {
    UpdateScreenSize((XEvent *)&xrrsc);
}

void YWindowManager::UpdateScreenSize(XEvent *event) {
#if RANDR_MAJOR >= 1
    XRRUpdateConfiguration(event);
#endif

    unsigned nw = xapp->displayWidth();
    unsigned nh = xapp->displayHeight();
    updateXineramaInfo(nw, nh);

    if (width() != nw ||
        height() != nh)
    {

        MSG(("xrandr: %d %d",
             nw,
             nh));

        unsigned long data[2];
        data[0] = nw;
        data[1] = nh;
        XChangeProperty(xapp->display(), manager->handle(),
                        _XA_NET_DESKTOP_GEOMETRY, XA_CARDINAL,
                        32, PropModeReplace, (unsigned char *)&data, 2);

        setSize(nw, nh);
        updateWorkArea();
        if (taskBar) {
            if (taskBar->workspacesPane() && pagerShowPreview)
                taskBar->workspacesPane()->updateButtons();
            taskBar->relayout();
            taskBar->relayoutNow();
        }
        if (edgeHorzWorkspaceSwitching) {
            if (fLeftSwitch) {
                fLeftSwitch->setGeometry(YRect(0, 0, 1, height()));
            }
            if (fRightSwitch) {
                fRightSwitch->setGeometry(YRect(width() - 1, 0, 1, height()));
            }
        }
        if (edgeVertWorkspaceSwitching) {
            if (fTopSwitch) {
                fTopSwitch->setGeometry(YRect(0, 0, width(), 1));
            }
            if (fBottomSwitch) {
                fBottomSwitch->setGeometry(YRect(0, height() - 1, width(), 1));
            }
        }

/// TODO #warning "make something better"
        if (arrangeWindowsOnScreenSizeChange) {
            wmActionListener->actionPerformed(actionArrange, 0);
        }
    }
}
#endif

void YWindowManager::appendCreatedFrame(YFrameWindow *f) {
    fCreationOrder.append(f);
    fCreatedUpdated = true;
}

void YWindowManager::removeCreatedFrame(YFrameWindow *f) {
    fCreationOrder.remove(f);
    fCreatedUpdated = true;
}

void YWindowManager::insertFocusFrame(YFrameWindow* frame, bool focused) {
    if (focused || fFocusedOrder.count() < 1) {
        fFocusedOrder.append(frame);
    }
    else {
        fFocusedOrder.insertBefore(frame, fFocusedOrder.back());
    }
}

void YWindowManager::removeFocusFrame(YFrameWindow* frame) {
    fFocusedOrder.remove(frame);
}

void YWindowManager::lowerFocusFrame(YFrameWindow* frame) {
    fFocusedOrder.remove(frame);
    fFocusedOrder.prepend(frame);
}

void YWindowManager::raiseFocusFrame(YFrameWindow* frame) {
    fFocusedOrder.remove(frame);
    fFocusedOrder.append(frame);
}

// vim: set sw=4 ts=4 et:
