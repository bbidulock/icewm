/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#include "config.h"
#include "ylib.h"
#include "wmtitle.h"

#include "wmframe.h"
#include "wmaction.h"
#include "wmbutton.h"
#include "wmapp.h"
#include "default.h"
#include "yconfig.h"

#include <string.h>
#include "ycstring.h"

YStrPrefProperty YFrameTitleBar::gTitleButtonsSupported("icewm", "TitleButtonsSupported", "xmis");
YStrPrefProperty YFrameTitleBar::gTitleButtonsLeft("icewm", "TitleButtonsLeft", "s");
YStrPrefProperty YFrameTitleBar::gTitleButtonsRight("icewm", "TitleButtonsRight", "xmir");
YNumPrefProperty YFrameTitleBar::gTitleMaximizeButton("icewm", "TitleMaximizeButton", 1);
YNumPrefProperty YFrameTitleBar::gTitleRollupButton("icewm", "TitleRollupButton", 2);
YBoolPrefProperty YFrameTitleBar::gTitleBarCentered("icewm", "TitleBarCentered", false);
YBoolPrefProperty YFrameTitleBar::gRaiseOnClickTitleBar("icewm", "RaiseOnClickTitleBar", true);

YFontPrefProperty YFrameTitleBar::gTitleFont("icewm", "FontTitleBar", BOLDFONT(120));
YColorPrefProperty YFrameTitleBar::gTitleNormalBg("icewm", "ColorInactiveTitleBar", "rgb:80/80/80");
YColorPrefProperty YFrameTitleBar::gTitleNormalFg("icewm", "ColorInactiveTitleBarText", "rgb:00/00/00");
YColorPrefProperty YFrameTitleBar::gTitleActiveBg("icewm", "ColorActiveTitleBar", "rgb:00/00/A0");
YColorPrefProperty YFrameTitleBar::gTitleActiveFg("icewm", "ColorActiveTitleBarText", "rgb:FF/FF/FF");

YPixmapPrefProperty YFrameTitleBar::gTitleAL("icewm", "PixmapTitleAL", "titleAL.xpm", LIBDIR);
YPixmapPrefProperty YFrameTitleBar::gTitleAS("icewm", "PixmapTitleAS", "titleAS.xpm", LIBDIR);
YPixmapPrefProperty YFrameTitleBar::gTitleAP("icewm", "PixmapTitleAP", "titleAP.xpm", LIBDIR);
YPixmapPrefProperty YFrameTitleBar::gTitleAT("icewm", "PixmapTitleAT", "titleAT.xpm", LIBDIR);
YPixmapPrefProperty YFrameTitleBar::gTitleAM("icewm", "PixmapTitleAM", "titleAM.xpm", LIBDIR);
YPixmapPrefProperty YFrameTitleBar::gTitleAB("icewm", "PixmapTitleAB", "titleAB.xpm", LIBDIR);
YPixmapPrefProperty YFrameTitleBar::gTitleAR("icewm", "PixmapTitleAR", "titleAR.xpm", LIBDIR);

YPixmapPrefProperty YFrameTitleBar::gTitleIL("icewm", "PixmapTitleIL", "titleIL.xpm", LIBDIR);
YPixmapPrefProperty YFrameTitleBar::gTitleIS("icewm", "PixmapTitleIS", "titleIS.xpm", LIBDIR);
YPixmapPrefProperty YFrameTitleBar::gTitleIP("icewm", "PixmapTitleIP", "titleIP.xpm", LIBDIR);
YPixmapPrefProperty YFrameTitleBar::gTitleIT("icewm", "PixmapTitleIT", "titleIT.xpm", LIBDIR);
YPixmapPrefProperty YFrameTitleBar::gTitleIM("icewm", "PixmapTitleIM", "titleIM.xpm", LIBDIR);
YPixmapPrefProperty YFrameTitleBar::gTitleIB("icewm", "PixmapTitleIB", "titleIB.xpm", LIBDIR);
YPixmapPrefProperty YFrameTitleBar::gTitleIR("icewm", "PixmapTitleIR", "titleIR.xpm", LIBDIR);

YFrameTitleBar::YFrameTitleBar(YWindow *parent, YFrameWindow *frame):
    YWindow(parent)
{
    fFrame = frame;

    if (!isButton('m')) /// optimize strchr (flags)
        fMaximizeButton = 0;
    else {
        fMaximizeButton = new YFrameButton(this, fFrame, actionMaximize, actionMaximizeVert);
        //fMaximizeButton->setWinGravity(NorthEastGravity);
        fMaximizeButton->show();
        fMaximizeButton->_setToolTip("Maximize");
    }

    if (!isButton('i'))
        fMinimizeButton = 0;
    else {
        fMinimizeButton = new YFrameButton(this, fFrame, actionMinimize, actionHide);
        //fMinimizeButton->setWinGravity(NorthEastGravity);
        fMinimizeButton->_setToolTip("Minimize");
        fMinimizeButton->show();
    }

    if (!isButton('x'))
        fCloseButton = 0;
    else {
        fCloseButton = new YFrameButton(this, fFrame, actionClose, actionKill);
        //fCloseButton->setWinGravity(NorthEastGravity);
        fCloseButton->_setToolTip("Close");
        fCloseButton->show();
    }

    if (!isButton('h'))
        fHideButton = 0;
    else {
        fHideButton = new YFrameButton(this, fFrame, actionHide, actionHide);
        //fHideButton->setWinGravity(NorthEastGravity);
        fHideButton->_setToolTip("Hide");
        fHideButton->show();
    }

    if (!isButton('r'))
        fRollupButton = 0;
    else {
        fRollupButton = new YFrameButton(this, fFrame, actionRollup, actionRollup);
        //fRollupButton->setWinGravity(NorthEastGravity);
        fRollupButton->_setToolTip("Rollup");
        fRollupButton->show();
    }

    if (!isButton('d'))
        fDepthButton = 0;
    else {
        fDepthButton = new YFrameButton(this, fFrame, actionDepth, actionDepth);
        //fDepthButton->setWinGravity(NorthEastGravity);
        fDepthButton->_setToolTip("Raise/Lower");
        fDepthButton->show();
    }

    if (!isButton('s'))
        fMenuButton = 0;
    else {
        fMenuButton = new YFrameButton(this, fFrame, 0);
        fMenuButton->show();
        fMenuButton->setActionListener(fFrame);
    }
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

void YFrameTitleBar::handleButton(const XButtonEvent &button) {
    if (button.type == ButtonPress) {
        if (getFrame()->shouldRaise(button) &&
            (button.state & (app->getAltMask() | ControlMask | Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask)) == 0)
        {
            getFrame()->activate();
            if (gRaiseOnClickTitleBar.getBool())
                getFrame()->wmRaise();
        }
    }
#ifdef CONFIG_WINLIST
    else if (button.type == ButtonRelease) {
        if (button.button == 1 &&
            IS_BUTTON(BUTTON_MODMASK(button.state), Button1Mask + Button3Mask))
        {
            fFrame->getRoot()->showWindowList(button.x_root, button.y_root);
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
        unsigned int maxb = gTitleMaximizeButton.getNum();
        unsigned int rolb = gTitleRollupButton.getNum();

        if (up.button == maxb &&
            ISMASK(KEY_MODMASK(up.state), 0, ControlMask))
        {
            if (getFrame()->canMaximize())
                getFrame()->wmMaximize();
        } else if (up.button == maxb &&
                   ISMASK(KEY_MODMASK(up.state), ShiftMask, ControlMask))
        {
            if (getFrame()->canMaximize())
                getFrame()->wmMaximizeVert();
        } else if (up.button == maxb && app->getAltMask() &&
                   ISMASK(KEY_MODMASK(up.state), app->getAltMask() + ShiftMask, ControlMask))
        {
            if (getFrame()->canMaximize())
                getFrame()->wmMaximizeHorz();
        } else if (up.button == rolb &&
                 ISMASK(KEY_MODMASK(up.state), 0, ControlMask))
        {
            if (getFrame()->canRollup())
                getFrame()->wmRollup();
        } else if (up.button == rolb &&
                   ISMASK(KEY_MODMASK(up.state), ShiftMask, ControlMask))
        {
            if (getFrame()->canMaximize())
                getFrame()->wmMaximizeHorz();
        }
    } else if (count == 1) {
        if (up.button == 3 && (KEY_MODMASK(up.state) & (app->getAltMask())) == 0) {
            getFrame()->popupSystemMenu(up.x_root, up.y_root, -1, -1,
                                        YPopupWindow::pfCanFlipVertical |
                                        YPopupWindow::pfCanFlipHorizontal);
        } else if (up.button == 1 && KEY_MODMASK(up.state) == app->getAltMask()) {
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
    if (wmLook == lookWin95 && menuButton())
        menuButton()->repaint();
#endif
#ifdef CONFIG_LOOK_PIXMAP
    if (wmLook == lookPixmap || wmLook == lookMetal || wmLook == lookGtk) {
        if (menuButton()) menuButton()->repaint();
        if (closeButton()) closeButton()->repaint();
        if (maximizeButton()) maximizeButton()->repaint();
        if (minimizeButton()) minimizeButton()->repaint();
        if (hideButton()) hideButton()->repaint();
        if (rollupButton()) rollupButton()->repaint();
        if (depthButton()) depthButton()->repaint();
    }
#endif
}

void YFrameTitleBar::deactivate() {
    activate(); // for now
}

int YFrameTitleBar::titleLen() {
    const CStr *title = getFrame()->client()->windowTitle();
    YFont *font = gTitleFont.getFont();
    int tlen = (title && font) ? font->textWidth(title) : 0;
    return tlen;
}

void YFrameTitleBar::paint(Graphics &g, int , int , unsigned int , unsigned int ) {
    YColor *bg = getFrame()->focused() ? gTitleActiveBg.getColor() : gTitleNormalBg.getColor();
    YColor *fg = getFrame()->focused() ? gTitleActiveFg.getColor() : gTitleNormalFg.getColor();
    int onLeft = 0;
    int onRight = 0;

    if (!getFrame()->client())
        return ;

    if (gTitleButtonsLeft.getStr()) {
        int minX = 0;

        for (const char *bc = gTitleButtonsLeft.getStr(); *bc; bc++) {
            YWindow *b = getButton(*bc);
            if (b) {
                int r = b->x() + b->width();
                if (r > minX)
                    minX = r;
            }
        }
        onLeft = minX;
    }
    if (gTitleButtonsRight.getStr()) {
        int maxX = width();

        for (const char *bc = gTitleButtonsRight.getStr(); *bc; bc++) {
            YWindow *b = getButton(*bc);
            if (b) {
                int l = b->x();
                if (l < maxX)
                    maxX = l;
            }
        }
        onRight = width() - maxX;
    }

    YFont *font = gTitleFont.getFont();
    g.setFont(font);
    int stringOffset = onLeft + 3;

#ifdef CONFIG_LOOK_MOTIF
    if (wmLook == lookMotif)
        stringOffset++;
#endif
#ifdef CONFIG_LOOK_WARP4
    if (wmLook == lookWarp4)
        stringOffset++;
#endif

    bool center = gTitleBarCentered.getBool();
    const CStr *title = getFrame()->client()->windowTitle();
    int yPos =
        (height() - font->height()) / 2
        + font->ascent();
    int tlen = title ? font->textWidth(title) : 0;

    if (center) {
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
            g.setColor(gTitleNormalBg.getColor());
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

            YPixmap *titleS = 0;
            YPixmap *titleP = 0;
            YPixmap *titleL = 0;
            YPixmap *titleM = 0;
            YPixmap *titleT = 0;
            YPixmap *titleB = 0;
            YPixmap *titleR = 0;

            if (n == 1) {
                titleS = gTitleAS.tiledPixmap(true);
                titleP = gTitleAP.getPixmap();
                titleL = gTitleAL.getPixmap();
                titleM = gTitleAM.getPixmap();
                titleT = gTitleAT.tiledPixmap(true);
                titleB = gTitleAM.tiledPixmap(true);
                titleR = gTitleAM.getPixmap();
            } else {
                titleS = gTitleIS.tiledPixmap(true);
                titleP = gTitleIP.getPixmap();
                titleL = gTitleIL.getPixmap();
                titleM = gTitleIM.getPixmap();
                titleT = gTitleIT.tiledPixmap(true);
                titleB = gTitleIM.tiledPixmap(true);
                titleR = gTitleIM.getPixmap();
            }

            g.fillRect(width() - onRight, 0, onRight, height());
            if (center && titleS != 0 && titleP != 0) {
            } else {
                stringOffset = onLeft;
                if (titleL)
                    stringOffset += titleL->width();
                else
                    stringOffset += 2;
                if (titleP)
                    stringOffset += titleP->width();
            }
            if (titleL) {
                g.drawPixmap(titleL, xx, 0); xx += titleL->width();
            }
            if (center && titleS != 0 && titleP != 0) {
                int l = stringOffset - xx;
                if (titleP)
                    l -= titleP->width();
                if (l < 0)
                    l = 0;
                if (titleS) {
                    g.repHorz(titleS, xx, 0, l); xx += l;
                }
                if (titleP) {
                    g.drawPixmap(titleP, xx, 0); xx += titleP->width();
                }
            } else if (titleP) {
                g.drawPixmap(titleP, xx, 0); xx += titleP->width();
            }
            if (titleT) {
                g.repHorz(titleT, xx, 0, tlen); xx += tlen;
            }
            if (titleM) {
                g.drawPixmap(titleM, xx, 0); xx += titleM->width();
            }
            if (titleB)
                g.repHorz(titleB, xx, 0, width() - onRight - xx);
            else {
                g.fillRect(xx, 0, width() - onRight - xx, height());
            }
            if (titleR) {
                xx = width() - onRight - titleR->width();
                g.drawPixmap(titleR, xx, 0);
            }
        }
        break;
#endif
    default:
        break;
    }
    g.setColor(fg);
    if (title && title->c_str()) {
#if 0
        g.drawChars(title, 0, strlen(title),
                    stringOffset, yPos);
#else
        g.drawCharsEllipsis(title->c_str(), title->length(),
                            stringOffset, yPos, (width() - onRight) - 1 - stringOffset);
#endif
    }
}

bool YFrameTitleBar::isButton(char c) {
    if (strchr(gTitleButtonsSupported.getStr(), c) == 0)
        return false;
    if (strchr(gTitleButtonsRight.getStr(), c) != 0 ||
        strchr(gTitleButtonsLeft.getStr(), c) != 0)
        return true;
    return false;
}

YFrameButton *YFrameTitleBar::getButton(char c) {
    unsigned long decors = getFrame()->frameDecors();
    switch (c) {
    case 's': if (decors & YFrameWindow::fdSysMenu) return fMenuButton; break;
    case 'x': if (decors & YFrameWindow::fdClose) return fCloseButton; break;
    case 'm': if (decors & YFrameWindow::fdMaximize) return fMaximizeButton; break;
    case 'i': if (decors & YFrameWindow::fdMinimize) return fMinimizeButton; break;
    case 'h': if (decors & YFrameWindow::fdHide) return fHideButton; break;
    case 'r': if (decors & YFrameWindow::fdRollup) return fRollupButton; break;
    case 'd': if (decors & YFrameWindow::fdDepth) return fDepthButton; break;
    default:
        return 0;
    }
    return 0;
}

void YFrameTitleBar::positionButton(YFrameButton *b, int &xPos, bool onRight) {
    int titleY = getFrame()->titleY();
    /// !!! clean this up
    if (b == fMenuButton) {
        if (onRight) xPos -= titleY;
        b->setGeometry(xPos, 0, titleY, titleY);
        if (!onRight) xPos += titleY;
    } else if (
#if 0 //!!!
               wmLook == lookPixmap ||
#endif
               wmLook == lookMetal || wmLook == lookGtk) {
        int bw = b->getImage(0) ? b->getImage(0)->width() : titleY;

        if (onRight) xPos -= bw;
        b->setGeometry(xPos, 0, bw, titleY);
        if (!onRight) xPos += bw;
    } else if (wmLook == lookWin95) {
        if (onRight) xPos -= titleY;
        b->setGeometry(xPos, 2, titleY, titleY - 3);
        if (!onRight) xPos += titleY;
    } else {
        if (onRight) xPos -= titleY;
        b->setGeometry(xPos, 0, titleY, titleY);
        if (!onRight) xPos += titleY;
    }
}

void YFrameTitleBar::layoutButtons() {
    if (getFrame()->titleY() == 0)
        return ;

    unsigned long decors = getFrame()->frameDecors();

    if (fMinimizeButton)
        if (decors & YFrameWindow::fdMinimize)
            fMinimizeButton->show();
        else
            fMinimizeButton->hide();

    if (fMaximizeButton)
        if (decors & YFrameWindow::fdMaximize)
            fMaximizeButton->show();
        else
            fMaximizeButton->hide();

    if (fRollupButton)
        if (decors & YFrameWindow::fdRollup)
            fRollupButton->show();
        else
            fRollupButton->hide();

    if (fHideButton)
        if (decors & YFrameWindow::fdHide)
            fHideButton->show();
        else
            fHideButton->hide();

    if (fCloseButton)
        if ((decors & YFrameWindow::fdClose))
            fCloseButton->show();
        else
            fCloseButton->hide();

    if (fMenuButton)
        if (decors & YFrameWindow::fdSysMenu)
            fMenuButton->show();
        else
            fMenuButton->hide();

    if (fDepthButton)
        if (decors & YFrameWindow::fdDepth)
            fDepthButton->show();
        else
            fDepthButton->hide();

    if (gTitleButtonsLeft.getStr()) {
        int xPos = 0;
        for (const char *bc = gTitleButtonsLeft.getStr(); *bc; bc++) {
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

    if (gTitleButtonsRight.getStr()) {
        int xPos = width();//!!!??? - (borderLeft() + borderRight());

        for (const char *bc = gTitleButtonsRight.getStr(); *bc; bc++) {
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
