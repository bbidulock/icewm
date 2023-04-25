/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "wmframe.h"
#include "wmmgr.h"
#include "wmtitle.h"
#include "wmapp.h"
#include "wmcontainer.h"
#include "wmtaskbar.h"
#include "workspaces.h"
#include "wpixmaps.h"
#include "ymenuitem.h"
#include "yrect.h"
#include "prefs.h"
#include "intl.h"

void YFrameWindow::updateMenu() {
    YMenu *windowMenu = this->windowMenu();
    // enable all commands
    windowMenu->setActionListener(this);
    windowMenu->enableCommand(actionNull);

    if (!canMaximize()) {
        windowMenu->disableCommand(actionMaximize);
        windowMenu->disableCommand(actionMaximizeVert);
        windowMenu->disableCommand(actionMaximizeHoriz);
    }
    if (!canMinimize())
        windowMenu->disableCommand(actionMinimize);
    if (!canRestore())
        windowMenu->disableCommand(actionRestore);
    if (isMinimized() || isHidden() || !canSize() || !visibleNow())
        windowMenu->disableCommand(actionSize);
    if (isMinimized() || isHidden() || !canMove() || !visibleNow())
        windowMenu->disableCommand(actionMove);
    if (!canLower())
        windowMenu->disableCommand(actionLower);
    if (!canRaise())
        windowMenu->disableCommand(actionRaise);
    if (!canHide())
        windowMenu->disableCommand(actionHide);
    if (!canRollup())
        windowMenu->disableCommand(actionRollup);
    if (!canClose())
        windowMenu->disableCommand(actionClose);

    bool full = isFullscreen();
    bool vert = !full && isMaximizedVert();
    bool hori = !full && isMaximizedHoriz();
    windowMenu->checkCommand(actionMinimize, isMinimized());
    windowMenu->checkCommand(actionMaximize, vert && hori);
    windowMenu->checkCommand(actionMaximizeVert, vert && !hori);
    windowMenu->checkCommand(actionMaximizeHoriz, hori && !vert);
    windowMenu->checkCommand(actionFullscreen, full);
    windowMenu->checkCommand(actionHide, isHidden());
    windowMenu->checkCommand(actionRollup, isRollup());
    windowMenu->checkCommand(actionOccupyAllOrCurrent, isAllWorkspaces());
#if DO_NOT_COVER_OLD
    windowMenu->checkCommand(actionDoNotCover, doNotCover());
#endif
    updateSubmenus();
}

