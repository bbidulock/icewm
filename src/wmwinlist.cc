/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
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
                    if (action == actionHide)
                        if (item->getFrame()->isHidden())
                            continue;
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
                    moveMenu->enableCommand(0);
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
            moveMenu->enableCommand(0);
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

WindowList::WindowList(YWindow *aParent): YFrameClient(aParent, 0) {
    scroll = new YScrollView(this);
    list = new WindowListBox(scroll, scroll);
    scroll->setView(list);
    list->show();
    scroll->show();

    YMenu *closeSubmenu = new YMenu();
    assert(closeSubmenu != 0);

    closeSubmenu->addItem("Close", 0, "Delete", actionClose);
    closeSubmenu->addSeparator();
    closeSubmenu->addItem("Kill Client", 0, 0, actionKill);
#if 0
    closeSubmenu->addItem("Terminate Process", 0, 0, actionTermProcess)->setEnabled(false);
    closeSubmenu->addItem("Kill Process", 5, 0, actionKillProcess)->setEnabled(false);
#endif

    windowListPopup = new YMenu();
    windowListPopup->setActionListener(list);
    windowListPopup->addItem("Show", 0, 0, actionShow);
    windowListPopup->addItem("Hide", 0, 0, actionHide);
    windowListPopup->addItem("Minimize", 0, 0, actionMinimize);
    windowListPopup->addSubmenu("Move To", 5, moveMenu);
    windowListPopup->addSeparator();
    windowListPopup->addItem("Tile Vertically", 5, 0, actionTileVertical);
    windowListPopup->addItem("Tile Horizontally", 1, 0, actionTileHorizontal);
    windowListPopup->addItem("Cascade", 1, 0, actionCascade);
    windowListPopup->addItem("Arrange", 1, 0, actionArrange);
    windowListPopup->addSeparator();
    windowListPopup->addItem("Close", 0, actionClose, closeSubmenu);

    windowListAllPopup = new YMenu();
    windowListAllPopup->setActionListener(wmapp);
    windowListAllPopup->addItem("Tile Vertically", 5, 0, actionTileVertical);
    windowListAllPopup->addItem("Tile Horizontally", 1, 0, actionTileHorizontal);
    windowListAllPopup->addItem("Cascade", 1, 0, actionCascade);
    windowListAllPopup->addItem("Arrange", 1, 0, actionArrange);
    windowListAllPopup->addItem("Minimize All", 0, "", actionMinimizeAll);
    windowListAllPopup->addItem("Hide All", 0, "", actionHideAll);
    windowListAllPopup->addItem("Undo", 1, "", actionUndoArrange);

    int w = desktop->width();
    int h = desktop->height();

    setGeometry(w / 3, h / 3, w / 3, h / 3);

    windowList = this;
    setWindowTitle("Window list");
    setIconTitle("Window list");
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
    if (focus.type == FocusIn) {
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

void WindowList::configure(int x, int y, unsigned int width, unsigned int height) {
    YFrameClient::configure(x, y, width, height);
    scroll->setGeometry(0, 0, width, height);
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

            px = x - getFrame()->width() / 2;
            py = y - getFrame()->height() / 2;
            if (px + getFrame()->width() > desktop->width())
                px = desktop->width() - getFrame()->width();
            if (py + getFrame()->height() > desktop->height())
                py = desktop->height() - getFrame()->height();
            if (px < 0)
                px = 0;
            if (py < 0)
                py = 0;
            getFrame()->setPosition(px, py);
        }
        getFrame()->setLayer(WinLayerAboveDock);
        getFrame()->setState(WinStateAllWorkspaces, WinStateAllWorkspaces);
        getFrame()->activate(true);
    }
}
#endif
