/*
 * IceWM
 *
 * Copyright (C) 1997-2003 Marko Macek
 *
 * Window list
 */
#include "config.h"
#include "ykey.h"
#include "ypaint.h"
#include "wmwinlist.h"
#include "ymenuitem.h"

#include "prefs.h"
#include "wmaction.h"
#include "wmclient.h"
#include "wmframe.h"
#include "wmmgr.h"
#include "wmapp.h"
#include "sysdep.h"
#include "yrect.h"

#include "intl.h"

#ifdef CONFIG_WINLIST

WindowList *windowList = 0;

WindowListItem::WindowListItem(ClientData *frame, int workspace): YListItem() {
    fFrame = frame;
    fWorkspace = workspace;
}

WindowListItem::~WindowListItem() {
    if (fFrame) {
        fFrame->setWinListItem(0);
        fFrame = 0;
    }
}

int WindowListItem::getOffset() {
    int ofs = -20;
    ClientData *w = getFrame();

    if (w) {
        ofs += 40;
        while (w->owner()) {
            ofs += 20;
            w = w->owner();
        }
    }
    return ofs;
}

const char *WindowListItem::getText() {
    if (fFrame)
        return getFrame()->getTitle();
    else
        if (fWorkspace < 0 || fWorkspace >= workspaceCount)
            return _("All Workspaces");
        else
            return workspaceNames[fWorkspace];
}

YIcon *WindowListItem::getIcon() {
    if (fFrame)
        return getFrame()->getIcon();
    else
        return 0;
}


WindowListBox::WindowListBox(YScrollView *view, YWindow *aParent):
    YListBox(view, aParent)
{
}

WindowListBox::~WindowListBox() {
}

void WindowListBox::activateItem(YListItem *item) {
    WindowListItem *i = (WindowListItem *)item;
    ClientData *f = i->getFrame();
    if (f) {
        f->activateWindow(true);
        windowList->getFrame()->wmHide();
    } else {
        int w = i->getWorkspace();

        if (w != -1) {
            manager->activateWorkspace(w);
            windowList->getFrame()->wmHide();
        }
    }
}

void WindowListBox::getSelectedWindows(YArray<YFrameWindow *> &frames) {
    if (hasSelection()) {
        for (YListItem *i = getFirst(); i; i = i->getNext()) {
            if (isSelected(i)) {
                WindowListItem *item = (WindowListItem *)i;
                ClientData *f = item->getFrame();
                if (f)
                    frames.append((YFrameWindow *)f);
            }
        }
    }
}

void WindowListBox::actionPerformed(YAction *action, unsigned int modifiers) {
    YArray<YFrameWindow *> frameList;
    getSelectedWindows(frameList);

    if (action == actionTileVertical ||
        action == actionTileHorizontal)
    {
        if (frameList.getCount() > 0)
            manager->tileWindows(frameList.getItemPtr(0),
                                 frameList.getCount(),
                                 (action == actionTileVertical) ? true : false);
    } else if (action == actionCascade ||
               action == actionArrange)
    {
        if (frameList.getCount() > 0) {
            if (action == actionCascade) {
                manager->cascadePlace(frameList.getItemPtr(0),
                                      frameList.getCount());
            } else if (action == actionArrange) {
                manager->smartPlace(frameList.getItemPtr(0),
                                    frameList.getCount());
            }
        }
    } else {
        for (unsigned int i = 0; i < frameList.getCount(); i++) {
#ifndef CONFIG_PDA
            if (action == actionHide)
                if (frameList[i]->isHidden())
                    continue;
#endif
            if (action == actionMinimize)
                if (frameList[i]->isMinimized())
                    continue;
            frameList[i]->actionPerformed(action, modifiers);
        }
    }
}

bool WindowListBox::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        KeySym k = XKeycodeToKeysym(xapp->display(), key.keycode, 0);
        int m = KEY_MODMASK(key.state);

        switch (k) {
        case XK_Escape:
            windowList->getFrame()->wmHide();
            return true;
        case XK_F10:
        case XK_Menu:
            if (k != XK_F10 || m == ShiftMask) {
                if (hasSelection()) {
                    enableCommands(windowListPopup);
                    windowListPopup->popup(0, 0, 0,
                                           key.x_root, key.y_root,
                                           YPopupWindow::pfCanFlipVertical |
                                           YPopupWindow::pfCanFlipHorizontal |
                                           YPopupWindow::pfPopupMenu);
                } else {
                    windowListAllPopup->popup(0, 0, 0, key.x_root, key.y_root,
                                              YPopupWindow::pfCanFlipVertical |
                                              YPopupWindow::pfCanFlipHorizontal |
                                              YPopupWindow::pfPopupMenu);
                }
            }
            break;
        case XK_Delete:
            {
                if (m & ShiftMask)
                    actionPerformed(actionKill, key.state);
                else
                    actionPerformed(actionClose, key.state);
            }
            break;
        }
    }
    return YListBox::handleKey(key);
}

void WindowListBox::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 3 && count == 1 && IS_BUTTON(up.state, Button3Mask)) {
        int no = findItemByPoint(up.x, up.y);

        if (no != -1) {
            YListItem *i = getItem(no);
            if (!isSelected(i)) {
                focusSelectItem(no);
            } else {
                //fFocusedItem = -1;
            }
            enableCommands(windowListPopup);
            windowListPopup->popup(0, 0, 0,
                                   up.x_root, up.y_root,
                                   YPopupWindow::pfCanFlipVertical |
                                   YPopupWindow::pfCanFlipHorizontal |
                                   YPopupWindow::pfPopupMenu);
        } else {
            windowListAllPopup->popup(0, 0, 0, up.x_root, up.y_root,
                                      YPopupWindow::pfCanFlipVertical |
                                      YPopupWindow::pfCanFlipHorizontal |
                                      YPopupWindow::pfPopupMenu);

        }
        return ;
    }
    YListBox::handleClick(up, count);
}

void WindowListBox::enableCommands(YMenu *popup) {
    bool noItems = true;
    long workspace = -1;
    bool sameWorkspace = false;
    bool notHidden = false;
    bool notMinimized = false;

    // enable minimize,hide if appropriate
    // enable workspace selections if appropriate

    popup->enableCommand(0);
    for (YListItem *i = getFirst(); i; i = i->getNext()) {
        if (isSelected(i)) {
            WindowListItem *item = (WindowListItem *)i;
            if (!item->getFrame()) {
                continue;
            }
            noItems = false;

            if (!item->getFrame()->isHidden())
                notHidden = true;
            if (!item->getFrame()->isMinimized())
                notMinimized = true;

            long ws = item->getFrame()->getWorkspace();
            if (workspace == -1) {
                workspace = ws;
                sameWorkspace = true;
            } else if (workspace != ws) {
                sameWorkspace = false;
            }
            if (item->getFrame()->isSticky())
                sameWorkspace = false;
        }
    }
    if (!notHidden)
        popup->disableCommand(actionHide);
    if (!notMinimized)
        popup->disableCommand(actionMinimize);

    moveMenu->enableCommand(0);
    if (sameWorkspace && workspace != -1) {
        for (int i = 0; i < moveMenu->itemCount(); i++) {
            YMenuItem *item = moveMenu->getItem(i);
            for (int w = 0; w < workspaceCount; w++)
                if (item && item->getAction() == workspaceActionMoveTo[w])
                    item->setEnabled(w != workspace);
        }
    }
    if (noItems) {
        moveMenu->disableCommand(0);
        popup->disableCommand(0);
    }
}

WindowList::WindowList(YWindow *aParent):
YFrameClient(aParent, 0) {
    scroll = new YScrollView(this);
    list = new WindowListBox(scroll, scroll);
    scroll->setView(list);
    list->show();
    scroll->show();

    workspaceItem = new WindowListItem *[workspaceCount + 1];
    for (int ws = 0; ws < workspaceCount; ws++) {
        workspaceItem[ws] = new WindowListItem(0, ws);
        list->addItem(workspaceItem[ws]);
    }
    workspaceItem[workspaceCount] = new WindowListItem(0, -1);
    list->addItem(workspaceItem[workspaceCount]);

    YMenu *closeSubmenu = new YMenu();
    assert(closeSubmenu != 0);

    closeSubmenu->addItem(_("_Close"), -2, _("Del"), actionClose);
    closeSubmenu->addSeparator();
    closeSubmenu->addItem(_("_Kill Client"), -2, 0, actionKill);
#if 0
    closeSubmenu->addItem(_("_Terminate Process"), -2, 0, actionTermProcess)->setEnabled(false);
    closeSubmenu->addItem(_("Kill _Process"), -2, 0, actionKillProcess)->setEnabled(false);
#endif

    windowListPopup = new YMenu();
    windowListPopup->setActionListener(list);
    windowListPopup->addItem(_("_Show"), -2, 0, actionShow);
#ifndef CONFIG_PDA
    windowListPopup->addItem(_("_Hide"), -2, 0, actionHide);
#endif
    windowListPopup->addItem(_("_Minimize"), -2, 0, actionMinimize);
    windowListPopup->addSubmenu(_("Move _To"), -2, moveMenu);
    windowListPopup->addSeparator();
    windowListPopup->addItem(_("Tile _Vertically"), -2, KEY_NAME(gKeySysTileVertical), actionTileVertical);
    windowListPopup->addItem(_("T_ile Horizontally"), -2, KEY_NAME(gKeySysTileHorizontal), actionTileHorizontal);
    windowListPopup->addItem(_("Ca_scade"), -2, KEY_NAME(gKeySysCascade), actionCascade);
    windowListPopup->addItem(_("_Arrange"), -2, KEY_NAME(gKeySysArrange), actionArrange);
    windowListPopup->addSeparator();
    windowListPopup->addItem(_("_Minimize All"), -2, KEY_NAME(gKeySysMinimizeAll), actionMinimizeAll);
    windowListPopup->addItem(_("_Hide All"), -2, KEY_NAME(gKeySysHideAll), actionHideAll);
    windowListPopup->addItem(_("_Undo"), -2, KEY_NAME(gKeySysUndoArrange), actionUndoArrange);
    windowListPopup->addSeparator();
    windowListPopup->addItem(_("_Close"), -2, actionClose, closeSubmenu);

    windowListAllPopup = new YMenu();
    windowListAllPopup->setActionListener(wmapp);
    windowListAllPopup->addItem(_("Tile _Vertically"), -2, KEY_NAME(gKeySysTileVertical), actionTileVertical);
    windowListAllPopup->addItem(_("T_ile Horizontally"), -2, KEY_NAME(gKeySysTileHorizontal), actionTileHorizontal);
    windowListAllPopup->addItem(_("Ca_scade"), -2, KEY_NAME(gKeySysCascade), actionCascade);
    windowListAllPopup->addItem(_("_Arrange"), -2, KEY_NAME(gKeySysArrange), actionArrange);
    windowListAllPopup->addItem(_("_Minimize All"), -2, KEY_NAME(gKeySysMinimizeAll), actionMinimizeAll);
    windowListAllPopup->addItem(_("_Hide All"), -2, KEY_NAME(gKeySysHideAll), actionHideAll);
    windowListAllPopup->addItem(_("_Undo"), -2, KEY_NAME(gKeySysUndoArrange), actionUndoArrange);

    int dx, dy, dw, dh;
    manager->getScreenGeometry(&dx, &dy, &dw, &dh, 0);

    int w = dw;
    int h = dh;

    setGeometry(YRect(w / 4, h / 4, w / 2, h / 2));

    windowList = this;
    setWindowTitle(_("Window list"));
    setIconTitle(_("Window list"));
    setWinStateHint(WinStateAllWorkspaces, WinStateAllWorkspaces);
    setWinWorkspaceHint(0);
    setWinLayerHint(WinLayerAboveDock);
}

WindowList::~WindowList() {
    delete list; list = 0;
    delete scroll; scroll = 0;
    windowList = 0;

}

void WindowList::handleFocus(const XFocusChangeEvent &focus) {
    if (focus.type == FocusIn && focus.mode != NotifyUngrab) {
        list->setWindowFocus();
    } else if (focus.type == FocusOut) {
    }
}

void WindowList::relayout() {
    list->repaint();
}

WindowListItem *WindowList::addWindowListApp(YFrameWindow *frame) {
    if (!frame->client()->adopted())
        return 0;
    WindowListItem *item = new WindowListItem(frame);
    if (item) {
        insertApp(item);
    }
    return item;
}

void WindowList::insertApp(WindowListItem *item) {
    ClientData *frame = item->getFrame();
    if (frame->owner() &&
        frame->owner()->winListItem())
    {
        list->addAfter(frame->owner()->winListItem(), item);
    } else {
        int nw = frame->getWorkspace();
        if (!frame->isSticky())
            list->addAfter(workspaceItem[nw], item);
        else
            list->addItem(item);
    }
}

void WindowList::removeWindowListApp(WindowListItem *item) {
    if (item) {
        list->removeItem(item);
        delete item;
    }
}

void WindowList::updateWindowListApp(WindowListItem *item) {
    if (item) {
        list->removeItem(item);
        insertApp(item);
    }
}

void WindowList::configure(const YRect &r, const bool resized) {
    YFrameClient::configure(r, resized);
    if (resized) scroll->setGeometry(YRect(0, 0, r.width(), r.height()));
}

void WindowList::handleClose() {
    if (!getFrame()->isHidden())
        getFrame()->wmHide();
}

void WindowList::showFocused(int x, int y) {
    YFrameWindow *f = manager->getFocus();

    if (f != getFrame()) {
        if (f)
            list->focusSelectItem(list->findItem(f->winListItem()));
        else
            list->focusSelectItem(0);
    }
    if (getFrame() == 0)
        manager->manageClient(handle(), false);
    if (getFrame() != 0) {
        if (x != -1 && y != -1) {
            int px, py;

            int xiscreen = manager->getScreenForRect(x, y, 1, 1);
            int dx, dy, dw, dh;
            manager->getScreenGeometry(&dx, &dy, &dw, &dh, xiscreen);

            px = x - getFrame()->width() / 2;
            py = y - getFrame()->height() / 2;
            if (px + getFrame()->width() > dx + dw)
                px = dx + dw - getFrame()->width();
            if (py + getFrame()->height() > dy + dh)
                py = dx + dh - getFrame()->height();
            if (px < dx)
                px = dx;
            if (py < dy)
                py = dy;
            getFrame()->setNormalPositionOuter(px, py);
        }
        getFrame()->setRequestedLayer(WinLayerAboveDock);
        getFrame()->setState(WinStateAllWorkspaces, WinStateAllWorkspaces);
        getFrame()->activateWindow(true);
    }
}
#endif
