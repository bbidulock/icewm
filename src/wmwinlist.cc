/*
 * IceWM
 *
 * Copyright (C) 1997-2003 Marko Macek
 *
 * Window list
 */
#include "config.h"
#include "wmwinlist.h"
#include "ymenuitem.h"
#include "prefs.h"
#include "wmframe.h"
#include "wmmgr.h"
#include "wmapp.h"
#include "workspaces.h"
#include "wpixmaps.h"
#include "yxcontext.h"
#include <assert.h>
#include "intl.h"

WindowListProxy windowList;

void ClientListItem::goodbye() {
    if (windowList)
        windowList->removeWindowListApp(this);
}

void ClientListItem::update() {
    if (windowList)
        windowList->updateWindowListApp(this);
}

void ClientListItem::repaint() {
    if (windowList)
        windowList->repaintItem(this);
}

int ClientListItem::getOffset() {
    int ofs = 20;
    Window own = fClient->ownerWindow();
    if (own) {
        YArray<Window> arr;
        YFrameClient* cli = fClient;
        arr.append(cli->handle());
        while (cli && (own = cli->ownerWindow()) != None) {
            if (find(arr, own) >= 0)
                break;
            arr.append(own);
            ofs += 20;
            cli = clientContext.find(own);
        }
    }
    return ofs;
}

int ClientListItem::getOrder() const {
    const WindowOption* wo = fClient->getWindowOption();
    return wo ? wo->order : 0;
}

int ClientListItem::getWorkspace() const {
    YFrameWindow* f = fClient->obtainFrame();
    if (f)
        return f->getWorkspace();
    int ws = 0;
    if (fClient->getNetWMDesktopHint(&ws) && 0 <= ws && ws < workspaceCount)
        return ws;
    return 0;
}

void ClientListItem::activate() {
    YFrameWindow* f = fClient->obtainFrame();
    if (f && fClient != f->client())
        f->selectTab(fClient);
    if (f) {
        f->activateWindow(true, false);
        windowList->handleClose();
    }
}

mstring DesktopListItem::getText() {
    if (getWorkspace() < 0)
        return _("All Workspaces");
    else if (getWorkspace() < workspaceCount)
        return workspaceNames[fDesktop];
    else
        return null;
}

void DesktopListItem::activate() {
    if (fDesktop >= 0) {
        manager->activateWorkspace(fDesktop);
        windowList->handleClose();
    }
}

WindowListBox::WindowListBox(YScrollView *view, YWindow *aParent):
    YListBox(view, aParent),
    fKeyPressed(0)
{
}

WindowListBox::~WindowListBox() {
}

void WindowListBox::activateItem(YListItem *item) {
    item->activate();
}

YArrange WindowListBox::getSelectedWindows() {
    YArray<YFrameWindow*> frames;
    for (IterType iter(getIterator()); ++iter; ) {
        if (isSelected(*iter)) {
            ClientListItem* item = dynamic_cast<ClientListItem*>(*iter);
            if (item) {
                YFrameWindow* frame = item->getClient()->obtainFrame();
                if (frame && find(frames, frame) < 0) {
                    frames += frame;
                }
            }
        }
    }
    const int count = frames.getCount();
    YFrameWindow** copy = new YFrameWindow*[count];
    if (count)
        memcpy(copy, &*frames, count * sizeof(YFrameWindow*));

    return YArrange(copy, count);
}

void WindowListBox::actionPerformed(YAction action, unsigned int modifiers) {
    bool save = focusCurrentWorkspace;
    if (save) focusCurrentWorkspace = false;

    YArrange arrange = getSelectedWindows();
    if (arrange == false) {
    }
    else if (action == actionTileVertical) {
        manager->tileWindows(arrange, true);
    }
    else if (action == actionTileHorizontal) {
        manager->tileWindows(arrange, false);
    }
    else if (action == actionCascade) {
        manager->cascadePlace(arrange);
    }
    else if (action == actionArrange) {
        manager->smartPlace(arrange);
    }
    else if (action == actionClose || action == actionKill
          || action == actionUntab) {
        for (IterType iter(getReverseIterator()); ++iter; ) {
            if (isSelected(*iter)) {
                ClientListItem* item = dynamic_cast<ClientListItem*>(*iter);
                if (item)
                    item->getClient()->actionPerformed(action);
            }
        }
    }
    else {
        if (arrange.size() == 1 && 1 < arrange.begin()[0]->tabCount()) {
            int count = 0;
            YFrameClient* client = nullptr;
            for (IterType iter(getIterator()); ++iter; ) {
                if (isSelected(*iter)) {
                    ClientListItem* item = dynamic_cast<ClientListItem*>(*iter);
                    if (item) {
                        client = item->getClient();
                        count++;
                    }
                }
            }
            if (count == 1 && client->getFrame() == nullptr) {
                YFrameWindow* frame = client->obtainFrame();
                if (frame)
                    frame->selectTab(client);
            }
        }
        for (YFrameWindow* frame : arrange) {
            if ((action != actionHide || !frame->isHidden()) &&
                (action != actionMinimize || !frame->isMinimized())) {
                frame->actionPerformed(action, modifiers);
            }
        }
    }

    if (save) focusCurrentWorkspace = save;
}

bool WindowListBox::handleKey(const XKeyEvent &key) {
    const KeySym k = keyCodeToKeySym(key.keycode);
    const int m = KEY_MODMASK(key.state);

    if (key.type == KeyPress) {
        fKeyPressed = k;
        if (k == XK_Menu || (k == XK_F10 && m == ShiftMask)) {
            YMenu* menu;
            if (hasSelection()) {
                menu = windowList->getWindowListPopup();
                enableCommands(menu);
            } else {
                menu = windowList->getWindowListAllPopup();
            }
            menu->popup(nullptr, nullptr, nullptr, key.x_root, key.y_root,
                        YPopupWindow::pfCanFlipVertical |
                        YPopupWindow::pfCanFlipHorizontal |
                        YPopupWindow::pfPopupMenu);
            return true;
        }
        else if (k == XK_Delete) {
            if (m & ShiftMask)
                actionPerformed(actionKill, key.state);
            else
                actionPerformed(actionClose, key.state);
            return true;
        }
    }
    else if (key.type == KeyRelease) {
        if (k == XK_Escape && k == fKeyPressed && m == 0) {
            windowList->handleClose();
            return true;
        }
    }
    return YListBox::handleKey(key);
}

void WindowListBox::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 3 && count == 1 && xapp->isButton(up.state, Button3Mask)) {
        int no = findItemByPoint(up.x, up.y);

        if (no != -1) {
            YListItem *i = getItem(no);
            if (!isSelected(i)) {
                focusSelectItem(no);
            } else {
                //fFocusedItem = -1;
            }
            YMenu* windowListPopup = windowList->getWindowListPopup();
            enableCommands(windowListPopup);
            windowListPopup->popup(nullptr, nullptr, nullptr,
                                   up.x_root, up.y_root,
                                   YPopupWindow::pfCanFlipVertical |
                                   YPopupWindow::pfCanFlipHorizontal |
                                   YPopupWindow::pfPopupMenu);
        } else {
            YMenu* windowListAllPopup = windowList->getWindowListAllPopup();
            windowListAllPopup->popup(nullptr, nullptr, nullptr,
                                      up.x_root, up.y_root,
                                      YPopupWindow::pfCanFlipVertical |
                                      YPopupWindow::pfCanFlipHorizontal |
                                      YPopupWindow::pfPopupMenu);

        }
        return ;
    }
    YListBox::handleClick(up, count);
}

