/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 *
 * Window list
 */
#include "config.h"
#include "ykey.h"
#include "wmwinlist.h"
#include "ymenuitem.h"

#include "wmaction.h"
#include "wmclient.h"
#include "wmframe.h"
#include "wmmgr.h"
#include "wmapp.h"
#include "sysdep.h"

#include "intl.h"

#ifdef CONFIG_WINLIST

WindowList *windowList = 0;

WindowListItem::WindowListItem(ClientData *frame): YListItem() {
    fFrame = frame;
}

WindowListItem::~WindowListItem() {
    if (fFrame) {
        fFrame->setWinListItem(0);
        fFrame = 0;
    }
}

int WindowListItem::getOffset() {
    int ofs = 0;
    ClientData *w = getFrame();

    while (w->owner()) {
        ofs += 20;
        w = w->owner();
    }
    return ofs;
}

const char *WindowListItem::getText() {
    return getFrame()->getTitle();
}

YIcon *WindowListItem::getIcon() {
    return getFrame()->getIcon();
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
    }
}

void WindowListBox::actionPerformed(YAction *action, unsigned int modifiers) {
    if (action == actionTileVertical ||
        action == actionTileHorizontal)
    {
        if (hasSelection()) {
            YListItem *i;
            int count = 0;
            YFrameWindow **w;

            for (i = getFirst(); i; i = i->getNext())
                if (isSelected(i))
                    count++;

            if (count > 0) {
                w = new YFrameWindow *[count];
                if (w) {
                    int n = 0;

                    for (i = getFirst(); i; i = i->getNext())
                        if (isSelected(i)) {
                            WindowListItem *item = (WindowListItem *)i;
                            w[n++] = (YFrameWindow *)item->getFrame();
                        }
                    PRECONDITION(n == count);

                    manager->tileWindows(w, count,
                                         (action == actionTileVertical) ? true : false);
                    delete w;
                }
            }
        }
    } else if (action == actionCascade ||
               action == actionArrange)
    {
        if (hasSelection()) {
            YFrameWindow *f;
            YListItem *i;
            int count = 0;
            YFrameWindow **w;

            for (f = manager->topLayer(); f; f = f->nextLayer()) {
                i = f->winListItem();
                if (i && isSelected(i))
                    count++;
            }
            if (count > 0) {
                w = new YFrameWindow *[count];
                if (w) {
                    int n = 0;
                    for (f = manager->topLayer(); f; f = f->nextLayer()) {
                        i = f->winListItem();
                        if (i && isSelected(i))
                            w[n++] = f;
                    }

                    if (action == actionCascade) {
                        manager->cascadePlace(w, count);
                    } else if (action == actionArrange) {
                        manager->smartPlace(w, count);
                    }
                    delete w;
                }
            }
        }
    } else {
        if (hasSelection()) {
            YListItem *i;

            for (i = getFirst(); i; i = i->getNext()) {
                if (isSelected(i)) {
                    WindowListItem *item = (WindowListItem *)i;
#ifndef CONFIG_PDA		    
                    if (action == actionHide)
                        if (item->getFrame()->isHidden())
                            continue;
#endif			    
                    if (action == actionMinimize)
                        if (item->getFrame()->isMinimized())
                            continue;
                    item->getFrame()->actionPerformed(action, modifiers);
                }
            }
        }
    }
}

bool WindowListBox::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        KeySym k = XKeycodeToKeysym(app->display(), key.keycode, 0);
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
                    windowListPopup->popup(0, 0,
                                           key.x_root, key.y_root, -1, -1,
                                           YPopupWindow::pfCanFlipVertical |
                                           YPopupWindow::pfCanFlipHorizontal |
                                           YPopupWindow::pfPopupMenu);
                } else {
                    windowListAllPopup->popup(0, 0, key.x_root, key.y_root, -1, -1,
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
            windowListPopup->popup(0, 0,
                                   up.x_root, up.y_root, -1, -1,
                                   YPopupWindow::pfCanFlipVertical |
                                   YPopupWindow::pfCanFlipHorizontal |
                                   YPopupWindow::pfPopupMenu);
        } else {
            windowListAllPopup->popup(0, 0, up.x_root, up.y_root, -1, -1,
                                      YPopupWindow::pfCanFlipVertical |
                                      YPopupWindow::pfCanFlipHorizontal |
                                      YPopupWindow::pfPopupMenu);

        }
        return ;
    }
    YListBox::handleClick(up, count);
}

void WindowListBox::enableCommands(YMenu *popup) {
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
}

WindowList::WindowList(YWindow *aParent):
YFrameClient(aParent, 0) {
    scroll = new YScrollView(this);
    list = new WindowListBox(scroll, scroll);
    scroll->setView(list);
    list->show();
    scroll->show();

    YMenu *closeSubmenu = new YMenu();
    assert(closeSubmenu != 0);

    closeSubmenu->addItem(_("_Close"), -2, _("Delete"), actionClose);
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

    setGeometry(w / 3, h / 3, w / 3, h / 3);

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
        if (frame->owner() &&
            frame->owner()->winListItem())
        {
            list->addAfter(frame->owner()->winListItem(), item);
        } else {
            list->addItem(item);
        }
    }
    return item;
}

void WindowList::removeWindowListApp(WindowListItem *item) {
    if (item) {
        list->removeItem(item);
        delete item;
    }
}

void WindowList::configure(const int x, const int y, 
			   const unsigned width, const unsigned height, 
			   const bool resized) {
    YFrameClient::configure(x, y, width, height, resized);
    if (resized) scroll->setGeometry(0, 0, width, height);
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
            getFrame()->setPosition(px, py);
        }
        getFrame()->setLayer(WinLayerAboveDock);
        getFrame()->setState(WinStateAllWorkspaces, WinStateAllWorkspaces);
        getFrame()->activate(true);
    }
}
#endif
