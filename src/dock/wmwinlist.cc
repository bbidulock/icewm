/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 *
 * Window list
 */
#include "config.h"
#include "yfull.h"
//#include "ykey.h"
#include "wmdesktop.h"
#include "wmwinlist.h"
#include "ymenuitem.h"

//#include "wmaction.h"
//#include "wmclient.h"
//#include "wmframe.h"
//#include "wmmgr.h"
//#include "wmapp.h"
#include "yapp.h"
#include "dockaction.h"
#include "sysdep.h"

//WindowList *windowList = 0;

YMenu *windowListMenu = 0;
YMenu *windowListPopup = 0;
YMenu *windowListAllPopup = 0;

WindowListItem::WindowListItem(WindowInfo *frame): YListItem() {
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
    WindowInfo *w = getFrame();

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


WindowListBox::WindowListBox(WindowList *windowList, YScrollView *view, YWindow *aParent):
    YListBox(view, aParent)
{
    fWindowList = windowList;
}

WindowListBox::~WindowListBox() {
}

void WindowListBox::activateItem(YListItem *item) {
    WindowListItem *i = (WindowListItem *)item;
    WindowInfo *f = i->getFrame();
    if (f) {
        f->activateWindow(true);
#if 0
        fWindowList->getFrame()->wmHide();
#endif
    }
}

bool WindowListBox::handleKeySym(const XKeyEvent &key, KeySym ksym, int vmod) {
    if (key.type == KeyPress) {
        switch (ksym) {
        case XK_Escape:
#if 0
            fWindowList->getFrame()->wmHide();
#endif
            return true;
        case XK_F10:
        case XK_Menu:
            if (ksym != XK_F10 || vmod == kfShift) {
                if (hasSelection()) {
#if 0
                    moveMenu->enableCommand(0);
#endif
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
                fWindowList->actionPerformed(actionClose, key.state);
            }
            break;
        }
    }
    return YListBox::handleKeySym(key, ksym, vmod);
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
#if 0
            moveMenu->enableCommand(0);
#endif
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

WindowList::WindowList(YWindowManager *root, YWindow *aParent): YWindow(aParent, 0) {
    fRoot = root;
    scroll = new YScrollView(this);
    list = new WindowListBox(this, scroll, scroll);
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
    windowListPopup->setActionListener(this);
    windowListPopup->addItem("Show", 0, 0, actionShow);
    windowListPopup->addItem("Hide", 0, 0, actionHide);
    windowListPopup->addItem("Minimize", 0, 0, actionMinimize);
#if 0
    windowListPopup->addSubmenu("Move To", 5, moveMenu);
#endif
    windowListPopup->addSeparator();
    windowListPopup->addItem("Tile Vertically", 5, 0, actionTileVertical);
    windowListPopup->addItem("Tile Horizontally", 1, 0, actionTileHorizontal);
    windowListPopup->addItem("Cascade", 1, 0, actionCascade);
    windowListPopup->addItem("Arrange", 1, 0, actionArrange);
    windowListPopup->addSeparator();
    windowListPopup->addItem("Close", 0, actionClose, closeSubmenu);

    windowListAllPopup = new YMenu();
    windowListAllPopup->setActionListener(this);
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

#if 0
    setWindowTitle("Window list");
    setIconTitle("Window list");
#endif
#if 0
#ifdef GNOME1_HINTS
    setWinStateHint(WinStateAllWorkspaces, WinStateAllWorkspaces);
    setWinWorkspaceHint(0);
    setWinLayerHint(WinLayerAboveDock);
#endif
#endif
}

WindowList::~WindowList() {
    delete windowListPopup; windowListPopup = 0;
    delete windowListAllPopup; windowListAllPopup = 0;
    delete list; list = 0;
    delete scroll; scroll = 0;
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

WindowListItem *WindowList::addWindowListApp(WindowInfo *frame) {
#if 0
    if (!frame->client()->adopted())
        return 0;
#endif
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
    YWindow::configure(x, y, width, height);
    scroll->setGeometry(0, 0, width, height);
}

void WindowList::handleClose() {
    hide();
#if 0
    if (!getFrame()->isHidden())
        getFrame()->wmHide();
#endif
}

void WindowList::showFocused(int x, int y) {
    setPosition(x, y);
    show();
#if 0
    WindowInfo *f = fRoot->getFocus();

    if (f != getFrame()) {
        if (f)
            list->focusSelectItem(list->findItem(f->winListItem()));
        else
            list->focusSelectItem(0);
    }
    if (getFrame() == 0)
        fRoot->manageClient(handle(), false);
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
#endif
}

void WindowList::actionPerformed(YAction *action, unsigned int modifiers) {
#if 0
    if (action == actionTileVertical ||
        action == actionTileHorizontal)
    {
        if (list->hasSelection()) {
            YListItem *i;
            int count = 0;
            WindowInfo **w;

            for (i = list->getFirst(); i; i = i->getNext())
                if (list->isSelected(i))
                    count++;

            if (count > 0) {
                w = new WindowInfo *[count];
                if (w) {
                    int n = 0;

                    for (i = list->getFirst(); i; i = i->getNext())
                        if (list->isSelected(i)) {
                            WindowListItem *item = (WindowListItem *)i;
                            w[n++] = (WindowInfo *)item->getFrame();
                        }
                    PRECONDITION(n == count);

#if 0
                    fRoot->tileWindows(w, count,
                                       (action == actionTileVertical) ? true : false);
#endif
                    delete w;
                }
            }
        }
    } else if (action == actionCascade ||
               action == actionArrange)
    {
        if (list->hasSelection()) {
            WindowInfo *f;
            YListItem *i;
            int count = 0;
            WindowInfo **w;

            for (f = fRoot->topLayer(); f; f = f->nextLayer()) {
                i = f->winListItem();
                if (i && list->isSelected(i))
                    count++;
            }
            if (count > 0) {
                w = new WindowInfo *[count];
                if (w) {
                    int n = 0;
                    for (f = fRoot->topLayer(); f; f = f->nextLayer()) {
                        i = f->winListItem();
                        if (i && list->isSelected(i))
                            w[n++] = f;
                    }

                    if (action == actionCascade) {
                        fRoot->cascadePlace(w, count);
                    } else if (action == actionArrange) {
                        fRoot->smartPlace(w, count);
                    }
                    delete w;
                }
            }
        }
    } else {
        if (list->hasSelection()) {
            YListItem *i;

            for (i = list->getFirst(); i; i = i->getNext()) {
                if (list->isSelected(i)) {
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
#endif
}