void YFrameWindow::updateSubmenus() {
    if (moveMenu) {
        YMenuItem *item = windowMenu()->findSubmenu(moveMenu);
        if (item) {
            bool enable = !isAllWorkspaces() && 1 < workspaceCount;
            item->setEnabled(enable);
            if (enable) {
                moveMenu->updatePopup();
                moveMenu->setActionListener(this);

                for (int i(0); i < moveMenu->itemCount(); i++) {
                    item = moveMenu->getItem(i);
                    if (item && item->getAction() == workspaceActionMoveTo[i]) {
                        bool const e(i == getWorkspace());
                        item->setEnabled(!e);
                        item->setChecked(e);
                    }
                }
            }
        }
    }

    if (layerMenu) {
        YMenuItem *item = windowMenu()->findSubmenu(layerMenu);
        if (item) {
            layerMenu->updatePopup();
            layerMenu->setActionListener(this);

            int layer = WinLayerCount - 1;
            for (int j(0); j < layerMenu->itemCount(); j++) {
                YMenuItem* item = layerMenu->getItem(j);
                YAction action = item->getAction();
                while (action < layerActionSet[layer] && 0 < layer) {
                    --layer;
                }
                if (action == layerActionSet[layer]) {
                    bool const e(layer == getActiveLayer());
                    item->setEnabled(!e);
                    item->setChecked(e);
                }
            }
        }
    }

    if (tileMenu) {
        YMenuItem *item = windowMenu()->findSubmenu(tileMenu);
        if (item) {
            bool enable = canMove();
            item->setEnabled(enable);
            if (enable) {
                tileMenu->updatePopup();
                tileMenu->setActionListener(this);
            }
        }
    }

    if (tabCount() > 1 && (TabsMenu *) tabsMenu != nullptr) {
        if (windowMenu()->findSubmenu(tabsMenu) == nullptr) {
            YMenu* menus[] = {
                moveMenu._ptr(), tileMenu._ptr(), layerMenu._ptr()
            };
            for (YMenu* menu : menus) {
                if (menu) {
                    YMenuItem* item = windowMenu()->findSubmenu(menu);
                    if (item) {
                        windowMenu()->addSubmenu(_("Tabs"), -2, tabsMenu, item);
                        break;
                    }
                }
            }
        }
    }
    else if (tabsMenu) {
        windowMenu()->removeSubmenu(tabsMenu);
        tabsMenu = null;
    }

    YMenuItem* item = windowMenu()->findAction(actionToggleTray);
    if (item) {
        bool checked = (getTrayOption() != WinTrayIgnore);
        bool enabled = notbit(frameOptions(), foIgnoreTaskBar);
        item->setChecked(checked);
        item->setEnabled(enabled || checked);
    }

#if 0
    if (trayMenu) for (int k(0); k < trayMenu->itemCount(); k++) {
        item = trayMenu->getItem(k);
        for (int opt(0); opt < WinTrayOptionCount; opt++)
            if (item && item->getAction() == trayOptionActionSet[opt]) {
                bool const e(opt == getTrayOption());
                item->setEnabled(!e);
                item->setChecked(e);
            }
    }
#endif
}

void YFrameWindow::setShape() {
#ifdef CONFIG_SHAPE
    if (!shapes.supported)
        return ;

    if (client()->shaped()) {
        MSG(("setting shape w=%d, h=%d", width(), height()));
        if (isRollup() || isIconic()) {
            XRectangle full;
            full.x = 0;
            full.y = 0;
            full.width = width();
            full.height = height();
            XShapeCombineRectangles(xapp->display(), handle(),
                                    ShapeBounding,
                                    0, 0, &full, 1,
                                    ShapeSet, Unsorted);
        } else {
            XRectangle rect[6];
            int nrect = 0;

            if ((frameDecors() & (fdResize | fdBorder)) == fdResize + fdBorder) {
                rect[0].x = 0;
                rect[0].y = 0;
                rect[0].width = width();
                rect[0].height = borderY();

                rect[1] = rect[0];
                rect[1].y = height() - borderY();

                rect[2].x = 0;
                rect[2].y = borderY();
                rect[2].width = borderX();
                rect[2].height = height() - 2 * borderY();

                rect[3] = rect[2];
                rect[3].x = width() - borderX();

                nrect = 4;
            }

            if (titleY() > 0) {
                rect[nrect].x = borderX();
                rect[nrect].y = borderY();
                rect[nrect].width  = width() - 2 * borderX();
                rect[nrect].height = titleY();
                nrect++;
            }

            if (nrect !=  0)
                XShapeCombineRectangles(xapp->display(), handle(),
                                        ShapeBounding,
                                        0, 0, rect, nrect,
                                        ShapeSet, Unsorted);
            XShapeCombineShape(xapp->display(), handle(),
                               ShapeBounding,
                               borderX(),
                               borderY() + titleY(),
                               client()->handle(),
                               ShapeBounding, nrect ? ShapeUnion : ShapeSet);
        }
    }
#endif
}

void YFrameWindow::layoutShape() {
#ifdef CONFIG_SHAPE
    if (fShapeWidth != width() ||
        fShapeHeight != height() ||
        fShapeTitleY != titleY() ||
        fShapeBorderX != borderX() ||
        fShapeBorderY != borderY() ||
        fShapeTabCount != tabCount() ||
        fShapeDecors != frameDecors() ||
        fShapeTitle != getTitle() ||
        fShapeLessTabs != lessTabs() ||
        fShapeMoreTabs != moreTabs())
    {
        fShapeWidth = width();
        fShapeHeight = height();
        fShapeTitleY = titleY();
        fShapeBorderX = borderX();
        fShapeBorderY = borderY();
        fShapeTabCount = tabCount();
        fShapeDecors = frameDecors();
        fShapeTitle = getTitle();
        fShapeLessTabs = lessTabs();
        fShapeMoreTabs = moreTabs();

        if (shapes.supported &&
            hasBorders() &&
            !isIconic())
        {
            int const a(focused());
            int const t((frameDecors() & fdResize) ? 0 : 1);

            Pixmap shape = XCreatePixmap(xapp->display(), desktop->handle(),
                                         width(), height(), 1);
            Graphics g(shape, width(), height(), 1);

            g.setColorPixel(1);
            g.fillRect(0, 0, width(), height());

            const int
                xTL(frameTL[t][a] != null ? frameTL[t][a]->width() : 0),
                xTR(width() - (frameTR[t][a] != null ? frameTR[t][a]->width() : 0)),
                xBL(frameBL[t][a] != null ? frameBL[t][a]->width() : 0),
                xBR(width() - (frameBR[t][a] != null ? frameBR[t][a]->width() : 0));
            const int
                yTL(frameTL[t][a] != null ? frameTL[t][a]->height() : 0),
                yBL(height() - (frameBL[t][a] != null ? frameBL[t][a]->height() : 0)),
                yTR(frameTR[t][a] != null ? frameTR[t][a]->height() : 0),
                yBR(height() - (frameBR[t][a] != null ? frameBR[t][a]->height() : 0));

            if (frameTL[t][a] != null) {
                g.copyDrawable(frameTL[t][a]->mask(), 0, 0,
                               frameTL[t][a]->width(), frameTL[t][a]->height(),
                               0, 0);
                if (protectClientWindow)
                    g.fillRect(borderX(), borderY(),
                               frameTL[t][a]->width() - borderX(),
                               frameTL[t][a]->height() - borderY());
            }
            if (frameTR[t][a] != null) {
                g.copyDrawable(frameTR[t][a]->mask(), 0, 0,
                               frameTR[t][a]->width(), frameTR[t][a]->height(),
                               xTR, 0);
                if (protectClientWindow)
                    g.fillRect(xTR, borderY(),
                               frameTR[t][a]->width() - borderX(),
                               frameTR[t][a]->height() - borderY());
            }
            if (frameBL[t][a] != null) {
                g.copyDrawable(frameBL[t][a]->mask(), 0, 0,
                               frameBL[t][a]->width(), frameBL[t][a]->height(),
                               0, yBL);
                if (protectClientWindow)
                    g.fillRect(borderX(), yBL,
                               frameBL[t][a]->width() - borderX(),
                               frameBL[t][a]->height() - borderY());
            }
            if (frameBR[t][a] != null) {
                g.copyDrawable(frameBR[t][a]->mask(), 0, 0,
                               frameBR[t][a]->width(), frameBL[t][a]->height(),
                               xBR, yBR);
                if (protectClientWindow)
                    g.fillRect(xBR, yBR,
                               frameBR[t][a]->width() - borderX(),
                               frameBR[t][a]->width() - borderY());
            }

            if (frameT[t][a] != null)
                g.repHorz(frameT[t][a]->mask(),
                          frameT[t][a]->width(), frameT[t][a]->height(),
                          xTL, 0, xTR - xTL);
            if (frameB[t][a] != null)
                g.repHorz(frameB[t][a]->mask(),
                          frameB[t][a]->width(), frameB[t][a]->height(),
                          xBL, height() - frameB[t][a]->height(), xBR - xBL);
            if (frameL[t][a] != null)
                g.repVert(frameL[t][a]->mask(),
                          frameL[t][a]->width(), frameL[t][a]->height(),
                          0, yTL, yBL - yTL);
            if (frameR[t][a] != null)
                g.repVert(frameR[t][a]->mask(),
                          frameR[t][a]->width(), frameR[t][a]->height(),
                          width() - frameR[t][a]->width(), yTR, yBR - yTR);

            if (titleY() && titlebar())
                titlebar()->renderShape(g);
            XShapeCombineMask(xapp->display(), handle(),
                              ShapeBounding, 0, 0, shape, ShapeSet);
            XFreePixmap(xapp->display(), shape);
        } else {
            XShapeCombineMask(xapp->display(), handle(),
                              ShapeBounding, 0, 0, None, ShapeSet);
        }
        setShape();
    }
#endif
}

void YFrameWindow::configure(const YRect2& r) {
    MSG(("%s %d %d %d %d", __func__, r.x(), r.y(), r.width(), r.height()));

    if (r.resized()) {
        performLayout();
        if (taskBar)
            taskBar->workspacesRepaint(getWorkspace());
    }
    if (affectsWorkArea()) {
        manager->updateWorkArea();
    }
}

void YFrameWindow::performLayout()
{
    layoutTitleBar();
    layoutClient();
    layoutShape();
    if (fTitleBar)
        fTitleBar->activate();
    layoutResizeIndicators();
}

void YFrameWindow::layoutTitleBar() {
    if (titleY()) {
        if (fTitleBar) {
            fTitleBar->relayout();
        } else {
            fTitleBar = new YFrameTitleBar(this);
        }
    }
    else if (fTitleBar) {
        delete fTitleBar;
        fTitleBar = nullptr;
    }
}

void YFrameWindow::layoutResizeIndicators() {
    if (isUnmapped() || !hasBorders() || !isResizable()) {
        if (indicatorsCreated) {
            Window* indicators[] = {
                &topSide, &leftSide, &rightSide, &bottomSide,
                &topLeft, &topRight, &bottomLeft, &bottomRight
            };
            for (Window* window : indicators) {
                XDestroyWindow(xapp->display(), *window);
                *window = None;
            }
            indicatorsCreated = false;
        }
        return;
    }
    if (indicatorsCreated == false)
        createPointerWindows();

    int vo = int(min(height(), topSideVerticalOffset));
    int ww(max(3, (int) width()));
    int hh(max(3, (int) height() - vo));
    int bx(max(1, (int) borderX()));
    int bt(max(2, (int) borderY() - vo));
    int bb(max(1, (int) borderY()));
    int cx(max(1, (int) wsCornerX));
    int cy(max(1, (int) wsCornerY));
    int xx(min(cx, ww / 2));
    int yy(min(cy, hh / 2));

    XMoveResizeWindow(xapp->display(), topSide,
                      xx, vo, max(1, ww - 2 * xx), bt);
    XMoveResizeWindow(xapp->display(), leftSide,
                      0, yy + vo, bx, max(1, hh - 2 * yy));
    XMoveResizeWindow(xapp->display(), rightSide,
                      ww - bx, yy + vo, bx, max(1, hh - 2 * yy));
    XMoveResizeWindow(xapp->display(), bottomSide,
                      xx, hh - bb + vo, max(1, ww - 2 * xx), bb);

    XMoveResizeWindow(xapp->display(), topLeft,
                      0, vo, xx, yy);
    XMoveResizeWindow(xapp->display(), topRight,
                      ww - xx, vo, xx, yy);
    XMoveResizeWindow(xapp->display(), bottomLeft,
                      0, hh - yy + vo, xx, yy);
    XMoveResizeWindow(xapp->display(), bottomRight,
                      ww - xx, hh - yy + vo, xx, yy);

    XRaiseWindow(xapp->display(), topSide);
}

void YFrameWindow::layoutClient() {
    if (!isRollup()) {
        int x = borderX();
        int y = borderY();
        int title = titleY();
        int w = max(1, int(width()) - 2 * x);
        int h = max(1, int(height()) - 2 * y - title);
        bool moved = (x != container()->x() || !fManaged ||
                      y != container()->y() - title);

        container()->setGeometry(YRect(x, y + title, w, h));
        client()->setGeometry(YRect(0, 0, w, h));

        if (moved) {
            sendConfigure();
            client()->setNetFrameExtents(x, x, y + title, y);
        }
    }
}

bool YFrameWindow::isGroupModalFor(const YFrameWindow* other) const {
    bool have = false;
    if (hasState(WinStateModal) && !client()->isTransient()) {
        Window leader = client()->clientLeader();
        if (leader && leader == other->client()->clientLeader()) {
            bool self = false, that = false;
            for (auto& modal : groupModals) {
                if (modal == this) {
                    self = true;
                }
                if (modal == other) {
                    that = true;
                }
            }
            have = self && !that;
        }
    }
    return have;
}

bool YFrameWindow::isTransientFor(const YFrameWindow* other) const {
    return client() && other->client()
        && client()->isTransientFor(other->client()->handle());
}

bool YFrameWindow::canLower() const {
    for (YFrameWindow* w = next(); w; w = w->next()) {
        if (isTransientFor(w) == false && isGroupModalFor(w) == false) {
            return true;
        }
    }
    return false;
}

bool YFrameWindow::canRaise(bool ignoreTaskBar) const {
    for (YFrameWindow *w = prev(); w; w = w->prev()) {
        if (w->visibleNow() || w->visibleOn(getWorkspace())) {
            if (w->isTransientFor(this) == false &&
                w->isGroupModalFor(this) == false &&
                (ignoreTaskBar == false || w->client() != taskBar)) {
                return true;
            }
        }
    }
    return false;
}

unsigned YFrameWindow::overlap(YFrameWindow* f) {
    if (false == f->isHidden() &&
        false == f->isMinimized() &&
        f->visibleNow())
    {
        return geometry().intersect(f->geometry()).pixels();
    }
    return 0;
}

bool YFrameWindow::overlaps(YFrameWindow* f) {
    return f->visible() &&
            (x() < f->x()
                 ? f->x() < x() + int(width())
                 : x() < f->x() + int(f->width())) &&
            (y() < f->y()
                 ? f->y() < y() + int(height())
                 : y() < f->y() + int(f->height()));
}

bool YFrameWindow::overlapping() {
    for (YFrameWindow* f = next(); f; f = f->next())
        if (overlaps(f))
            return true;
    return false;
}

bool YFrameWindow::overlapped() {
    for (YFrameWindow* f = prev(); f; f = f->prev())
        if (overlaps(f) && client()->getOwner() != f->client())
            return true;
    return false;
}

bool YFrameWindow::hasBorders() const {
    return hasbit(frameDecors(), fdBorder) && !isFullscreen() &&
         !(hideBordersMaximized && isMaximizedFully()) &&
         (hasbit(frameDecors(), fdResize) ?
          (wsBorderX | wsBorderY) != 0 :
          (wsDlgBorderX | wsDlgBorderY) != 0);
}

/// TODO #warning "should precalculate these"
int YFrameWindow::borderX() const {
    return
        isFullscreen() ? 0 : borderXN();
}

int YFrameWindow::borderXN() const {
    return
        ((frameDecors() & fdBorder) && !(hideBordersMaximized && isMaximizedFully()))
        ? ((frameDecors() & fdResize) ? wsBorderX : wsDlgBorderX)
        : 0;
}

int YFrameWindow::borderY() const {
    return
        isFullscreen() ? 0 : borderYN();
}

int YFrameWindow::borderYN() const {
    return
        ((frameDecors() & fdBorder) && !(hideBordersMaximized && isMaximizedFully()))
        ? ((frameDecors() & fdResize) ? wsBorderY : wsDlgBorderY)
        : 0;
}

int YFrameWindow::titleY() const {
    return isFullscreen() ? 0 : titleYN();
}

int YFrameWindow::titleYN() const {
    if (hideTitleBarWhenMaximized && isMaximizedVert())
        return 0;
    return (frameDecors() & fdTitleBar) ? wsTitleBar : 0;
}

// vim: set sw=4 ts=4 et:
