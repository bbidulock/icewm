/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "wmframe.h"

#include "wmtitle.h"
#include "wmapp.h"
#include "wmcontainer.h"
#include "workspaces.h"
#include "wpixmaps.h"
#include "ymenuitem.h"
#include "yrect.h"
#include "prefs.h"

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

    windowMenu->checkCommand(actionMinimize, isMinimized());
    windowMenu->checkCommand(actionMaximize, isMaximizedFully());
    windowMenu->checkCommand(actionMaximizeVert, isMaximizedVert());
    windowMenu->checkCommand(actionMaximizeHoriz, isMaximizedHoriz());
    windowMenu->checkCommand(actionFullscreen, isFullscreen());
    windowMenu->checkCommand(actionHide, isHidden());
    windowMenu->checkCommand(actionRollup, isRollup());
    windowMenu->checkCommand(actionOccupyAllOrCurrent, isAllWorkspaces());
#if DO_NOT_COVER_OLD
    windowMenu->checkCommand(actionDoNotCover, doNotCover());
#endif
    updateSubmenus();
}

void YFrameWindow::updateSubmenus() {
    YMenuItem *item = windowMenu()->findSubmenu(moveMenu);
    if (item)
        item->setEnabled(!isAllWorkspaces());

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

    layerMenu->updatePopup();
    layerMenu->setActionListener(this);
    int layer = WinLayerCount - 1;
    for (int j(0); j < layerMenu->itemCount(); j++) {
        item = layerMenu->getItem(j);
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

    item = windowMenu()->findAction(actionToggleTray);
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
        fShapeDecors != frameDecors() ||
        fShapeTitle != getTitle())
    {
        fShapeWidth = width();
        fShapeHeight = height();
        fShapeTitleY = titleY();
        fShapeBorderX = borderX();
        fShapeBorderY = borderY();
        fShapeDecors = frameDecors();
        fShapeTitle = getTitle();

        if (shapes.supported &&
            (frameDecors() & fdBorder) &&
            !isIconic() &&
            !isFullscreen())
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
    }
    if (affectsWorkArea()) {
        manager->updateWorkArea();
    }
}

void YFrameWindow::performLayout()
{
    layoutTitleBar();
    layoutResizeIndicators();
    layoutClient();
    layoutShape();
    if (fTitleBar)
        fTitleBar->activate();
}

void YFrameWindow::layoutTitleBar() {
    int height = titleY();
    if (height == 0) {
        if (fTitleBar) {
            delete fTitleBar;
            fTitleBar = nullptr;
        }
    }
    else if (isIconic() == false && titlebar()) {
        int x = borderX(), y = borderY();
        int w = max(1, int(width()) - 2 * x);
        fTitleBar->setGeometry(YRect(x, y, w, height));
        fTitleBar->layoutButtons();
        fTitleBar->show();
    }
}

void YFrameWindow::layoutResizeIndicators() {
    if (hasbits(frameDecors(), fdResize | fdBorder) &&
        hasbit(frameFunctions(), ffResize) &&
        !hasState(WinStateRollup | WinStateMinimized | WinStateFullscreen))
    {
        if (!indicatorsCreated)
            createPointerWindows();
        if (!indicatorsVisible) {
            indicatorsVisible = true;

            XMapWindow(xapp->display(), topSide);
            XMapWindow(xapp->display(), leftSide);
            XMapWindow(xapp->display(), rightSide);
            XMapWindow(xapp->display(), bottomSide);

            XMapWindow(xapp->display(), topLeft);
            XMapWindow(xapp->display(), topRight);
            XMapWindow(xapp->display(), bottomLeft);
            XMapWindow(xapp->display(), bottomRight);
        }
    } else {
        if (indicatorsVisible) {
            indicatorsVisible = false;

            XUnmapWindow(xapp->display(), topSide);
            XUnmapWindow(xapp->display(), leftSide);
            XUnmapWindow(xapp->display(), rightSide);
            XUnmapWindow(xapp->display(), bottomSide);

            XUnmapWindow(xapp->display(), topLeft);
            XUnmapWindow(xapp->display(), topRight);
            XUnmapWindow(xapp->display(), bottomLeft);
            XUnmapWindow(xapp->display(), bottomRight);
        }
    }
    if (!indicatorsVisible)
        return;

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
    if (!isRollup() && !isIconic()) {
        int x = borderX();
        int y = borderY();
        int title = titleY();
        int w = max(1, int(width()) - 2 * x);
        int h = max(1, int(height()) - 2 * y - title);

        fClientContainer->setGeometry(YRect(x, y + title, w, h));
        fClient->setGeometry(YRect(0, 0, w, h));

        long state = fWinState ^ fOldState;
        long mask = WinStateFullscreen | WinStateMaximizedBoth;
        if (hasbit(state, mask)) {
            sendConfigure();
        }
    }
}

bool YFrameWindow::canClose() const {
    if (frameFunctions() & ffClose)
        return true;
    else
        return false;
}

bool YFrameWindow::canMaximize() const {
    if (frameFunctions() & ffMaximize)
        return true;
    else
        return false;
}

bool YFrameWindow::canMinimize() const {
    if (frameFunctions() & ffMinimize)
        return true;
    else
        return false;
}

bool YFrameWindow::canRollup() const {
    if ((frameFunctions() & ffRollup) && titleY() > 0)
        return true;
    else
        return false;
}

bool YFrameWindow::canHide() const {
    if (frameFunctions() & ffHide)
        return true;
    else
        return false;
}

bool YFrameWindow::canLower() const {
    for (YFrameWindow* w = next(); w; w = w->next()) {
        for (YFrameWindow* o = owner(); o != w; o = o->owner()) {
            if (o == nullptr) {
                return true;
            }
        }
    }
    return false;
}

bool YFrameWindow::canRaise() {
    for (YFrameWindow *w = prev(); w; w = w->prev()) {
        if (w->visibleNow()) {
            for (YFrameWindow* o = w; o != this; o = o->owner()) {
                if (o == nullptr) {
                    return true;
                }
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

bool YFrameWindow::overlaps(bool isAbove) {
    YFrameWindow* f = isAbove ? prev() : next();
    for (; f; f = isAbove ? f->prev() : f->next())
        if (overlap(f))
            return true;
    return false;
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
