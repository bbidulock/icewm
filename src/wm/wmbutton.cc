/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#include "config.h"

#include "ylib.h"
#include "wmbutton.h"

#include "wmaction.h"
#include "wmframe.h"
#include "wmtitle.h"
#include "yapp.h"
#include "default.h"
#include "yconfig.h"

YBoolPrefProperty YFrameButton::gShowFrameIcon("icewm", "ShowFrameIcon", true);
YBoolPrefProperty YFrameButton::gRaiseOnClickButton("icewm", "RaiseOnClickButton", true);

YColorPrefProperty YFrameButton::gColorBgA("icewm", "ColorActiveTitleButton", "rgb:C0/C0/C0");
YColorPrefProperty YFrameButton::gColorBgI("icewm", "ColorNormalTitleButton", "rgb:C0/C0/C0");

YPixmapPrefProperty YFrameButton::gPixmapDepthA("icewm", "PixmapDepthButtonA", "depthA.xpm", DATADIR);
YPixmapPrefProperty YFrameButton::gPixmapCloseA("icewm", "PixmapCloseButtonA", "closeA.xpm", DATADIR);
YPixmapPrefProperty YFrameButton::gPixmapMinimizeA("icewm", "PixmapMinimizeButtonA", "minimizeA.xpm", DATADIR);
YPixmapPrefProperty YFrameButton::gPixmapMaximizeA("icewm", "PixmapMaximizeButtonA", "maximizeA.xpm", DATADIR);
YPixmapPrefProperty YFrameButton::gPixmapRestoreA("icewm", "PixmapRestoreButtonA", "restoreA.xpm", DATADIR);
YPixmapPrefProperty YFrameButton::gPixmapHideA("icewm", "PixmapHideButtonA", "hideA.xpm", DATADIR);
YPixmapPrefProperty YFrameButton::gPixmapRollupA("icewm", "PixmapRollupButtonA", "rollupA.xpm", DATADIR);
YPixmapPrefProperty YFrameButton::gPixmapRolldownA("icewm", "PixmapRolldownButtonA", "rolldownA.xpm", DATADIR);
YPixmapPrefProperty YFrameButton::gPixmapMenuA("icewm", "PixmapMenuButtonA", "menuA.xpm", DATADIR);

YPixmapPrefProperty YFrameButton::gPixmapDepthI("icewm", "PixmapDepthButtonI", "depthI.xpm", DATADIR);
YPixmapPrefProperty YFrameButton::gPixmapCloseI("icewm", "PixmapCloseButtonI", "closeI.xpm", DATADIR);
YPixmapPrefProperty YFrameButton::gPixmapMinimizeI("icewm", "PixmapMinimizeButtonI", "minimizeI.xpm", DATADIR);
YPixmapPrefProperty YFrameButton::gPixmapMaximizeI("icewm", "PixmapMaximizeButtonI", "maximizeI.xpm", DATADIR);
YPixmapPrefProperty YFrameButton::gPixmapRestoreI("icewm", "PixmapRestoreButtonI", "restoreI.xpm", DATADIR);
YPixmapPrefProperty YFrameButton::gPixmapHideI("icewm", "PixmapHideButtonI", "hideI.xpm", DATADIR);
YPixmapPrefProperty YFrameButton::gPixmapRollupI("icewm", "PixmapRollupButtonI", "rollupI.xpm", DATADIR);
YPixmapPrefProperty YFrameButton::gPixmapRolldownI("icewm", "PixmapRolldownButtonI", "rolldownI.xpm", DATADIR);
YPixmapPrefProperty YFrameButton::gPixmapMenuI("icewm", "PixmapMenuButtonI", "menuI.xpm", DATADIR);

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


    int w = 18, h = 18;

    YPixmap *minimizePixmap = gPixmapMinimizeI.getPixmap();
    if (minimizePixmap) {
        w = minimizePixmap->width();
        h = minimizePixmap->height();
    }
    switch (wmLook) {
#ifdef CONFIG_LOOK_WARP3
    case lookWarp3:
        setSize(w + 2, h + 2);
        break;
#endif
#if defined(CONFIG_LOOK_NICE) || defined(CONFIG_LOOK_WIN95)
#ifdef CONFIG_LOOK_NICE
    case lookNice:
#endif
#ifdef CONFIG_LOOK_WIN95
    case lookWin95:
#endif
        setSize(w + 3, h + 3);
        break;
#endif
#ifdef CONFIG_LOOK_MOTIF
    case lookMotif:
        setSize(w + 2, h + 2);
        break;
#endif
#ifdef CONFIG_LOOK_PIXMAP
    case lookPixmap:
    case lookMetal:
    case lookGtk:
        setSize(w, h);
        break;
#endif
    default:
        break;
    }
}