void WindowListBox::enableCommands(YMenu *popup) {
    bool selected = false;
    bool minified = true;
    bool maxified = true;
    bool horified = true;
    bool vertified = true;
    bool fullscreen = true;
    bool ishidden = true;
    bool rolledup = true;
    bool trayicon = true;
    int workspace = AllWorkspaces;
    bool sameWorkspace = false;
    bool restores = false;
    bool minifies = false;
    bool maxifies = false;
    bool showable = true;
    bool hidable = false;
    bool rollable = false;
    bool raiseable = false;
    bool lowerable = false;
    bool traytoggle = false;
    bool closable = false;
    bool hastabs = false;

    // enable minimize,hide if appropriate
    // enable workspace selections if appropriate

    popup->enableCommand(actionNull);
    for (IterType iter(getIterator()); ++iter; ) {
        if (isSelected(*iter)) {
            ClientListItem* item = dynamic_cast<ClientListItem*>(*iter);
            if (!item)
                continue;
            YFrameWindow* frame = item->getClient()->obtainFrame();
            if (!frame) {
                continue;
            }
            selected = true;

            fullscreen &= frame->isFullscreen();
            minified &= frame->isMinimized();
            maxified &= frame->isMaximizedFully() && !fullscreen;
            vertified &= frame->isMaximizedVert() && !fullscreen && !maxified;
            horified &= frame->isMaximizedHoriz() && !fullscreen && !maxified;
            ishidden &= frame->isHidden();
            rolledup &= frame->isRollup();
            trayicon &= (frame->getTrayOption() != WinTrayIgnore);

            restores |= (frame->canRestore());
            minifies |= (frame->canMinimize() && !frame->isMinimized());
            maxifies |= (frame->canMaximize());
            showable |= (frame->canShow());
            hidable |= (frame->canHide() && !frame->isHidden());
            rollable |= (frame->canRollup());
            raiseable |= (frame->canRaise());
            lowerable |= (frame->canLower());
            traytoggle |= notbit(frame->frameOptions(), YFrameWindow::foIgnoreTaskBar);
            closable |= (frame->canClose());
            hastabs |= (1 < frame->tabCount());

            int ws = frame->getWorkspace();
            if (workspace == AllWorkspaces) {
                workspace = ws;
                sameWorkspace = true;
            } else if (workspace != ws) {
                sameWorkspace = false;
            }
            if (frame->isAllWorkspaces())
                sameWorkspace = false;
            frame->updateSubmenus();
        }
    }
    popup->checkCommand(actionMinimize, selected && minified);
    popup->checkCommand(actionMaximize, selected && maxified);
    popup->checkCommand(actionMaximizeVert, selected && vertified);
    popup->checkCommand(actionMaximizeHoriz, selected && horified);
    popup->checkCommand(actionFullscreen, selected && fullscreen);
    popup->checkCommand(actionHide, selected && ishidden);
    popup->checkCommand(actionRollup, selected && rolledup);
    popup->checkCommand(actionToggleTray, selected && trayicon);
    popup->checkCommand(actionOccupyAllOrCurrent,
                        selected && (workspace == AllWorkspaces));
    if (!restores)
        popup->disableCommand(actionRestore);
    if (!minifies)
        popup->disableCommand(actionMinimize);
    if (!maxifies)
        popup->disableCommand(actionMaximize);
    if (!maxifies)
        popup->disableCommand(actionMaximizeVert);
    if (!maxifies)
        popup->disableCommand(actionMaximizeHoriz);
    if (!showable)
        popup->disableCommand(actionShow);
    if (!hidable)
        popup->disableCommand(actionHide);
    if (!rollable)
        popup->disableCommand(actionRollup);
    if (!raiseable)
        popup->disableCommand(actionRaise);
    if (!lowerable)
        popup->disableCommand(actionLower);
    if (!traytoggle)
        popup->disableCommand(actionToggleTray);
    if (!closable)
        popup->disableCommand(actionClose);
    if (hastabs != popup->haveCommand(actionUntab)) {
        if (hastabs) {
            YMenuItem* after = popup->findSubmenu(moveMenu);
            if (after) {
                YMenuItem* item = new YMenuItem(_("Move to New _Window"), -2,
                                                null, actionUntab, nullptr);
                popup->add(item, after);
            }
        } else {
            popup->removeCommand(actionUntab);
        }
    }

    moveMenu->enableCommand(actionNull);
    if (sameWorkspace && workspace != AllWorkspaces) {
        for (int i = 0; i < moveMenu->itemCount(); i++) {
            YMenuItem *item = moveMenu->getItem(i);
            for (int w = 0; w < workspaceCount; w++)
                if (item && item->getAction() == workspaceActionMoveTo[w])
                    item->setEnabled(w != workspace);
        }
    }
    if (selected == false) {
        popup->disableCommand(actionNull);
    }
    else {
        moveMenu->setActionListener(this);
        tileMenu->setActionListener(this);
        layerMenu->setActionListener(this);
    }
}

