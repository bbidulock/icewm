/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "ylib.h"
#include "wmtitle.h"

#include "wmframe.h"
#include "wmwinlist.h"
#include "wmapp.h"

#include <string.h>

static YFont *titleFont = 0;
YColor *activeTitleBarBg = 0;
YColor *activeTitleBarFg = 0;

YColor *inactiveTitleBarBg = 0;
YColor *inactiveTitleBarFg = 0;

#ifdef CONFIG_LOOK_PIXMAP
YPixmap *titleL[2] = { 0, 0 };
YPixmap *titleS[2] = { 0, 0 };
YPixmap *titleP[2] = { 0, 0 };
YPixmap *titleT[2] = { 0, 0 };
YPixmap *titleM[2] = { 0, 0 };
YPixmap *titleB[2] = { 0, 0 };
YPixmap *titleR[2] = { 0, 0 };
#endif

YFrameTitleBar::YFrameTitleBar(YWindow *parent, YFrameWindow *frame):
    YWindow(parent)
{
    if (titleFont == 0)
        titleFont = YFont::getFont(titleFontName);
    if (activeTitleBarBg == 0)
        activeTitleBarBg = new YColor(clrActiveTitleBar);
    if (activeTitleBarFg == 0)
        activeTitleBarFg = new YColor(clrActiveTitleBarText);
    if (inactiveTitleBarBg == 0)
        inactiveTitleBarBg = new YColor(clrInactiveTitleBar);
    if (inactiveTitleBarFg == 0)
        inactiveTitleBarFg = new YColor(clrInactiveTitleBarText);

    movingWindow = 0; fFrame = frame;
}

YFrameTitleBar::~YFrameTitleBar() {
}

void YFrameTitleBar::handleButton(const XButtonEvent &button) {
    if (button.type == ButtonPress) {
        if ((buttonRaiseMask & (1 << (button.button - 1))) &&
            (button.state & (app->AltMask | ControlMask | Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask)) == 0)
        {
            getFrame()->activate();
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
        if (up.button == titleMaximizeButton &&
            ISMASK(KEY_MODMASK(up.state), 0, ControlMask))
        {
            if (getFrame()->canMaximize())
                getFrame()->wmMaximize();
        } else if (up.button == titleMaximizeButton &&
                   ISMASK(KEY_MODMASK(up.state), ShiftMask, ControlMask))
        {
            if (getFrame()->canMaximize())
                getFrame()->wmMaximizeVert();
        } else if (up.button == titleMaximizeButton && app->AltMask &&
                   ISMASK(KEY_MODMASK(up.state), app->AltMask + ShiftMask, ControlMask))
        {
            if (getFrame()->canMaximize())
                getFrame()->wmMaximizeHorz();
        } else if (up.button == titleRollupButton &&
                 ISMASK(KEY_MODMASK(up.state), 0, ControlMask))
        {
            if (getFrame()->canRollup())
                getFrame()->wmRollup();
        } else if (up.button == titleRollupButton &&
                   ISMASK(KEY_MODMASK(up.state), ShiftMask, ControlMask))
        {
            if (getFrame()->canMaximize())
                getFrame()->wmMaximizeHorz();
        }
    } else if (count == 1) {
        if (up.button == 3 && (KEY_MODMASK(up.state) & (app->AltMask)) == 0) {
            getFrame()->popupSystemMenu(up.x_root, up.y_root, -1, -1,
                                        YPopupWindow::pfCanFlipVertical |
                                        YPopupWindow::pfCanFlipHorizontal);
        } else if (up.button == 1 && KEY_MODMASK(up.state) == app->AltMask) {
            if (getFrame()->canLower())
                getFrame()->wmLower();
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
    if (wmLook == lookPixmap || wmLook == lookMetal || wmLook == lookGtk) {
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

void YFrameTitleBar::paint(Graphics &g, int , int , unsigned int , unsigned int ) {
    YColor *bg = getFrame()->focused() ? activeTitleBarBg : inactiveTitleBarBg;
    YColor *fg = getFrame()->focused() ? activeTitleBarFg : inactiveTitleBarFg;
    int onLeft = 0;
    int onRight = 0;

    if (!getFrame()->client())
        return ;

    if (titleButtonsLeft) {
        int minX = 0;

        for (const char *bc = titleButtonsLeft; *bc; bc++) {
            YWindow *b = getFrame()->getButton(*bc);
            if (b) {
                int r = b->x() + b->width();
                if (r > minX)
                    minX = r;
            }
        }
        onLeft = minX;
    }
    {
        int maxX = width();

        for (const char *bc = titleButtonsRight; *bc; bc++) {
            YWindow *b = getFrame()->getButton(*bc);
            if (b) {
                int l = b->x();
                if (l < maxX)
                    maxX = l;
            }
        }
        onRight = width() - maxX;
    }
    
    g.setFont(titleFont);
    int stringOffset = onLeft + 3;

#ifdef CONFIG_LOOK_MOTIF
    if (wmLook == lookMotif)
        stringOffset++;
#endif
#ifdef CONFIG_LOOK_WARP4
    if (wmLook == lookWarp4)
        stringOffset++;
#endif

    const char *title = getFrame()->client()->windowTitle();
    int yPos =
        (height() - titleFont->height()) / 2
        + titleFont->ascent();
    int tlen = title ? titleFont->textWidth(title) : 0;

    if (titleBarCentered) {
        int w = width() - onLeft - onRight;
        stringOffset = onLeft + w / 2 - tlen / 2;
        if (stringOffset < onLeft + 3)
            stringOffset = onLeft + 3;
    }

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
        if (getFrame()->focused()) {
            g.fillRect(1, 1, width() - 2, height() - 2);
            g.setColor(inactiveTitleBarBg);
            g.draw3DRect(onLeft, 0, width() - onRight - 1, height() - 1, false);
        } else {
            g.fillRect(0, 0, width(), height());
        }
        break;
#endif
#ifdef CONFIG_LOOK_MOTIF
    case lookMotif:
        g.fillRect(1, 1, width() - 2, height() - 2);
        g.draw3DRect(onLeft, 0, width() - onRight - 1 - onLeft, height() - 1, true);
        break;
#endif
#ifdef CONFIG_LOOK_PIXMAP
    case lookPixmap:
    case lookMetal:
    case lookGtk:
        {
            int xx = onLeft;
            int n = getFrame()->focused() ? 1 : 0;

            g.fillRect(width() - onRight, 0, onRight, height());
            if (titleBarCentered && titleS[n] != 0 && titleP[n] != 0) {
            } else {
                stringOffset = onLeft;
                if (titleL[n])
                    stringOffset += titleL[n]->width();
                else
                    stringOffset += 2;
                if (titleP[n])
                    stringOffset += titleP[n]->width();
            }
            if (titleL[n]) {
                g.drawPixmap(titleL[n], xx, 0); xx += titleL[n]->width();
            }
            if (titleBarCentered && titleS[n] != 0 && titleP[n] != 0) {
                int l = stringOffset - xx;
                if (titleP[n])
                    l -= titleP[n]->width();
                if (l < 0)
                    l = 0;
                if (titleS[n]) {
                    g.repHorz(titleS[n], xx, 0, l); xx += l;
                }
                if (titleP[n]) {
                    g.drawPixmap(titleP[n], xx, 0); xx += titleP[n]->width();
                }
            } else if (titleP[n]) {
                g.drawPixmap(titleP[n], xx, 0); xx += titleP[n]->width();
            }
            if (titleT[n]) {
                g.repHorz(titleT[n], xx, 0, tlen); xx += tlen;
            }
            if (titleM[n]) {
                g.drawPixmap(titleM[n], xx, 0); xx += titleM[n]->width();
            }
            if (titleB[n])
                g.repHorz(titleB[n], xx, 0, width() - onRight - xx);
            else {
                g.fillRect(xx, 0, width() - onRight - xx, height());
            }
            if (titleR[n]) {
                xx = width() - onRight - titleR[n]->width();
                g.drawPixmap(titleR[n], xx, 0);
            }
        }
        break;
#endif
    default:
        break;
    }
    g.setColor(fg);
    if (title) {
#if 0
        g.drawChars(title, 0, strlen(title),
                    stringOffset, yPos);
#else
        g.drawCharsEllipsis(title, strlen(title),
                            stringOffset, yPos, (width() - onRight) - 1 - stringOffset);
#endif
    }
}
