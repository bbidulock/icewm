/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "wmframe.h"
#include "wmtitle.h"
#include "wmbutton.h"
#include "wmwinlist.h"
#include "wmapp.h"
#include "wpixmaps.h"
#include "yprefs.h"
#include "prefs.h"
#include "intl.h"

static ref<YFont> titleFont;

static YColorName titleBarBackground[2] = {
    &clrInactiveTitleBar, &clrActiveTitleBar
};
static YColorName titleBarForeground[2] = {
    &clrInactiveTitleBarText, &clrActiveTitleBarText
};
static YColorName titleBarShadowText[2] = {
    &clrInactiveTitleBarShadow, &clrActiveTitleBarShadow
};

void YFrameTitleBar::initTitleColorsFonts() {
    if (titleFont == null) {
        titleFont = YFont::getFont(XFA(titleFontName));
    }
}

void freeTitleColorsFonts() {
    titleFont = null;
}

YColor YFrameTitleBar::background(bool active) {
    return titleBarBackground[active];
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
    setWinGravity(NorthGravity);
}

YFrameButton* YFrameTitleBar::maximizeButton() {
    if (fMaximizeButton == 0 && isButton('m') &&
        hasbit(decors(), YFrameWindow::fdMaximize))
    {
        bool right = (strchr(titleButtonsRight, 'm') != 0);
        fMaximizeButton = new YFrameButton(this, right,
                getFrame(), actionMaximize, actionMaximizeVert);
        fMaximizeButton->setToolTip(_("Maximize"));
        fMaximizeButton->setTitle("Maximize");
        fMaximizeButton->show();
    }
    return fMaximizeButton;
}

YFrameButton* YFrameTitleBar::minimizeButton() {
    if (fMinimizeButton == 0 && isButton('i') &&
        hasbit(decors(), YFrameWindow::fdMinimize))
    {
        bool right = (strchr(titleButtonsRight, 'i') != 0);
        fMinimizeButton = new YFrameButton(this, right,
                getFrame(), actionMinimize,
                actionHide
                );
        fMinimizeButton->setToolTip(_("Minimize"));
        fMinimizeButton->setTitle("Minimize");
        fMinimizeButton->show();
    }
    return fMinimizeButton;
}

YFrameButton* YFrameTitleBar::closeButton() {
    if (fCloseButton == 0 && isButton('x') &&
        hasbit(decors(), YFrameWindow::fdClose))
    {
        bool right = (strchr(titleButtonsRight, 'x') != 0);
        fCloseButton = new YFrameButton(this, right,
                getFrame(), actionClose, actionKill);
        fCloseButton->setToolTip(_("Close"));
        fCloseButton->setTitle("Close");
        fCloseButton->show();
    }
    return fCloseButton;
}

YFrameButton* YFrameTitleBar::hideButton() {
    if (fHideButton == 0 && isButton('h') &&
        hasbit(decors(), YFrameWindow::fdHide))
    {
        bool right = (strchr(titleButtonsRight, 'h') != 0);
        fHideButton = new YFrameButton(this, right,
                getFrame(), actionHide, actionHide);
        fHideButton->setToolTip(_("Hide"));
        fHideButton->setTitle("Hide");
        fHideButton->show();
    }
    return fHideButton;
}

YFrameButton* YFrameTitleBar::rollupButton() {
    if (fRollupButton == 0 && isButton('r') &&
        hasbit(decors(), YFrameWindow::fdRollup))
    {
        bool right = (strchr(titleButtonsRight, 'r') != 0);
        fRollupButton = new YFrameButton(this, right,
                getFrame(), actionRollup, actionRollup);
        fRollupButton->setToolTip(_("Rollup"));
        fRollupButton->setTitle("Rollup");
        fRollupButton->show();
    }
    return fRollupButton;
}

YFrameButton* YFrameTitleBar::depthButton() {
    if (fDepthButton == 0 && isButton('d') &&
        hasbit(decors(), YFrameWindow::fdDepth))
    {
        bool right = (strchr(titleButtonsRight, 'd') != 0);
        fDepthButton = new YFrameButton(this, right,
                getFrame(), actionDepth, actionDepth);
        fDepthButton->setToolTip(_("Raise/Lower"));
        fDepthButton->setTitle("Lower");
        fDepthButton->show();
    }
    return fDepthButton;
}

YFrameButton* YFrameTitleBar::menuButton() {
    if (fMenuButton == 0 && isButton('s') &&
        hasbit(decors(), YFrameWindow::fdSysMenu))
    {
        bool right = (strchr(titleButtonsRight, 's') != 0);
        fMenuButton = new YFrameButton(this, right,
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
    switch (c) {
    case 's': return menuButton();
    case 'x': return closeButton();
    case 'm': return maximizeButton();
    case 'i': return minimizeButton();
    case 'h': return hideButton();
    case 'r': return rollupButton();
    case 'd': return depthButton();
    default: return 0;
    }
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
    else if (button.type == ButtonRelease) {
        if (button.button == Button1 &&
            IS_BUTTON(BUTTON_MODMASK(button.state), Button1Mask + Button3Mask))
        {
            if (windowList)
                windowList->showFocused(button.x_root, button.y_root);
        }
    }
    YWindow::handleButton(button);
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
        if (up.button == Button3 && notbit(KEY_MODMASK(up.state), xapp->AltMask)) {
            getFrame()->popupSystemMenu(this, up.x_root, up.y_root,
                                        YPopupWindow::pfCanFlipVertical |
                                        YPopupWindow::pfCanFlipHorizontal);
        } else if (up.button == Button1) {
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
    if (down.subwindow == None ?
        down.y == 0 && down.button == Button1 :
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
    struct { YFrameButton* fbut; unsigned flag; } buttons[] = {
        { fMinimizeButton, YFrameWindow::fdMinimize },
        { fMaximizeButton, YFrameWindow::fdMaximize },
        { fRollupButton,   YFrameWindow::fdRollup   },
        { fHideButton,     YFrameWindow::fdHide     },
        { fCloseButton,    YFrameWindow::fdClose    },
        { fMenuButton,     YFrameWindow::fdSysMenu  },
        { fDepthButton,    YFrameWindow::fdDepth    },
    };
    for (int i = 0; i < int ACOUNT(buttons); ++i) {
        YFrameButton* b = buttons[i].fbut;
        bool visibility = hasbit(decors, buttons[i].flag);
        if (b && b->visible() != visibility)
            b->setVisible(visibility);
    }

    bool const pi(focused());

    if (titleButtonsLeft) {
        int xPos(titleJ[pi] != null ? int(titleJ[pi]->width()) : 0);

        for (const char *bc = titleButtonsLeft; *bc; bc++) {
            if (*bc == ' ')
                xPos++;
            else {
                YFrameButton *b = getButton(*bc);
                if (b && b->visible() && b->onRight() == false)
                    positionButton(b, xPos, false);
            }
        }
    }

    if (titleButtonsRight) {
        int xPos(int(getFrame()->width() - 2 * getFrame()->borderX() -
                 (titleQ[pi] != null ? titleQ[pi]->width() : 0)));

        for (const char *bc = titleButtonsRight; *bc; bc++) {
            if (*bc == ' ')
                xPos--;
            else {
                YFrameButton *b = getButton(*bc);
                if (b && b->visible() && b->onRight())
                    positionButton(b, xPos, true);
            }
        }
    }
}

void YFrameTitleBar::deactivate() {
    activate(); // for now
}

int YFrameTitleBar::titleLen() const {
    ustring title = getFrame()->client()->windowTitle();
    int tlen = title != null ? titleFont->textWidth(title) : 0;
    return tlen;
}

void YFrameTitleBar::paint(Graphics &g, const YRect &/*r*/) {
    if (getFrame()->client() == NULL || visible() == false)
        return;

    YColor bg = titleBarBackground[focused()];
    YColor fg = titleBarForeground[focused()];

    int onLeft(0);
    int onRight((int) width());

    if (titleQ[focused()] != null)
        onRight -= int(titleQ[focused()]->width());

    if (titleButtonsLeft) {
        for (const char *bc = titleButtonsLeft; *bc; bc++) {
            if (*bc == ' ')
                ++onLeft;
            else {
                YFrameButton const *b(getButton(*bc));
                if (b && b->visible() && b->onRight() == false)
                    onLeft = max(onLeft, (int)(b->x() + b->width()));
            }
        }
    }

    if (titleButtonsRight) {
        for (const char *bc = titleButtonsRight; *bc; bc++) {
            if (*bc == ' ')
                --onRight;
            else {
                YFrameButton const *b(getButton(*bc));
                if (b && b->visible() && b->onRight())
                    onRight = max(onRight - (int) b->width(), b->x());
            }
        }
    }

    g.setFont(titleFont);

    ustring title = getFrame()->getTitle();
    int const yPos(int(height() - titleFont->height()) / 2 +
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
            int y = 0;
            int y2 = int(height()) - 1;

            g.fillRect(0, y, width(), height() - 1);
            g.setColor(focused() ? fg.darker() : bg.darker());
            g.drawLine(0, y2, width(), y2);
        }
        break;
    case lookWarp4:
        if (titleBarJustify == 0)
            stringOffset++;
        else if (titleBarJustify == 100)
            stringOffset--;

        if (focused()) {
            g.fillRect(1, 1, width() - 2, height() - 2);
            g.setColor(titleBarBackground[false]);
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
        bool const pi(focused());

        // !!! we really need a fallback mechanism for small windows
        if (titleL[pi] != null) {
            g.drawPixmap(titleL[pi], onLeft, 0);
            onLeft += int(titleL[pi]->width());
        }

        if (titleR[pi] != null) {
            onRight -= int(titleR[pi]->width());
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
            if (rgbTitleT[pi] != null) {
                int const gx(titleBarJoinLeft ? lLeft - onLeft : 0);
                int const gw((titleBarJoinRight ? onRight : lRight) -
                             (titleBarJoinLeft ? onLeft : lLeft));
                g.drawGradient(rgbTitleT[pi], lLeft, 0,
                               lRight - lLeft, height(), gx, 0, gw, height());
            }
            else if (titleT[pi] != null)
                g.repHorz(titleT[pi], lLeft, 0, lRight - lLeft);
            else
                g.fillRect(lLeft, 0, lRight - lLeft, height());
        }

        if (titleP[pi] != null) {
            lLeft -= int(titleP[pi]->width());
            g.drawPixmap(titleP[pi], lLeft, 0);
        }
        if (titleM[pi] != null) {
            g.drawPixmap(titleM[pi], lRight, 0);
            lRight += int(titleM[pi]->width());
        }

        if (onLeft < lLeft) {
            if (rgbTitleS[pi] != null) {
                int const gw((titleBarJoinLeft ? titleBarJoinRight ?
                              onRight : lRight : lLeft) - onLeft);
                g.drawGradient(rgbTitleS[pi], onLeft, 0,
                               lLeft - onLeft, height(), 0, 0, gw, height());
            }
            else if (titleS[pi] != null && titleS[pi]->pixmap())
                g.repHorz(titleS[pi], onLeft, 0, lLeft - onLeft);
            else
                g.fillRect(onLeft, 0, lLeft - onLeft, height());
        }
        if (lRight < onRight) {
            if (rgbTitleB[pi] != null) {
                int const gx(titleBarJoinRight ? titleBarJoinLeft ?
                             lRight - onLeft: lRight - lLeft : 0);
                int const gw(titleBarJoinRight ? titleBarJoinLeft ?
                             onRight - onLeft : onRight - lLeft : onRight - lRight);

                g.drawGradient(rgbTitleB[pi], lRight, 0,
                               onRight - lRight, height(), gx, 0, gw, height());
            }
            else if (titleB[pi] != null && titleB[pi]->pixmap())
                g.repHorz(titleB[pi], lRight, 0, onRight - lRight);
            else
                g.fillRect(lRight, 0, onRight - lRight, height());
        }

        if (titleJ[pi] != null)
            g.drawPixmap(titleJ[pi], 0, 0);
        if (titleQ[pi] != null)
            g.drawPixmap(titleQ[pi], int(width() - titleQ[pi]->width()), 0);

        break;
    }
    }

    if (title != null && tlen) {
        stringOffset += titleBarHorzOffset;

        if (titleBarShadowText[focused()]) {
            g.setColor(titleBarShadowText[focused()]);
            g.drawStringEllipsis(stringOffset + 1, yPos + 1, title, tlen);
        }

        g.setColor(fg);
        g.drawStringEllipsis(stringOffset, yPos, title, tlen);
    }
}

#ifdef CONFIG_SHAPE
void YFrameTitleBar::renderShape(Pixmap shape) {
    if (LOOK(lookPixmap | lookMetal | lookGtk | lookFlat))
    {
        Graphics g(shape, getFrame()->width(), getFrame()->height(), xapp->depth());

        int onLeft(0);
        int onRight((int) width());

        if (titleButtonsLeft)
            for (const char *bc = titleButtonsLeft; *bc; bc++) {
                if (*bc == ' ')
                    ++onLeft;
                else {
                    YFrameButton const *b(getButton(*bc));
                    if (b && b->visible() && b->onRight() == false) {
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
            }

        if (titleButtonsRight)
            for (const char *bc = titleButtonsRight; *bc; bc++) {
                if (*bc == ' ')
                    --onRight;
                else {
                    YFrameButton const *b(getButton(*bc));
                    if (b && b->visible() && b->onRight()) {
                        onRight = max(onRight - (int) b->width(), b->x());

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
            }

        onLeft += x();
        onRight += x();

        ustring title = getFrame()->getTitle();
        int tlen = title != null ? titleFont->textWidth(title) : 0;
        bool const pi(focused());

        if (titleL[pi] != null) {
            g.drawMask(titleL[pi], onLeft, y());
            onLeft += int(titleL[pi]->width());
        }

        if (titleR[pi] != null) {
            onRight -= int(titleR[pi]->width());
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
            lLeft -= int(titleP[pi]->width());
            g.drawMask(titleP[pi], lLeft, y());
        }
        if (titleM[pi] != null) {
            g.drawMask(titleM[pi], lRight, y());
            lRight += int(titleM[pi]->width());
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
            g.drawMask(titleQ[pi], int(x() + width() - titleQ[pi]->width()), y());
    }
}
#endif


// vim: set sw=4 ts=4 et:
