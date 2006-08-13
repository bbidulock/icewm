/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "ylib.h"
#include "ypixbuf.h"
#include "wmtitle.h"

#include "wmframe.h"
#include "wmwinlist.h"
#include "wmapp.h"
#include "yprefs.h"
#include "prefs.h"

#include <string.h>

static ref<YFont> titleFont;
YColor *activeTitleBarBg = 0;
YColor *activeTitleBarFg = 0;
YColor *activeTitleBarSt = 0;

YColor *inactiveTitleBarBg = 0;
YColor *inactiveTitleBarFg = 0;
YColor *inactiveTitleBarSt = 0;

#ifdef CONFIG_LOOK_PIXMAP
ref<YPixmap> titleJ[2]; // Frame <=> Left buttons
ref<YPixmap> titleL[2]; // Left buttons <=> Left pane
ref<YPixmap> titleS[2]; // Left pane
ref<YPixmap> titleP[2]; // Left pane <=> Title
ref<YPixmap> titleT[2]; // Title
ref<YPixmap> titleM[2]; // Title <=> Right pane
ref<YPixmap> titleB[2]; // Right pane
ref<YPixmap> titleR[2]; // Right pane <=> Right buttons
ref<YPixmap> titleQ[2]; // Right buttons <=> Frame
#endif

#ifdef CONFIG_GRADIENTS
ref<YPixbuf> rgbTitleS[2];
ref<YPixbuf> rgbTitleT[2];
ref<YPixbuf> rgbTitleB[2];
#endif

YFrameTitleBar::YFrameTitleBar(YWindow *parent, YFrameWindow *frame):
    YWindow(parent)
{
    if (titleFont == null)
        titleFont = YFont::getFont(XFA(titleFontName));

    if (activeTitleBarBg == 0)
        activeTitleBarBg = new YColor(clrActiveTitleBar);
    if (activeTitleBarFg == 0)
        activeTitleBarFg = new YColor(clrActiveTitleBarText);
    if (activeTitleBarSt == 0 && *clrActiveTitleBarShadow)
        activeTitleBarSt = new YColor(clrActiveTitleBarShadow);

    if (inactiveTitleBarBg == 0)
        inactiveTitleBarBg = new YColor(clrInactiveTitleBar);
    if (inactiveTitleBarFg == 0)
        inactiveTitleBarFg = new YColor(clrInactiveTitleBarText);
    if (inactiveTitleBarSt == 0 && *clrInactiveTitleBarShadow)
        inactiveTitleBarSt = new YColor(clrInactiveTitleBarShadow);

    movingWindow = 0; fFrame = frame;
    wasCanRaise = false;
}

YFrameTitleBar::~YFrameTitleBar() {
}

void YFrameTitleBar::handleButton(const XButtonEvent &button) {
    if (button.type == ButtonPress) {
        if ((buttonRaiseMask & (1 << (button.button - 1))) &&
           !(button.state & (xapp->AltMask | ControlMask | xapp->ButtonMask))) {
            getFrame()->activate();
            wasCanRaise = getFrame()->canRaise();
            if (raiseOnClickTitleBar)
                getFrame()->wmRaise();
        }
    }
#ifdef CONFIG_WINLIST
    else if (button.type == ButtonRelease) {
        if (button.button == 1 &&
            IS_BUTTON(BUTTON_MODMASK(button.state), Button1Mask + Button3Mask))
        {
            if (windowList)
                windowList->showFocused(button.x_root, button.y_root);
        }
    }
#endif
    YWindow::handleButton(button);
}

void YFrameTitleBar::handleMotion(const XMotionEvent &motion) {
    YWindow::handleMotion(motion);
}

void YFrameTitleBar::handleClick(const XButtonEvent &up, int count) {
    if (count >= 2 && (count % 2 == 0)) {
        if (up.button == (unsigned) titleMaximizeButton &&
            ISMASK(KEY_MODMASK(up.state), 0, ControlMask))
        {
            if (getFrame()->canMaximize())
                getFrame()->wmMaximize();
        } else if (up.button == (unsigned) titleMaximizeButton &&
                   ISMASK(KEY_MODMASK(up.state), ShiftMask, ControlMask))
        {
            if (getFrame()->canMaximize())
                getFrame()->wmMaximizeVert();
        } else if (up.button == (unsigned) titleMaximizeButton && xapp->AltMask &&
                   ISMASK(KEY_MODMASK(up.state), xapp->AltMask + ShiftMask, ControlMask))
        {
            if (getFrame()->canMaximize())
                getFrame()->wmMaximizeHorz();
        } else if (up.button == (unsigned) titleRollupButton &&
                 ISMASK(KEY_MODMASK(up.state), 0, ControlMask))
        {
            if (getFrame()->canRollup())
                getFrame()->wmRollup();
        } else if (up.button == (unsigned) titleRollupButton &&
                   ISMASK(KEY_MODMASK(up.state), ShiftMask, ControlMask))
        {
            if (getFrame()->canMaximize())
                getFrame()->wmMaximizeHorz();
        }
    } else if (count == 1) {
        if (up.button == 3 && (KEY_MODMASK(up.state) & (xapp->AltMask)) == 0) {
            getFrame()->popupSystemMenu(this, up.x_root, up.y_root,
                                        YPopupWindow::pfCanFlipVertical |
                                        YPopupWindow::pfCanFlipHorizontal);
        } else if (up.button == 1) {
            if (KEY_MODMASK(up.state) == xapp->AltMask) {
                if (getFrame()->canLower()) getFrame()->wmLower();
            } else if (lowerOnClickWhenRaised &&
                       (buttonRaiseMask & (1 << (up.button - 1))) &&
                       ((up.state & (ControlMask | xapp->ButtonMask)) ==
                        Button1Mask))
            {
                if (!wasCanRaise) {
                    if (getFrame()->canLower())
                        getFrame()->wmLower();
                    wasCanRaise = true;
                }
            }
        }
    }
}

void YFrameTitleBar::handleBeginDrag(const XButtonEvent &down, const XMotionEvent &/*motion*/) {
    if (getFrame()->canMove()) {
        getFrame()->startMoveSize(1, 1,
                                  0, 0,
                                  down.x + x(), down.y + y());
    }
}

void YFrameTitleBar::activate() {
    repaint();
#ifdef CONFIG_LOOK_WIN95
    if (wmLook == lookWin95 && getFrame()->menuButton())
        getFrame()->menuButton()->repaint();
#endif
#ifdef CONFIG_LOOK_PIXMAP
    if (wmLook == lookPixmap || wmLook == lookMetal || wmLook == lookGtk || wmLook == lookFlat) {
        if (getFrame()->menuButton()) getFrame()->menuButton()->repaint();
        if (getFrame()->closeButton()) getFrame()->closeButton()->repaint();
        if (getFrame()->maximizeButton()) getFrame()->maximizeButton()->repaint();
        if (getFrame()->minimizeButton()) getFrame()->minimizeButton()->repaint();
        if (getFrame()->hideButton()) getFrame()->hideButton()->repaint();
        if (getFrame()->rollupButton()) getFrame()->rollupButton()->repaint();
        if (getFrame()->depthButton()) getFrame()->depthButton()->repaint();
    }
#endif
}

void YFrameTitleBar::deactivate() {
    activate(); // for now
}

int YFrameTitleBar::titleLen() {
    const char *title = getFrame()->client()->windowTitle();
    int tlen = title ? titleFont->textWidth(title) : 0;
    return tlen;
}

void YFrameTitleBar::paint(Graphics &g, const YRect &/*r*/) {
    if (getFrame()->client() == NULL)
        return;

    YColor *bg = getFrame()->focused() ? activeTitleBarBg : inactiveTitleBarBg;
    YColor *fg = getFrame()->focused() ? activeTitleBarFg : inactiveTitleBarFg;
    YColor *st = getFrame()->focused() ? activeTitleBarSt : inactiveTitleBarSt;

    int onLeft(0);
    int onRight(width());

    if (titleButtonsLeft)
        for (const char *bc = titleButtonsLeft; *bc; bc++) {
            YWindow const *b(getFrame()->getButton(*bc));
            if (b) onLeft = max(onLeft, (int)(b->x() + b->width()));
        }

    if (titleButtonsRight)
        for (const char *bc = titleButtonsRight; *bc; bc++) {
            YWindow const *b(getFrame()->getButton(*bc));
            if (b) onRight = min(onRight, b->x());
        }

    g.setFont(titleFont);

    char const *title(getFrame()->getTitle());
    int const yPos((height() - titleFont->height()) / 2 +
                   titleFont->ascent() + titleBarVertOffset);
    int tlen(title ? titleFont->textWidth(title) : 0);

    int stringOffset(onLeft + (onRight - onLeft - tlen)
                     * (int) titleBarJustify / 100);
    g.setColor(bg);
    switch (wmLook) {
#ifdef CONFIG_LOOK_WIN95
    case lookWin95:
#endif
#ifdef CONFIG_LOOK_NICE
    case lookNice:
#endif
        g.fillRect(0, 0, width(), height());
        break;
#ifdef CONFIG_LOOK_WARP3
    case lookWarp3:
        {
#ifdef TITLEBAR_BOTTOM
            int y = 1;
            int y2 = 0;
#else
            int y = 0;
            int y2 = height() - 1;
#endif

            g.fillRect(0, y, width(), height() - 1);
            g.setColor(getFrame()->focused() ? fg->darker() : bg->darker());
            g.drawLine(0, y2, width(), y2);
        }
        break;
#endif
#ifdef CONFIG_LOOK_WARP4
    case lookWarp4:
        if (titleBarJustify == 0)
            stringOffset++;
        else if (titleBarJustify == 100)
            stringOffset--;

        if (getFrame()->focused()) {
            g.fillRect(1, 1, width() - 2, height() - 2);
            g.setColor(inactiveTitleBarBg);
            g.draw3DRect(onLeft, 0, onRight - 1, height() - 1, false);
        } else {
            g.fillRect(0, 0, width(), height());
        }
        break;
#endif
#ifdef CONFIG_LOOK_MOTIF
    case lookMotif:
        if (titleBarJustify == 0)
            stringOffset++;
        else if (titleBarJustify == 100)
            stringOffset--;

        g.fillRect(1, 1, width() - 2, height() - 2);
        g.draw3DRect(onLeft, 0, onRight - 1 - onLeft, height() - 1, true);
        break;
#endif
#ifdef CONFIG_LOOK_PIXMAP
    case lookPixmap:
    case lookMetal:
    case lookFlat:
    case lookGtk: {
        int const pi(getFrame()->focused() ? 1 : 0);

        // !!! we really need a fallback mechanism for small windows
        if (titleL[pi] != null) {
            g.drawPixmap(titleL[pi], onLeft, 0);
            onLeft += titleL[pi]->width();
        }

        if (titleR[pi] != null) {
            onRight-= titleR[pi]->width();
            g.drawPixmap(titleR[pi], onRight, 0);
        }

        int lLeft(onLeft + (titleP[pi] != null ? (int)titleP[pi]->width() : 0)),
            lRight(onRight - (titleM[pi] != null ? (int)titleM[pi]->width() : 0));

        tlen = clamp(lRight - lLeft, 0, tlen);
        stringOffset = lLeft + (lRight - lLeft - tlen)
            * (int) titleBarJustify / 100;

        lLeft = stringOffset;
        lRight = stringOffset + tlen;

        if (lLeft < lRight) {
#ifdef CONFIG_GRADIENTS
            if (rgbTitleT[pi] != null) {
                int const gx(titleBarJoinLeft ? lLeft - onLeft : 0);
                int const gw((titleBarJoinRight ? onRight : lRight) -
                             (titleBarJoinLeft ? onLeft : lLeft));
                g.drawGradient(rgbTitleT[pi], lLeft, 0,
                               lRight - lLeft, height(), gx, 0, gw, height());
            } else
#endif
                if (titleT[pi] != null)
                    g.repHorz(titleT[pi], lLeft, 0, lRight - lLeft);
                else
                    g.fillRect(lLeft, 0, lRight - lLeft, height());
        }

        if (titleP[pi] != null) {
            lLeft-= titleP[pi]->width();
            g.drawPixmap(titleP[pi], lLeft, 0);
        }
        if (titleM[pi] != null) {
            g.drawPixmap(titleM[pi], lRight, 0);
            lRight+= titleM[pi]->width();
        }

        if (onLeft < lLeft) {
#ifdef CONFIG_GRADIENTS
            if (rgbTitleS[pi] != null) {
                int const gw((titleBarJoinLeft ? titleBarJoinRight ?
                              onRight : lRight : lLeft) - onLeft);
                g.drawGradient(rgbTitleS[pi], onLeft, 0,
                               lLeft - onLeft, height(), 0, 0, gw, height());
            } else
#endif
                if (titleS[pi] != null)
                    g.repHorz(titleS[pi], onLeft, 0, lLeft - onLeft);
                else
                    g.fillRect(onLeft, 0, lLeft - onLeft, height());
        }
        if (lRight < onRight) {
#ifdef CONFIG_GRADIENTS
            if (rgbTitleB[pi] != null) {
                int const gx(titleBarJoinRight ? titleBarJoinLeft ?
                             lRight - onLeft: lRight - lLeft : 0);
                int const gw(titleBarJoinRight ? titleBarJoinLeft ?
                             onRight - onLeft : onRight - lLeft : onRight - lRight);

                g.drawGradient(rgbTitleB[pi], lRight, 0,
                               onRight - lRight, height(), gx, 0, gw, height());
            } else
#endif
                if (titleB[pi] != null)
                    g.repHorz(titleB[pi], lRight, 0, onRight - lRight);
                else
                    g.fillRect(lRight, 0, onRight - lRight, height());
        }

        if (titleJ[pi] != null)
            g.drawPixmap(titleJ[pi], 0, 0);
        if (titleQ[pi] != null)
            g.drawPixmap(titleQ[pi], width() - titleQ[pi]->width(), 0);

        break;
    }
#endif
    default:
        break;
    }

    if (title && tlen) {
        stringOffset+= titleBarHorzOffset;

        if (st) {
            g.setColor(st);
            g.drawStringEllipsis(stringOffset + 1, yPos + 1, title, tlen);
        }

        g.setColor(fg);
        g.drawStringEllipsis(stringOffset, yPos, title, tlen);
    }
}

#ifdef CONFIG_SHAPED_DECORATION
void YFrameTitleBar::renderShape(Pixmap shape) {
#ifdef CONFIG_LOOK_PIXMAP
    if (wmLook == lookPixmap || wmLook == lookMetal || wmLook == lookGtk || wmLook == lookFlat) {
        Graphics g(shape, getFrame()->width(), getFrame()->height());

        int onLeft(0);
        int onRight(width());

        if (titleButtonsLeft)
            for (const char *bc = titleButtonsLeft; *bc; bc++) {
                YFrameButton const *b(getFrame()->getButton(*bc));
                if (b) {
                    onLeft = max(onLeft, (int)(b->x() + b->width()));

                    ref<YPixmap> pixmap = b->getImage(0);
                    if (pixmap != null && b->getImage(1) != null ) { 
                        g.copyDrawable(pixmap->mask(), 0, 0,
                                       b->width(),
                                       b->height(),
                                       x() + b->x(),
                                       y() + b->y());
                    }
                }
            }

        if (titleButtonsRight)
            for (const char *bc = titleButtonsRight; *bc; bc++) {
                YFrameButton const *b(getFrame()->getButton(*bc));
                if (b) {
                    onRight = min(onRight, b->x());

                    ref<YPixmap> pixmap = b->getImage(0);
                    if ( pixmap != null && b->getImage(1) != null ) {
                        g.copyDrawable(pixmap->mask(), 0, 0,
                                       b->width(),
                                       b->height(),
                                       x() + b->x(),
                                       y() + b->y());
                    }
                }
            }

        onLeft+= x();
        onRight+= x();

        char const *title(getFrame()->getTitle());
        int tlen(title ? titleFont->textWidth(title) : 0);
        int stringOffset(onLeft + (onRight - onLeft - tlen)
                         * (int) titleBarJustify / 100);

        int const pi(getFrame()->focused() ? 1 : 0);

        if (titleL[pi] != null) {
            g.drawMask(titleL[pi], onLeft, y());
            onLeft+= titleL[pi]->width();
        }

        if (titleR[pi] != null) {
            onRight-= titleR[pi]->width();
            g.drawMask(titleR[pi], onRight, y());
        }

        int lLeft(onLeft + (titleP[pi] != null ? (int)titleP[pi]->width() : 0)),
            lRight(onRight - (titleM[pi] != null ? (int)titleM[pi]->width() : 0));

        tlen = clamp(lRight - lLeft, 0, tlen);
        stringOffset = lLeft + (lRight - lLeft - tlen)
            * (int) titleBarJustify / 100;

        lLeft = stringOffset;
        lRight = stringOffset + tlen;

        if (lLeft < lRight && titleT[pi] != null)
            g.repHorz(titleT[pi]->mask(),
                      titleT[pi]->width(), titleT[pi]->height(),
                      lLeft, y(), lRight - lLeft);

        if (titleP[pi] != null) {
            lLeft-= titleP[pi]->width();
            g.drawMask(titleP[pi], lLeft, y());
        }
        if (titleM[pi] != null) {
            g.drawMask(titleM[pi], lRight, y());
            lRight+= titleM[pi]->width();
        }

        if (onLeft < lLeft && titleS[pi] != null)
            g.repHorz(titleS[pi]->mask(),
                      titleS[pi]->width(), titleS[pi]->height(),
                      onLeft, y(), lLeft - onLeft);

        if (lRight < onRight && titleB[pi] != null)
            g.repHorz(titleB[pi]->mask(),
                      titleB[pi]->width(), titleB[pi]->height(),
                      lRight, y(), onRight - lRight);

        if (titleJ[pi] != null)
            g.drawMask(titleJ[pi], x(), y());
        if (titleQ[pi] != null)
            g.drawMask(titleQ[pi], x() + width() - titleQ[pi]->width(), y());
    }
#endif
}
#endif

