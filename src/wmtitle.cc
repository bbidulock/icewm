/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "ylib.h"
#include "wmtitle.h"

#include "wmframe.h"
#include "wmwinlist.h"
#include "wmapp.h"
#include "wpixmaps.h"
#include "yprefs.h"
#include "prefs.h"
#include "yrect.h"

#include "intl.h"

static ref<YFont> titleFont;
YColor *activeTitleBarBg = 0;
YColor *activeTitleBarFg = 0;
YColor *activeTitleBarSt = 0;

YColor *inactiveTitleBarBg = 0;
YColor *inactiveTitleBarFg = 0;
YColor *inactiveTitleBarSt = 0;

void YFrameTitleBar::initTitleColorsFonts() {
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
}

YFrameTitleBar::YFrameTitleBar(YWindow *parent, YFrameWindow *frame):
    YWindow(desktop),
    fFrame(frame),
    wasCanRaise(false),
    fCloseButton(0),
    fMenuButton(0),
    fMaximizeButton(0),
    fMinimizeButton(0),
    fHideButton(0),
    fRollupButton(0),
    fDepthButton(0)
{
    reparent(parent, 0, 0);

    initTitleColorsFonts();
    setTitle("TitleBar");
}

YFrameButton* YFrameTitleBar::maximizeButton() {
    if (fMaximizeButton == 0 && isButton('m')) {
        fMaximizeButton = new YFrameButton(this,
                getFrame(), actionMaximize, actionMaximizeVert);
        //fMaximizeButton->setWinGravity(NorthEastGravity);
        fMaximizeButton->setToolTip(_("Maximize"));
        fMaximizeButton->setTitle("Maximize");
        fMaximizeButton->show();
    }
    return fMaximizeButton;
}

YFrameButton* YFrameTitleBar::minimizeButton() {
    if (fMinimizeButton == 0 && isButton('i')) {
        fMinimizeButton = new YFrameButton(this,
                getFrame(), actionMinimize,
#ifdef CONFIG_PDA
                actionNull
#else
                actionHide
#endif
                );
        //fMinimizeButton->setWinGravity(NorthEastGravity);
        fMinimizeButton->setToolTip(_("Minimize"));
        fMinimizeButton->setTitle("Minimize");
        fMinimizeButton->show();
    }
    return fMinimizeButton;
}

YFrameButton* YFrameTitleBar::closeButton() {
    if (fCloseButton == 0 && isButton('x')) {
        fCloseButton = new YFrameButton(this,
                getFrame(), actionClose, actionKill);
        //fCloseButton->setWinGravity(NorthEastGravity);
        fCloseButton->setToolTip(_("Close"));
        fCloseButton->setTitle("Close");
        fCloseButton->show();
    }
    return fCloseButton;
}

YFrameButton* YFrameTitleBar::hideButton() {
#ifndef CONFIG_PDA
    if (fHideButton == 0 && isButton('h')) {
        fHideButton = new YFrameButton(this,
                getFrame(), actionHide, actionHide);
        //fHideButton->setWinGravity(NorthEastGravity);
        fHideButton->setToolTip(_("Hide"));
        fHideButton->setTitle("Hide");
        fHideButton->show();
    }
#endif
    return fHideButton;
}

YFrameButton* YFrameTitleBar::rollupButton() {
    if (fRollupButton == 0 && isButton('r')) {
        fRollupButton = new YFrameButton(this,
                getFrame(), actionRollup, actionRollup);
        //fRollupButton->setWinGravity(NorthEastGravity);
        fRollupButton->setToolTip(_("Rollup"));
        fRollupButton->setTitle("Rollup");
        fRollupButton->show();
    }
    return fRollupButton;
}

YFrameButton* YFrameTitleBar::depthButton() {
    if (fDepthButton == 0 && isButton('d')) {
        fDepthButton = new YFrameButton(this,
                getFrame(), actionDepth, actionDepth);
        //fDepthButton->setWinGravity(NorthEastGravity);
        fDepthButton->setToolTip(_("Raise/Lower"));
        fDepthButton->setTitle("Lower");
        fDepthButton->show();
    }
    return fDepthButton;
}

YFrameButton* YFrameTitleBar::menuButton() {
    if (fMenuButton == 0 && isButton('s')) {
        fMenuButton = new YFrameButton(this,
                getFrame(), actionNull, actionNull);
        fMenuButton->setTitle("SysMenu");
        fMenuButton->show();
        fMenuButton->setActionListener(getFrame());
    }
    return fMenuButton;
}

YFrameTitleBar::~YFrameTitleBar() {
    delete fMenuButton; fMenuButton = 0;
    delete fCloseButton; fCloseButton = 0;
    delete fMaximizeButton; fMaximizeButton = 0;
    delete fMinimizeButton; fMinimizeButton = 0;
    delete fHideButton; fHideButton = 0;
    delete fRollupButton; fRollupButton = 0;
    delete fDepthButton; fDepthButton = 0;
}

bool YFrameTitleBar::isButton(char c) {
    return strchr(titleButtonsSupported, c) &&
              (strchr(titleButtonsRight, c) ||
               strchr(titleButtonsLeft, c));
}

YFrameButton *YFrameTitleBar::getButton(char c) {
    unsigned decors = getFrame()->frameDecors();
    switch (c) {
    case 's':
        if (decors & YFrameWindow::fdSysMenu)
            return menuButton();
        break;
    case 'x':
        if (decors & YFrameWindow::fdClose)
            return closeButton();
        break;
    case 'm':
        if (decors & YFrameWindow::fdMaximize)
            return maximizeButton();
        break;
    case 'i':
        if (decors & YFrameWindow::fdMinimize)
            return minimizeButton();
        break;
    case 'h':
        if (decors & YFrameWindow::fdHide)
            return hideButton();
        break;
    case 'r':
        if (decors & YFrameWindow::fdRollup)
            return rollupButton();
        break;
    case 'd':
        if (decors & YFrameWindow::fdDepth)
            return depthButton();
        break;
    default:
        return 0;
    }
    return 0;
}

void YFrameTitleBar::raiseButtons() {
    raise();
    if (fCloseButton)
        fCloseButton->raise();
    if (fMenuButton)
        fMenuButton->raise();
    if (fMaximizeButton)
        fMaximizeButton->raise();
    if (fMinimizeButton)
        fMinimizeButton->raise();
    if (fHideButton)
        fHideButton->raise();
    if (fRollupButton)
        fRollupButton->raise();
    if (fDepthButton)
        fDepthButton->raise();
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

void YFrameTitleBar::handleBeginDrag(
        const XButtonEvent &down,
        const XMotionEvent &motion)
{
    // check for a drag on the reparented resize handles
    if (down.subwindow != None &&
        getFrame()->hasIndicators() &&
        (down.subwindow == getFrame()->topSideIndicator() ||
         down.subwindow == getFrame()->topLeftIndicator() ||
         down.subwindow == getFrame()->topRightIndicator()))
    {
        getFrame()->handleBeginDrag(down, motion);
        return;
    } else

    if (getFrame()->canMove()) {
        getFrame()->startMoveSize(true, true,
                                  0, 0,
                                  down.x + x(), down.y + y());
    }
}

void YFrameTitleBar::activate() {
    repaint();
    if (wmLook == lookWin95 && menuButton())
        menuButton()->repaint();
    if (LOOK(lookPixmap | lookMetal | lookGtk | lookFlat)) {
        if (menuButton()) menuButton()->repaint();
        if (closeButton()) closeButton()->repaint();
        if (maximizeButton()) maximizeButton()->repaint();
        if (minimizeButton()) minimizeButton()->repaint();
        if (hideButton()) hideButton()->repaint();
        if (rollupButton()) rollupButton()->repaint();
        if (depthButton()) depthButton()->repaint();
    }
}

void YFrameTitleBar::positionButton(YFrameButton *b, int &xPos, bool onRight) {
    const int titleY = getFrame()->titleY();

    /// !!! clean this up
    if (b == fMenuButton) {
        const unsigned bw(((LOOK(lookPixmap | lookMetal | lookGtk |
                                 lookFlat | lookMotif ) && showFrameIcon) ||
                            b->getPixmap(0) == null) ?
                            titleY : b->getPixmap(0)->width());

        if (onRight) xPos -= bw;
        b->setGeometry(YRect(xPos, 0, bw, titleY));
        if (!onRight) xPos += bw;
    } else if (LOOK(lookPixmap | lookMetal | lookGtk | lookFlat)) {
        const unsigned bw(b->getPixmap(0) != null
                ? b->getPixmap(0)->width() : titleY);

        if (onRight) xPos -= bw;
        b->setGeometry(YRect(xPos, 0, bw, titleY));
        if (!onRight) xPos += bw;
    } else if (wmLook == lookWin95) {
        if (onRight) xPos -= titleY;
        b->setGeometry(YRect(xPos, 2, titleY, titleY - 3));
        if (!onRight) xPos += titleY;
    } else {
        if (onRight) xPos -= titleY;
        b->setGeometry(YRect(xPos, 0, titleY, titleY));
        if (!onRight) xPos += titleY;
    }
}

void YFrameTitleBar::layoutButtons() {
    if (getFrame()->titleY() == 0)
        return ;

    unsigned decors = getFrame()->frameDecors();

    if (fMinimizeButton) {
        if (decors & YFrameWindow::fdMinimize)
            fMinimizeButton->show();
        else
            fMinimizeButton->hide();
    }

    if (fMaximizeButton) {
        if (decors & YFrameWindow::fdMaximize)
            fMaximizeButton->show();
        else
            fMaximizeButton->hide();
    }

    if (fRollupButton) {
        if (decors & YFrameWindow::fdRollup)
            fRollupButton->show();
        else
            fRollupButton->hide();
    }

    if (fHideButton) {
        if (decors & YFrameWindow::fdHide)
            fHideButton->show();
        else
            fHideButton->hide();
    }

    if (fCloseButton) {
        if (decors & YFrameWindow::fdClose)
            fCloseButton->show();
        else
            fCloseButton->hide();
    }

    if (fMenuButton) {
        if (decors & YFrameWindow::fdSysMenu)
            fMenuButton->show();
        else
            fMenuButton->hide();
    }

    if (fDepthButton) {
        if (decors & YFrameWindow::fdDepth)
            fDepthButton->show();
        else
            fDepthButton->hide();
    }

    const int pi(focused() ? 1 : 0);

    if (titleButtonsLeft) {
        int xPos(titleJ[pi] != null ? titleJ[pi]->width() : 0);

        for (const char *bc = titleButtonsLeft; *bc; bc++) {
            YFrameButton *b = 0;

            switch (*bc) {
            case ' ':
                xPos++;
                b = 0;
                break;
            default:
                b = getButton(*bc);
                break;
            }

            if (b)
                positionButton(b, xPos, false);
        }
    }

    if (titleButtonsRight) {
        int xPos(getFrame()->width() - 2 * getFrame()->borderX() -
                 (titleQ[pi] != null ? titleQ[pi]->width() : 0));

        for (const char *bc = titleButtonsRight; *bc; bc++) {
            YFrameButton *b = 0;

            switch (*bc) {
            case ' ':
                xPos--;
                b = 0;
                break;
            default:
                b = getButton(*bc);
                break;
            }

            if (b)
                positionButton(b, xPos, true);
        }
    }
}

void YFrameTitleBar::deactivate() {
    activate(); // for now
}

int YFrameTitleBar::titleLen() {
    ustring title = getFrame()->client()->windowTitle();
    int tlen = title != null ? titleFont->textWidth(title) : 0;
    return tlen;
}

void YFrameTitleBar::paint(Graphics &g, const YRect &/*r*/) {
    if (getFrame()->client() == NULL || visible() == false)
        return;

    YColor *bg = getFrame()->focused() ? activeTitleBarBg : inactiveTitleBarBg;
    YColor *fg = getFrame()->focused() ? activeTitleBarFg : inactiveTitleBarFg;
    YColor *st = getFrame()->focused() ? activeTitleBarSt : inactiveTitleBarSt;

    int onLeft(0);
    int onRight(width());

    if (titleButtonsLeft) {
        for (const char *bc = titleButtonsLeft; *bc; bc++) {
            YWindow const *b(getButton(*bc));
            if (b) onLeft = max(onLeft, (int)(b->x() + b->width()));
        }
    }

    if (titleButtonsRight) {
        for (const char *bc = titleButtonsRight; *bc; bc++) {
            YWindow const *b(getButton(*bc));
            if (b) onRight = min(onRight, b->x());
        }
    }

    g.setFont(titleFont);

    ustring title = getFrame()->getTitle();
    int const yPos((height() - titleFont->height()) / 2 +
                   titleFont->ascent() + titleBarVertOffset);
    int tlen = title != null ? titleFont->textWidth(title) : 0;

    int stringOffset(onLeft + (onRight - onLeft - tlen)
                     * (int) titleBarJustify / 100);
    g.setColor(bg);
    switch (wmLook) {
    case lookWin95:
    case lookNice:
        g.fillRect(0, 0, width(), height());
        break;
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
    case lookMotif:
        if (titleBarJustify == 0)
            stringOffset++;
        else if (titleBarJustify == 100)
            stringOffset--;

        g.fillRect(1, 1, width() - 2, height() - 2);
        g.draw3DRect(onLeft, 0, onRight - 1 - onLeft, height() - 1, true);
        break;
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
    default:
        break;
    }

    if (title != null && tlen) {
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
    if (LOOK(lookPixmap | lookMetal | lookGtk | lookFlat))
    {
        Graphics g(shape, getFrame()->width(), getFrame()->height(), xapp->depth());

        int onLeft(0);
        int onRight(width());

        if (titleButtonsLeft)
            for (const char *bc = titleButtonsLeft; *bc; bc++) {
                YFrameButton const *b(getButton(*bc));
                if (b) {
                    onLeft = max(onLeft, (int)(b->x() + b->width()));

                    ref<YPixmap> pixmap = b->getPixmap(0);
                    if (pixmap != null && b->getPixmap(1) != null ) {
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
                YFrameButton const *b(getButton(*bc));
                if (b) {
                    onRight = min(onRight, b->x());

                    ref<YPixmap> pixmap = b->getPixmap(0);
                    if ( pixmap != null && b->getPixmap(1) != null ) {
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

        ustring title = getFrame()->getTitle();
        int tlen = title != null ? titleFont->textWidth(title) : 0;
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
        int stringOffset = lLeft + (lRight - lLeft - tlen)
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
}
#endif


// vim: set sw=4 ts=4 et:
