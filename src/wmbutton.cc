/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"

#include "ypixbuf.h"
#include "ylib.h"
#include "wmbutton.h"

#include "wmaction.h"
#include "wmframe.h"
#include "wmtitle.h"
#include "yxapp.h"
#include "yicon.h"
#include "yprefs.h"
#include "prefs.h"

static YColor *titleButtonBg = 0;
static YColor *titleButtonFg = 0;

//!!! get rid of this
extern YColor *activeTitleBarBg;
extern YColor *inactiveTitleBarBg;

#ifdef CONFIG_LOOK_PIXMAP
ref<YPixmap> menuButton[3];
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

    setSize(0,0);
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
    } else if (up.button == 3 && (KEY_MODMASK(up.state) & (xapp->AltMask)) == 0) {
        if (!isPopupActive())
            getFrame()->popupSystemMenu(this, up.x_root, up.y_root,
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

ref<YPixmap> YFrameButton::getPixmap(int pn) const {
    if (fAction == actionMaximize)
        return maximizePixmap[pn];
    else if (fAction == actionMinimize)
        return minimizePixmap[pn];
    else if (fAction == actionRestore)
        return restorePixmap[pn];
    else if (fAction == actionClose)
        return closePixmap[pn];
#ifndef CONFIG_PDA
    else if (fAction == actionHide)
        return hidePixmap[pn];
#endif
    else if (fAction == actionRollup)
        return getFrame()->isRollup() ? rolldownPixmap[pn] : rollupPixmap[pn];
    else if (fAction == actionDepth)
        return depthPixmap[pn];
#ifdef CONFIG_LOOK_PIXMAP
    else if (fAction == 0 &&
             (wmLook == lookPixmap || wmLook == lookMetal || wmLook == lookGtk || wmLook == lookFlat))
        return menuButton[pn];
#endif
    else
        return null;
}

void YFrameButton::paint(Graphics &g, const YRect &/*r*/) {
    int xPos = 1, yPos = 1;
    int pn = (wmLook == lookPixmap || wmLook == lookMetal ||
              wmLook == lookGtk || wmLook == lookFlat) && getFrame()->focused() ? 1 : 0;
    const bool armed(isArmed());

    g.setColor(titleButtonBg);

    if (fOver && rolloverTitleButtons) {
        if (pn == 1) {
            pn = 2;
        } else {
            pn = 0;
        }
    }

    int iconSize =
#ifdef LITE
        0;
#else
    YIcon::smallSize();
#endif
#ifdef LITE
    ref<YIcon> icon;
#else
    ref<YIcon> icon =
        (fAction == 0) ? getFrame()->clientIcon() : null;
#endif

    ref<YPixmap> pixmap = getPixmap(pn);
    if (pixmap == null) pixmap = getPixmap(0);

    switch (wmLook) {
#ifdef CONFIG_LOOK_WARP4
    case lookWarp4:
        if (fAction == 0) {
            g.fillRect(0, 0, width(), height());

            if (armed)
                g.setColor(activeTitleBarBg);

            g.fillRect(1, 1, width() - 2, height() - 2);

            if (icon != null && showFrameIcon)
                icon->draw(g,
                           (width() - iconSize) / 2,
                           (height() - iconSize) / 2,
                           iconSize);
        } else {
            g.fillRect(0, 0, width(), height());

            if (pixmap != null)
                g.copyPixmap(pixmap, 0, armed ? 20 : 0,
                             pixmap->width(), pixmap->height() / 2,
                             (width() - pixmap->width()) / 2,
                             (height() - pixmap->height() / 2));
        }
        break;
#endif

#if defined(CONFIG_LOOK_MOTIF) || \
    defined(CONFIG_LOOK_WARP3) || \
    defined(CONFIG_LOOK_NICE)

    CASE_LOOK_MOTIF:
    CASE_LOOK_WARP3:
    CASE_LOOK_NICE: {
        g.draw3DRect(0, 0, width() - 1, height() - 1, !armed);

        if (!LOOK_IS_MOTIF) {
            if (armed) {
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
        /* else if (LOOK_IS_WIN95) {
         xPos = 2;
         yPos = 2;
         }*/

        unsigned const w(LOOK_IS_MOTIF ? width() - 2 : width() - 4);
        unsigned const h(LOOK_IS_MOTIF ? height() - 2 : height() - 4);

        if (fAction == 0) {
            g.fillRect(xPos, yPos, w, h);

            if (icon != null && showFrameIcon)
                icon->draw(g,
                           xPos + (w - iconSize) / 2,
                           yPos + (h - iconSize) / 2,
                           iconSize);
        } else {
            if (pixmap != null)
                g.drawCenteredPixmap(xPos, yPos, w, h, pixmap);
        }

        break;
    }
#endif

#ifdef CONFIG_LOOK_WIN95
CASE_LOOK_WIN95:
        if (fAction == 0) {
            if (!armed) {
                YColor * bg(getFrame()->focused() ? activeTitleBarBg
                            : inactiveTitleBarBg);
                g.setColor(bg);
            }

            g.fillRect(0, 0, width(), height());

            if (icon != null && showFrameIcon)
                icon->draw(g,
                           (width() - iconSize) / 2,
                           (height() - iconSize) / 2,
                           iconSize);
        } else {
            g.drawBorderW(0, 0, width() - 1, height() - 1, !armed);

            if (armed)
                xPos = yPos = 2;

            if (pixmap != null)
                g.drawCenteredPixmap(xPos, yPos, width() - 3, height() - 3,
                                     pixmap);
        }
        break;
#endif
#ifdef CONFIG_LOOK_PIXMAP
    case lookPixmap:
    case lookMetal:
    case lookFlat:
    case lookGtk:
        if (pixmap != null) {
            if ( getPixmap(1) != null ) {
            int const h(pixmap->height() / 2);
            g.copyPixmap(pixmap, 0, armed ? h : 0, pixmap->width(), h, 0, 0);
            } else { //If we have only an image we change the over or armed color and paint it
               g.fillRect(0, 0, width(), height());
               if (armed)
                   g.setColor(activeTitleBarBg->darker());
               else if (rolloverTitleButtons && fOver)
                   g.setColor(activeTitleBarBg->brighter());
               g.fillRect(1, 1, width()-2, height()-3);
               int x(((int)width()  - (int)pixmap->width())  / 2);
               int y(((int)height() - (int)pixmap->height()) / 2);
               g.drawPixmap(pixmap, x, y);
            }
        } else
            g.fillRect(0, 0, width(), height());

        if (fAction == 0 && icon != null && showFrameIcon)
            icon->draw(g,
                       ((int)width() - (int)iconSize) / 2,
                       ((int)height() - (int)iconSize) / 2,
                       iconSize);

        break;
#endif
    default:
        break;
    }
}


void YFrameButton::paintFocus(Graphics &/*g*/, const YRect &/*r*/) {
}
