/*
 * IceWM
 *
 * Copyright (C) 1997-2003 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "wmmgr.h"

#include "aaddressbar.h"
#include "atasks.h"
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
#include "yrect.h"

XContext frameContext;
XContext clientContext;

YAction *layerActionSet[WinLayerCount];

#ifdef CONFIG_TRAY
YAction *trayOptionActionSet[WinTrayOptionCount];
#endif

YWindowManager::YWindowManager(YWindow *parent, Window win):
    YDesktop(parent, win)
{
    wmState = wmSTARTUP;
    fShuttingDown = false;
    fOtherScreenFocused = false;
    fFocusWin = 0;
    for (int l(0); l < WinLayerCount; l++) {
        layerActionSet[l] = new YAction();
        fTop[l] = fBottom[l] = 0;
    }
    fFirst = fLast = 0;
#ifdef CONFIG_TRAY
    for (int k(0); k < WinTrayOptionCount; k++)
        trayOptionActionSet[k] = new YAction();
#endif
    fColormapWindow = 0;
    fActiveWorkspace = WinWorkspaceInvalid;
    fLastWorkspace = WinWorkspaceInvalid;
    fArrangeCount = 0;
    fArrangeInfo = 0;
    fWorkAreaMoveWindows = false;
    fWorkArea = 0;
    fWorkAreaCount = 0;
#if 0
    fMinX = 0;
    fMinY = 0;
    fMaxX = width();
    fMaxY = height();
#endif

    frameContext = XUniqueContext();
    clientContext = XUniqueContext();

    setStyle(wsManager);
    setPointer(YApplication::leftPointer);

    fTopWin = new YWindow();;
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
    XSync(app->display(), False);

    YWindow::setWindowFocus();
}

YWindowManager::~YWindowManager() {
}

void YWindowManager::grabKeys() {
#ifdef CONFIG_ADDRESSBAR
    ///if (taskBar && taskBar->addressBar())
        GRAB_WMKEY(gKeySysAddressBar);
#endif
    if (runDlgCommand && runDlgCommand[0])
        GRAB_WMKEY(gKeySysRun);
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

    {
        KProgram *k = keyProgs;
        while (k) {
            grabVKey(k->key(), k->modifiers());
            k = k->getNext();
        }
    }
    if (app->WinMask) {
        //fix -- allow apps to use remaining key combos (except single press)
        if (app->Win_L) grabKey(app->Win_L, 0);
        if (app->Win_R) grabKey(app->Win_R, 0);
    }

    if (useMouseWheel) {
        grabButton(4, ControlMask | app->AltMask);
        grabButton(5, ControlMask | app->AltMask);
        if (app->WinMask) {
            grabButton(4, app->WinMask);
            grabButton(5, app->WinMask);
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

            XChangeProperty(app->display(), manager->handle(),
                            _XA_WIN_DESKTOP_BUTTON_PROXY, XA_CARDINAL, 32,
                            PropModeReplace, (unsigned char *)&rid, 1);
            XChangeProperty(app->display(), rootProxy->handle(),
                            _XA_WIN_DESKTOP_BUTTON_PROXY, XA_CARDINAL, 32,
                            PropModeReplace, (unsigned char *)&rid, 1);
        }
    }
}

bool YWindowManager::handleKey(const XKeyEvent &key) {
    YFrameWindow *frame = getFocus();

    if (key.type == KeyPress) {
        KeySym k = XKeycodeToKeysym(app->display(), key.keycode, 0);
        unsigned int m = KEY_MODMASK(key.state);
        unsigned int vm = VMod(m);

        MSG(("down key: %d, mod: %d", k, m));

#ifndef LITE
        if (quickSwitch && switchWindow) {
            if (IS_WMKEY(k, vm, gKeySysSwitchNext)) {
                switchWindow->begin(1, key.state);
            } else if (IS_WMKEY(k, vm, gKeySysSwitchLast)) {
                switchWindow->begin(0, key.state);
            }
        }
#endif
        if (IS_WMKEY(k, vm, gKeySysWinNext)) {
            if (frame) frame->wmNextWindow();
        } else if (IS_WMKEY(k, vm, gKeySysWinPrev)) {
            if (frame) frame->wmPrevWindow();
        } else if (IS_WMKEY(k, vm, gKeySysWinMenu)) {
            if (frame) frame->popupSystemMenu();
#ifndef LITE
        } else if (IS_WMKEY(k, vm, gKeySysDialog)) {
            if (ctrlAltDelete) ctrlAltDelete->activate();
#endif
#ifdef CONFIG_WINMENU
        } else if (IS_WMKEY(k, vm, gKeySysWinListMenu)) {
            popupWindowListMenu();
#endif
        } else if (IS_WMKEY(k, vm, gKeySysMenu)) {
            popupStartMenu();
#ifdef CONFIG_WINLIST
        } else if (IS_WMKEY(k, vm, gKeySysWindowList)) {
            if (windowList) windowList->showFocused(-1, -1);
#endif
        } else if (IS_WMKEY(k, vm, gKeySysWorkspacePrev)) {
            XUngrabKeyboard(app->display(), CurrentTime);
            switchToPrevWorkspace(false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspaceNext)) {
            XUngrabKeyboard(app->display(), CurrentTime);
            switchToNextWorkspace(false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspaceLast)) {
            XUngrabKeyboard(app->display(), CurrentTime);
            switchToLastWorkspace(false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspacePrevTakeWin)) {
            switchToPrevWorkspace(true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspaceNextTakeWin)) {
            switchToNextWorkspace(true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspaceLastTakeWin)) {
            switchToLastWorkspace(true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace1)) {
            switchToWorkspace(0, false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace2)) {
            switchToWorkspace(1, false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace3)) {
            switchToWorkspace(2, false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace4)) {
            switchToWorkspace(3, false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace5)) {
            switchToWorkspace(4, false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace6)) {
            switchToWorkspace(5, false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace7)) {
            switchToWorkspace(6, false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace8)) {
            switchToWorkspace(7, false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace9)) {
            switchToWorkspace(8, false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace10)) {
            switchToWorkspace(9, false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace11)) {
            switchToWorkspace(10, false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace12)) {
            switchToWorkspace(11, false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace1TakeWin)) {
            switchToWorkspace(0, true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace2TakeWin)) {
            switchToWorkspace(1, true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace3TakeWin)) {
            switchToWorkspace(2, true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace4TakeWin)) {
            switchToWorkspace(3, true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace5TakeWin)) {
            switchToWorkspace(4, true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace6TakeWin)) {
            switchToWorkspace(5, true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace7TakeWin)) {
            switchToWorkspace(6, true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace8TakeWin)) {
            switchToWorkspace(7, true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace9TakeWin)) {
            switchToWorkspace(8, true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace10TakeWin)) {
            switchToWorkspace(9, true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace11TakeWin)) {
            switchToWorkspace(10, true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace12TakeWin)) {
            switchToWorkspace(11, true);
	} else if(IS_WMKEY(k, vm, gKeySysTileVertical)) {
	    wmapp->actionPerformed(actionTileVertical, 0);
	} else if(IS_WMKEY(k, vm, gKeySysTileHorizontal)) {
	    wmapp->actionPerformed(actionTileHorizontal, 0);
	} else if(IS_WMKEY(k, vm, gKeySysCascade)) {
	    wmapp->actionPerformed(actionCascade, 0);
	} else if(IS_WMKEY(k, vm, gKeySysArrange)) {
	    wmapp->actionPerformed(actionArrange, 0);
	} else if(IS_WMKEY(k, vm, gKeySysUndoArrange)) {
	    wmapp->actionPerformed(actionUndoArrange, 0);
	} else if(IS_WMKEY(k, vm, gKeySysArrangeIcons)) {
	    wmapp->actionPerformed(actionArrangeIcons, 0);
	} else if(IS_WMKEY(k, vm, gKeySysMinimizeAll)) {
	    wmapp->actionPerformed(actionMinimizeAll, 0);
	} else if(IS_WMKEY(k, vm, gKeySysHideAll)) {
	    wmapp->actionPerformed(actionHideAll, 0);
#ifdef CONFIG_ADDRESSBAR
        } else if (IS_WMKEY(k, vm, gKeySysAddressBar)) {
            if (taskBar) {
                taskBar->popOut();
                if (taskBar->addressBar()) {
                    taskBar->addressBar()->showNow();
		}
            }
#endif
        } else if (IS_WMKEY(k, vm, gKeySysRun)) {
            if (runDlgCommand && runDlgCommand[0])
                app->runCommand(runDlgCommand);
        } else {
            KProgram *p = keyProgs;
            while (p) {
                //msg("%X=%X %X=%X", k, p->key(), vm, p->modifiers());
                if (p->isKey(k, vm))
                    p->open();
                p = p->getNext();
            }
        }

        if (app->WinMask) {
            if (k == app->Win_L || k == app->Win_R) {
                /// !!! needs sync grab
                XAllowEvents(app->display(), ReplayKeyboard, CurrentTime);
            } else if (m & app->WinMask) {
                /// !!! needs sync grab
                XAllowEvents(app->display(), ReplayKeyboard, CurrentTime);
            }
        }
    } else if (key.type == KeyRelease) {
#ifdef DEBUG
        KeySym k = XKeycodeToKeysym(app->display(), key.keycode, 0);
        unsigned int m = KEY_MODMASK(key.state);

        MSG(("up key: %d, mod: %d", k, m));
#endif
    }
    return true;
}

void YWindowManager::handleButton(const XButtonEvent &button) {
    if (rootProxy && button.window == handle() &&
        !(useRootButtons & (1 << (button.button - 1))) &&
       !((button.state & (ControlMask + app->AltMask)) == ControlMask + app->AltMask))
    {
        if (button.send_event == False) {
            XUngrabPointer(app->display(), CurrentTime);
            XSendEvent(app->display(),
                       rootProxy->handle(),
                       False,
                       SubstructureNotifyMask,
                       (XEvent *) &button);
        }
        return ;
    }
    YFrameWindow *frame = 0;
    if (useMouseWheel && ((frame = getFocus()) != 0) && button.type == ButtonPress &&
        ((KEY_MODMASK(button.state) == app->WinMask && app->WinMask) ||
         (KEY_MODMASK(button.state) == ControlMask + app->AltMask && app->AltMask)))
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
                rootMenu->popup(0, 0, button.x, button.y, -1, -1,
                                YPopupWindow::pfCanFlipVertical |
                                YPopupWindow::pfCanFlipHorizontal |
                                YPopupWindow::pfPopupMenu |
                                YPopupWindow::pfButtonDown);
            break;
        }
#endif
#ifdef CONFIG_WINMENU
        if (button.button + 10 == (unsigned) rootWinMenuButton) {
            popupWindowListMenu(button.x, button.y);
            break;
        }
#endif
#if 0
#ifdef CONFIG_WINLIST
        if (button.button + 10 == rootWinListButton) {
            if (windowList)
                windowList->showFocused(button.x_root, button.y_root);
            break;
        }
#endif
#endif
    } while (0);
    YWindow::handleButton(button);
}

void YWindowManager::handleClick(const XButtonEvent &up, int count) {
    if (count == 1) do {
#ifndef NO_CONFIGURE_MENUS
        if (up.button == (unsigned) rootMenuButton) {
            if (rootMenu)
                rootMenu->popup(0, 0, up.x, up.y, -1, -1,
                                YPopupWindow::pfCanFlipVertical |
                                YPopupWindow::pfCanFlipHorizontal |
                                YPopupWindow::pfPopupMenu);
            break;
        }
#endif
#ifdef CONFIG_WINMENU
        if (up.button == (unsigned) rootWinMenuButton) {
            popupWindowListMenu(up.x, up.y);
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
        XConfigureWindow(app->display(), configureRequest.window,
                         configureRequest.value_mask, &xwc);
#if 0
        {
            Window rootw;
            unsigned int bw, depth;
            int fX, fY;
            unsigned int fWidth, fHeight;

            XGetGeometry(app->display(), configureRequest.window, &rootw,
                         &fX, &fY, &fWidth, &fHeight, &bw, &depth);
            /*fX = attributes.x;
             fY = attributes.y;
             fWidth = attributes.width;
             fHeight = attributes.height;*/

            MSG(("changed geometry (%d:%d %dx%d)", fX, fY, fWidth, fHeight));
        }
#endif
    }
}

void YWindowManager::handleMapRequest(const XMapRequestEvent &mapRequest) {
    mapClient(mapRequest.window);
}

void YWindowManager::handleUnmap(const XUnmapEvent &unmap) {
    if (unmap.send_event)
        unmanageClient(unmap.window, false);
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

    XQueryTree(app->display(), root, &root, &parent, &clients, &nClients);

    if (clients) {
	unsigned n;

	for (n = 0; !firstMatch && n < nClients; ++n) {
	    XClassHint wmclass;

	    if (XGetClassHint(app->display(), clients[n], &wmclass)) {
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

    if (XFindContext(app->display(), win,
                     frameContext, (XPointer *)&frame) == 0)
        return frame;
    else
        return 0;
}

YFrameClient *YWindowManager::findClient(Window win) {
    YFrameClient *client;

    if (XFindContext(app->display(), win,
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
    YFrameClient *c = f ? f->client() : 0;
    Window w = desktop->handle();

    MSG(("SET FOCUS f=%lX", f));

    if (f == 0) {
        YFrameWindow *ff = getFocus();
        if (ff) switchFocusFrom(ff);
    }

    bool input = true;
    XWMHints *hints = c ? c->hints() : 0;

    if (hints && (hints->flags & InputHint) && !hints->input)
        input = false;

    if (f && f->visible()) {
        if (c && c->visible() && !(f->isRollup() || f->isIconic()))
            w = c->handle();
        else
            w = f->handle();

        if (input)
            switchFocusTo(f);
        f->setWmUrgency(false);
    }
#if 0
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

    if (input || w == desktop->handle()) {
        XSetInputFocus(app->display(), w, RevertToNone, CurrentTime);
    }

    if (c && w == c->handle() && c->protocols() & YFrameClient::wpTakeFocus) {
        c->sendMessage(_XA_WM_TAKE_FOCUS);
    }

    if (!pointerColormap)
        setColormapWindow(f);

#ifndef LITE
    /// !!! /* warp pointer sucks */
    if (f &&
        canWarp &&
        !clickFocus &&
        warpPointer &&
        wmState == wmRUNNING)
    {
        XWarpPointer(app->display(), None, handle(), 0, 0, 0, 0,
                     f->x() + f->borderX(), f->y() + f->borderY() + f->titleY());
    }

#endif
    MSG(("SET FOCUS END"));
    updateFullscreenLayer();
}

void YWindowManager::loseFocus(YFrameWindow *window) {
    PRECONDITION(window != 0);
#ifdef DEBUG
    if (debug_z) dumpZorder("losing focus: ", window);
#endif
    YFrameWindow *w = window->findWindow(YFrameWindow::fwfNext |
                                         YFrameWindow::fwfVisible |
                                         YFrameWindow::fwfCycle |
                                         YFrameWindow::fwfFocusable |
                                         YFrameWindow::fwfLayers |
                                         YFrameWindow::fwfWorkspace);

    PRECONDITION(w != window);
    setFocus(w, false);
}

void YWindowManager::loseFocus(YFrameWindow *window,
                               YFrameWindow *next,
                               YFrameWindow *prev)
{
    PRECONDITION(window != 0);
#ifdef DEBUG
    if (debug_z) dumpZorder("close: losing focus: ", window);
#endif

    YFrameWindow *w = 0;

    if (next)
        w = next->findWindow(YFrameWindow::fwfVisible |
                             YFrameWindow::fwfCycle |
                             YFrameWindow::fwfFocusable |
                             YFrameWindow::fwfLayers |
                             YFrameWindow::fwfWorkspace);
    else if (prev)
        w = prev->findWindow(YFrameWindow::fwfNext |
                             YFrameWindow::fwfVisible |
                             YFrameWindow::fwfCycle |
                             YFrameWindow::fwfFocusable |
                             YFrameWindow::fwfLayers |
                             YFrameWindow::fwfWorkspace);

    if (w == window)
        w = 0;
    //msg("loseFocus to %s", w ? w->getTitle() : "<none>");
    setFocus(w, false);
}

void YWindowManager::activate(YFrameWindow *window, bool canWarp) {
    if (window) {
        window->wmRaise();
        window->activate(canWarp);
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
    if (app->hasColormap()) {
        //MSG(("installing colormap 0x%lX", cmap));
        if (app->grabWindow() == 0) {
            if (cmap == None) {
                XInstallColormap(app->display(), app->colormap());
            } else {
                XInstallColormap(app->display(), cmap);
            }
        }
    }
}

void YWindowManager::setColormapWindow(YFrameWindow *frame) {
    if (fColormapWindow != frame) {
        fColormapWindow = frame;

        if (colormapWindow())
            installColormap(colormapWindow()->client()->colormap());
        else
            installColormap(None);
    }
}

void YWindowManager::manageClients() {
    unsigned int clientCount;
    Window winRoot, winParent, *winClients;

    XGrabServer(app->display());
    XSync(app->display(), False);
    XQueryTree(app->display(), handle(),
               &winRoot, &winParent, &winClients, &clientCount);

    if (winClients)
        for (unsigned int i = 0; i < clientCount; i++)
            if (findClient(winClients[i]) == 0)
                manageClient(winClients[i]);

    XUngrabServer(app->display());
    if (winClients)
        XFree(winClients);
    updateWorkArea();
    wmState = wmRUNNING;
    focusTopWindow();
}

void YWindowManager::unmanageClients() {
    Window w;

    setFocus(0);
    XGrabServer(app->display());
    for (unsigned int l = 0; l < WinLayerCount; l++) {
        while (bottom(l)) {
            w = bottom(l)->client()->handle();
            unmanageClient(w, true);
        }
    }
    XUngrabServer(app->display());
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
                f->setPosition(x, y);
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
        f->setPosition(x, y);
    }
}

void YWindowManager::setWindows(YFrameWindow **w, int count, YAction *action) {
    saveArrange(w, count);

    if (count == 0)
        return;

    for (int i = count; i > 0; i--) {
        YFrameWindow *f = w[i - 1];
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

void YWindowManager::placeWindow(YFrameWindow *frame, int x, int y,
				 bool newClient, bool &
#ifdef CONFIG_SESSION
                                 canActivate
#endif
                                )
{
    YFrameClient *client = frame->client();

    int frameWidth = 2 * frame->borderX();
    int frameHeight = 2 * frame->borderY() + frame->titleY();
    int posWidth = client->width() + frameWidth;
    int posHeight = client->height() + frameHeight;
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
            posX -= frame->borderX();
            posY -= frame->borderY() + frame->titleY();
        } else {
            int gx, gy;

            client->gravityOffsets(gx, gy);

            if (gx > 0)
                posX -= 2 * frame->borderX() - 1 - client->getBorder();
            if (gy > 0)
                posY -= 2 * frame->borderY() + frame->titleY() - 1 - client->getBorder();
        }
    }

setGeo:
    MSG(("mapping geometry (%d:%d %dx%d)", posX, posY, posWidth, posHeight));
    frame->setGeometry(YRect(posX, posY, posWidth, posHeight));

}

YFrameWindow *YWindowManager::manageClient(Window win, bool mapClient) {
    YFrameWindow *frame(NULL);
    YFrameClient *client(NULL);
    int cx(0);
    int cy(0);
    bool canManualPlace(false);
    long workspace(0), layer(0), state_mask(0), state(0);
    bool canActivate(true);
#ifdef CONFIG_TRAY
    long tray(0);
#endif

    MSG(("managing window 0x%lX", win));
    frame = findFrame(win);
    PRECONDITION(frame == 0);

    XGrabServer(app->display());
#if 0
    XSync(app->display(), False);
    {
        XEvent xev;
        if (XCheckTypedWindowEvent(app->display(), win, DestroyNotify, &xev))
            goto end;
    }
#endif

    client = findClient(win);
    if (client == 0) {
        XWindowAttributes attributes;

        if (!XGetWindowAttributes(app->display(), win, &attributes))
            goto end;

        if (attributes.override_redirect)
            goto end;

        ///!!! is this correct ?
        if (!mapClient && attributes.map_state == IsUnmapped)
            goto end;

        client = new YFrameClient(0, 0, win);
        if (client == 0)
            goto end;

        client->setBorder(attributes.border_width);
        client->setColormap(attributes.colormap);
    }

#ifdef CONFIG_WM_SESSION
    setTopLevelProcess(client->pid());
#endif

    MSG(("initial geometry (%d:%d %dx%d)",
         client->x(), client->y(), client->width(), client->height()));

    cx = client->x();
    cy = client->y();

    if (client->visible() && wmState == wmSTARTUP)
        mapClient = true;

    frame = new YFrameWindow(0, client);
    if (frame == 0) {
        delete client;
        goto end;
    }

    placeWindow(frame, cx, cy, (wmState != wmSTARTUP), canActivate);

#ifdef CONFIG_SHAPE
    frame->setShape();
#endif

    MSG(("Map - Frame: %d", frame->visible()));
    MSG(("Map - Client: %d", frame->client()->visible()));

#ifdef GNOME1_HINTS
    Atom net_wm_window_type;
    if (frame->client()->getNetWMWindowType(&net_wm_window_type)) {
        if (net_wm_window_type ==
            _XA_NET_WM_WINDOW_TYPE_DOCK)
        {
            frame->setSticky(true);
            frame->setLayer(WinLayerDock);
        }
        if (net_wm_window_type ==
            _XA_NET_WM_WINDOW_TYPE_DESKTOP)
        {
            frame->setSticky(true);
            frame->setLayer(WinLayerDesktop);
        }
    } else if (frame->client()->getWinLayerHint(&layer))
        frame->setLayer(layer);
#endif

    if (frame->client()->getWinStateHint(&state_mask, &state)) {
        frame->setState(state_mask, state);
    } else {
        FrameState st = frame->client()->getFrameState();

        if (st == WithdrawnState) {
            XWMHints *h = frame->client()->hints();
            if (h && (h->flags & StateHint))
                st = h->initial_state;
            else
                st = NormalState;
        }
        MSG(("FRAME state = %d", st));
        switch (st) {
        case IconicState:
            frame->setState(WinStateMinimized, WinStateMinimized);
            break;

        case NormalState:
        case WithdrawnState:
            break;
        }
    }

    if (frame->client()->getWinWorkspaceHint(&workspace))
        frame->setWorkspace(workspace);

#ifdef CONFIG_TRAY
    if (frame->client()->getWinTrayHint(&tray))
        frame->setTrayOption(tray);
#endif

    if ((limitSize || limitPosition) &&
        (wmState != wmSTARTUP) &&
        !frame->affectsWorkArea())
    {
        int posX(frame->x() + frame->borderX()),
	    posY(frame->y() + frame->borderY()),
	    posWidth(frame->width() - 2 * frame->borderX()),
	    posHeight(frame->height() - 2 * frame->borderY());

        if (limitSize) {
            int Mw, Mh;
            manager->getWorkAreaSize(frame, &Mw, &Mh);

            posWidth = min(posWidth, Mw);
            posHeight = min(posHeight, Mh);

#warning "cleanup the constrainSize code, there is some duplication"
            posHeight -= frame->titleY();
            frame->client()->constrainSize(posWidth, posHeight,
                                           ///frame->getLayer(),
                                           0);
            posHeight += frame->titleY();
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

        posX -= frame->borderX();
        posY -= frame->borderY();
        posWidth += 2 * frame->borderX();
        posHeight += 2 * frame->borderY();
        frame->setGeometry(YRect(posX, posY, posWidth, posHeight));
    }

    if (!mapClient) {
        /// !!! fix (new internal state)
        frame->setState(WinStateHidden, WinStateHidden);
    }
    frame->setManaged(true);

    if (canActivate && manualPlacement && wmState == wmRUNNING &&
#ifdef CONFIG_WINLIST
        client != windowList &&
#endif
        !frame->owner() &&
        (!client->sizeHints() ||
         !(client->sizeHints()->flags & (USPosition | PPosition))))
        canManualPlace = true;

    if (mapClient) {
        if (frame->getState() == 0 || frame->isRollup()) {
            if (canManualPlace && !opaqueMove)
                frame->manualPlace();
        }
    }
    frame->updateState();
    frame->updateProperties();
#ifdef CONFIG_TASKBAR
    frame->updateTaskBar();
#endif
    frame->updateNormalSize();
    frame->updateLayout();
    if (frame->affectsWorkArea())
	updateWorkArea();
    if (mapClient) {
        if (frame->getState() == 0 || frame->isRollup()) {
            if (wmState == wmRUNNING && canActivate)
                frame->focusOnMap();
            if (canManualPlace && opaqueMove)
                frame->wmMove();
        }
    }
#if 1
    if (wmState == wmRUNNING) {
#ifndef NO_WINDOW_OPTIONS
        if (frame->frameOptions() & (YFrameWindow::foMaximizedVert | YFrameWindow::foMaximizedHorz))
            frame->setState(
                WinStateMaximizedVert | WinStateMaximizedHoriz,
                ((frame->frameOptions() & YFrameWindow::foMaximizedVert) ? WinStateMaximizedVert : 0) |
                ((frame->frameOptions() & YFrameWindow::foMaximizedHorz) ? WinStateMaximizedHoriz : 0));
#endif
    }
#endif


end:
    XUngrabServer(app->display());
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
    if (wmState != wmRUNNING)
        return ;
    if (!clickFocus && strongPointerFocus) {
        XSetInputFocus(app->display(), PointerRoot, RevertToNone, CurrentTime);
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

void YWindowManager::clearFullscreenLayer() {
    YFrameWindow *w;
    while ((w = bottom(WinLayerFullscreen)) != NULL)
        w->setLayer(WinLayerNormal);
}

void YWindowManager::updateFullscreenLayer() { /// HACK !!!
    YFrameWindow *focus = getFocus();

    while (focus && focus->owner())
        focus = focus->owner();

    if (focus == 0
        || focus->getLayer() != WinLayerFullscreen
        || !focus->isFullscreen())
    {
        YFrameWindow *w;
        while ((w = bottom(WinLayerFullscreen)) != NULL)
            w->setLayer(WinLayerNormal);
    }

    if (focus != 0) {
        if (focus->isFullscreen() && focus->getLayer() != WinLayerFullscreen)
            focus->setLayer(WinLayerFullscreen);
    }
}

void YWindowManager::restackWindows(YFrameWindow *win) {
    int count = 0;
    YFrameWindow *f;
    YPopupWindow *p;
    long ll;

//    updateFullscreenLayer();

    for (f = win; f; f = f->prev())
        //if (f->visibleNow())
            count++;

    for (ll = win->getLayer() + 1; ll < WinLayerCount; ll++) {
        f = bottom(ll);
        for (; f; f = f->prev())
            //if (f->visibleNow())
                count++;
    }

#ifndef LITE
    if (statusMoveSize && statusMoveSize->visible())
        count++;
#endif

    p = app->popup();
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

    p = app->popup();
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

    for (ll = WinLayerCount - 1; ll > win->getLayer(); ll--) {
        for (f = top(ll); f; f = f->next())
            //if (f->visibleNow())
                w[i++] = f->handle();
    }
    for (f = top(win->getLayer()); f; f = f->next()) {
        //if (f->visibleNow())
            w[i++] = f->handle();
        if (f == win)
            break;
    }

    if (count > 0) {
#if 0
        /* remove this code if ok !!! must determine correct top window */
#if 1
        XRaiseWindow(app->display(), w[0]);
#else
        if (win->next()) {
            XWindowChanges xwc;

            xwc.sibling = win->next()->handle();
            xwc.stack_mode = Above;
            XConfigureWindow(app->display(), w[0], CWSibling | CWStackMode, &xwc);
        }
#endif
        if (count > 1)
#endif
        XRestackWindows(app->display(), w, count);
    }
    if (i != count) {
        MSG(("i=%d, count=%d", i, count));
    }
    PRECONDITION(i == count);
    delete w;
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

#warning "rewrite workarea determine code (per workspace)"
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
            limitByDockLayer && w->getLayer() == WinLayerDock)
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
    long *area = new long[nw * 4];

    if (!area)
        return;

    for (int ws = 0; ws < nw; ws++) {
        area[ws * 4    ] = fWorkArea[ws].fMinX;
        area[ws * 4 + 1] = fWorkArea[ws].fMinY;
        area[ws * 4 + 2] = fWorkArea[ws].fMaxX;
        area[ws * 4 + 3] = fWorkArea[ws].fMaxY;
    }

#ifdef WMSPEC_HINTS
    XChangeProperty(app->display(), handle(),
                    _XA_NET_WORKAREA,
                    XA_CARDINAL,
                    32, PropModeReplace,
                    (unsigned char *)area, nw * 4);
#endif
    if (fActiveWorkspace != -1) {
        int cw = fActiveWorkspace;

#ifdef GNOME1_HINTS
        XChangeProperty(app->display(), handle(),
                        _XA_WIN_WORKAREA,
                        XA_CARDINAL,
                        32, PropModeReplace,
                        (unsigned char *)(area + 4 * cw), 4);
#endif
    }
}

void YWindowManager::relocateWindows(long workspace, int dx, int dy) {
#warning "needs a rewrite (save old work area) for each workspace"
#if 1
    for (YFrameWindow * f = topLayer(); f; f = f->nextLayer())
        if (f->inWorkArea()) {
            if (f->getWorkspace() == workspace ||
                (f->isSticky() && workspace == activeWorkspace()))
            {
                f->setPosition(f->x() + dx, f->y() + dy);
            }
        }
#endif
}

void YWindowManager::resizeWindows() {
    for (YFrameWindow * f = topLayer(); f; f = f->nextLayer())
        if (f->inWorkArea()) {
#warning "this needs serious recheck"
#if 0
	    if (f->isMaximized() || f->canSize())
                f->updateLayout();
#endif
#if 1
            if (f->isMaximized())
		f->updateLayout();
#endif
#if 0
	    if (isMaximizedFully())
		f->setGeometry(fMinX, fMinY, fMaxX - fMinX, fMaxY - fMinY);
	    else if (f->isMaximizedVert())
		f->setGeometry(f->x(), fMinY, f->width(), fMaxY - fMinY);
	    else if (f->isMaximizedHoriz())
		f->setGeometry(fMinX, f->y(), fMaxX - fMinX, f->height());
#endif
	}
}

void YWindowManager::activateWorkspace(long workspace) {
    if (workspace != fActiveWorkspace) {

        XSetInputFocus(app->display(), desktop->handle(), RevertToNone, CurrentTime);

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

        XChangeProperty(app->display(), handle(),
                        _XA_WIN_WORKSPACE,
                        XA_CARDINAL,
                        32, PropModeReplace,
                        (unsigned char *)&ws, 1);
#endif
#ifdef WMSPEC_HINTS

        XChangeProperty(app->display(), handle(),
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

        focusTopWindow();
        resetColormap(true);

#ifdef CONFIG_TASKBAR
        if (taskBar && taskBar->taskPane())
            taskBar->taskPane()->relayout();
#endif
#ifdef CONFIG_TRAY
        if (taskBar && taskBar->trayPane())
            taskBar->trayPane()->relayout();
#endif
#ifndef LITE
        if (workspaceSwitchStatus
#ifdef CONFIG_TASKBAR
            && (!showTaskBar || !taskBarShowWorkspaces)
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
    static int x = 0, y = 0;
    MiniIcon *iw = frame->getMiniIcon();

    int mx, my, Mx, My;
    manager->getWorkArea(frame, &mx, &my, &Mx, &My);

    x = max(x, mx);
    y = max(y, my);

    *iconX = x;
    *iconY = y;

    y += iw->height();
    if (y >= My) {
        x += iw->width();
        y = Mx;
        if (x >= Mx) {
            x = 0;
            y = 0;
        }
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

        if (XGetWindowProperty(app->display(), handle(),
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
    XChangeProperty(app->display(), desktop->handle(),
                    _XA_WIN_CLIENT_LIST,
                    XA_CARDINAL,
                    32, PropModeReplace,
                    (unsigned char *)ids, count);
#endif
#ifdef WMSPEC_HINTS
    XChangeProperty(app->display(), desktop->handle(),
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

    XChangeProperty(app->display(), desktop->handle(),
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
        }
        app->exit(0);
    }
}

void YWindowManager::removeClientFrame(YFrameWindow *frame) {
    YFrameWindow *p = frame->prevLayer(), *n = frame->nextLayer();

    if (fArrangeInfo) {
        for (int i = 0; i < fArrangeCount; i++)
            if (fArrangeInfo[i].frame == frame)
                fArrangeInfo[i].frame = 0;
    }
    if (frame == getFocus())
        manager->loseFocus(frame, n, p);
    if (frame == getFocus())
        setFocus(0);
    if (colormapWindow() == frame)
        setColormapWindow(getFocus());
    if (frame->affectsWorkArea())
	updateWorkArea();
}

void YWindowManager::switchFocusTo(YFrameWindow *frame) {
    if (frame != fFocusWin) {
        if (fFocusWin)
            fFocusWin->loseWinFocus();
        fFocusWin = frame;
        ///msg("setting %lX", fFocusWin);
        if (fFocusWin)
            fFocusWin->setWinFocus();
    }
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
void YWindowManager::popupWindowListMenu(int x, int y) {
    windowListMenu->popup(0, 0, x, y, -1, -1,
                          YPopupWindow::pfCanFlipVertical |
                          YPopupWindow::pfCanFlipHorizontal |
                          YPopupWindow::pfPopupMenu);
}
#endif

void YWindowManager::popupStartMenu() { // !! fix
#ifdef CONFIG_TASKBAR
    if (showTaskBar && taskBar && taskBarShowStartMenu)
        taskBar->popupStartMenu();
    else
#endif
    {
#ifndef NO_CONFIGURE_MENUS
        rootMenu->popup(0, 0, 0, 0, -1, -1,
                        YPopupWindow::pfCanFlipVertical |
                        YPopupWindow::pfCanFlipHorizontal |
                        YPopupWindow::pfPopupMenu);
#endif
    }
}

#ifdef CONFIG_WINMENU
void YWindowManager::popupWindowListMenu() {
#ifdef CONFIG_TASKBAR
    if (showTaskBar && taskBar && taskBarShowWindowListMenu)
        taskBar->popupWindowListMenu();
    else
#endif
        popupWindowListMenu(0, 0);
}
#endif

void YWindowManager::switchToWorkspace(long nw, bool takeCurrent) {
    if (nw >= 0 && nw < workspaceCount()) {
        YFrameWindow *frame = getFocus();
        if (takeCurrent && frame && !frame->isSticky()) {
            frame->wmOccupyAll();
            frame->wmRaise();
            activateWorkspace(nw);
            frame->wmOccupyOnlyWorkspace(nw);
        } else {
            activateWorkspace(nw);
        }
#ifdef CONFIG_TASKBAR
        if (taskBar) taskBar->popOut();
#endif
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
    tw -= 2 * w->borderX();
    th -= 2 * w->borderY() + w->titleY();
    w->client()->constrainSize(tw, th, ///WinLayerNormal,
                               0);
    tw += 2 * w->borderX();
    th += 2 * w->borderY() + w->titleY();
    w->setGeometry(YRect(tx, ty, tw, th));
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

void YWindowManager::getWindowsToArrange(YFrameWindow ***win, int *count) {
    YFrameWindow *w = topLayer(WinLayerNormal);

    *count = 0;
    while (w) {
        if (w->owner() == 0 && // not transient ?
            w->visibleOn(activeWorkspace()) && // visible
            !w->isSticky() && // not on all workspaces
            !w->isRollup() &&
            !w->isMinimized() &&
            !w->isHidden())
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
                !w->isSticky() && // not on all workspaces
                !w->isRollup() &&
                !w->isMinimized() &&
                !w->isHidden())
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
                f->setGeometry(YRect(fArrangeInfo[i].x,
                                     fArrangeInfo[i].y,
                                     fArrangeInfo[i].w,
                                     fArrangeInfo[i].h));
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

#ifdef CONFIG_WM_SESSION
void YWindowManager::setTopLevelProcess(pid_t p) {
    if (p != getpid() && p != PID_MAX) {
	msg("moving process %d to the top", p);
	fProcessList.push(p);
    }
}

void YWindowManager::removeLRUProcess() {
    pid_t const lru(fProcessList[0]);
    msg("Kernel sent a cleanup request. Closing process %d.", lru);

    Window leader(None);

    for (YFrameWindow * f(topLayer()); f; f = f->nextLayer())
	if (f->client()->pid() == lru &&
	   (None != (leader = f->client()->clientLeader()) ||
	    None != (leader = f->client()->handle())))
	    break;

    if (leader != None) { // the group leader doesn't have to be mapped
	msg("Sending WM_DELETE_WINDOW to %p", leader);
	YFrameClient::sendMessage(leader, _XA_WM_DELETE_WINDOW, CurrentTime);
    }

/* !!! TODO:	- windows which do not support WM_DELETE_WINDOW
		- unmapping -> removing from process list
		- leader == None --> loop over all processes?
		- s/msg/MSG/
		- apps launched from icewm ignore the PRELOAD library
*/
}
#endif

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
    setPointer(YApplication::leftPointer);
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
            setPointer(YApplication::leftPointer);
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
        setPointer(YApplication::leftPointer);
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