WindowList::WindowList(YWindow *aParent):
    YFrameClient(aParent, nullptr, None),
    allWorkspacesItem(nullptr),
    scroll(new YScrollView(this)),
    list(new WindowListBox(scroll, scroll))
{
    addStyle(wsNoExpose);

    if (listbackPixmap != null) {
        scroll->setBackgroundPixmap(listbackPixmap);
    }
    else if (nonempty(clrListBox)) {
        scroll->setBackground(YColorName(clrListBox));
    }

    scroll->setView(list);

    updateWorkspaces();
    setupClient();

    list->show();
    scroll->show();
}

void WindowList::updateWindowListApps() {
    for (YFrameIter frame = manager->focusedIterator(); ++frame; ) {
        frame->addToWindowList();
    }
}

YMenu* WindowList::getWindowListPopup() {
    if (windowListPopup == nullptr) {
        windowListPopup = new WindowListPopup();
        windowListPopup->setActionListener(list);
    }
    return windowListPopup;
}

WindowListPopup::WindowListPopup() {
    addItem(_("_Restore"), -2, gKeyWinRestore.name, actionRestore);
    addItem(_("Mi_nimize"), -2, gKeyWinMinimize.name, actionMinimize);
    addItem(_("Ma_ximize"), -2, gKeyWinMaximize.name, actionMaximize);
    addItem(_("Maximize_Vert"), -2, gKeyWinMaximizeVert.name, actionMaximizeVert);
    addItem(_("MaximizeHori_z"), -2, gKeyWinMaximizeHoriz.name, actionMaximizeHoriz);
    addItem(_("_Fullscreen"), -2, gKeyWinFullscreen.name, actionFullscreen);
    addItem(_("_Show"), -2, null, actionShow);
    addItem(_("_Hide"), -2, gKeyWinHide.name, actionHide);
    addItem(_("Roll_up"), -2, gKeyWinRollup.name, actionRollup);
    addItem(_("_Raise"), -2, gKeyWinRaise.name, actionRaise);
    addItem(_("_Lower"), -2, gKeyWinLower.name, actionLower);
    addSubmenu(_("La_yer"), -2, layerMenu);
    addSubmenu(_("Tile"), -2, tileMenu);
    addSeparator();
    addSubmenu(_("Move _To"), -2, moveMenu);
    addItem(_("Occupy _All"), -2, gKeyWinOccupyAll.name, actionOccupyAllOrCurrent);
    addItem(_("Tray _icon"), -2, null, actionToggleTray);
    addItem(_("R_ename title"), -2, null, actionRename);
    addSeparator();
    addItem(_("Tile _Vertically"), -2, gKeySysTileVertical.name, actionTileVertical);
    addItem(_("T_ile Horizontally"), -2, gKeySysTileHorizontal.name, actionTileHorizontal);
    addItem(_("Ca_scade"), -2, gKeySysCascade.name, actionCascade);
    addItem(_("_Arrange"), -2, gKeySysArrange.name, actionArrange);
    addSeparator();
    addItem(_("_Minimize All"), -2, gKeySysMinimizeAll.name, actionMinimizeAll);
    addItem(_("_Hide All"), -2, gKeySysHideAll.name, actionHideAll);
    addItem(_("_Undo"), -2, gKeySysUndoArrange.name, actionUndoArrange);
    addSeparator();

    YMenu *closeSubmenu = new YMenu();
    assert(closeSubmenu != 0);

    closeSubmenu->addItem(_("_Close"), -2, _("Del"), actionClose);
    closeSubmenu->addSeparator();
    closeSubmenu->addItem(_("_Kill Client"), -2, null, actionKill);
#if 0
    closeSubmenu->addItem(_("_Terminate Process"), -2, 0, actionTermProcess);
    closeSubmenu->addItem(_("Kill _Process"), -2, 0, actionKillProcess);
#endif

    addItem(_("_Close"), -2, actionClose, closeSubmenu);
}

