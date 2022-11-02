/*
 * IceWM
 *
 * Copyright (C) 1997-2013 Marko Macek
 */
#include "config.h"
#include "ymenu.h"
#include "wmmgr.h"
#include "sysdep.h"
#include "wmtaskbar.h"
#include "wmwinlist.h"
#include "wmwinmenu.h"
#include "wmswitch.h"
#include "wmstatus.h"
#include "wmminiicon.h"
#include "wmcontainer.h"
#include "wmconfig.h"
#include "wmframe.h"
#include "wmdialog.h"
#include "wmsession.h"
#include "wmprog.h"
#include "wmdock.h"
#include "wmapp.h"
#include "prefs.h"
#include "yprefs.h"
#include "yxcontext.h"
#include "workspaces.h"
#include "ystring.h"
#include "intl.h"
#include "ywordexp.h"

YContext<YFrameClient> clientContext("clientContext", false);

YAction layerActionSet[WinLayerCount] = {
    actionLayerDesktop,
    actionLayerOne,
    actionLayerBelow,
    actionLayerThree,
    actionLayerNormal,
    actionLayerFive,
    actionLayerOnTop,
    actionLayerSeven,
    actionLayerDock,
    actionLayerNine,
    actionLayerAboveDock,
    actionLayerEleven,
    actionLayerMenu,
    actionLayerThirteen,
    actionLayerFullscreen,
    actionLayerAboveAll,
};

Workspaces workspaces;
WorkspacesCount workspaceCount;
WorkspacesNames workspaceNames;
WorkspacesActive workspaceActionActivate;
WorkspacesMoveTo workspaceActionMoveTo;

YWindowManager::YWindowManager(
    IApp *app,
    YActionListener *wmActionListener,
    YSMListener *smListener,
    YWindow *parent,
    Window win)
    : YDesktop(parent, win),
    app(app),
    wmActionListener(wmActionListener),
    smActionListener(smListener),
    fLayout(_NET_WM_ORIENTATION_HORZ, OldMaxWorkspaces, 1, _NET_WM_TOPLEFT)
{
    fWmState = WMState(-1);
    fShowingDesktop = false;
    fExitWhenDone = false;
    fActiveWindow = (Window) -1;
    fFocusWin = nullptr;
    lockFocusCount = 0;
    fServerGrabCount = 0;
    fCascadeX = 0;
    fCascadeY = 0;
    fIconColumn = 0;
    fIconRow = 0;

    fColormapWindow = nullptr;
    fActiveWorkspace = WinWorkspaceInvalid;
    fLastWorkspace = WinWorkspaceInvalid;
    fArrangeCount = 0;
    fArrangeInfo = nullptr;
    rootProxy = nullptr;
    fWorkArea = nullptr;
    fWorkAreaWorkspaceCount = 0;
    fWorkAreaScreenCount = 0;
    fWorkAreaLock = 0;
    fWorkAreaUpdate = 0;
    fRestackLock = 0;
    fRestackUpdate = 0;
    fFullscreenEnabled = true;
    fCreatedUpdated = true;
    fLayeredUpdated = true;
    fDefaultKeyboard = 0;
    fSwitchWindow = nullptr;
    fDockApp = nullptr;

    manager = this;
    desktop = this;

    setWmState(wmSTARTUP);
    setStyle(wsManager);
    setPointer(YWMApp::leftPointer);
#ifdef CONFIG_XRANDR
    if (xrandr.supported) {
        XRRSelectInput(xapp->display(), handle(),
                       RRScreenChangeNotifyMask
                       | RRCrtcChangeNotifyMask
                       | RROutputChangeNotifyMask
                       | RROutputPropertyNotifyMask
                      );
    }
#endif

    fTopWin = new YTopWindow();
    fTopWin->setStyle(wsOverrideRedirect | wsInputOnly);
    fTopWin->setGeometry(YRect(-1, -1, 1, 1));
    fTopWin->setTitle("IceTopWin");
    fTopWin->show();

    fBottom = new YWindow();
    fBottom->setStyle(wsOverrideRedirect | wsInputOnly);
    fBottom->setGeometry(YRect(-1, -1, 1, 1));
    fBottom->setTitle("IceBottom");

    if (edgeHorzWorkspaceSwitching) {
        edges += new EdgeSwitch(this, -1, false);
        edges += new EdgeSwitch(this, +1, false);
    }
    if (edgeVertWorkspaceSwitching) {
        edges += new EdgeSwitch(this, -1, true);
        edges += new EdgeSwitch(this, +1, true);
    }

    YWindow::setWindowFocus();
}

YWindowManager::~YWindowManager() {
    if (fWorkArea) {
        delete [] fWorkArea[0];
        delete [] fWorkArea;
    }
    delete fTopWin;
    delete fBottom;
    delete rootProxy;
    delete fSwitchWindow;
    delete fDockApp;
}

void YWindowManager::setWmState(WMState newWmState) {
    if (fWmState != newWmState) {
        MSG(("wmstate %d", newWmState));
        fWmState = newWmState;
    }
}

void YWindowManager::grabServer() {
    if (0 == fServerGrabCount++)
        XGrabServer(xapp->display());
}

void YWindowManager::ungrabServer() {
    if (0 == --fServerGrabCount)
        XUngrabServer(xapp->display());
}

