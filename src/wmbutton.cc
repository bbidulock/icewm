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

static YColor *titleButtonBg = 0;
static YColor *titleButtonFg = 0;

//!!! get rid of this
extern YColor *activeTitleBarBg;
extern YColor *inactiveTitleBarBg;

#ifdef CONFIG_LOOK_PIXMAP
YPixmap *menuButton[2] = { 0, 0 };
#endif

YFrameButton::YFrameButton(YWindow *parent,
                           YFrameWindow *frame,
                           YAction *action,
                           YAction *action2): YButton(parent, 0)
{
    if (titleButtonBg == 0)
        titleButtonBg = new YColor(clrNormalTitleButton);
    if (titleButtonFg == 0)
        titleButtonFg = new YColor(clrNormalTitleButtonText);

    fFrame = frame;
    fAction = action;
    fAction2 = action2;
    if (fAction == 0)
        setPopup(frame->windowMenu());


    int w = 18, h = 18;
    if (minimizePixmap[0]) {
        w = minimizePixmap[0]->width();
        h = minimizePixmap[0]->height();
    }
    switch (wmLook) {
#ifdef CONFIG_LOOK_WARP3
    case lookWarp3:
        setSize(w + 2, h + 2);
        break;
#endif
#ifdef CONFIG_LOOK_WARP4
    case lookWarp4:
        setSize(w + 0, h / 2 + 0);
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
        (buttonRaiseMask & (1 << (button.button - 1))))
    {
        if (!(button.state & ControlMask) && raiseOnClickButton) {
            getFrame()->activate();
            if (raiseOnClickButton && (actionDepth == 0 || fAction != actionDepth))
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
    } else if (up.button == 3 && (KEY_MODMASK(up.state) & (app->AltMask)) == 0) {
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

    if (fAction == actionMaximize)
        pixmap = maximizePixmap[pn];
    else if (fAction == actionMinimize)
        pixmap = minimizePixmap[pn];
    else if (fAction == actionRestore)
        pixmap = restorePixmap[pn];
    else if (fAction == actionClose)
        pixmap = closePixmap[pn];
    else if (fAction == actionHide)
        pixmap = hidePixmap[pn];
    else if (fAction == actionRollup)
        pixmap = getFrame()->isRollup() ? rolldownPixmap[pn] : rollupPixmap[pn];
    else if (fAction == actionDepth)
        pixmap = depthPixmap[pn];
    return pixmap;
}

void YFrameButton::paint(Graphics &g, int , int , unsigned int , unsigned int ) {
    int xPos = 1, yPos = 1;
    YPixmap *pixmap = 0;
    YPixmap *icon = 0;
    int pn = 0;
    bool a = isArmed();

    if (wmLook == lookPixmap || wmLook == lookMetal || wmLook == lookGtk)
        pn = getFrame()->focused() ? 1 : 0;

    g.setColor(titleButtonBg);

    if (fAction == 0)
#ifndef LITE
        if (getFrame()->clientIcon())
            icon = getFrame()->clientIcon()->small();
        else
#endif
            icon = 0;
    else
        pixmap = getImage(pn);

    switch (wmLook) {
#ifdef CONFIG_LOOK_WARP4
    case lookWarp4:
        if (fAction == 0) {
            g.fillRect(0, 0, width(), height());

            if (a)
                g.setColor(activeTitleBarBg);

            g.fillRect(1, 1, width() - 2, height() - 2);

            if (icon && showFrameIcon)
                g.drawPixmap(icon,
                             (width() - icon->width()) / 2,
                             (height() - icon->height()) / 2);
        } else {
            int picYpos = a ? 20 : 0;

            g.fillRect(0, 0, width(), height());

            if (pixmap)
                XCopyArea(app->display(),
                          pixmap->pixmap(), handle(),
                          g.handle(),
                          0, picYpos,
                          pixmap->width(), pixmap->height() / 2,
                          (width() - pixmap->width()) / 2,
                          (height() - pixmap->height() / 2) / 2);
        }
        break;
#endif
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
        /* else if (wmLook == lookWin95) {
            xPos = 2;
            yPos = 2;
        }*/

        int xW, yW;

#ifdef CONFIG_LOOK_MOTIF
        if (wmLook == lookMotif)
        {
            xW = width() - 2;
            yW = height() - 2;
        } else
#endif
        {
            xW = width() - 4;
            yW = height() - 4;
        }
        if (fAction == 0) {
            g.fillRect(xPos, yPos, xW, yW);

            if (icon && showFrameIcon)
                g.drawPixmap(icon,
                             xPos + (xW - icon->width()) / 2,
                             yPos + (yW - icon->height()) / 2);
        } else {
            if (pixmap)
                g.drawCenteredPixmap(xPos, yPos, xW, yW, pixmap);
        }
        break;
#endif
#ifdef CONFIG_LOOK_WIN95
    case lookWin95:
        if (fAction == 0) {
            if (!a) {
                YColor *bg = getFrame()->focused()
                    ? activeTitleBarBg
                    : inactiveTitleBarBg;
                g.setColor(bg);
            }

            g.fillRect(0, 0, width(), height());
            if (icon && showFrameIcon)
                g.drawPixmap(icon,
                             (width() - icon->width()) / 2,
                             (height() - icon->height()) / 2);
        } else {
            g.drawBorderW(0, 0, width() - 1, height() - 1, a ? false : true);

            if (a)
                xPos = yPos = 2;

            if (pixmap)
                g.drawCenteredPixmap(xPos, yPos, width() - 3, height() - 3,
                                     pixmap);
        }
        break;
#endif
#ifdef CONFIG_LOOK_PIXMAP
    case lookPixmap:
    case lookMetal:
    case lookGtk:
        {
            int n = a ? 1 : 0;
            if (fAction == 0) {
                YPixmap *pixmap = menuButton[pn];
                if (pixmap) {
                    int h = pixmap->height() / 2;
                    g.copyPixmap(pixmap, 0, n * h, pixmap->width(), h, 0, 0);
                } else {
                    g.fillRect(0, 0, width(), height());
                }
                xPos = 0;
                yPos = 0;
                xW = width();
                yW = height();
                if (icon && showFrameIcon)
                    g.drawPixmap(icon,
                                 xPos + (xW - icon->width()) / 2,
                                 yPos + (yW - icon->height()) / 2);
            } else {
                if (pixmap) {
                    int h = pixmap->height() / 2;
                    g.copyPixmap(pixmap, 0, n * h, pixmap->width(), h, 0, 0);
                } else
                    g.fillRect(0, 0, width(), height());
            }
        }
        break;
#endif
    default:
        break;
    }
}


void YFrameButton::paintFocus(Graphics &/*g*/, int /*x*/, int /*y*/, unsigned int /*w*/, unsigned int /*h*/) {
}