YMenu* WindowList::getWindowListAllPopup() {
    if (windowListAllPopup == nullptr) {
        windowListAllPopup = new WindowListAllPopup();
        windowListAllPopup->setActionListener(list);
    }
    return windowListAllPopup;
}

WindowListAllPopup::WindowListAllPopup() {
    addItem(_("Tile _Vertically"), -2, gKeySysTileVertical.name, actionTileVertical);
    addItem(_("T_ile Horizontally"), -2, gKeySysTileHorizontal.name, actionTileHorizontal);
    addItem(_("Ca_scade"), -2, gKeySysCascade.name, actionCascade);
    addItem(_("_Arrange"), -2, gKeySysArrange.name, actionArrange);
    addItem(_("_Minimize All"), -2, gKeySysMinimizeAll.name, actionMinimizeAll);
    addItem(_("_Hide All"), -2, gKeySysHideAll.name, actionHideAll);
    addItem(_("_Undo"), -2, gKeySysUndoArrange.name, actionUndoArrange);
}

void WindowList::setupClient() {
    int dx, dy;
    unsigned dw, dh;
    desktop->getScreenGeometry(&dx, &dy, &dw, &dh);

    unsigned w = dw;
    unsigned h = dh;

    setGeometry(YRect(w / 4, h / 4, w / 2, h / 2));

    setTitle("WindowList");
    setWindowTitle(_("Window list"));
    setIconTitle(_("Window list"));
    setClassHint("windowList", "IceWM");
    setNetWindowType(_XA_NET_WM_WINDOW_TYPE_DIALOG);

    setWinHintsHint(WinHintsSkipTaskBar |
                    WinHintsSkipWindowMenu);
    setWorkspaceHint(AllWorkspaces);
    setLayerHint(WinLayerAboveDock);
}

WindowList::~WindowList() {
}

void WindowList::updateWorkspaces() {
    if (workspaceItem.isEmpty()) {
        allWorkspacesItem = new DesktopListItem(AllWorkspaces);
        workspaceItem.append(allWorkspacesItem);
        list->addItem(allWorkspacesItem);
    }
    for (int ws = workspaceItem.getCount() - 1; ws < workspaceCount; ++ws) {
        workspaceItem.insert(ws, new DesktopListItem(ws));
        list->addBefore(allWorkspacesItem, workspaceItem[ws]);
    }
    for (int ws = workspaceItem.getCount() - 1; ws > workspaceCount; --ws) {
        list->removeItem(workspaceItem[ws - 1]);
        workspaceItem.remove(ws - 1);
    }
}

void WindowList::handleFocus(const XFocusChangeEvent &focus) {
    if (focus.type == FocusIn && focus.mode != NotifyUngrab) {
        if (width() > 1 && height() > 1 && !getFrame()->isUnmapped()) {
            list->setWindowFocus();
        }
    } else if (focus.type == FocusOut) {
    }
}

void WindowList::relayout() {
    list->repaint();
}

void WindowList::addWindowListApp(YFrameClient* client) {
    if (client && !client->getClientItem()) {
        ClientListItem* item = new ClientListItem(client);
        if (item) {
            client->setClientItem(item);
            insertApp(item);
        }
    }
}

void WindowList::insertApp(ClientListItem* item) {
    const int work = item->getWorkspace();
    const bool valid = inrange(work, 0, workspaceItem.getCount() - 2);
    const int start = valid ? list->findItem(workspaceItem[work])
                            : list->findItem(allWorkspacesItem);
    const int stop = valid ? list->findItem(workspaceItem[work + 1], start + 1)
                           : list->getItemCount();
    const Window ownerWindow = item->getClient()->ownerWindow();
    const Window leader = item->getClient()->clientLeader();
    bool inserted = !(0 <= start && start < stop);

    if (inserted == false && leader == item->getClient()->handle()) {
        YFrameClient* trans = item->getClient()->firstTransient();
        if (trans) {
            int best = stop;
            for (; trans; trans = trans->nextTransient()) {
                YClientItem* ci = trans->getClientItem();
                if (ci) {
                    ClientListItem* li = dynamic_cast<ClientListItem*>(ci);
                    if (li) {
                        int i = list->findItem(li, start + 1, stop);
                        if (i < best && start < i)
                            best = i;
                    }
                }
            }
            if (best < stop && start < best) {
                list->insertAt(best, item);
                inserted = true;
            }
        }
    }
    if (inserted == false && ownerWindow) {
        YFrameClient* owner = clientContext.find(ownerWindow);
        if (owner) {
            ClientListItem* other =
                dynamic_cast<ClientListItem*>(owner->getClientItem());
            if (other) {
                int index = list->findItem(other, start + 1, stop);
                if (start < index && index < stop) {
                    list->insertAt(index + 1, item);
                    inserted = true;
                }
            }
        }
    }
    if (inserted == false && ownerWindow) {
        YFrameClient* trans = YFrameClient::firstTransient(ownerWindow);
        for (; trans; trans = trans->nextTransient()) {
            if (trans != item->getClient()) {
                ClientListItem* other =
                    dynamic_cast<ClientListItem*>(trans->getClientItem());
                if (other) {
                    int index = list->findItem(other, start + 1, stop);
                    if (start < index && index < stop) {
                        list->insertAt(index, item);
                        inserted = true;
                        break;
                    }
                }
            }
        }
    }
    if (inserted == false && leader) {
        int have = 0;
        for (int i = 1 + start; i < stop; ++i) {
            ClientListItem* test =
                dynamic_cast<ClientListItem*>(list->getItem(i));
            if (test && leader == test->getClient()->clientLeader()) {
                have = i + 1;
            }
            if (leader == item->getClient()->handle()) {
                have = i;
                break;
            }
        }
        if (have) {
            list->insertAt(have, item);
            inserted = true;
        }
    }
    if (inserted == false) {
        ClassHint* hint = item->getClient()->classHint();
        const int order = item->getOrder();
        int best = 1 + start;
        for (int i = best; i < stop && inserted == false; ++i) {
            ClientListItem* test =
                dynamic_cast<ClientListItem*>(list->getItem(i));
            if (test) {
                ClassHint* klas = test->getClient()->classHint();
                int value = test->getOrder();
                int cmp = order - value;
                if (cmp == 0 && hint != klas) {
                    if (hint == nullptr)
                        cmp = +1;
                    else if (klas == nullptr)
                        cmp = -1;
                    else if (nonempty(hint->res_class)) {
                        if (isEmpty(klas->res_class))
                            cmp = -1;
                        else
                            cmp = strcmp(hint->res_class,
                                         klas->res_class);
                    }
                    else if (nonempty(klas->res_class))
                        cmp = +1;
                    if (cmp == 0) {
                        if (nonempty(hint->res_name)) {
                            if (isEmpty(klas->res_name))
                                cmp = -1;
                            else
                                cmp = strcmp(hint->res_name,
                                             klas->res_name);
                        }
                        else if (nonempty(klas->res_name))
                            cmp = +1;
                    }
                }
                if (cmp < 0) {
                    list->insertAt(i, item);
                    inserted = true;
                }
            }
        }
        if (inserted == false) {
            list->insertAt(stop, item);
            inserted = true;
        }
    }
    if (inserted == false) {
        list->addItem(item);
    }
}

void WindowList::removeWindowListApp(ClientListItem* item) {
    if (item) {
        list->removeItem(item);
        delete item;
    }
}

void WindowList::updateWindowListApp(ClientListItem* item) {
    if (item) {
        list->removeItem(item);
        insertApp(item);
    }
}

void WindowList::repaintItem(ClientListItem* item) {
    if (item) {
        list->repaintItem(item);
    }
}

void WindowList::configure(const YRect2& r) {
    if (r.resized()) {
        scroll->setGeometry(YRect(0, 0, r.width(), r.height()));
    }
}

void WindowList::handleClose() {
    if (!getFrame()->isHidden())
        getFrame()->wmHide();
}

void WindowList::showFocused(int x, int y) {
    const YFrameWindow *focus = manager->getFocus();
    if (focus != getFrame() || focus == nullptr) {
        YListItem* item = nullptr;
        if (focus && focus->client()->getClientItem()) {
            item = dynamic_cast<YListItem*>(focus->client()->getClientItem());
        }
        if (item == nullptr) {
            item = workspaceItem[manager->activeWorkspace()];
        }
        list->focusSelectItem(list->findItem(item));
    }
    if (getFrame() == nullptr) {
        int scn = desktop->getScreenForRect(x, y, 1, 1);
        YRect geo(desktop->getScreenGeometry(scn));
        long dw = geo.width();
        long dh = long(geo.height() - max(100U, geo.height() / 10));
        long line = list->getLineHeight();
        long need = line * (1L + workspaceCount + manager->focusedCount());
        unsigned w = unsigned(dw / 2);
        unsigned h = unsigned(max(dh / 2, clamp(need, line + 1L, dh)));
        int x = int((dw - w) / 2);
        int y = int((dh - h) / 2);
        setGeometry(YRect(x, y, w, h));
        manager->manageClient(this);
    }
    if (getFrame()) {
        if (x == -1 && y == -1) {
            int ix, iy, iw, ih;
            getFrame()->getNormalGeometryInner(&ix, &iy, &iw, &ih);
            YRect f(ix, iy, iw, ih);
            YRect r(desktop->getScreenGeometry(getFrame()->getScreen()));
            if (r.overlap(f) * 4 < f.pixels()) {
                x = r.xx + r.ww / 2;
                y = r.yy + r.hh / 2;
            }
        }
        if (x != -1 && y != -1) {
            int px, py;

            int xiscreen = desktop->getScreenForRect(x, y, 1, 1);
            int dx, dy;
            unsigned uw, uh;
            desktop->getScreenGeometry(&dx, &dy, &uw, &uh, xiscreen);
            int dw = int(uw), dh = int(uh);

            px = x - int(getFrame()->width() / 2);
            py = y - int(getFrame()->height() / 2);
            if (px + int(getFrame()->width()) > dx + dw)
                px = dx + dw - int(getFrame()->width());
            if (py + int(getFrame()->height()) > dy + dh)
                py = dx + dh - int(getFrame()->height());
            if (px < dx)
                px = dx;
            if (py < dy)
                py = dy;
            getFrame()->setNormalPositionOuter(px, py);
        }
        getFrame()->setRequestedLayer(WinLayerAboveDock);
        getFrame()->setAllWorkspaces();
        getFrame()->wmRaise();
        getFrame()->wmShow();
        xapp->sync();
        focusTimer.setTimer(None, this, true);
    }
}

bool WindowList::handleTimer(YTimer* timer) {
    if (timer == &focusTimer) {
        if (visible()) {
            manager->setFocus(getFrame());
        }
        return false;
    } else {
        return YFrameClient::handleTimer(timer);
    }
}

WindowList* WindowListProxy::acquire() {
    if (wlist == nullptr) {
        wlist = new WindowList(desktop);
        wlist->updateWindowListApps();
    }
    return wlist;
}

// vim: set sw=4 ts=4 et:
