/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#include "config.h"

#include "ybuttonevent.h"
#include "wmbutton.h"

#include "wmaction.h"
#include "wmframe.h"
#include "wmtitle.h"
#include "yapp.h"
#include "default.h"
#include "yconfig.h"
#include "ypaint.h"

#define DATADIR_WM DATADIR "/icewm"

YBoolPrefProperty YFrameButton::gShowFrameIcon("icewm", "ShowFrameIcon", true);
YBoolPrefProperty YFrameButton::gRaiseOnClickButton("icewm", "RaiseOnClickButton", true);

YColorPrefProperty YFrameButton::gColorBgA("icewm", "ColorActiveTitleButton", "rgb:C0/C0/C0");
YColorPrefProperty YFrameButton::gColorBgI("icewm", "ColorNormalTitleButton", "rgb:C0/C0/C0");

YPixmapPrefProperty YFrameButton::gPixmapDepthA("icewm", "PixmapDepthButtonA", "depth.xpm", DATADIR_WM);
YPixmapPrefProperty YFrameButton::gPixmapCloseA("icewm", "PixmapCloseButtonA", "close.xpm", DATADIR_WM);
YPixmapPrefProperty YFrameButton::gPixmapMinimizeA("icewm", "PixmapMinimizeButtonA", "minimize.xpm", DATADIR_WM);
YPixmapPrefProperty YFrameButton::gPixmapMaximizeA("icewm", "PixmapMaximizeButtonA", "maximize.xpm", DATADIR_WM);
YPixmapPrefProperty YFrameButton::gPixmapRestoreA("icewm", "PixmapRestoreButtonA", "restore.xpm", DATADIR_WM);
YPixmapPrefProperty YFrameButton::gPixmapHideA("icewm", "PixmapHideButtonA", "hide.xpm", DATADIR_WM);
YPixmapPrefProperty YFrameButton::gPixmapRollupA("icewm", "PixmapRollupButtonA", "rollup.xpm", DATADIR_WM);
YPixmapPrefProperty YFrameButton::gPixmapRolldownA("icewm", "PixmapRolldownButtonA", "rolldown.xpm", DATADIR_WM);
YPixmapPrefProperty YFrameButton::gPixmapMenuA("icewm", "PixmapMenuButtonA", "menu.xpm", DATADIR_WM);

YPixmapPrefProperty YFrameButton::gPixmapDepthI("icewm", "PixmapDepthButtonI", "depth.xpm", DATADIR_WM);
YPixmapPrefProperty YFrameButton::gPixmapCloseI("icewm", "PixmapCloseButtonI", "close.xpm", DATADIR_WM);
YPixmapPrefProperty YFrameButton::gPixmapMinimizeI("icewm", "PixmapMinimizeButtonI", "minimize.xpm", DATADIR_WM);
YPixmapPrefProperty YFrameButton::gPixmapMaximizeI("icewm", "PixmapMaximizeButtonI", "maximize.xpm", DATADIR_WM);
YPixmapPrefProperty YFrameButton::gPixmapRestoreI("icewm", "PixmapRestoreButtonI", "restore.xpm", DATADIR_WM);
YPixmapPrefProperty YFrameButton::gPixmapHideI("icewm", "PixmapHideButtonI", "hide.xpm", DATADIR_WM);
YPixmapPrefProperty YFrameButton::gPixmapRollupI("icewm", "PixmapRollupButtonI", "rollup.xpm", DATADIR_WM);
YPixmapPrefProperty YFrameButton::gPixmapRolldownI("icewm", "PixmapRolldownButtonI", "rolldown.xpm", DATADIR_WM);
YPixmapPrefProperty YFrameButton::gPixmapMenuI("icewm", "PixmapMenuButtonI", "menu.xpm", DATADIR_WM);

YFrameButton::YFrameButton(YWindow *parent,
                           YFrameWindow *frame,
                           YAction *action,
                           YAction *action2): YButton(parent, 0)
{
    fFrame = frame;
    fAction = action;
    fAction2 = action2;
    if (fAction == 0)
        setPopup(frame->windowMenu());

    int w = 18;
    int h = 18;

    YPixmap *minimizePixmap = gPixmapMinimizeI.getPixmap();
    if (minimizePixmap) {
        w = minimizePixmap->width();
        h = minimizePixmap->height();
    }
    setSize(w, h);
}

YFrameButton::~YFrameButton() {
}

bool YFrameButton::eventButton(const YButtonEvent &button) {
    if (button.type() == YEvent::etButtonPress &&
        getFrame()->shouldRaise(button))
    {
        if (gRaiseOnClickButton.getBool()) {
            if (!button.isCtrl()) {
                getFrame()->activate();
                if ((actionDepth == 0 || fAction != actionDepth))
                    getFrame()->wmRaise();
            }
        }
    }
    return YButton::eventButton(button);
}

bool YFrameButton::eventClick(const YClickEvent &up) {
    if (fAction == 0 && up.leftButton()) {
        if (up.isDoubleClick()) {
            setArmed(false, false);
            getFrame()->wmClose();
        }
        return true;
    } else if (up.rightButton() && !(up.isAlt())) {
        if (!isPopupActive())
            getFrame()->popupSystemMenu(up.x_root(), up.y_root(), -1, -1,
                                        YPopupWindow::pfCanFlipVertical |
                                        YPopupWindow::pfCanFlipHorizontal);
        return true;
    }
    return YWindow::eventClick(up);
}

bool YFrameButton::eventBeginDrag(const YButtonEvent &down, const YMotionEvent &motion) {
    if (down.rightButton() && getFrame()->canMove()) {
        if (!isPopupActive())
            getFrame()->startMoveSize(1, 1,
                                      0, 0,
                                      down.x() + x() + getFrame()->titlebar()->x(),
                                      down.y() + y() + getFrame()->titlebar()->y());
        return true;
    }
    return YWindow::eventBeginDrag(down, motion);
}

void YFrameButton::setActions(YAction *action, YAction *action2) {
    fAction2 = action2;
    if (action != fAction) {
        fAction = action;
        repaint();
    }
}

void YFrameButton::updatePopup() {
    getFrame()->updateMenu();
}

void YFrameButton::actionPerformed(YAction * /*action*/, unsigned int modifiers) {
    if ((modifiers & YEvent::mShift) && fAction2 != 0)
        getFrame()->actionPerformed(fAction2, modifiers);
    else
        getFrame()->actionPerformed(fAction, modifiers);
}

YPixmap *YFrameButton::getImage(int pn) {
    YPixmap *pixmap = 0;

    if (pn == 1) {
        if (fAction == actionMaximize)
            pixmap = gPixmapMaximizeA.getPixmap();
        else if (fAction == actionMinimize)
            pixmap = gPixmapMinimizeA.getPixmap();
        else if (fAction == actionRestore)
            pixmap = gPixmapRestoreA.getPixmap();
        else if (fAction == actionClose)
            pixmap = gPixmapCloseA.getPixmap();
        else if (fAction == actionHide)
            pixmap = gPixmapHideA.getPixmap();
        else if (fAction == actionRollup)
            pixmap = getFrame()->isRollup() ? gPixmapRolldownA.getPixmap() : gPixmapRollupA.getPixmap();
        else if (fAction == actionDepth)
            pixmap = gPixmapDepthA.getPixmap();
        else if (fAction == 0)
            pixmap = gPixmapMenuA.getPixmap();
    } else {
        if (fAction == actionMaximize)
            pixmap = gPixmapMaximizeI.getPixmap();
        else if (fAction == actionMinimize)
            pixmap = gPixmapMinimizeI.getPixmap();
        else if (fAction == actionRestore)
            pixmap = gPixmapRestoreI.getPixmap();
        else if (fAction == actionClose)
            pixmap = gPixmapCloseI.getPixmap();
        else if (fAction == actionHide)
            pixmap = gPixmapHideI.getPixmap();
        else if (fAction == actionRollup)
            pixmap = getFrame()->isRollup() ? gPixmapRolldownI.getPixmap() : gPixmapRollupI.getPixmap();
        else if (fAction == actionDepth)
            pixmap = gPixmapDepthI.getPixmap();
        else if (fAction == 0)
            pixmap = gPixmapMenuI.getPixmap();
    }
    return pixmap;
}

void YFrameButton::paint(Graphics &g, const YRect &/*er*/) {
    bool a = isArmed();
    int pn = getFrame()->focused() ? 1 : 0;
    YPixmap *pixmap = getImage(pn);

    if (pn)
        g.setColor(gColorBgA);
    else
        g.setColor(gColorBgI);

    if (!pixmap) {
        g.draw3DRect(0, 0, width() - 1, height() - 1, a ? false : true);
        g.fillRect(1, 1, width() - 2, height() - 2);
    } else {
        if (pixmap->mask()) {
            g.fillRect(0, 0, width(), height());
        }
        int n = a ? 1 : 0;
        int h = pixmap->height() / 2;
        g.copyPixmap(pixmap, 0, n * h, pixmap->width(), h, 0, 0);
    }

#ifndef LITE
    if (fAction == 0 && gShowFrameIcon.getBool()) {
        YPixmap *icon = 0;

        if (getFrame()->clientIcon())
            icon = getFrame()->clientIcon()->small();

        int xW = width();
        int yW = height();
        if (icon)
            g.drawPixmap(icon,
                         (xW - icon->width()) / 2,
                         (yW - icon->height()) / 2);
    }
#endif
}

void YFrameButton::paintFocus(Graphics &/*g*/, const YRect &/*er*/) {
}

void YFrameButton::activate() {
}

void YFrameButton::deactivate() {
}
