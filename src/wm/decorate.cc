/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#include "config.h"
#include "yxlib.h"
#include "wmframe.h"

#include "wmaction.h"
#include "wmtitle.h"
#include "wmapp.h"
#include "wmclient.h"
#include "wmcontainer.h"
#include "yrect.h"
#include "ymenuitem.h"
#include "base.h"

#ifdef CONFIG_LOOK_PIXMAP
YPixmap *frameTL[2][2] = { { 0, 0 }, { 0, 0 } };
YPixmap *frameT[2][2] = { { 0, 0 }, { 0, 0 } };
YPixmap *frameTR[2][2] = { { 0, 0 }, { 0, 0 } };
YPixmap *frameL[2][2] = { { 0, 0 }, { 0, 0 } };
YPixmap *frameR[2][2] = { { 0, 0 }, { 0, 0 } };
YPixmap *frameBL[2][2] = { { 0, 0 }, { 0, 0 } };
YPixmap *frameB[2][2] = { { 0, 0 }, { 0, 0 } };
YPixmap *frameBR[2][2] = { { 0, 0 }, { 0, 0 } };
#endif

void YFrameWindow::updateMenu() {
    YMenu *windowMenu = this->windowMenu();
    // enable all commands
    windowMenu->setActionListener(this);
    windowMenu->enableCommand(0);

    if (isMaximized())
        windowMenu->disableCommand(actionMaximize);
    if (isMinimized())
        windowMenu->disableCommand(actionMinimize);
    if (!(isMaximized() || isMinimized() || isHidden()))
        windowMenu->disableCommand(actionRestore);
    if (isMinimized() || isHidden() || isRollup() || !visibleNow())
        windowMenu->disableCommand(actionSize);
    if (isMinimized() || isHidden() || !visibleNow())
        windowMenu->disableCommand(actionMove);
    if (!canLower())
        windowMenu->disableCommand(actionLower);
    if (!canRaise())
        windowMenu->disableCommand(actionRaise);

    unsigned long func = frameFunctions();

    if (!(func & ffMove))
        windowMenu->disableCommand(actionMove);
    if (!(func & ffResize))
        windowMenu->disableCommand(actionSize);
    if (!(func & ffMinimize))
        windowMenu->disableCommand(actionMinimize);
    if (!(func & ffHide))
        windowMenu->disableCommand(actionHide);
    if (!(func & ffRollup) || !titlebar()->visible())
        windowMenu->disableCommand(actionRollup);
    if (!(func & ffMaximize))
        windowMenu->disableCommand(actionMaximize);
    if (!(func & ffClose))
        windowMenu->disableCommand(actionClose);

    YMenuItem *item = windowMenu->findAction(actionRollup);
    if (item)
        item->setChecked(isRollup() ? true : false);
    item = windowMenu->findAction(actionOccupyAllOrCurrent);
    if (item)
        item->setChecked(isSticky() ? true : false);
    item = windowMenu->findSubmenu(moveMenu);
    if (item != 0)
        item->setEnabled(isSticky() ? false : true);
    for (int i = 0; i < moveMenu->itemCount(); i++) {
        item = moveMenu->item(i);
        for (int w = 0; w < fRoot->workspaceCount(); w++) {
            if (item && item->action() == workspaceActionMoveTo[w]) {
                bool t = (w == getWorkspace()) ? false : true;
                item->setEnabled(t);
            }
        }
    }
    for (int j = 0; j < layerMenu->itemCount(); j++) {
        item = layerMenu->item(j);
        for (int layer = 0; layer < WinLayerCount; layer++) {
            if (item && item->action() == layerActionSet[layer]) {
                bool e = (layer == getLayer()) ? true : false;
                item->setEnabled(!e);
                item->setChecked(e);
            }
        }
    }
}

#ifdef SHAPE
void YFrameWindow::setShape() {
    if (!shapesSupported)
        return;

    if (client()->shaped()) {
        MSG(("setting shape w=%d, h=%d", width(), height()));
        if (isRollup() || isIconic()) {
            XRectangle full;
            full.x = 0;
            full.y = 0;
            full.width = width();
            full.height = height();
            XShapeCombineRectangles(app->display(), handle(),
                                    ShapeBounding,
                                    0, 0, &full, 1,
                                    ShapeSet, Unsorted);
        } else {
            XRectangle *r = 0;
            int count;
            int ordering;

            r = XShapeGetRectangles(app->display(), client()->handle(),
                                    ShapeBounding, &count, &ordering);

            if (r == NULL) // !!! recheck
                return;
            XRectangle *rect = new XRectangle[count + 6];
            int nrect = 0;

            for (int i = 0; i < count; i++) {
                rect[nrect].x = r[i].x + borderLeft();
                rect[nrect].y = r[i].y + borderTop() + titleY();
                rect[nrect].width  = r[i].width;
                rect[nrect].height = r[i].height;
                nrect++;
            }

            if ((frameDecors() & (fdResize | fdBorder)) == fdResize + fdBorder) {
                rect[nrect].x = 0;
                rect[nrect].y = 0;
                rect[nrect].width = width();
                rect[nrect].height = borderTop();
                nrect++;

                rect[nrect] = rect[nrect - 1];
                rect[nrect].y = height() - borderBottom();
                rect[nrect].y = borderBottom();
                nrect++;

                rect[nrect].x = 0;
                rect[nrect].y = borderTop();
                rect[nrect].width = borderLeft();
                rect[nrect].height = height() - (borderTop() + borderBottom());
                nrect++;

                rect[nrect] = rect[nrect - 1];
                rect[nrect].x = width() - borderRight();
                rect[nrect].width = borderRight();
                nrect++;
            }

            if (titleY() > 0) {
                rect[nrect].x = borderLeft();
                rect[nrect].y = borderTop();
                rect[nrect].width  = width() - (borderLeft() + borderRight());
                rect[nrect].height = titleY();
                nrect++;
            }
            if (nrect != 0)
                XShapeCombineRectangles(app->display(), handle(),
                                        ShapeBounding,
                                        0, 0, rect, nrect,
                                        ShapeSet, Unsorted);
#if 0
            XShapeCombineShape (app->display(), handle(),
                                ShapeBounding,
                                borderLeft(),
                                borderTop() + titleY(),
                                client()->handle(),
                                ShapeBounding, nrect ? ShapeUnion : ShapeSet);
#endif
            delete [] rect;
        }
    }
}
#endif

void YFrameWindow::configure(const YRect &cr) {
    //int oldX = this->x();
    //int oldY= this->y();

    MSG(("configure %d %d %d %d", cr.x(), cr.y(), cr.width(), cr.height()));

#ifdef SHAPE
    unsigned int oldWidth = container()->width();
    unsigned int oldHeight = container()->height();
    int oldcx = container()->x();
    int oldcy = container()->y();
#endif

    YWindow::configure(cr);

    layoutTitleBar();

    layoutResizeIndicators();

    layoutClient();

    // ??? !!!
    //if (x != oldX || y != oldY)
    sendConfigure();

#ifdef SHAPE
    int cx = container()->x();
    int cy = container()->y();
    if (oldWidth != container()->width() || oldHeight != container()->height() ||
        cx != oldcx || cy != oldcy)
        setShape();
#endif

    if (getLayer() == WinLayerDock)
        fRoot->updateWorkArea();
}

void YFrameWindow::layoutTitleBar() {
    if (titleY() == 0) {
        titlebar()->hide();
    } else {

        int title_width = width() - (borderLeft() + borderRight());
        titlebar()->setGeometry(borderLeft(),
                                borderTop(),
                                (title_width > 0) ? title_width : 1,
                                titleY());

        if (titlebar())
            titlebar()->layoutButtons();

        titlebar()->show();
    }
}

void YFrameWindow::layoutResizeIndicators() {
#warning "consider getting rid of these windows"
    if (((frameDecors() & (fdResize | fdBorder)) == fdResize + fdBorder) &&
        !isRollup() && !isMinimized()) {
        if (!indicatorsVisible) {
            indicatorsVisible = 1;

            XMapWindow(app->display(), topSide);
            XMapWindow(app->display(), leftSide);
            XMapWindow(app->display(), rightSide);
            XMapWindow(app->display(), bottomSide);

            XMapWindow(app->display(), topLeftCorner);
            XMapWindow(app->display(), topRightCorner);
            XMapWindow(app->display(), bottomLeftCorner);
            XMapWindow(app->display(), bottomRightCorner);
        }
    } else {
        if (indicatorsVisible) {
            indicatorsVisible = 0;

            XUnmapWindow(app->display(), topSide);
            XUnmapWindow(app->display(), leftSide);
            XUnmapWindow(app->display(), rightSide);
            XUnmapWindow(app->display(), bottomSide);

            XUnmapWindow(app->display(), topLeftCorner);
            XUnmapWindow(app->display(), topRightCorner);
            XUnmapWindow(app->display(), bottomLeftCorner);
            XUnmapWindow(app->display(), bottomRightCorner);
        }
    }
    if (!indicatorsVisible)
        return;

    int cx = gCornerX.getNum();
    int cy = gCornerY.getNum();

    XMoveResizeWindow(app->display(), topSide, 0, 0, width(), borderTop());
    XMoveResizeWindow(app->display(), leftSide, 0, 0, borderLeft(), height());
    XMoveResizeWindow(app->display(), rightSide, width() - borderRight(), 0, borderRight(), height());
    XMoveResizeWindow(app->display(), bottomSide, 0, height() - borderBottom(), width(), borderBottom());

    XMoveResizeWindow(app->display(), topLeftCorner, 0, 0, cx, cy);
    XMoveResizeWindow(app->display(), topRightCorner, width() - cx, 0, cx, cy);
    XMoveResizeWindow(app->display(), bottomLeftCorner, 0, height() - cy, cx, cy);
    XMoveResizeWindow(app->display(), bottomRightCorner, width() - cx, height() - cy, cx, cy);
}

void YFrameWindow::layoutClient() {
    if (!isRollup() && !isIconic()) {
        //int x = this->x() + borderX();
        //int y = this->y() + borderY();
        int w = this->width() - (borderLeft() + borderRight());
        int h = this->height() - (borderTop() + borderBottom()) - titleY();

        container()->setGeometry(borderLeft(), borderTop() + titleY(), w, h);
        client()->setGeometry(0, 0, w, h);
    }
}

bool YFrameWindow::canClose() {
    if (frameFunctions() & ffClose)
        return true;
    else
        return false;
}

bool YFrameWindow::canMaximize() {
    if (frameFunctions() & ffMaximize)
        return true;
    else
        return false;
}

bool YFrameWindow::canMinimize() {
    if (frameFunctions() & ffMinimize)
        return true;
    else
        return false;
}

bool YFrameWindow::canRollup() {
    if ((frameFunctions() & ffRollup) && titleY() > 0)
        return true;
    else
        return false;
}

bool YFrameWindow::canHide() {
    if (frameFunctions() & ffHide)
        return true;
    else
        return false;
}

bool YFrameWindow::canLower() {
    if (this != fRoot->bottom(getLayer()))
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

bool YFrameWindow::Overlaps(bool above) {
    YFrameWindow *f;
    int w1x2, w1y2, w2x2, w2y2;
    long curWorkspace = fRoot->activeWorkspace();
    bool B, C, D, E, F, H;

    if (above)
        f = prev();
    else
        f = next();

    while (f) {
        if (!f->isMinimized() && !f->isHidden() && f->visibleOn(curWorkspace)) {
            w2x2 = f->x() + (int)f->width() - 1;
            w2y2 = f->y() + (int)f->height() - 1;
            w1x2 = x() + (int)width() - 1;
            w1y2 = y() + (int)height() - 1;
            B = w2x2 >= x();
            C = y() >= f->y();
            E = w1x2 >= f->x();
            F = w2x2 >= w1x2;
            H = w2y2 >= w1y2;
            if (w1y2 >= f->y()) {
                if (F) {
                    if (E && H) {
                        return true;
                    }
                } else {
                    if (B && !C) {
                        return true;
                    }
                }
            }
            D = w2y2 >= y();
            if (x() >= f->x()) {
                if (C) {
                    if (B && D) {
                        return true;
                    }
                } else {
                    if (F && !H) {
                        return true;
                    }
                }
            } else {
                if (H) {
                    if (C && !F) {
                        return true;
                    }
                } else {
                    if (E && D) {
                        return true;
                    }
                }
            }
        }

        if (above)
            f = f->prev();
        else
            f = f->next();
    }
    return false;
}

unsigned int YFrameWindow::borderLeft() const {
    return
        (frameDecors() & fdBorder) ?
        ((frameDecors() & fdResize) ? gBorderL.getNum() : gDlgBorderL.getNum()) : 0;
}
unsigned int YFrameWindow::borderRight() const {
    return
        (frameDecors() & fdBorder) ?
        ((frameDecors() & fdResize) ? gBorderR.getNum() : gDlgBorderR.getNum()) : 0;
}
unsigned int YFrameWindow::borderTop() const {
    return
        (frameDecors() & fdBorder) ?
        ((frameDecors() & fdResize) ? gBorderT.getNum() : gDlgBorderT.getNum()) : 0;
}
unsigned int YFrameWindow::borderBottom() const {
    return
        (frameDecors() & fdBorder) ?
        ((frameDecors() & fdResize) ? gBorderB.getNum() : gDlgBorderB.getNum()) : 0;
}
unsigned int YFrameWindow::titleY() const {
    return (frameDecors() & fdTitleBar) ? gTitleHeight.getNum() : 0;
}