void YWindowManager::grabKeys() {
    XUngrabKey(xapp->display(), AnyKey, AnyModifier, handle());

    ///if (taskBar && taskBar->addressBar())
        grab(gKeySysAddressBar);
    if (quickSwitch) {
        grab(gKeySysSwitchNext);
        grab(gKeySysSwitchLast);
        grab(gKeySysSwitchClass);
    }
    grab(gKeySysWinNext);
    grab(gKeySysWinPrev);
    grab(gKeySysDialog);

    grab(gKeySysWorkspacePrev);
    grab(gKeySysWorkspaceNext);
    grab(gKeySysWorkspaceLast);

    grab(gKeySysWorkspacePrevTakeWin);
    grab(gKeySysWorkspaceNextTakeWin);
    grab(gKeySysWorkspaceLastTakeWin);

    grab(gKeySysWinMenu);
    grab(gKeySysMenu);
    grab(gKeySysWindowList);
    grab(gKeySysWinListMenu);

    grab(gKeySysWorkspace1);
    grab(gKeySysWorkspace2);
    grab(gKeySysWorkspace3);
    grab(gKeySysWorkspace4);
    grab(gKeySysWorkspace5);
    grab(gKeySysWorkspace6);
    grab(gKeySysWorkspace7);
    grab(gKeySysWorkspace8);
    grab(gKeySysWorkspace9);
    grab(gKeySysWorkspace10);
    grab(gKeySysWorkspace11);
    grab(gKeySysWorkspace12);

    grab(gKeySysWorkspace1TakeWin);
    grab(gKeySysWorkspace2TakeWin);
    grab(gKeySysWorkspace3TakeWin);
    grab(gKeySysWorkspace4TakeWin);
    grab(gKeySysWorkspace5TakeWin);
    grab(gKeySysWorkspace6TakeWin);
    grab(gKeySysWorkspace7TakeWin);
    grab(gKeySysWorkspace8TakeWin);
    grab(gKeySysWorkspace9TakeWin);
    grab(gKeySysWorkspace10TakeWin);
    grab(gKeySysWorkspace11TakeWin);
    grab(gKeySysWorkspace12TakeWin);

    grab(gKeySysTileVertical);
    grab(gKeySysTileHorizontal);
    grab(gKeySysCascade);
    grab(gKeySysArrange);
    grab(gKeySysUndoArrange);

    if (minimizeToDesktop)
    grab(gKeySysArrangeIcons);
    grab(gKeySysMinimizeAll);
    grab(gKeySysHideAll);

    grab(gKeySysShowDesktop);
    if (taskBar || showTaskBar) {
        grab(gKeySysCollapseTaskBar);
        grab(gKeyTaskBarSwitchNext);
        grab(gKeyTaskBarSwitchPrev);
        grab(gKeyTaskBarMoveNext);
        grab(gKeyTaskBarMovePrev);
    }
    if (configKeyboards.nonempty())
        grab(gKeySysKeyboardNext);

    {
        KProgramIterType k = keyProgs.iterator();
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

    for (YFrameWindow *ff = topLayer(); ff; ff = ff->nextLayer()) {
        ff->grabKeys();
    }
}

YProxyWindow::YProxyWindow(YWindow *parent): YWindow(parent) {
    setStyle(wsOverrideRedirect);
    setTitle("IceRootProxy");
}

YProxyWindow::~YProxyWindow() {
}

void YProxyWindow::handleButton(const XButtonEvent &/*button*/) {
}

void YWindowManager::setupRootProxy() {
    if (grabRootWindow) {
        rootProxy = new YProxyWindow(nullptr);
    }
}

bool YWindowManager::handleTimer(YTimer* timer) {
    if (timer == fSwitchDownTimer) {
        fSwitchDownTimer = null;
        if (switchWindowVisible() == false) {
            delete fSwitchWindow;
            fSwitchWindow = nullptr;
        }
    }
    if (timer == fLayoutTimer) {
        fLayoutTimer = null;
        if (taskBar) {
            taskBar->keyboardUpdate(null);
        }
    }
    return false;
}

void YWindowManager::handlePopDown(YPopupWindow* popup) {
    if (popup == fSwitchWindow) {
        fSwitchDownTimer->setTimer(30000, this, true);
    }
}

SwitchWindow* YWindowManager::getSwitchWindow() {
    if (fSwitchWindow == nullptr && quickSwitch) {
        fSwitchWindow = new SwitchWindow(desktop, nullptr, quickSwitchVertical);
    }
    return fSwitchWindow;
}

bool YWindowManager::switchWindowVisible() const {
    return fSwitchWindow && fSwitchWindow->visible();
}

bool YWindowManager::handleSwitchWorkspaceKey(const XKeyEvent& key,
        KeySym k, unsigned vm)
{
    if (IS_WMKEY(k, vm, gKeySysWorkspacePrev)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToPrevWorkspace(false);
        return true;
    }
    else if (IS_WMKEY(k, vm, gKeySysWorkspaceNext)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToNextWorkspace(false);
        return true;
    }
    else if (IS_WMKEY(k, vm, gKeySysWorkspaceLast)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToLastWorkspace(false);
        return true;
    }
    else if (IS_WMKEY(k, vm, gKeySysWorkspace1)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(0, false);
        return true;
    }
    else if (IS_WMKEY(k, vm, gKeySysWorkspace2)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(1, false);
        return true;
    }
    else if (IS_WMKEY(k, vm, gKeySysWorkspace3)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(2, false);
        return true;
    }
    else if (IS_WMKEY(k, vm, gKeySysWorkspace4)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(3, false);
        return true;
    }
    else if (IS_WMKEY(k, vm, gKeySysWorkspace5)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(4, false);
        return true;
    }
    else if (IS_WMKEY(k, vm, gKeySysWorkspace6)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(5, false);
        return true;
    }
    else if (IS_WMKEY(k, vm, gKeySysWorkspace7)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(6, false);
        return true;
    }
    else if (IS_WMKEY(k, vm, gKeySysWorkspace8)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(7, false);
        return true;
    }
    else if (IS_WMKEY(k, vm, gKeySysWorkspace9)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(8, false);
        return true;
    }
    else if (IS_WMKEY(k, vm, gKeySysWorkspace10)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(9, false);
        return true;
    }
    else if (IS_WMKEY(k, vm, gKeySysWorkspace11)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(10, false);
        return true;
    }
    else if (IS_WMKEY(k, vm, gKeySysWorkspace12)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        switchToWorkspace(11, false);
        return true;
    }
    return false;
}

bool YWindowManager::handleWMKey(const XKeyEvent &key, KeySym k, unsigned vm) {
    YFrameWindow *frame = getFocus();

    for (KProgramIterType p = keyProgs.iterator(); ++p; ) {
        if (p->isKey(k, vm)) {
            XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
            p->open(key.state);
            return true;
        }
    }

    if (IS_WMKEY(k, vm, gKeySysSwitchNext)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        if (getSwitchWindow())
            getSwitchWindow()->begin(true, key.state);
        return true;
    }
    else if (IS_WMKEY(k, vm, gKeySysSwitchLast)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        if (getSwitchWindow())
            getSwitchWindow()->begin(false, key.state);
        return true;
    }
    else if (gKeySysSwitchClass.eq(k, vm)) {
        XAllowEvents(xapp->display(), AsyncKeyboard, key.time);
        if (getSwitchWindow()) {
            char *prop = frame && frame->client()->adopted()
                       ? frame->client()->classHint()->resource() : nullptr;
            getSwitchWindow()->begin(true, key.state, prop);
        }
        return true;
    }
    else if (IS_WMKEY(k, vm, gKeySysWinNext)) {
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
    }
    else if (handleSwitchWorkspaceKey(key, k, vm)) {
        return true;
    }
    else if (IS_WMKEY(k, vm, gKeySysWorkspacePrevTakeWin)) {
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
        wmActionListener->actionPerformed(actionCollapseTaskbar, 0);
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
    } else if (IS_WMKEY(k, vm, gKeySysKeyboardNext)) {
        if (configKeyboards.nonempty())
            setKeyboard((fDefaultKeyboard + 1) % configKeyboards.getCount());
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
        bool handled = handleWMKey(key, k, vm);
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
        notbit(useRootButtons, 1 << (button.button - Button1)) &&
        !hasbits(button.state, (ControlMask | xapp->AltMask)))
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

    if (button.type == ButtonPress) {
        if (useMouseWheel && inrange(int(button.button), Button4, Button5) &&
            (xapp->hasControlAlt(button.state) || xapp->hasWinMask(button.state)))
        {
            YFrameWindow* frame = getFocus();
            if (frame) {
                if (button.button == Button4)
                    frame->wmNextWindow();
                else if (button.button == Button5)
                    frame->wmPrevWindow();
            }
        }
        else if (button.button + 10 == rootMenuButton) {
            if (rootMenu)
                rootMenu->popup(nullptr, nullptr, nullptr, button.x, button.y,
                                YPopupWindow::pfCanFlipVertical |
                                YPopupWindow::pfCanFlipHorizontal |
                                YPopupWindow::pfPopupMenu |
                                YPopupWindow::pfButtonDown);
        }
        else if (button.button + 10 == rootWinMenuButton) {
            popupWindowListMenu(this, button.x, button.y);
        }
        else {
            // allow buttons to trigger actions from "keys" for #333.
            KeySym k = button.button - Button1 + XK_Pointer_Button1;
            unsigned int m = KEY_MODMASK(button.state);
            unsigned int vm = VMod(m);
            KProgramIterType p = keyProgs.iterator();
            while (++p) {
                if (p->isKey(k, vm)) {
                    p->open(m);
                    break;
                }
            }
        }
    }
    YWindow::handleButton(button);
}

void YWindowManager::handleClick(const XButtonEvent &up, int count) {
    if (count == 1) {
        if (up.button == rootMenuButton) {
            if (rootMenu)
                rootMenu->popup(nullptr, nullptr, nullptr, up.x, up.y,
                                YPopupWindow::pfCanFlipVertical |
                                YPopupWindow::pfCanFlipHorizontal |
                                YPopupWindow::pfPopupMenu);
        }
        else if (up.button == rootWinMenuButton) {
            popupWindowListMenu(this, up.x, up.y);
        }
        else if (up.button == rootWinListButton) {
            windowList->showFocused(up.x_root, up.y_root);
        }
    }
}

void YWindowManager::handleConfigure(const XConfigureEvent &configure) {
    if (configure.window == handle()) {
#ifdef CONFIG_XRANDR
        updateScreenSize((XEvent *)&configure);
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
    if (unmap.send_event) {
        YFrameClient* client = findClient(unmap.window);
        if (client) {
            if (client->visible() && client->getFrame()) {
                client->getFrame()->actionPerformed(actionHide);
            } else {
                client->handleUnmapNotify(unmap);
            }
        }
    }
}

void YWindowManager::handleClientMessage(const XClientMessageEvent &message) {
    if (message.message_type == _XA_NET_CURRENT_DESKTOP) {
        MSG(("ClientMessage: _NET_CURRENT_DESKTOP => %ld", message.data.l[0]));
        long ws = message.data.l[0];
        if (inrange(ws, 0L, workspaceCount - 1L)) {
            activateWorkspace(int(ws));
        }
        return;
    }
    if (message.message_type == _XA_NET_NUMBER_OF_DESKTOPS) {
        MSG(("ClientMessage: _NET_NUMBER_OF_DESKTOPS => %ld", message.data.l[0]));
        long num = message.data.l[0];
        long now = workspaceCount;
        if (now < num && num <= NewMaxWorkspaces) {
            extendWorkspaces(int(num));
        }
        else if (0 < num && num < now) {
            lessenWorkspaces(int(num));
        }
        return;
    }
    if (message.message_type == _XA_NET_SHOWING_DESKTOP) {
        MSG(("ClientMessage: _NET_SHOWING_DESKTOP => %ld", message.data.l[0]));
        if (message.data.l[0] == 0 && fShowingDesktop) {
            undoArrange();
            setShowingDesktop(false);
        }
        else if (message.data.l[0] == True && !fShowingDesktop) {
            YArrange arrange = getWindowsToArrange(true, true, true);
            if (arrange) {
                setWindows(arrange, actionMinimizeAll);
                arrange.discard();
            }
            setShowingDesktop(true);
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

        bool border = true;
        bool dialog = false;

        XSizeHints* sh = XAllocSizeHints();
        if (sh) {
            long supplied = None;
            if (XGetWMNormalHints(xapp->display(), win, sh, &supplied)) {
                if (hasbits(sh->flags, PMinSize | PMaxSize) &&
                    sh->min_width == sh->max_width &&
                    sh->min_height == sh->max_height) {
                    dialog = true;
                }
            }
            XFree(sh);
        }

        MwmHints mwm;
        Atom type = None;
        int format = None;
        unsigned long count = None;
        unsigned long after = None;
        long* data = nullptr;
        if (XGetWindowProperty(xapp->display(), win,
                               _XATOM_MWM_HINTS, None,
                               PROP_MWM_HINTS_ELEMENTS, False,
                               _XATOM_MWM_HINTS, &type,
                               &format, &count, &after,
                               (unsigned char **) &data) == Success) {
            if (count == PROP_MWM_HINTS_ELEMENTS &&
                type == _XATOM_MWM_HINTS &&
                format == F32) {
                memcpy((void *) &mwm, (void *) data, sizeof mwm);
                if (mwm.hasFuncs()) {
                    dialog = notbit(mwm.funcs(), MWM_FUNC_RESIZE);
                }
                if (mwm.hasDecor()) {
                    dialog = notbit(mwm.decor(), MWM_DECOR_RESIZEH);
                    border = hasbit(mwm.decor(), MWM_DECOR_BORDER);
                }
            }
            XFree(data);
            data = nullptr;
            count = None;
            format = None;
        }

        if (XGetWindowProperty(xapp->display(), win,
                               _XA_NET_WM_WINDOW_TYPE, None,
                               16, False,
                               AnyPropertyType, &type,
                               &format, &count, &after,
                               (unsigned char **) &data) == Success) {
            WindowType wt = wtNormal;
            struct { Atom atom; WindowType wt; } types[] = {
                { _XA_NET_WM_WINDOW_TYPE_COMBO,         wtCombo },
                { _XA_NET_WM_WINDOW_TYPE_DESKTOP,       wtDesktop },
                { _XA_NET_WM_WINDOW_TYPE_DIALOG,        wtDialog },
                { _XA_NET_WM_WINDOW_TYPE_DND,           wtDND },
                { _XA_NET_WM_WINDOW_TYPE_DOCK,          wtDock },
                { _XA_NET_WM_WINDOW_TYPE_DROPDOWN_MENU, wtDropdownMenu },
                { _XA_NET_WM_WINDOW_TYPE_MENU,          wtMenu },
                { _XA_NET_WM_WINDOW_TYPE_NORMAL,        wtNormal },
                { _XA_NET_WM_WINDOW_TYPE_NOTIFICATION,  wtNotification },
                { _XA_NET_WM_WINDOW_TYPE_POPUP_MENU,    wtPopupMenu },
                { _XA_NET_WM_WINDOW_TYPE_SPLASH,        wtSplash },
                { _XA_NET_WM_WINDOW_TYPE_TOOLBAR,       wtToolbar },
                { _XA_NET_WM_WINDOW_TYPE_TOOLTIP,       wtTooltip, },
                { _XA_NET_WM_WINDOW_TYPE_UTILITY,       wtUtility },
            };
            for (int i = 0; i < int(count) && format == F32; ++i) {
                for (auto type : types) {
                    if (type.atom == Atom(data[i])) {
                        wt = type.wt;
                        i = count;
                        break;
                    }
                }
            }
            switch (wt) {
                case wtCombo:
                case wtDesktop:
                case wtDND:
                case wtDock:
                case wtDropdownMenu:
                case wtMenu:
                case wtNotification:
                case wtPopupMenu:
                case wtSplash:
                case wtTooltip:
                    border = dialog = false;
                    break;
                case wtDialog:
                case wtNormal:
                case wtToolbar:
                case wtUtility:
                    break;
            }
            XFree(data);
        }

        long bx = border ? dialog ? wsDlgBorderX : wsBorderX : 0;
        long by = border ? dialog ? wsDlgBorderY : wsBorderY : 0;
        long ty = wsTitleBar;

        long extents[] = { bx, bx, by + ty, by, };
        XChangeProperty(xapp->display(), win,
                        _XA_NET_FRAME_EXTENTS, XA_CARDINAL,
                        32, PropModeReplace,
                        (unsigned char *) extents, 4);
        return;
    }
    if (message.message_type == _XA_WM_PROTOCOLS &&
        message.data.l[0] == (long) _XA_NET_WM_PING) {
        YFrameClient* client = findClient(message.data.l[2]);
        if (client) {
            client->recvPing(message);
        }
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
        case ICEWM_ACTION_WINOPTIONS:
        case ICEWM_ACTION_RELOADKEYS:
            smActionListener->handleSMAction(action);
            break;
        }
    }
}

void YWindowManager::handleFocus(const XFocusChangeEvent &focus) {
    if (focus.mode == NotifyNormal) {
        if (focus.type == FocusIn) {
            if (focus.detail == NotifyDetailNone) {
                if (clickFocus || !strongPointerFocus)
                    focusLastWindow();
            }
        } else if (focus.type == FocusOut) {
            if (focus.detail != NotifyInferior) {
                if (fFocusWin) {
                    fFocusWin->client()->testDestroyed();
                }
                /* This causes more problems than it solves, as a change of
                 * focus has already been processed for some other event.
                 * This is likely to be an old event and no longer relevant.
                 * Maybe consider only NotifyDetailNone or NotifyPointerRoot.
                switchFocusFrom(fFocusWin);
                 * Need to test strongPointerFocus. */
            }
        }
    }
}

Window YWindowManager::findWindow(const char *resource) {
    if (isEmpty(resource))
        return None;

    for (YFrameIter iter = focusedReverseIterator(); ++iter; ) {
        for (YFrameClient* cli : iter->clients()) {
            if (cli && cli->adopted() && !cli->destroyed()) {
                if (cli->classHint()->match(resource)) {
                    return cli->handle();
                }
            }
        }
    }

    Window match = None;
    xsmart<Window> clients;
    unsigned count = 0;
    if (xapp->children(handle(), &clients, &count)) {
        for (unsigned i = 0; match == None && i < count; ++i) {
            YWindow* ywin = windowContext.find(clients[i]);
            if (nullptr == ywin) {
                match = matchWindow(clients[i], resource);
            }
        }
    }

    return match;
}

Window YWindowManager::findWindow(Window win, char const* resource,
                                  int maxdepth) {
    if (isEmpty(resource))
        return None;

    Window match = None;
    xsmart<Window> clients;
    unsigned count = 0;
    if (xapp->children(win, &clients, &count)) {
        for (unsigned i = 0; match == None && i < count; ++i) {
            if (matchWindow(clients[i], resource))
                match = clients[i];
        }
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
    return hint.get(handle()) && hint.match(resource);
}

YFrameWindow *YWindowManager::findFrame(Window win) {
    YFrameClient* client = clientContext.find(win);
    return client ? client->getFrame() : nullptr;
}

YFrameClient *YWindowManager::findClient(Window win) {
    return clientContext.find(win);
}

void YWindowManager::setFocus(YFrameWindow *f, bool canWarp, bool reorder) {
    YFrameClient* c = nullptr;
    Window w = None;

    if (focusLocked())
        return;
    MSG(("SET FOCUS f=%p", f));

    if (f == nullptr || (fFocusWin && fFocusWin->visible() == false)) {
        switchFocusFrom(fFocusWin);
    }

    if (f && f->visible()) {
        c = f->client();
        if (c && c->visible() && f->isMapped())
            w = c->handle();
        else
            w = f->handle();

        if (f->getInputFocusHint() ||
                ((f->isRollup() || f->isFullscreen())
                    && !f->avoidFocus()))
            switchFocusTo(f, reorder);

        f->setWmUrgency(false);
    }
#ifdef DEBUG
    if (w == desktop->handle()) {
        MSG(("%lX Focus 0x%lX desktop",
             xapp->getEventTime("focus1"), w));
    } else if (f && w == f->handle()) {
        MSG(("%lX Focus 0x%lX frame %s",
             xapp->getEventTime("focus1"), w, f->getTitle().c_str()));
    } else if (f && c && w == c->handle()) {
        MSG(("%lX Focus 0x%lX client %s",
             xapp->getEventTime("focus1"), w, f->getTitle().c_str()));
    } else {
        MSG(("%lX Focus 0x%lX",
             xapp->getEventTime("focus1"), w));
    }
#endif

    bool focusUnset(fFocusWin == nullptr ||
                    fFocusWin->visible() == false ||
                    fFocusWin->isRollup());
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
        notifyActive(nullptr);
        fTopWin->setFrame(f);
    } else {
        fTopWin->setFrame(nullptr);
    }

    if (c &&
        w == c->handle() &&
        (c->protocol(YFrameClient::wpTakeFocus) ||
         f->frameOption(YFrameWindow::foAppTakesFocus)))
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
        isRunning())
    {
        XWarpPointer(xapp->display(), None, handle(), 0, 0, 0, 0,
                     f->x() + f->borderX(), f->y() + f->borderY() + f->titleY());
    }

    if (f) {
        const WindowOption* wo = f->client()->getWindowOption();
        if (wo && nonempty(wo->keyboard)) {
            setKeyboard(wo->keyboard);
        } else {
            setKeyboard(fDefaultKeyboard);
        }
    }
    else {
        setKeyboard(fDefaultKeyboard);
    }

    MSG(("SET FOCUS END"));
}

YFrameWindow *YWindowManager::top(int layer) const {
    PRECONDITION(validLayer(layer));
    return fLayers[layer].front();
}

void YWindowManager::setTop(int layer, YFrameWindow *top) {
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

YFrameWindow *YWindowManager::bottom(int layer) const {
    return fLayers[layer].back();
}

void YWindowManager::setBottom(int layer, YFrameWindow *bottom) {
    fLayers[layer].append(bottom);
    fLayeredUpdated = true;
}

void YWindowManager::installColormap(Colormap cmap) {
    if (xapp->hasColormap()) {
        //MSG(("installing colormap 0x%lX", cmap));
        if (xapp->grabWindow() == nullptr) {
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
    YWindow sheet(this);
    sheet.setStyle(wsOverrideRedirect);
    sheet.lower();
    sheet.setGeometry(geometry());
    sheet.show();
    xapp->sync();

    setWmState(wmSTARTUP);
    lockWorkArea();
    lockRestack();
    grabServer();
    if (fDockApp == nullptr) {
        fDockApp = new DockApp;
    }

    struct restore {
        unsigned name;
        YFrameWindow* frame;
        int count;
        long* tabs;
        restore(unsigned n, int k) :
            name(n), frame(nullptr), count(k), tabs(new long[k])
        { }
        ~restore() { delete[] tabs; }
        int find(Window window) {
            for (int i = 0; i < count; ++i)
                if (Window(tabs[i]) == window)
                    return i;
            return -1;
        }
    };
    YObjectArray<restore> restoreTabs;
    YContext<restore> tabbing;
    YProperty tabs(this, _XA_ICEWM_TABS, F32, 8192, XA_CARDINAL, True);
    if (tabs) {
        for (int i = 0; i + 3 < int(tabs.size()); ) {
            unsigned name = unsigned(tabs[i]);
            int count = int(tabs[i + 1]);
            if (2 <= count && count <= int(tabs.size()) - i - 2) {
                restore* res = new restore(name, count);
                for (int k = 0; k < count; ++k) {
                    long cli = tabs[i + 2 + k];
                    res->tabs[k] = cli;
                    tabbing.save(cli, res);
                }
                restoreTabs += res;
                i += 2 + count;
            } else {
                break;
            }
        }
    }

    unsigned count = 0;
    xsmart<Window> clients;
    if (xapp->children(handle(), &clients, &count)) {
        const int igsize = 5;
        Window ignore[igsize] = {
            fTopWin ? fTopWin->handle() : None,
            fBottom ? fBottom->handle() : None,
            rootProxy ? rootProxy->handle() : None,
            fDockApp ? fDockApp->handle() : None,
            sheet.handle()
        };
        for (unsigned i = 0; i < count; i++) {
            const Window win = clients[i];
            int k = 0;
            while (k < igsize && ignore[k] != win)
                ++k;
            if (k == igsize && findClient(win) == nullptr) {
                YFrameClient* client = allocateClient(win, false);
                if (client) {
                    restore* res = tabbing.find(client->handle());
                    if (res && res->frame) {
                        int orig = res->find(client->handle());
                        int pos = 0;
                        int num = 0;
                        for (YFrameClient* cli : res->frame->clients()) {
                            Window w = cli->handle();
                            int fd = res->find(w);
                            if (0 <= fd && fd < orig)
                                pos = num + 1;
                            ++num;
                        }
                        fCreatedUpdated = fLayeredUpdated = true;
                        res->frame->createTab(client, pos);
                        if (fSwitchWindow)
                            fSwitchWindow->createdClient(res->frame, client);
                    } else {
                        manageClient(client);
                        if (client->getFrame() == nullptr) {
                            delete client;
                        }
                        else if (res) {
                            res->frame = client->getFrame();
                            if (res->name && !res->frame->getFrameName())
                                res->frame->setFrameName(res->name);
                        }
                    }
                }
            }
        }
    }
    sheet.hide();

    setWmState(wmRUNNING);
    ungrabServer();
    unlockRestack();
    unlockWorkArea();

    YProperty prop(this, _XA_NET_ACTIVE_WINDOW, F32, 1, XA_WINDOW);
    if (prop && prop[0]) {
        YFrameWindow* frame = findFrame(prop[0]);
        if (frame && frame->visible() && frame->visibleNow() &&
            frame->canFocus() && frame->client()->adopted())
        {
            setFocus(frame);
        }
    }
    if (getFocus() == nullptr) {
        focusTopWindow();
    }
    for (MiniIcon* icon : MiniIcon::fIcons) {
        if (icon->getFrame()->isIconic()) {
            icon->lower();
            icon->show();
        }
    }

    for (restore* res : restoreTabs) {
        for (int i = 0; i < res->count; ++i) {
            tabbing.remove(res->tabs[i]);
        }
    }
}

void YWindowManager::unmanageClients() {
    setWmState(wmSHUTDOWN);
    lockWorkArea();
    if (taskBar)
        taskBar->detachDesktopTray();
    setFocus(nullptr);

    YArray<Atom> tabbed;
    for (YFrameWindow* frame : YFrameWindow::tabbing()) {
        tabbed += frame->getFrameName();
        tabbed += frame->tabCount();
        for (YFrameClient* client : frame->clients()) {
            tabbed += client->handle();
        }
    }
    if (tabbed.nonempty()) {
        setProperty(_XA_ICEWM_TABS, XA_CARDINAL, &*tabbed, tabbed.getCount());
    }

    grabServer();

    for (int l = 0; l < WinLayerCount; l++) {
        while (bottom(l)) {
            YFrameWindow* frame = bottom(l);
            YFrameClient* client = frame->client();

            frame->unmanage();
            delete frame;

            if (client->adopted()) {
                client->show();
                delete client;
            }
        }
    }
    delete fDockApp; fDockApp = nullptr;

    XSetInputFocus(xapp->display(), PointerRoot, RevertToNone, CurrentTime);
    notifyActive(nullptr);
    ungrabServer();
    XSync(xapp->display(), True);
    unlockWorkArea();
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

    YFrameWindow *frame = nullptr;

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

void YWindowManager::tryCover(bool down, YFrameWindow *frame,
                              int x, int y, int w, int h,
                              int& px, int& py, int& cover,
                              int mx, int my, int Mx, int My)
{
    int ncover;

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
    getWorkArea(frame1, &mx, &my, &Mx, &My, xiscreen);

    x = mx;
    y = my;

    int cover, px, py;
    int *xcoord, *ycoord;
    int xcount, ycount;
    int n = 0;
    YFrameWindow *frame = nullptr;

    if (down) {
        frame = top(frame1->getActiveLayer());
    } else {
        frame = frame1;
    }
    YFrameWindow *f = nullptr;

    for (f = frame; f; f = (down ? f->next() : f->prev()))
        n++;
    n = (n + 1) * 2;
    xcoord = new int[n];
    if (xcoord == nullptr)
        return false;
    ycoord = new int[n];
    if (ycoord == nullptr) {
        delete[] xcoord;
        return false;
    }

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
    while (true) {
        x = xcoord[xn];
        y = ycoord[yn];

        tryCover(down, frame1, x - w, y - h, w, h, px, py, cover, mx, my, Mx, My);
        tryCover(down, frame1, x - w, y    , w, h, px, py, cover, mx, my, Mx, My);
        tryCover(down, frame1, x    , y - h, w, h, px, py, cover, mx, my, Mx, My);
        tryCover(down, frame1, x    , y    , w, h, px, py, cover, mx, my, Mx, My);

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

void YWindowManager::smartPlace(YArrange arrange) {
    if (saveArrange(arrange) == false)
        return;

    int* screens = new int[arrange.size()];
    int k = 0;
    for (YFrameWindow* f : arrange) {
        screens[k++] = f->getScreen();
    }

    int n = getScreenCount();
    for (int s = 0; s < n; s++)
    {
        k = 0;
        for (YFrameWindow* f : arrange) {
            if (s == screens[k]) {
                int x = f->x();
                int y = f->y();
                if (getSmartPlace(false, f, x, y, f->width(), f->height(), s)) {
                    f->setNormalPositionOuter(x, y);
                }
            }
            k++;
        }
    }

    delete[] screens;
}

void YWindowManager::getCascadePlace(YFrameWindow *frame, int &lastX, int &lastY, int &x, int &y, int w, int h) {
    int mx, my, Mx, My;
    getWorkArea(frame, &mx, &my, &Mx, &My);

    /// !!! make auto placement cleaner and (optionally) smarter
    if (lastX < mx) lastX = mx;
    if (lastY < my) lastY = my;

    x = lastX;
    y = lastY;
    y -= min(frame->borderYN(), int(topSideVerticalOffset));

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

void YWindowManager::cascadePlace(YArrange arrange) {
    if (saveArrange(arrange) == false)
        return;

    int mx, my, Mx, My;
    getWorkArea(nullptr, &mx, &my, &Mx, &My);

    int lx = mx;
    int ly = my;
    for (int i = arrange.size(); --i >= 0; ) {
        YFrameWindow* f = arrange[i];
        int x;
        int y;

        getCascadePlace(f, lx, ly, x, y, f->width(), f->height());
        f->setNormalPositionOuter(x, y);
    }
}

void YWindowManager::setWindows(YArrange arrange, YAction action) {
    if (saveArrange(arrange) == false)
        return;

    lockFocus();
    for (YFrameWindow* f : arrange) {
        if (action == actionHideAll) {
            if (!f->isHidden())
                f->setState(WinStateUnmapped, WinStateHidden);
        } else if (action == actionMinimizeAll) {
            if (!f->isMinimized())
                f->setState(WinStateUnmapped, WinStateMinimized);
        }
    }
    unlockFocus();
    focusTopWindow();
}

void YWindowManager::getNewPosition(YFrameWindow *frame, int &x, int &y,
                                    int w, int h, int xiscreen) {
    if (centerTransientsOnOwner && frame->owner()) {
        int mx, my, Mx, My;
        getWorkArea(frame->owner(), &mx, &my, &Mx, &My);
        mx -= frame->borderXN();
        my -= frame->borderYN() - topSideVerticalOffset;
        YRect o(frame->owner()->geometry());
        int cx = max(mx, o.x()) / 2 + min(Mx, o.x() + int(o.width())) / 2 - w / 2;
        int cy = max(my, o.y()) / 2 + min(My, o.y() + int(o.height())) / 2 - h / 2;
        x = max(mx, min(Mx - w, cx));
        y = max(my, min(My - h, cy));
    }
    else if (smartPlacement) {
        getSmartPlace(true, frame, x, y, w, h, xiscreen);
    }
    else {
        getCascadePlace(frame, fCascadeX, fCascadeY, x, y, w, h);
    }
    if (centerLarge) {
        int mx, my, Mx, My;
        getWorkArea(frame, &mx, &my, &Mx, &My);
        int dw = (Mx - mx) / 2, dh = (My - my) / 2;
        if (w > dw && h > dh) {
            x = max(mx, mx + dw - w / 2);
            y = max(my, my + dh - h / 2);
        }
    }
}

void YWindowManager::placeWindow(YFrameWindow *frame,
                                 int x, int y, int cw, int ch,
                                 bool newClient, bool& doActivate)
{
    YFrameClient *client = frame->client();

    int borderWidth = frame->borderXN();
    int borderHeight = frame->borderYN();
    int borderOffset = newClient * min(borderHeight, int(topSideVerticalOffset));
    int titleHeight = frame->titleYN();
    int frameWidth = 2 * borderWidth;
    int frameHeight = 2 * borderHeight + titleHeight;
    int posWidth = cw + frameWidth;
    int posHeight = ch + frameHeight;
    int posX = x;
    int posY = y;

#ifdef CONFIG_SESSION
    if (smapp->haveSessionManager() && findWindowInfo(frame)) {
        if (frame->getWorkspace() != activeWorkspace())
            doActivate = false;
    }
    else
#endif
    if (newClient) {
        if (frame->frameOption(YFrameWindow::foClose)) {
            frame->wmClose();
            doActivate = false;
        }

        const WindowOption* wo = client->getWindowOption();
        if (wo) {
            if (wo->opacity && inrange(wo->opacity, 1, 100)) {
                Atom omax = 0xFFFFFFFF;
                Atom oper = omax / 100;
                Atom orem = omax - oper * 100;
                Atom opaq = wo->opacity * oper + wo->opacity * orem / 100;
                frame->setNetOpacity(opaq);
            }
            if (hasbits(wo->gflags, WidthValue | HeightValue) && wo->gh && wo->gw) {
                posWidth = wo->gw + frameWidth;
                posHeight = wo->gh + frameHeight;
            }
            if (hasbits(wo->gflags, XValue | YValue)) {
                posX = wo->gx;
                posY = wo->gy;
                if (wo->gflags & XNegative)
                    posX += desktop->width() - posWidth;
                if (wo->gflags & YNegative)
                    posY += desktop->height() - posHeight;
                goto setGeo; /// FIX
            }
        }
    }

    if (newClient && client->adopted() && client->sizeHints() &&
        (!(client->sizeHints()->flags & (USPosition | PPosition)) ||
         ((client->sizeHints()->flags & PPosition)
          && frame->frameOption(YFrameWindow::foIgnorePosition)
         )))
    {
        int xiscreen = (fFocusWin ? fFocusWin->getScreen() :
                        frame->owner() ? frame->owner()->getScreen() :
                        xineramaPrimaryScreen);
        getNewPosition(frame, x, y, posWidth, posHeight - borderOffset, xiscreen);
        posX = x;
        posY = y - borderOffset;
    } else {
        int gx, gy;
        client->gravityOffsets(gx, gy);
        if (gx > 0)
            posX -= 2 * borderWidth - client->getBorder() - 1;
        if (gy > 0)
            posY -= 2 * borderHeight + titleHeight - client->getBorder() - 1;
        if (gx == 0 && gy == 0 && client->winGravity() == StaticGravity) {
            posX -= borderWidth;
            posY -= borderHeight + titleHeight;
        }
        posY -= borderOffset;
    }

setGeo:
    MSG(("mapping geometry 1 (%d:%d %dx%d)", posX, posY, posWidth, posHeight));
    frame->setNormalGeometryOuter(posX, posY, posWidth, posHeight);
}

bool YWindowManager::ignoreOverride(Window win, const XWindowAttributes& attr,
                                    int* layer) {
    bool ignoring = false;
    if (WindowOptions::anyOption(YFrameWindow::foIgnoreOverrideRedirect) &&
        1 < attr.width && 1 < attr.height &&
        1 < desktop->geometry().overlap(attr))
    {
        ClassHint hint;
        if (hint.get(win) &&
            nonempty(hint.res_name) && hint.res_name[0] != '/')
        {
            WindowOption wo(hint.res_name);
            if (hintOptions)
                hintOptions->mergeWindowOption(wo, hint.res_name, false);
            if (defOptions)
                defOptions->mergeWindowOption(wo, hint.res_name, false);
            if (wo.hasOption(YFrameWindow::foIgnoreOverrideRedirect)) {
                ignoring = true;
                *layer = wo.layer;
            }
        }
    }
    return ignoring;
}

YFrameClient* YWindowManager::allocateClient(Window win, bool mapClient) {
    YFrameClient* client = nullptr;
    XWindowAttributes attributes;
    int layer = WinLayerInvalid;
    if (XGetWindowAttributes(xapp->display(), win, &attributes) &&
        (attributes.override_redirect == false ||
         ignoreOverride(win, attributes, &layer)) &&
        (mapClient || attributes.map_state > IsUnmapped))
    {
        client = new YFrameClient(nullptr, nullptr, win,
                                  attributes.depth,
                                  attributes.visual,
                                  attributes.colormap);
        if (client && client->isKdeTrayWindow()) {
            if (taskBar && taskBar->windowTrayRequestDock(win)) {
                delete client; client = nullptr;
            }
        }
        if (client && attributes.border_width)
            client->setBorder(attributes.border_width);
        if (client && fDockApp && fDockApp->dock(client))
            client = nullptr;
        if (client && attributes.override_redirect && validLayer(layer))
            client->setLayerHint(layer);
    }
    return client;
}

YFrameWindow* YWindowManager::allocateFrame(YFrameClient* client) {
    unsigned depth = (client->depth() == 32) ? 32 : xapp->depth();
    bool sameDepth = (depth == xapp->depth());
    Visual* visual = (sameDepth ? xapp->visual() : client->visual());
    Colormap clmap = (sameDepth ? xapp->colormap() : client->colormap());
    return new YFrameWindow(wmActionListener, depth, visual, clmap);
}

void YWindowManager::manageClient(YFrameClient* client, bool mapClient) {
    bool canManualPlace = false;
    bool doActivate = (mapClient && (isRunning() || client->visible()));
    bool requestFocus = true;

    MSG(("manage client 0x%lX", client->handle()));
    PRECONDITION(client->getFrame() == nullptr);

    int cx = client->x();
    int cy = client->y();
    int cw = client->width();
    int ch = client->height();
    MSG(("initial geometry 1 (%d:%d %dx%d)", cx, cy, cw, ch));

    YRestackLock restack;
    YFullscreenLock full;

    YFrameWindow* frame = allocateFrame(client);
    if (frame == nullptr) {
        return;
    }
    MSG(("initial geometry 2 (%d:%d %dx%d)",
         client->x(), client->y(), client->width(), client->height()));

    frame->doManage(client, doActivate, requestFocus);
    MSG(("initial geometry 3 (%d:%d %dx%d)",
         client->x(), client->y(), client->width(), client->height()));
    if (frame->client() == nullptr) {
        client->setFrame(nullptr);
        delete frame;
        return;
    }

    placeWindow(frame, cx, cy, cw, ch, isRunning(), doActivate);

    if ((limitSize || limitPosition) &&
        isRunning() &&
        !frame->affectsWorkArea())
    {
        int posX(frame->x() + frame->borderXN()),
            posY(frame->y() + frame->borderYN()),
            posWidth(frame->width() - 2 * frame->borderXN()),
            posHeight(frame->height() - 2 * frame->borderYN());

        MSG(("mapping geometry 2 (%d:%d %dx%d)", posX, posY, posWidth, posHeight));

        if (limitSize) {
            int Mw, Mh;
            getWorkAreaSize(frame, &Mw, &Mh);

            posWidth = min(posWidth, Mw);
            posHeight = min(posHeight, Mh);

/// TODO #warning "cleanup the constrainSize code, there is some duplication"
            posHeight -= frame->titleYN();
            client->constrainSize(posWidth, posHeight, 0);
            posHeight += frame->titleYN();
        }

        if (limitPosition &&
            !(client->sizeHints() &&
              (client->sizeHints()->flags & USPosition)))
        {
            int mx, my, Mx, My;
            getWorkArea(frame, &mx, &my, &Mx, &My);

            posX = clamp(posX, mx, Mx - posWidth);
            posY = clamp(posY, my, My - posHeight);
        }
        posHeight -= frame->titleYN();

        MSG(("mapping geometry 3 (%d:%d %dx%d)", posX, posY, posWidth, posHeight));
        frame->setNormalGeometryInner(posX, posY, posWidth, posHeight);
    }

    if (isRunning()) {
        if (frame->frameOption(YFrameWindow::foAllWorkspaces))
            frame->setAllWorkspaces();
        if (frame->frameOption(YFrameWindow::foFullscreen))
            frame->setState(WinStateFullscreen, WinStateFullscreen);
        else if (frame->frameOption(YFrameWindow::foMaximizedBoth))
            frame->setState(WinStateMaximizedBoth | WinStateFullscreen,
                           (WinStateMaximizedBoth & frame->frameOptions()));
        if (frame->startMinimized()) {
            frame->setState(WinStateUnmapped, WinStateMinimized);
            doActivate = false;
            requestFocus = false;
        }

        if (frame->frameOption(YFrameWindow::foNoFocusOnMap))
            requestFocus = false;
    }

    frame->setManaged(true);

    if (doActivate && manualPlacement && isRunning() &&
        client != windowList &&
        client != taskBar &&
        !frame->owner() &&
        (!client->sizeHints() ||
         !(client->sizeHints()->flags & (USPosition | PPosition))))
        canManualPlace = true;

    if (doActivate && canManualPlace && !opaqueMove &&
        frame->notState(WinStateHidden | WinStateMinimized | WinStateFullscreen)) {
        frame->manualPlace();
    }
    frame->updateState();
    frame->updateProperties();
    frame->updateTaskBar();
    if (frame->affectsWorkArea())
        updateWorkArea();
    if (isRunning()) {
        if (doActivate) {
            frame->activateWindow(true);
            if (canManualPlace && opaqueMove)
                frame->wmMove();
        } else {
            if (mapInactiveOnTop)
                frame->wmRaise();
            if (requestFocus && frame->avoidFocus() == false)
                frame->setWmUrgency(true);
        }
        if (fSwitchWindow)
            fSwitchWindow->createdFrame(frame);
    }
}

void YWindowManager::handleMapNotify(const XMapEvent& map) {
    if (map.override_redirect &&
        map.send_event == false &&
        map.event == xapp->root() &&
        WindowOptions::anyOption(YFrameWindow::foIgnoreOverrideRedirect) &&
        windowContext.find(map.window) == nullptr)
    {
        XWindowAttributes attributes;
        int layer;
        if (XGetWindowAttributes(xapp->display(), map.window, &attributes) &&
            ignoreOverride(map.window, attributes, &layer))
        {
            mapClient(map.window);
        }
    }
}

void YWindowManager::mapClient(Window win) {
    MSG(("mapping window 0x%lX", win));
    YFrameClient* client = findClient(win);
    YFrameWindow* frame = client ? client->getFrame() : nullptr;
    if (frame) {
        if (frame->isUnmapped())
            frame->makeMapped();
        if (clickFocus || !strongPointerFocus)
            frame->activate(true);/// !!! is this ok
    }
    else {
        grabServer();
        lockWorkArea();

        if (client == nullptr) {
            client = allocateClient(win, true);
        }
        if (client && client->adopted()) {
            const WindowOption* wo = client->getWindowOption();
            if (wo && wo->frame) {
                for (YFrameWindow* frame : YFrameWindow::fnaming()) {
                    if (frame->getFrameName() == wo->frame) {
                        int place = frame->tabCount();
                        fCreatedUpdated = fLayeredUpdated = true;
                        frame->createTab(client, place);
                        if (client->activateOnMap())
                            frame->selectTab(client);
                        if (fSwitchWindow)
                            fSwitchWindow->createdClient(frame, client);
                        client = nullptr;
                        break;
                    }
                }
            }
        }
        if (client) {
            manageClient(client, true);
            if (client->getFrame() == nullptr) {
                delete client;
            }
            else {
                YFrameWindow* f = client->getFrame();
                if (f->isFullscreen() && f == getFocus() &&
                    f->getActiveLayer() != WinLayerFullscreen) {
                    f->updateLayer();
                }
            }
        }

        ungrabServer();
        unlockWorkArea();
    }
}

void YWindowManager::unmanageClient(YFrameClient* client) {
    MSG(("unmanaging window 0x%lX", client->handle()));
    YFrameWindow* frame = client->getFrame();
    if (frame) {
        frame->closeTab(client);
        if (frame->isEmpty())
            delete frame;
    }
    else if (client->isDocked() && fDockApp) {
        fDockApp->undock(client);
    }
    else {
        YClientContainer* conter = client->getContainer();
        if (conter) {
            frame = conter->getFrame();
            frame->removeTab(client);
        }
    }
    delete client;
}

void YWindowManager::clientDestroyed(YFrameClient* client) {
    fCreatedUpdated = fLayeredUpdated = true;
    if (notShutting())
        updateClientList();
    if (fSwitchWindow)
        fSwitchWindow->destroyedClient(client);
}

void YWindowManager::clientTransfered(YFrameClient* client, YFrameWindow* frame) {
    fLayeredUpdated = true;
    if (fSwitchWindow)
        fSwitchWindow->transfer(client, frame);
}

void YWindowManager::destroyedClient(Window win) {
    YFrameClient* client = findClient(win);
    if (client) {
        unmanageClient(client);
    } else {
        MSG(("destroyed: unknown window: 0x%lX", win));
    }
}

void YWindowManager::focusTopWindow() {
    if (notRunning() || focusLocked())
        return ;
    if (!clickFocus && strongPointerFocus) {
        XSetInputFocus(xapp->display(), PointerRoot, RevertToNone, CurrentTime);
        notifyActive(nullptr);
        return ;
    }
    if (!focusTop(topLayer(WinLayerOnTop)))
        focusTop(topLayer());
}

bool YWindowManager::focusTop(YFrameWindow *f) {
    if (!f)
        return false;

    f = f->findWindow(YFrameWindow::fwfVisible |
                      YFrameWindow::fwfFocusable |
                      YFrameWindow::fwfNotHidden |
                      YFrameWindow::fwfWorkspace |
                      YFrameWindow::fwfSame |
                      YFrameWindow::fwfLayers |
                      YFrameWindow::fwfCycle);
    //msg("found focus %lX", f);
    setFocus(f);
    return f;
}

YFrameWindow *YWindowManager::getFrameUnderMouse(long workspace) {
    YFrameWindow* frame;
    Window root, xwin = None;
    YWindow* ywin;
    int ignore;
    unsigned ignore2;
    if (XQueryPointer(xapp->display(), xapp->root(), &root, &xwin,
                      &ignore, &ignore, &ignore, &ignore, &ignore2) &&
        xwin != None &&
        (ywin = windowContext.find(xwin)) != nullptr &&
        !ywin->adopted() &&
        (frame = dynamic_cast<YFrameWindow *>(ywin)) != nullptr &&
        frame->isManaged() &&
        frame->visibleOn(workspace) &&
        frame->avoidFocus() == false &&
        frame->client()->destroyed() == false &&
        frame->client()->visible() &&
        frame->client() != taskBar)
    {
        return frame;
    }
    return nullptr;
}

YFrameWindow *YWindowManager::getLastFocus(bool skipAllWorkspaces, long workspace) {
    if (workspace == AllWorkspaces)
        workspace = activeWorkspace();

    YFrameWindow *toFocus = nullptr;

    if (toFocus == nullptr) {
        toFocus = workspaces[workspace].focused;
    }

    if (toFocus != nullptr) {
        if (toFocus->isMinimized() ||
            toFocus->isHidden() ||
            !toFocus->visibleOn(workspace) ||
            toFocus->client()->destroyed() ||
            toFocus->client() == taskBar ||
            toFocus->avoidFocus())
        {
            toFocus = nullptr;
        }
    }

    if (toFocus == nullptr && clickFocus == false) {
        toFocus = getFrameUnderMouse(workspace);
    }

    if (toFocus == nullptr) {
        int pass = 0;
        if (!skipAllWorkspaces)
            pass = 1;
        for (; pass < 3; pass++) {
            YFrameIter w = focusedReverseIterator();
            while (++w) {
                if (!w->client()->adopted())
                    continue;
                if (w->isMinimized())
                    continue;
                if (w->isHidden())
                    continue;
                if (!w->visibleOn(workspace))
                    continue;
                if (w->avoidFocus() || pass == 2)
                    continue;
                if ((w->isAllWorkspaces() && w != fFocusWin) || pass == 1) {
                    if (w->client() != taskBar && toFocus == nullptr)
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
            xapp->getEventTime(0), w, f->getTitle().c_str());
    } else if (f && c && w == c->handle()) {
        msg("%lX Focus 0x%lX client %s",
            xapp->getEventTime(0), w, f->getTitle().c_str());
    } else {
        msg("%lX Focus 0x%lX",
            xapp->getEventTime(0), w);
    }
#endif
    return toFocus;
}

void YWindowManager::focusLastWindow() {
    if (notRunning() || focusLocked())
        return;
    if (!clickFocus && strongPointerFocus) {
        XSetInputFocus(xapp->display(), PointerRoot, RevertToNone, CurrentTime);
        notifyActive(nullptr);
        return ;
    }

    YFrameWindow *toFocus = getLastFocus(false);
    if (toFocus == nullptr || toFocus->client() == taskBar) {
        focusTopWindow();
    } else {
        if (raiseOnFocus)
            toFocus->wmRaise();
        setFocus(toFocus);
    }
}

YFrameWindow* YWindowManager::topLayer(int layer) {
    for (int l = layer; l >= 0; l--)
        if (fLayers[l])
            return fLayers[l].front();

    return nullptr;
}

YFrameWindow* YWindowManager::bottomLayer(int layer) {
    for (int l = layer; l < WinLayerCount; l++)
        if (fLayers[l])
            return fLayers[l].back();

    return nullptr;
}

bool YWindowManager::setAbove(YFrameWindow* frame, YFrameWindow* above) {
    const int layer = frame->getActiveLayer();
    if (above != nullptr && layer != above->getActiveLayer()) {
        MSG(("ignore z-order change between layers: win=0x%lX (above: 0x%lX) ",
                    frame->handle(), above->client()->handle()));
        return false;
    }

#ifdef DEBUG
    if (debug_z) dumpZorder("before setAbove", frame, above);
#endif
    bool change = false;
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
        change = true;
    }
    return change;
}

bool YWindowManager::setBelow(YFrameWindow* frame, YFrameWindow* below) {
    if (below != nullptr &&
        frame->getActiveLayer() != below->getActiveLayer())
    {
        MSG(("ignore z-order change between layers: win=0x%lX (below %ld)",
                   frame->handle(), below->client()->handle()));
        return false;
    }
    if (below != frame->prev() && below != frame) {
        return setAbove(frame, below ? below->next() : nullptr);
    }
    return false;
}

void YWindowManager::removeLayeredFrame(YFrameWindow *frame) {
    int layer = frame->getActiveLayer();
    PRECONDITION(validLayer(layer));
    fLayers[layer].remove(frame);
    fLayeredUpdated = true;
}

void YWindowManager::restackWindows() {
    if (fRestackLock) {
        fRestackUpdate++;
        return;
    }

    YArray<Window> w(12 + focusedCount());

    w.append(fTopWin->handle());

    for (YPopupWindow* p = xapp->popup(); p; p = p->prevPopup())
        w.append(p->handle());

    if (taskBar)
        w.append(taskBar->edgeTriggerWindow());

    for (auto edge : edges)
        w.append(edge->handle());

    if (wmapp->hasCtrlAltDelete()) {
        if (wmapp->getCtrlAltDelete()->visible()) {
            w.append(wmapp->getCtrlAltDelete()->handle());
        }
    }

    if (statusMoveSize && statusMoveSize->visible())
        w.append(statusMoveSize->handle());
    else if (statusWorkspace && statusWorkspace->visible())
        w.append(statusWorkspace->handle());

    int top = w.getCount();
    int dockAppLayer = fDockApp && fDockApp->created()
                     ? fDockApp->layer() : WinLayerInvalid;
    for (int layer = WinLayerCount - 1; layer >= 0; --layer) {
        if (layer == dockAppLayer)
            w.append(fDockApp->handle());
        for (YFrameWindow* f = fLayers[layer].front(); f; f = f->next())
            w.append(f->handle());
    }

    if (quickSwitchRaiseCandidate && switchWindowVisible()) {
        YFrameWindow* active = fSwitchWindow->current();
        if (active) {
            Window handle = active->handle();
            if (findRemove(w, handle))
                w.insert(top, handle);
        }
    }

    w.append(fBottom->handle());
    XRestackWindows(xapp->display(), &*w, w.getCount());

    if (taskBar) {
        taskBar->workspacesRepaint(activeWorkspace());
        taskBar->workspacesRepaint(lastWorkspace());
        taskBar->updateFullscreen();
    }
}

void YWindowManager::getWorkArea(int *mx, int *my, int *Mx, int *My) {
    int s = max(0, min(xineramaPrimaryScreen, getScreenCount() - 1));
    if (fWorkArea && 0 < fWorkAreaWorkspaceCount) {
        *mx = fWorkArea[0][s].fMinX;
        *my = fWorkArea[0][s].fMinY;
        *Mx = fWorkArea[0][s].fMaxX;
        *My = fWorkArea[0][s].fMaxY;
    } else {
        unsigned dw, dh;
        getScreenGeometry(mx, my, &dw, &dh, s);
        *Mx = dw; *My = dh;
    }
}

void YWindowManager::getWorkArea(const YFrameWindow* frame,
                                 int *mx, int *my, int *Mx, int *My,
                                 int xiscreen)
{
    int ws = WinWorkspaceInvalid;

    if (frame) {
        if (xiscreen == -1)
            xiscreen = frame->getScreen();

        if (frame->inWorkArea() || frame->isIconic()) {
            if (frame->isAllWorkspaces())
                ws = activeWorkspace();
            else
                ws = frame->getWorkspace();
        }
    }

    if (inrange(ws, 0, fWorkAreaWorkspaceCount - 1) && 0 <= xiscreen) {
        *mx = fWorkArea[ws][xiscreen].fMinX;
        *my = fWorkArea[ws][xiscreen].fMinY;
        *Mx = fWorkArea[ws][xiscreen].fMaxX;
        *My = fWorkArea[ws][xiscreen].fMaxY;
    } else {
        *mx = 0;
        *my = 0;
        *Mx = width();
        *My = height();
    }

    if (0 <= xiscreen) {
        int dx, dy;
        unsigned dw, dh;
        getScreenGeometry(&dx, &dy, &dw, &dh, xiscreen);

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
    getWorkArea(frame, &mx, &my, &Mx, &My);
    *Mw = Mx - mx;
    *Mh = My - my;
}

void YWindowManager::updateArea(int workspace, int screen_number,
                                int l, int t, int r, int b)
{
    int low = 0, lim = fWorkAreaWorkspaceCount - 1;
    if (inrange(workspace, low, lim)) {
        low = lim = workspace;
    } else if (workspace != WinWorkspaceInvalid) {
        return;
    }
    for (int ws = low; ws <= lim; ++ws) {
        WorkAreaRect *wa = fWorkArea[ws] + screen_number;
        if (l > wa->fMinX) wa->fMinX = l;
        if (t > wa->fMinY) wa->fMinY = t;
        if (r < wa->fMaxX) wa->fMaxX = r;
        if (b < wa->fMaxY) wa->fMaxY = b;
    }
}

void YWindowManager::debugWorkArea(const char* prefix) {
#ifdef DEBUG
    for (int i = 0; i < fWorkAreaWorkspaceCount; i++) {
        for (int j = 0; j < fWorkAreaScreenCount; j++) {
            MSG(("%s: workarea w:%d s:%d %d %d %d %d",
                prefix,
                i, j,
                fWorkArea[i][j].fMinX,
                fWorkArea[i][j].fMinY,
                fWorkArea[i][j].fMaxX,
                fWorkArea[i][j].fMaxY));
        }
    }
#endif
}

void YWindowManager::updateWorkArea() {
    if (fWorkAreaLock) {
        requestWorkAreaUpdate();
    }
    else {
        bool update = false;
        fWorkAreaLock = true;
        do {
            fWorkAreaUpdate = 0;
            update |= updateWorkAreaInner();
        } while (fWorkAreaUpdate);
        fWorkAreaLock = false;
        if (update) {
            workAreaUpdated();
        }
    }
}

bool YWindowManager::updateWorkAreaInner() {
    int oldWorkAreaWorkspaceCount = fWorkAreaWorkspaceCount;
    int oldWorkAreaScreenCount = fWorkAreaScreenCount;
    WorkAreaRect **oldWorkArea = new WorkAreaRect *[::workspaceCount];
    if (oldWorkArea == nullptr)
        return false;
    else {
        long areaCount = ::workspaceCount * getScreenCount();
        oldWorkArea[0] = new WorkAreaRect[areaCount];
        if (oldWorkArea[0] == nullptr) {
            delete[] oldWorkArea;
            return false;
        }
        else {
            fWorkAreaWorkspaceCount = ::workspaceCount;
            fWorkAreaScreenCount = getScreenCount();
            swap(fWorkArea, oldWorkArea);
        }
    }

    for (int i = 0; i < fWorkAreaWorkspaceCount; i++) {
        if (i)
            fWorkArea[i] = fWorkArea[i - 1] + fWorkAreaScreenCount;
        for (int j = 0; j < fWorkAreaScreenCount; j++)
            fWorkArea[i][j] = xiInfo[j];
    }

    debugWorkArea("before");

    for (YFrameWindow *w = topLayer(); w; w = w->nextLayer()) {
        if (w->isUnmapped()) {
            continue;
        }

        MSG(("workarea window %s: ws:%d s:%d x:%d y:%d w:%d h:%d",
            w->getTitle().c_str(), w->getWorkspace(), w->getScreen(),
            w->x(), w->y(), w->width(), w->height()));
        if (w->haveStruts())
        {
            int ws = w->getWorkspace();
            int s = w->getScreen();
            int sx = xiInfo[s].x_org;
            int sy = xiInfo[s].y_org;
            int sw = xiInfo[s].width;
            int sh = xiInfo[s].height;

            int l = sx + w->strutLeft();
            int t = sy + w->strutTop();
            int r = sx + sw - w->strutRight();
            int b = sy + sh - w->strutBottom();

            MSG(("strut %d %d %d %d", w->strutLeft(), w->strutTop(),
                                      w->strutRight(), w->strutBottom()));
            MSG(("limit %d %d %d %d", l, t, r, b));
            updateArea(ws, s, l, t, r, b);
        }

        if (w->doNotCover() ||
            (limitByDockLayer && w->getActiveLayer() == WinLayerDock))
        {
            int ws = w->getWorkspace();
            int s = w->getScreen();
            int sx = xiInfo[s].x_org;
            int sy = xiInfo[s].y_org;
            int sw = xiInfo[s].width;
            int sh = xiInfo[s].height;

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
        debugWorkArea("updated");
    }
    debugWorkArea("after");

    bool changed = false;
    if (oldWorkArea == nullptr ||
        oldWorkAreaWorkspaceCount != fWorkAreaWorkspaceCount ||
        oldWorkAreaScreenCount != fWorkAreaScreenCount) {
        changed = true;
    } else {
        for (int ws = 0; ws < fWorkAreaWorkspaceCount; ws++) {
            for (int s = 0; s < fWorkAreaScreenCount; s++) {
                if (fWorkArea[ws][s] != oldWorkArea[ws][s]) {
                    changed = true;
                    break;
                }
            }
            if (changed)
                break;
        }
    }

    bool resize = false;
    if (changed) {
        MSG(("announceWorkArea"));
        announceWorkArea();
        int spaces = min(fWorkAreaWorkspaceCount, oldWorkAreaWorkspaceCount);
        int screens = min(fWorkAreaScreenCount, oldWorkAreaScreenCount);
        for (YFrameWindow* f = topLayer(); f; f = f->nextLayer()) {
            if (f->x() >= 0 && f->y() >= 0 && f->inWorkArea()) {
                int s = f->getScreen();
                int w = f->isAllWorkspaces()
                      ? activeWorkspace() : f->getWorkspace();
                if (s < spaces && w < spaces &&
                    fWorkArea[w][s].displaced(oldWorkArea[w][s])) {
                    int dx =
                        (f->x() >= oldWorkArea[w][s].fMinX &&
                         f->x() < fWorkArea[w][s].fMinX) ?
                        fWorkArea[w][s].fMinX - f->x() :
                        (f->x() == oldWorkArea[w][s].fMinX &&
                         f->x() > fWorkArea[w][s].fMinX) ?
                        fWorkArea[w][s].fMinX - f->x() :
                        0;
                    int dy =
                        (f->y() >= oldWorkArea[w][s].fMinY &&
                         f->y() < fWorkArea[w][s].fMinY) ?
                        fWorkArea[w][s].fMinY - f->y() :
                        (f->y() == oldWorkArea[w][s].fMinY &&
                         f->y() > fWorkArea[w][s].fMinY) ?
                        fWorkArea[w][s].fMinY - f->y() :
                        0;
                    if (dx || dy) {
                        f->setNormalPositionOuter(f->x() + dx, f->y() + dy);
                    }
                }
            }
        }
        if (screens < oldWorkAreaScreenCount) {
            resize = true;
        }
        else {
            for (int ws = 0; ws < spaces; ws++) {
                for (int s = 0; s < screens; s++) {
                    if (fWorkArea[ws][s].width() < oldWorkArea[ws][s].width())
                        resize = true;
                    if (fWorkArea[ws][s].height() < oldWorkArea[ws][s].height())
                        resize = true;
                }
            }
        }
    }

    if (oldWorkArea) {
        delete [] oldWorkArea[0];
        delete [] oldWorkArea;
    }
    if (resize) {
        MSG(("resizeWindows"));
        resizeWindows();
    }
    return resize | changed;
}

void YWindowManager::announceWorkArea() {
    int nw = workspaceCount;
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

    if (getScreenCount() > 1 && netWorkAreaBehaviour != 1) {
        for (int i = 0; i < getScreenCount(); i++) {
            if (xiInfo[i].x_org != 0 || xiInfo[i].y_org != 0) {
                isCloned = false;
                break;
            }
        }
    }

    const YRect desktopArea(desktop->geometry());
    for (int ws = 0; ws < nw; ws++) {
        YRect r(desktopArea);
        if (netWorkAreaBehaviour != 1) {
            r = fWorkArea[ws][0];
        }

        if (getScreenCount() > 1 && ! isCloned && netWorkAreaBehaviour != 1) {
            if (netWorkAreaBehaviour == 0) {
                // STRUTS information is messy and broken for multimonitor,
                // but there is no solution for this problem.
                // So we imitate metacity's behaviour := merge,
                // but limit height of each screen and hope for the best
                for (int i = 1; i < getScreenCount(); i++) {
                    r.unionRect(fWorkArea[ws][i].fMinX, fWorkArea[ws][i].fMinY,
                                fWorkArea[ws][i].width(),
                                fWorkArea[ws][0].height());
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
}

void YWindowManager::resizeWindows() {
    for (YFrameWindow * f = topLayer(); f; f = f->nextLayer()) {
        if (f->visibleNow() && f->inWorkArea() && !f->client()->destroyed()) {
            if (f->isMaximized())
                f->updateDerivedSize(WinStateMaximizedBoth);
            f->updateLayout();
        }
    }
}

void YWindowManager::workAreaUpdated() {
    if (isRunning() && (taskBar || !showTaskBar)) {
        for (YFrameIter frame = fCreationOrder.iterator(); ++frame; ) {
            if (frame->isIconic()) {
                frame->getMiniIcon()->show();
            }
        }
        if (fDockApp && fDockApp->visible()) {
            fDockApp->adapt();
        }
    }
}

void YWindowManager::arrangeIcons() {
    fIconColumn = fIconRow = 0;
    for (bool tf : {false, true}) {
        for (MiniIcon* icon : MiniIcon::fIcons) {
            if (icon->getFrame()->isMinimized() == tf) {
                icon->getFrame()->updateIconPosition();
            }
        }
    }
}

void YWindowManager::initWorkspaces() {
    long initialWorkspace = 0;

    readDesktopNames(true, true);
    setDesktopCount();
    setDesktopGeometry();
    setDesktopViewport();
    setShowingDesktop();
    readCurrentDesktop(initialWorkspace);
    readDesktopLayout();
    activateWorkspace(initialWorkspace);
}

void YWindowManager::activateWorkspace(int workspace) {
    if (workspace != fActiveWorkspace) {
        lockWorkArea();
        lockFocus();

        if (taskBar && fActiveWorkspace != WinWorkspaceInvalid) {
            taskBar->setWorkspaceActive(fActiveWorkspace, false);
        }
        fLastWorkspace = fActiveWorkspace;
        fActiveWorkspace = workspace;
        if (taskBar) {
            taskBar->setWorkspaceActive(fActiveWorkspace, true);
        }

        setProperty(_XA_NET_CURRENT_DESKTOP, XA_CARDINAL, fActiveWorkspace);

        resizeWindows();

        YArray<YFrameWindow*> stack;
        for (YFrameWindow* w = topLayer(); w; w = w->nextLayer()) {
            if (w->visibleNow()) {
                w->updateState();
                w->updateTaskBar();
            }
            else if (w->getWorkspace() == fLastWorkspace) {
                stack.append(w);
            }
        }
        for (; stack.nonempty(); stack.pop()) {
            YFrameWindow* w = stack.last();
            w->updateState();
            w->updateTaskBar();
        }
        unlockFocus();

        YFrameWindow *toFocus = getLastFocus(true, workspace);
        setFocus(toFocus, false, !switchWindowVisible());
        resetColormap(true);

        if (taskBar) {
            taskBar->relayout();
            taskBar->relayoutNow();
        }

        if (workspaceSwitchStatus
            && (!showTaskBar || !taskBarShowWorkspaces || taskBarAutoHide
                || (taskBar && taskBar->hidden()))
           )
            statusWorkspace->begin(workspace);
        wmapp->signalGuiEvent(geWorkspaceChange);
        unlockWorkArea();
    }
}

void YWindowManager::extendWorkspaces(int target) {
    int ws = workspaceCount;
    if (ws < target && target <= NewMaxWorkspaces) {
        char buf[32];
        do {
            if (nonempty(workspaces.spare(ws))) {
                strlcpy(buf, workspaces.spare(ws), sizeof buf);
            } else {
                snprintf(buf, sizeof buf, ws < 999 ? "%3d " : "%d", 1 + ws);
            }
        } while (workspaces.add(buf) && ++ws < target);

        updateWorkspaces(true);
    }
}

void YWindowManager::lessenWorkspaces(int target) {
    if (target < 1 || workspaceCount <= target)
        return;

    const int last = target - 1;

    // switch away from the workspace being removed
    bool refocus = (fActiveWorkspace > last);
    if (refocus) {
        setFocus(nullptr);
        activateWorkspace(last);
    }

    // move windows away from the workspace being removed
    for (bool changed(true); changed; ) {
        changed = false;
        for (YFrameIter frame(focusedIterator()); ++frame; ) {
            if (frame->getWorkspace() > last) {
                frame->setWorkspace(last);
                changed = true;
            }
        }
    }

    for (int i = workspaceCount - target; 0 < i && target < workspaceCount; --i)
        workspaces.drop();

    updateWorkspaces(false);

    if (refocus)
        focusLastWindow();
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
        taskBar->workspacesUpdateButtons();
    }
    if (windowList) {
        windowList->updateWorkspaces();
    }
    updateMoveMenu();
}

bool YWindowManager::readCurrentDesktop(long &workspace) {
    YProperty netp(this, _XA_NET_CURRENT_DESKTOP, F32, 1L, XA_CARDINAL);
    if (netp) {
        workspace = clamp(*netp, 0L, workspaceCount - 1L);
        return true;
    }
    return false;
}

void YWindowManager::setDesktopGeometry() {
    Atom data[2] = { desktop->width(), desktop->height() };
    MSG(("setting: _NET_DESKTOP_GEOMETRY = (%lu,%lu)", data[0], data[1]));
    setProperty(_XA_NET_DESKTOP_GEOMETRY, XA_CARDINAL, data, 2);
}

void YWindowManager::setShowingDesktop() {
    MSG(("setting: _NET_SHOWING_DESKTOP = %d", fShowingDesktop));
    setProperty(_XA_NET_SHOWING_DESKTOP, XA_CARDINAL, fShowingDesktop);
}

void YWindowManager::setShowingDesktop(bool setting) {

    if (fShowingDesktop != setting) {
        fShowingDesktop = setting;
        setShowingDesktop();
    }
}

void YWindowManager::updateTaskBarNames() {
    if (taskBar) {
        taskBar->workspacesRelabelButtons();
    }
}

void YWindowManager::updateMoveMenu() {
    if (moveMenu) {
        moveMenu->removeAll();
        moveMenu->updatePopup();
    }
}

bool YWindowManager::readDesktopLayout() {
    bool success = false;
    YProperty prop(this, _XA_NET_DESKTOP_LAYOUT, F32, 4L, XA_CARDINAL);
    if (prop && 3 <= prop.size()) {
        int orient = (int) prop[0];
        int cols   = (int) prop[1];
        int rows   = (int) prop[2];
        int corner = (prop.size() == 3) ? _NET_WM_TOPLEFT : (int) prop[3];
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
    MSG(("read: _NET_DESKTOP_LAYOUT(%d): %s (%d, %lu) { %d, %d, %d, %d }",
        (int) _XA_NET_DESKTOP_LAYOUT, boolstr(success), prop.bits(), prop.size(),
        fLayout.orient, fLayout.columns, fLayout.rows, fLayout.corner));

    return success;
}

void YWindowManager::readDesktopNames(bool init, bool net) {
    YStringList netList;
    bool haveNet = readNetDesktopNames(netList);

    if (init) {
        if (haveNet && !strncmp(netList[0], "Workspace", 9))
            haveNet = false;
        for (int i = 0; i < configWorkspaces.getCount(); ++i)
            workspaces.add(configWorkspaces[i]);
        if (workspaces.count() < 1)
            workspaces + " 1 " + " 2 " + " 3 " + " 4 ";
    }

    if (haveNet) {
        compareDesktopNames(netList);
    }
    else {
        setDesktopNames();
    }
}

bool YWindowManager::compareDesktopNames(const YStringList& list) {
    bool changed = false;

    // old strings must persist until after the update
    asmart<csmart> oldWorkspaceNames(new csmart[list.count]);

    for (int i = 0; i < list.count; i++) {
        if (i >= workspaces.count()) {
            workspaces.spare(i, list[i]);
        }
        else if (strcmp(list[i], workspaces[i])) {
            char* name(newstr(list[i]));
            swap(name, *workspaces[i]);
            oldWorkspaceNames[i] = name;
            changed = true;
            MSG(("Workspace %d: '%s' -> '%s'", i, name, list[i]));
        }
    }

    if (changed) {
        updateTaskBarNames();
        updateMoveMenu();
    }

    return changed;
}

bool YWindowManager::readNetDesktopNames(YStringList& list) {
    bool success = false;

    MSG(("reading: _NET_DESKTOP_NAMES(%d)",(int)_XA_NET_DESKTOP_NAMES));

    XTextProperty names;
    if (XGetTextProperty(xapp->display(), handle(), &names,
                         _XA_NET_DESKTOP_NAMES) && names.nitems > 0) {
        if (XmbTextPropertyToTextList(xapp->display(), &names,
                                      &list.strings, &list.count) == Success) {
            if (list.count > 0 && isEmpty(list.last()))
                list.count--;
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

void YWindowManager::setNetDesktopNames(long count) {
    MSG(("setting: _NET_DESKTOP_NAMES"));
    static char terminator[] = { '\0' };
    asmart<char *> strings(new char *[count + 1]);
    for (long i = 0; i < count; i++) {
        strings[i] = i < workspaces.count()
                   ? *workspaces[i]
                   : const_cast<char *>(workspaces.spare(i));
    }
    strings[count] = terminator;
    XTextProperty names;
    if (XmbTextListToTextProperty(xapp->display(), strings, count + 1,
                                  XUTF8StringStyle, &names) == Success) {
        XSetTextProperty(xapp->display(), handle(), &names,
                         _XA_NET_DESKTOP_NAMES);
        XFree(names.value);
    }
}

void YWindowManager::setDesktopNames(long count) {
    MSG(("setting: %ld desktop names", count));
    setNetDesktopNames(count);
}

void YWindowManager::setDesktopNames() {
    setDesktopNames(workspaceCount);
}

void YWindowManager::setDesktopCount() {
    MSG(("setting: _NET_NUMBER_OF_DESKTOPS = %lu", Atom(workspaceCount)));
    setProperty(_XA_NET_NUMBER_OF_DESKTOPS, XA_CARDINAL, Atom(workspaceCount));
}

void YWindowManager::setDesktopViewport() {
    MSG(("setting: _NET_DESKTOP_VIEWPORT"));
    const int n = 2 * workspaceCount;
    Atom data[n];
    for (int i = 0; i < n; ++i)
        data[i] = 0;
    setProperty(_XA_NET_DESKTOP_VIEWPORT, XA_CARDINAL, data, n);
}

void YWindowManager::wmCloseSession() { // ----------------- shutdow started ---
    for (YFrameWindow * f(topLayer()); f; f = f->nextLayer())
        if (f->client()->adopted()) // not to ourselves?
            f->wmClose();
}

void YWindowManager::getIconPosition(MiniIcon* iw, int *iconX, int *iconY) {
    if (isStartup() || fWorkAreaUpdate || (showTaskBar && taskBar == nullptr)) {
        return;
    }

    YFrameWindow* frame = iw->getFrame();

    int mrow, mcol, Mrow, Mcol; /* Minimum and maximum for rows and columns */
    int width, height; /* column width and row height */
    int drow, dcol; /* row and column directions */
    int *iconRow, *iconCol;
    const int margin = 4;

    if (miniIconsPlaceHorizontal) {
        getWorkArea(frame, &mcol, &mrow, &Mcol, &Mrow);
        width = iw->width() + 2 * margin;
        height = iw->height() + 2 * margin;
        drow = miniIconsBottomToTop ? -1 : +1;
        dcol = miniIconsRightToLeft ? -1 : +1;
        iconRow = iconY;
        iconCol = iconX;
    } else {
        getWorkArea(frame, &mrow, &mcol, &Mrow, &Mcol);
        width = iw->height() + 2 * margin;
        height = iw->width() + 2 * margin;
        drow = miniIconsRightToLeft ? -1 : +1;
        dcol = miniIconsBottomToTop ? -1 : +1;
        iconRow = iconX;
        iconCol = iconY;
    }
    if (inrange(*iconX, mcol, Mcol + 1 - (width - 2 * margin)) &&
        inrange(*iconY, mrow, Mrow + 1 - (height - 2 * margin)))
    {
        return;
    }

    /* Calculate start row and start column */
    int srow = (drow > 0) ? mrow : (Mrow - height);
    int scol = (dcol > 0) ? mcol : (Mcol - width);

    if (MiniIcon::fIcons.getCount() < 256) {
        int r = srow, c = scol;
        int omax = width * height, over = omax;
        YRect best(r, c, width, height);
        do {
            YRect geo(c, r, width, height);
            int olap = 0;
            for (MiniIcon* icon : MiniIcon::fIcons) {
                if (icon != iw) {
                    olap += geo.overlap(icon->geometry());
                }
            }
            if (olap < over) {
                over = olap;
                best = geo;
            }
            c += width * dcol;
            if (c >= Mcol - width / 2 || c < mcol - width / 2) {
                c = scol;
                r += height * drow;
                if (r >= Mcol - height / 2 || r < mrow - height / 2) {
                    break;
                }
            }
        } while (0 < over);
        if (over == 0) {
            *iconCol = best.x();
            *iconRow = best.y();
            if ((drow > 0 ? *iconRow > fIconRow : *iconRow < fIconRow)
                || (fIconRow == *iconRow &&
                    (dcol > 0 ? *iconCol > fIconColumn
                      : *iconCol < fIconColumn))) {
                fIconRow = *iconRow;
                fIconColumn = *iconCol;
            }
            return;
        }
    }

    if ((fIconColumn == 0 && fIconRow == 0) ||
        !(inrange(fIconRow, mrow, Mrow - height) &&
          inrange(fIconColumn, mcol, Mcol - width)))
    {
        fIconRow = srow;
        fIconColumn = scol;
    }

    /* Return values */
    *iconRow = fIconRow + margin;
    *iconCol = fIconColumn + margin;

    /* Set row and column to new position */
    fIconColumn += width * dcol;

    int w2 = width / 2;
    if (fIconColumn >= Mcol - w2 || fIconColumn < mcol - w2) {
        fIconRow += height * drow;
        fIconColumn = scol;
        int h2 = height / 2;
        if (fIconRow >= Mrow - h2 || fIconRow < mrow - h2)
            fIconColumn = fIconRow = 0;
    }
}

int YWindowManager::windowCount(long workspace) {
    int count = 0;

    for (int layer = 0 ; layer < WinLayerCount; layer++) {
        for (YFrameWindow *frame = top(layer); frame; frame = frame->next()) {
            if (!frame->visibleOn(workspace))
                continue;
            if (frame->frameOption(YFrameWindow::foIgnoreWinList))
                continue;
            if (workspace != activeWorkspace() &&
                frame->visibleNow())
                continue;
            count++;
        }
    }
    return count;
}

void YWindowManager::resetColormap(bool active) {
    if (active) {
        if (colormapWindow() && colormapWindow()->client())
            installColormap(colormapWindow()->client()->colormap());
    } else {
        installColormap(None);
    }
}

void YWindowManager::handleProperty(const XPropertyEvent &property) {
    if (property.atom == _XA_ICEWM_HINT) {
        YProperty prop(this, _XA_ICEWM_HINT, F8, 8192, _XA_ICEWM_HINT, True);
        if (prop) {
            unsigned long nitems = prop.size();
            unsigned char* propdata = prop.data<unsigned char>();
            for (unsigned i = 0; i + 3 < nitems; ) {
                const char* s[3] = { nullptr, nullptr, nullptr, };
                for (int k = 0; k < 3 && i < nitems; ++k) {
                    s[k] = i + (const char *) propdata;
                    while (i < nitems && propdata[i++]);
                }
                if (s[0] && *s[0] && s[1] && *s[1] && s[2] && *s[2] &&
                    propdata[i - 1] == 0) {
                    hintOptions->setWinOption(s[0], s[1], s[2]);
                }
            }
        }
    }
    else if (property.atom == _XA_NET_DESKTOP_NAMES) {
        MSG(("change: net desktop names"));
        readDesktopNames(false, true);
    }
    else if (property.atom == _XA_NET_DESKTOP_LAYOUT) {
        readDesktopLayout();
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
                    for (YFrameClient* cli : frame->clients()) {
                        if (cli->adopted())
                            ids.append(cli->handle());
                    }
                }
            }
        }

        const int num = ids.getCount();
        Atom* data = num ? &*ids : nullptr;
        setProperty(_XA_NET_CLIENT_LIST_STACKING, XA_WINDOW, data, num);
    }

    if (fCreatedUpdated) {
        fCreatedUpdated = false;

        ids.shrink(0);
        for (YFrameIter frame = fCreationOrder.iterator(); ++frame; ) {
            for (YFrameClient* cli : frame->clients()) {
                if (cli->adopted())
                    ids.append(cli->handle());
            }
        }

        const int num = ids.getCount();
        Atom* data = num ? &*ids : nullptr;
        setProperty(_XA_NET_CLIENT_LIST, XA_WINDOW, data, num);
    }
    checkLogout();
}

void YWindowManager::updateUserTime(const UserTime& userTime) {
    if (userTime.good() && fLastUserTime < userTime)
        fLastUserTime = userTime;
}

void YWindowManager::checkLogout() {
    if (fExitWhenDone && !haveClients()) {
        fExitWhenDone = false; /* Only run the command once */

        if (rebootOrShutdown == Reboot && nonempty(rebootCommand)) {
            msg("reboot... (%s)", rebootCommand);
            smActionListener->runCommand(rebootCommand);
        } else if (rebootOrShutdown == Shutdown && nonempty(shutdownCommand)) {
            msg("shutdown ... (%s)", shutdownCommand);
            smActionListener->runCommand(shutdownCommand);
        } else if (rebootOrShutdown == Logout)
            app->exit(0);
    }
}

void YWindowManager::removeClientFrame(YFrameWindow *frame) {
    if (fArrangeInfo) {
        for (int i = 0; i < fArrangeCount; i++)
            if (fArrangeInfo[i].frame == frame)
                fArrangeInfo[i].frame = nullptr;
    }
    for (int w = 0; w < workspaceCount; w++) {
        if (workspaces[w].focused == frame) {
            workspaces[w].focused = nullptr;
        }
    }
    if (wmState() == wmRUNNING) {
        if (frame == getFocus())
            focusLastWindow();
        if (frame == getFocus())
            setFocus(nullptr);
        if (colormapWindow() == frame)
            setColormapWindow(getFocus());
        if (frame->affectsWorkArea())
            updateWorkArea();
        if (fSwitchWindow)
            fSwitchWindow->destroyedFrame(frame);
    }
}

void YWindowManager::notifyActive(YFrameWindow *frame) {
    Window win = frame ? frame->client()->handle() : None;
    if (win != fActiveWindow && (win || wmState() == wmRUNNING)) {
        setProperty(_XA_NET_ACTIVE_WINDOW, XA_WINDOW, win);
        fActiveWindow = win;
    }
}

void YWindowManager::switchFocusTo(YFrameWindow *frame, bool reorderFocus) {

    if (frame != fFocusWin) {
        if (fFocusWin) {
            YFrameWindow* f = fFocusWin;
            fFocusWin = nullptr;
            f->loseWinFocus();
        }
        if (frame) {
            fFocusWin = frame;
            fFocusWin->setWinFocus();
        }

        workspaces[activeWorkspace()].focused = frame;
    }
    if (frame && reorderFocus) {
        raiseFocusFrame(frame);
    }
    notifyActive(frame);
    updateClientList();
}

void YWindowManager::switchFocusFrom(YFrameWindow *frame) {
    if (fFocusWin == frame && frame) {
        fFocusWin = nullptr;
        frame->loseWinFocus();
    }
}

void YWindowManager::popupWindowListMenu(YWindow *owner, int x, int y) {
    windowListMenu->popup(owner, nullptr, nullptr, x, y,
                          YPopupWindow::pfCanFlipVertical |
                          YPopupWindow::pfCanFlipHorizontal |
                          YPopupWindow::pfPopupMenu);
}

void YWindowManager::popupStartMenu(YWindow *owner) { // !! fix
    if (showTaskBar && taskBar && taskBarShowStartMenu)
        taskBar->popupStartMenu();
    else
        rootMenu->popup(owner, nullptr, nullptr, 0, 0,
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

void YWindowManager::switchToWorkspace(int nw, bool takeCurrent) {
    if (inrange(nw, 0, workspaceCount - 1) && nw != activeWorkspace()) {
        YFrameWindow* frame = takeCurrent ? getFocus() : nullptr;
        if (frame == nullptr || frame->isAllWorkspaces()) {
            activateWorkspace(nw);
        }
        else {
            workspaces[nw].focused = frame;
            lockWorkArea();
            lockFocus();
            frame->setAllWorkspaces();
            frame->wmRaise();
            activateWorkspace(nw);
            frame->wmOccupyWorkspace(nw);
            unlockFocus();
            frame->wmRaise();
            if (getFocus() != frame) {
                setFocus(frame);
            }
            unlockWorkArea();
        }
        if (taskBar && lastWorkspace() != activeWorkspace()) {
            taskBar->workspacesRepaint(lastWorkspace());
            taskBar->workspacesRepaint(activeWorkspace());
        }
    }
}

void YWindowManager::switchToPrevWorkspace(bool takeCurrent) {
    int nw = (activeWorkspace() + workspaceCount - 1) % workspaceCount;

    switchToWorkspace(nw, takeCurrent);
}

void YWindowManager::switchToNextWorkspace(bool takeCurrent) {
    int nw = (activeWorkspace() + 1) % workspaceCount;

    switchToWorkspace(nw, takeCurrent);
}

void YWindowManager::switchToLastWorkspace(bool takeCurrent) {
    switchToWorkspace(lastWorkspace(), takeCurrent);
}

void YWindowManager::tilePlace(YFrameWindow *w, int tx, int ty, int tw, int th) {
    long mask = WinStateUnmapped | WinStateMaximizedBoth;
    if (w->hasState(mask)) {
        w->setState(mask, 0);
    }
    int bx = w->borderXN();
    int bb = w->borderYN();
    int bt = bb + w->titleYN();
    int vo = min(bb, int(topSideVerticalOffset));
    int cw = tw - bx - bx;
    int ch = th - bb - (bt - vo);
    w->client()->constrainSize(cw, ch, None);
    int ow = cw + bx + bx;
    int oh = ch + bb + bt;
    w->setNormalGeometryOuter(tx, ty - vo, ow, oh);
}

void YWindowManager::tileWindows(YArrange arrange, bool vertical) {
    if (saveArrange(arrange) == false)
        return;

    int curWin = 0;
    int cols = 1;

    while (cols * cols <= arrange.size())
        cols++;
    cols--;

    int areaX, areaY, areaW, areaH;

    int mx, my, Mx, My;
    getWorkArea(arrange[0], &mx, &my, &Mx, &My);

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

    int normalRows = arrange.size() / cols;
    int normalWidth = areaW / cols;
    int windowX = areaX;

    for (int col = 0; col < cols; col++) {
        int rows = normalRows;
        int windowWidth = normalWidth;
        int windowY = areaY;

        if (col >= (cols * (1 + normalRows) - arrange.size()))
            rows++;
        if (col >= (cols * (1 + normalWidth) - areaW))
            windowWidth++;

        int normalHeight = areaH / max(1, rows);

        for (int row = 0; row < rows; row++) {
            int windowHeight = normalHeight;

            if (row >= (rows * (1 + normalHeight) - areaH))
                windowHeight++;

            if (vertical) // swap meaning of rows/cols
                tilePlace(arrange[curWin++],
                          windowY, windowX, windowHeight, windowWidth);
            else
                tilePlace(arrange[curWin++],
                          windowX, windowY, windowWidth, windowHeight);

            windowY += windowHeight;
        }
        windowX += windowWidth;
    }
}

YArrange YWindowManager::getWindowsToArrange(bool all, bool mini, bool full)
{
    int capacity = focusedCount() + 1;
    int indexarr = 0;
    YFrameWindow** arrange = new YFrameWindow *[capacity];
    if (capacity && arrange) {
        for (YFrameWindow *w = topLayer(WinLayerOnTop);
             w && w->getActiveLayer() >= WinLayerBelow;
             w = w->nextLayer()) {
            if (w->owner() == nullptr && // not transient ?
                w->visibleNow() && // visible
                (all || !w->isAllWorkspaces()) && // not on all workspaces
                !w->isUnmapped() &&
                ( ! mini || w->canMinimize()) &&
                indexarr + 1 < capacity)
            {
                arrange[indexarr] = w;
                indexarr++;
            }
        }
        if (full && getFocus() && getFocus()->isFullscreen() &&
            getFocus()->getActiveLayer() == WinLayerFullscreen &&
            getFocus()->getRequestedLayer() <= WinLayerOnTop) {
            YFrameWindow* w = getFocus();
            if (w->owner() == nullptr && // not transient ?
                w->visibleNow() && // visible
                (all || !w->isAllWorkspaces()) && // not on all workspaces
                !w->isUnmapped() &&
                ( ! mini || w->canMinimize()) &&
                indexarr + 1 < capacity)
            {
                arrange[indexarr] = w;
                indexarr++;
            }
        }
        arrange[indexarr] = nullptr;
    }
    if (indexarr == 0) {
        delete[] arrange;
        arrange = nullptr;
    }
    return YArrange(arrange, indexarr);
}

bool YWindowManager::saveArrange(YArrange arrange) {
    if (arrange.size() != fArrangeCount || fArrangeInfo == nullptr) {
        delete[] fArrangeInfo;
        fArrangeCount = arrange.size();
        fArrangeInfo = new WindowPosState[fArrangeCount];
    }
    if (fArrangeInfo) {
        WindowPosState* info = fArrangeInfo;
        for (YFrameWindow* frame : arrange) {
            info->x = frame->x();
            info->y = frame->y();
            info->w = frame->width();
            info->h = frame->height();
            info->state = frame->getState();
            info->frame = frame;
            info++;
        }
    }
    setShowingDesktop(false);
    return 0 < fArrangeCount;
}

void YWindowManager::undoArrange() {
    if (fArrangeInfo && 0 < fArrangeCount) {
        lockFocus();
        for (int i = 0; i < fArrangeCount; i++) {
            WindowPosState info(fArrangeInfo[i]);
            YFrameWindow* f = info.frame;
            if (f && (f->getState() & WIN_STATE_ALL) != info.state) {
                f->setState(WIN_STATE_ALL, info.state);
            }
            if (f && f->isFullscreen()) {
                f->updateDerivedSize(f->getState() & WinStateMaximizedBoth);
            } else if (f) {
                f->setNormalGeometryOuter(info.x, info.y, info.w, info.h);
            }
        }
        delete [] fArrangeInfo; fArrangeInfo = nullptr;
        fArrangeCount = 0;
        unlockFocus();
        focusTopWindow();
    }
    setShowingDesktop(false);
}

void YWindowManager::tileWindows(bool vertical) {
    YArrange arrange = getWindowsToArrange();
    if (arrange) {
        tileWindows(arrange, vertical);
        arrange.discard();
    }
}

void YWindowManager::arrangeWindows() {
    YArrange arrange = getWindowsToArrange();
    if (arrange) {
        smartPlace(arrange);
        arrange.discard();
    }
}

void YWindowManager::actionWindows(YAction action) {
    YArrange arrange = getWindowsToArrange();
    if (arrange) {
        setWindows(arrange, action);
        arrange.discard();
    }
}

void YWindowManager::toggleDesktop() {
    YArrange arrange = getWindowsToArrange(true, true, true);
    if (arrange) {
        setWindows(arrange, actionMinimizeAll);
        setShowingDesktop(true);
        arrange.discard();
    } else {
        undoArrange();
        setShowingDesktop(false);
    }
}

void YWindowManager::cascadeWindows() {
    YArrange arrange = getWindowsToArrange(true, true);
    if (arrange) {
        cascadePlace(arrange);
        arrange.discard();
    }
}

bool YWindowManager::haveClients() {
    for (YFrameWindow * f(topLayer()); f ; f = f->nextLayer())
        if (f->canClose() && f->client()->adopted())
            return true;

    return false;
}

void YWindowManager::exitAfterLastClient(bool exitWhenDone) {
    fExitWhenDone = exitWhenDone;
    checkLogout();
}

void YWindowManager::updateKeyboard(int configIndex) {
    setKeyboard(configIndex);
}

void YWindowManager::reflectKeyboard(int configIndex, mstring keyboard) {
    fDefaultKeyboard = configIndex;
    fCurrentKeyboard = keyboard;
}

void YWindowManager::setKeyboard(int configIndex) {
    if (inrange(configIndex, 0, configKeyboards.getCount() - 1)) {
        fDefaultKeyboard = configIndex;
        setKeyboard(configKeyboards[configIndex]);
    }
}

void YWindowManager::setKeyboard(mstring keyboard) {
    if (keyboard != null && keyboard != fCurrentKeyboard) {
        fCurrentKeyboard = keyboard;
        auto program = "setxkbmap";
        csmart path(path_lookup(program));
        if (path) {
            wordexp_t exp = {};
            exp.we_offs = 1;
            if (wordexp(keyboard, &exp, WRDE_NOCMD | WRDE_DOOFFS) == 0) {
                exp.we_wordv[0] = strdup(program);
                wmapp->runProgram(program, exp.we_wordv);
                wordfree(&exp);
            }
            if (taskBar) {
                taskBar->keyboardUpdate(keyboard);
            }
        }
        else if (ONCE) {
            new YMsgBox(YMsgBox::mbOK,
                        _("Missing program setxkbmap"),
                        _("For keyboard switching, please install setxkbmap."),
                        this, "key");
        }
    }
}

ClientData* YWindowManager::getFocused() const {
    return getFocus();
}

mstring YWindowManager::getKeyboard() {
    return fCurrentKeyboard;
}

void YWindowManager::kbLayout() {
    fLayoutTimer->setTimer(100L, this, true);
}

void YWindowManager::handleMsgBox(YMsgBox *msgbox, int operation) {
    delete msgbox;
}

int YWindowManager::edgeWorkspace(int x, int y) {
    for (auto edge : edges)
        if (edge->visible() && edge->geometry().contains(x, y))
            return edge->destination();
    return -1;
}

EdgeSwitch::EdgeSwitch(YWindowManager *manager, int delta, bool vertical):
    YDndWindow(desktop),
    fManager(manager),
    fCursor(delta < 0 ? vertical ? YWMApp::scrollUpPointer
                                 : YWMApp::scrollLeftPointer
                      : vertical ? YWMApp::scrollDownPointer
                                 : YWMApp::scrollRightPointer),
    fDelta(delta),
    fVert(vertical)
{
    setStyle(wsOverrideRedirect | wsInputOnly);
    setPointer(YWMApp::leftPointer);
    setGeometry();
    setTitle("IceEdgeSwitch");
    setDND(true);
    show();
}

EdgeSwitch::~EdgeSwitch() {
}

void EdgeSwitch::setGeometry() {
    int x = (!fVert && 0 < fDelta) ? int(desktop->width() - 1) : 0;
    int y = (fVert && 0 < fDelta) ? int(desktop->height() - 1) : 0;
    unsigned w = !fVert ? 1 : desktop->width();
    unsigned h = fVert ? 1 : desktop->height();

    YWindow::setGeometry(YRect(x, y, w, h));
}

void EdgeSwitch::handleCrossing(const XCrossingEvent &crossing) {
    if (crossing.type == EnterNotify && crossing.mode == NotifyNormal) {
        fEdgeSwitchTimer->setTimer(edgeSwitchDelay, this, true);
        setPointer(fCursor);
    } else if (crossing.type == LeaveNotify && crossing.mode == NotifyNormal) {
        if (fEdgeSwitchTimer && fEdgeSwitchTimer->getTimerListener() == this) {
            fEdgeSwitchTimer = null;
            setPointer(YWMApp::leftPointer);
        }
    }
}

void EdgeSwitch::handleDNDEnter() {
    fEdgeSwitchTimer->setTimer(edgeSwitchDelay, this, true);
}

void EdgeSwitch::handleDNDLeave() {
    if (fEdgeSwitchTimer && fEdgeSwitchTimer->getTimerListener() == this) {
        fEdgeSwitchTimer = null;
    }
}

int EdgeSwitch::destination() {
    int worksps = workspaceCount;
    int orient  = fManager->layout().orient;
    int columns = min(fManager->layout().columns, worksps);
    int rows    = min(fManager->layout().rows, worksps);
    int corner  = fManager->layout().corner;

    if (rows == 0)
        rows = (worksps + (columns - 1)) / non_zero(columns);
    if (columns == 0)
        columns = (worksps + (rows - 1)) / non_zero(rows);
    if (orient == _NET_WM_ORIENTATION_VERT) {
        columns = (worksps + (rows - 1)) / non_zero(rows);
    } else {
        rows = (worksps + (columns - 1)) / non_zero(columns);
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

    int col = fManager->activeWorkspace() % non_zero(columns);
    int row = fManager->activeWorkspace() / non_zero(columns);

    if (dx == 0 && rows == 1) {
        swap(dx, dy);
    }

    int wsp, end(100);
    do {
        if (0 == --end) {
            tlog("inflp");
            wsp = -1;
            break;
        }

        col = (col + dx + columns) % columns;
        row = (row + dy + rows) % rows;
        wsp = col + row * columns;
    } while (wsp >= worksps);

    return wsp;
}

bool EdgeSwitch::handleTimer(YTimer *t) {
    if (t == fEdgeSwitchTimer) {
        int ws = destination();
        if (0 <= ws) {
            fManager->switchToWorkspace(ws, false);
            if (warpPointerOnEdgeSwitch) {
                int dx = -fDelta * !fVert * (desktop->width() - 5);
                int dy = -fDelta * fVert * (desktop->height() - 5);
                XWarpPointer(xapp->display(), None, None, 0, 0, 0, 0, dx, dy);
            }
            if (edgeContWorkspaceSwitching)
                return true;
        }
        setPointer(YWMApp::leftPointer);
    }
    return false;
}

int YWindowManager::getSwitchScreen() {
    int s = fFocusWin ? fFocusWin->getScreen() : xineramaPrimaryScreen;
    return inrange(s, 0, getScreenCount() - 1) ? s : 0;
}

void YWindowManager::doWMAction(WMAction action, bool putback) {
    XClientMessageEvent xev;
    memset(&xev, 0, sizeof(xev));

    xev.type = ClientMessage;
    xev.window = xapp->root();
    xev.message_type = _XA_ICEWM_ACTION;
    xev.format = 32;
    xev.data.l[0] = CurrentTime;
    xev.data.l[1] = action;

    if (putback) {
        XEvent event;
        event.xclient = xev;
        XPutBackEvent(xapp->display(), &event);
    }
    else {
        xapp->send(xev, xapp->root(), SubstructureNotifyMask);
    }
}

#ifdef CONFIG_XRANDR
void YWindowManager::handleRRScreenChangeNotify(const XRRScreenChangeNotifyEvent &xrrsc) {
    // logRandrScreen((const union _XEvent&) xrrsc);
    updateScreenSize((XEvent *)&xrrsc);
}

void YWindowManager::handleRRNotify(const XRRNotifyEvent &notify) {
    // logRandrNotify((const union _XEvent&) notify);
}

void YWindowManager::updateScreenSize(XEvent *event) {
    unsigned nw = width();
    unsigned nh = height();

    XRRUpdateConfiguration(event);

    if (updateXineramaInfo(nw, nh)) {
        MSG(("xrandr: %d %d", nw, nh));
        Atom data[2] = { nw, nh };
        setProperty(_XA_NET_DESKTOP_GEOMETRY, XA_CARDINAL, data, 2);
        setSize(nw, nh);
        updateWorkArea();
        if (taskBar && pagerShowPreview) {
            taskBar->workspacesUpdateButtons();
        }
        if (taskBar) {
            taskBar->relayout();
            taskBar->relayoutNow();
        }
        for (int i = 0; i < edges.getCount(); ++i)
            edges[i]->setGeometry();

        /// TODO #warning "make something better"
        if (arrangeWindowsOnScreenSizeChange) {
            wmActionListener->actionPerformed(actionArrange, 0);
        }
    }

    refresh();
}
#endif

void YWindowManager::refresh() {
    if (taskBar) {
        taskBar->refresh();
    }
    for (YFrameIter frame(focusedIterator()); ++frame; ) {
        if (frame->visibleNow()) {
            frame->refresh();
        }
    }
}

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
    if (frame->YFocusedNode::nodePrev()) {
        fFocusedOrder.remove(frame);
        fFocusedOrder.prepend(frame);
    }
}

void YWindowManager::raiseFocusFrame(YFrameWindow* frame) {
    if (frame->YFocusedNode::nodeNext()) {
        fFocusedOrder.remove(frame);
        fFocusedOrder.append(frame);
    }
}

YTopWindow::YTopWindow() : YWindow(), fFrame(nullptr), fHandle(None) {
}

void YTopWindow::setFrame(YFrameWindow* frame) {
    fFrame = frame;
    fHandle = frame ? frame->handle() : None;
}

bool YTopWindow::handleKey(const XKeyEvent& key) {
    if (key.type == KeyPress &&
        manager->netActiveWindow() == None &&
        fHandle && fFrame && fFrame->visible() &&
        windowContext.find(fHandle) == fFrame &&
        xapp->AltMask)
    {
        YFrameWindow* f = fFrame;
        KeySym k = keyCodeToKeySym(key.keycode);
        unsigned m = KEY_MODMASK(key.state);
        unsigned vm = VMod(m);

        if (gKeyWinClose.eq(k, vm)) {
            f->actionPerformed(actionClose);
        }
        else if (gKeyWinPrev.eq(k, vm)) {
            f->wmPrevWindow();
        }
        else if (gKeyWinMaximizeVert.eq(k, vm)) {
            f->actionPerformed(actionMaximizeVert);
        }
        else if (gKeyWinMaximizeHoriz.eq(k, vm)) {
            f->actionPerformed(actionMaximizeHoriz);
        }
        else if (gKeyWinRaise.eq(k, vm)) {
            f->actionPerformed(actionRaise);
        }
        else if (gKeyWinOccupyAll.eq(k, vm)) {
            f->actionPerformed(actionOccupyAllOrCurrent);
        }
        else if (gKeyWinLower.eq(k, vm)) {
            f->actionPerformed(actionLower);
        }
        else if (gKeyWinRestore.eq(k, vm)) {
            f->actionPerformed(actionRestore);
        }
        else if (gKeyWinNext.eq(k, vm)) {
            f->wmNextWindow();
        }
        else if (gKeyWinMove.eq(k, vm)) {
            f->actionPerformed(actionMove);
        }
        else if (gKeyWinSize.eq(k, vm)) {
            f->actionPerformed(actionSize);
        }
        else if (gKeyWinMinimize.eq(k, vm)) {
            f->actionPerformed(actionMinimize);
        }
        else if (gKeyWinMaximize.eq(k, vm)) {
            f->actionPerformed(actionMaximize);
        }
        else if (gKeyWinHide.eq(k, vm)) {
            f->actionPerformed(actionHide);
        }
        else if (gKeyWinRollup.eq(k, vm)) {
            f->actionPerformed(actionRollup);
        }
        else if (gKeyWinFullscreen.eq(k, vm)) {
            f->actionPerformed(actionFullscreen);
        }
        else if (gKeyWinMenu.eq(k, vm)) {
            f->popupSystemMenu(this);
        }
        else if (f->isIconic() || f->isRollup()) {
            if ((k == XK_Menu) || (k == XK_F10 && m == ShiftMask)) {
                f->popupSystemMenu(this);
            }
        }
    }
    return true;
}

// vim: set sw=4 ts=4 et:
