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
#include "wpixmaps.h"
#include "ymenuitem.h"
#include "yrect.h"
#include "prefs.h"

void YFrameWindow::updateMenu() {
    YMenu *windowMenu = this->windowMenu();
    // enable all commands
    windowMenu->setActionListener(this);
    windowMenu->enableCommand(actionNull);

    if (!canMaximize())
        windowMenu->disableCommand(actionMaximize);
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

    YMenuItem *item;
    if ((item = windowMenu->findSubmenu(moveMenu)))
        item->setEnabled(!isAllWorkspaces());

    moveMenu->setActionListener(this);
    for (int i(0); i < moveMenu->itemCount(); i++) {
        item = moveMenu->getItem(i);
        if (item && item->getAction() == workspaceActionMoveTo[i]) {
            bool const e(i == getWorkspace());
            item->setEnabled(!e);
            item->setChecked(e);
        }
    }

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

    if ((item = windowMenu->findAction(actionToggleTray))) {
        bool enabled = false == (frameOptions() & foIgnoreTaskBar);
        bool checked = enabled && (getTrayOption() != WinTrayIgnore);
        item->setChecked(checked);
        item->setEnabled(enabled);
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

#ifdef CONFIG_SHAPE
void YFrameWindow::setShape() {
    if (!shapesSupported)
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
}
#endif

void YFrameWindow::layoutShape() {

    if (fShapeWidth != width() ||
        fShapeHeight != height() ||
        fShapeTitleY != titleY() ||
        fShapeBorderX != borderX() ||
        fShapeBorderY != borderY())
    {
        fShapeWidth = width();
        fShapeHeight = height();
        fShapeTitleY = titleY();
        fShapeBorderX = borderX();
        fShapeBorderY = borderY();

#ifdef CONFIG_SHAPE
        if (shapesSupported &&
            (frameDecors() & fdBorder) &&
            !(isIconic() || isFullscreen()))
        {
            int const a(focused() ? 1 : 0);
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

            if (titlebar() && titleY())
                titlebar()->renderShape(shape);
            XShapeCombineMask(xapp->display(), handle(),
                              ShapeBounding, 0, 0, shape, ShapeSet);
            XFreePixmap(xapp->display(), shape);
        } else {
            XShapeCombineMask(xapp->display(), handle(),
                              ShapeBounding, 0, 0, None, ShapeSet);
        }
#endif
#ifdef CONFIG_SHAPE
        setShape();
#endif
    }
}

void YFrameWindow::configure(const YRect &r) {
    MSG(("configure %d %d %d %d", r.x(), r.y(), r.width(), r.height()));

    YWindow::configure(r);

    performLayout();
    if (affectsWorkArea()) {
        manager->updateWorkArea();
    }

    sendConfigure();
}

void YFrameWindow::performLayout()
{
    layoutTitleBar();
    layoutResizeIndicators();
    layoutClient();
    layoutShape();
}

void YFrameWindow::layoutTitleBar() {
    if (titlebar() == 0)
        return;

    if (titleY() == 0) {
        titlebar()->hide();
    } else {
        titlebar()->show();

        int title_width = width() - 2 * borderX();
        titlebar()->setGeometry(
            YRect(borderX(),
                  borderY(),
                  max(1, title_width),
                  titleY()));

        titlebar()->layoutButtons();
    }
}

void YFrameWindow::layoutResizeIndicators() {
    if (((frameDecors() & (fdResize | fdBorder)) == (fdResize | fdBorder)) &&
        !isRollup() && !isMinimized() && (frameFunctions() & ffResize) &&
        !isFullscreen())
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

            XMapWindow(xapp->display(), topLeftSide);
            XMapWindow(xapp->display(), topRightSide);
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

            XUnmapWindow(xapp->display(), topLeftSide);
            XUnmapWindow(xapp->display(), topRightSide);
        }
    }
    if (!indicatorsVisible)
        return;

    int ww(max(3, (int) width()));
    int hh(max(3, (int) height()));
    int bx(max(1, (int) borderX()));
    int by(max(1, (int) borderY()));
    int cx(max(1, (int) wsCornerX));
    int cy(max(1, (int) wsCornerY));
    int xx(max(1, min(cx, ww / 2)));
    int yy(max(1, min(cy, hh / 2)));

    XMoveResizeWindow(xapp->display(), topSide,
                      xx, 0, max(1, ww - 2 * xx), by);
    XMoveResizeWindow(xapp->display(), leftSide,
                      0, yy, bx, max(1, hh - 2 * yy));
    XMoveResizeWindow(xapp->display(), rightSide,
                      ww - bx, yy, bx, max(1, hh - 2 * yy));
    XMoveResizeWindow(xapp->display(), bottomSide,
                      xx, hh - by, max(1, ww - 2 * xx), by);

    XMoveResizeWindow(xapp->display(), topLeft,
                      0, 0, xx, yy);
    XMoveResizeWindow(xapp->display(), topRight,
                      ww - xx, 0, xx, yy);
    XMoveResizeWindow(xapp->display(), bottomLeft,
                      0, hh - yy, xx, yy);
    XMoveResizeWindow(xapp->display(), bottomRight,
                      ww - xx, hh - yy, xx, yy);

    XMoveResizeWindow(xapp->display(), topLeftSide,
                      0, 0, bx, yy);
    XMoveResizeWindow(xapp->display(), topRightSide,
                      ww - bx, 0, bx, yy);
}

void YFrameWindow::layoutClient() {
    if (!isRollup() && !isIconic()) {
        int x = borderX();
        int y = borderY();
        int title = titleY();
        int w = this->width() - 2 * x;
        int h = this->height() - 2 * y - title;

        fClientContainer->setGeometry(YRect(x, y + title, w, h));
        fClient->setGeometry(YRect(0, 0, w, h));
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
    if (this != manager->bottom(getActiveLayer()))
        return true;
    else
        return false;
}

bool YFrameWindow::canRaise() {
    for (YFrameWindow *w = prev(); w; w = w->prev()) {
        if (w->visibleNow()) {
            YFrameWindow *o = w;
            while (o) {
                o = o->owner();
                if (o == this)
                    break;
                else if (o == 0)
                    return true;
            }
        }
    }
    return false;
}

unsigned YFrameWindow::overlap(YFrameWindow* f) {
    if (false == f->isHidden() &&
        false == f->isMinimized() &&
        f->visibleOn(manager->activeWorkspace()))
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