YFrameButton::~YFrameButton() {
}

void YFrameButton::handleButton(const XButtonEvent &button) {
    if (button.type == ButtonPress &&
        getFrame()->shouldRaise(button))
    {
        if (gRaiseOnClickButton.getBool())
            if (!(button.state & ControlMask)) {
                getFrame()->activate();
                if ((actionDepth == 0 || fAction != actionDepth))
                    getFrame()->wmRaise();
            }
    }
    YButton::handleButton(button);
}

void YFrameButton::handleClick(const XButtonEvent &up, int count) {
    if (fAction == 0 && up.button == 1) {
        if ((count % 2) == 0) {
            setArmed(false, false);
            getFrame()->wmClose();
        }
    } else if (up.button == 3 && (KEY_MODMASK(up.state) & (app->getAltMask())) == 0) {
        if (!isPopupActive())
            getFrame()->popupSystemMenu(up.x_root, up.y_root, -1, -1,
                                        YPopupWindow::pfCanFlipVertical |
                                        YPopupWindow::pfCanFlipHorizontal);
    }
}

void YFrameButton::handleBeginDrag(const XButtonEvent &down, const XMotionEvent &/*motion*/) {
    if (down.button == 3 && getFrame()->canMove()) {
        if (!isPopupActive())
            getFrame()->startMoveSize(1, 1,
                                      0, 0,
                                      down.x + x() + getFrame()->titlebar()->x(),
                                      down.y + y() + getFrame()->titlebar()->y());
    }
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
    if ((modifiers & ShiftMask) && fAction2 != 0)
        getFrame()->actionPerformed(fAction2, modifiers);
    else
        getFrame()->actionPerformed(fAction, modifiers);
}

YPixmap *YFrameButton::getImage(int pn) {
    YPixmap *pixmap = 0;

    //PRECONDITION(pn == 0); // for now !!!

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

void YFrameButton::paint(Graphics &g, int , int , unsigned int , unsigned int ) {
    bool a = isArmed();
    int pn = getFrame()->focused() ? 1 : 0;
    YPixmap *pixmap = getImage(pn);
    int xPos = 0, yPos = 0;

    if (!pixmap || pixmap->mask()) {
        if (pn)
            g.setColor(gColorBgA);
        else
            g.setColor(gColorBgI);
        g.fillRect(0, 0, width(), height());
    }

#if 0
    switch (wmLook) { // !!! add pref
#if defined(CONFIG_LOOK_MOTIF) || defined(CONFIG_LOOK_WARP3) || defined(CONFIG_LOOK_NICE)
#ifdef CONFIG_LOOK_MOTIF
    case lookMotif:
#endif
#ifdef CONFIG_LOOK_WARP3
    case lookWarp3:
#endif
#ifdef CONFIG_LOOK_NICE
    case lookNice:
#endif
        g.draw3DRect(0, 0, width() - 1, height() - 1, a ? false : true);
#ifdef CONFIG_LOOK_MOTIF
        if (wmLook != lookMotif)
#endif
        {
            if (a) {
                xPos = 3;
                yPos = 3;
                g.drawLine(1, 1, width() - 2, 1);
                g.drawLine(1, 1, 1, height() - 2);
                g.drawLine(2, 2, width() - 3, 2);
                g.drawLine(2, 2, 2, height() - 3);
            } else {
                xPos = 2;
                yPos = 2;
                g.drawRect(1, 1, width() - 3, width() - 3);
            }
        }
        break;
#endif
#ifdef CONFIG_LOOK_WIN95
    case lookWin95:
        if (fAction == 0) {
        } else {
            g.drawBorderW(0, 0, width() - 1, height() - 1, a ? false : true);

            if (a)
                xPos = yPos = 2;
            else
                xPos = yPos = 1;
        }
        break;
#endif
    default:
        break;
    }
#endif

    if (pixmap) {
        int n = a ? 1 : 0;
        int h = pixmap->height() / 2;
        g.copyPixmap(pixmap, 0, n * h, pixmap->width(), h, xPos, yPos);
    }

#ifndef LITE
    if (fAction == 0 && gShowFrameIcon.getBool()) {
        YPixmap *icon = 0;

        if (getFrame()->clientIcon())
            icon = getFrame()->clientIcon()->small();

        xPos = 0;
        yPos = 0;
        int xW = width();
        int yW = height();
        if (icon)
            g.drawPixmap(icon,
                         xPos + (xW - icon->width()) / 2,
                         yPos + (yW - icon->height()) / 2);
    }
#endif
}

void YFrameButton::paintFocus(Graphics &/*g*/, int /*x*/, int /*y*/, unsigned int /*w*/, unsigned int /*h*/) {
}
