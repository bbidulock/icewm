/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#include "config.h"
#include "wmtitle.h"

#include "wmframe.h"
#include "wmaction.h"
#include "wmbutton.h"
#include "wmapp.h"
#include "deffonts.h"
#include "yconfig.h"
#include "ypaint.h"
#include "ybuttonevent.h"

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

YPixmapPrefProperty YFrameTitleBar::gTitleAL("icewm", "PixmapTitleAL", "titleAL.xpm");
YPixmapPrefProperty YFrameTitleBar::gTitleAS("icewm", "PixmapTitleAS", "titleAS.xpm");
YPixmapPrefProperty YFrameTitleBar::gTitleAP("icewm", "PixmapTitleAP", "titleAP.xpm");
YPixmapPrefProperty YFrameTitleBar::gTitleAT("icewm", "PixmapTitleAT", "titleAT.xpm");
YPixmapPrefProperty YFrameTitleBar::gTitleAM("icewm", "PixmapTitleAM", "titleAM.xpm");
YPixmapPrefProperty YFrameTitleBar::gTitleAB("icewm", "PixmapTitleAB", "titleAB.xpm");
YPixmapPrefProperty YFrameTitleBar::gTitleAR("icewm", "PixmapTitleAR", "titleAR.xpm");

YPixmapPrefProperty YFrameTitleBar::gTitleIL("icewm", "PixmapTitleIL", "titleIL.xpm");
YPixmapPrefProperty YFrameTitleBar::gTitleIS("icewm", "PixmapTitleIS", "titleIS.xpm");
YPixmapPrefProperty YFrameTitleBar::gTitleIP("icewm", "PixmapTitleIP", "titleIP.xpm");
YPixmapPrefProperty YFrameTitleBar::gTitleIT("icewm", "PixmapTitleIT", "titleIT.xpm");
YPixmapPrefProperty YFrameTitleBar::gTitleIM("icewm", "PixmapTitleIM", "titleIM.xpm");
YPixmapPrefProperty YFrameTitleBar::gTitleIB("icewm", "PixmapTitleIB", "titleIB.xpm");
YPixmapPrefProperty YFrameTitleBar::gTitleIR("icewm", "PixmapTitleIR", "titleIR.xpm");

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
        fMaximizeButton->__setToolTip("Maximize");
    }

    if (!isButton('i'))
        fMinimizeButton = 0;
    else {
        fMinimizeButton = new YFrameButton(this, fFrame, actionMinimize, actionHide);
        //fMinimizeButton->setWinGravity(NorthEastGravity);
        fMinimizeButton->__setToolTip("Minimize");
        fMinimizeButton->show();
    }

    if (!isButton('x'))
        fCloseButton = 0;
    else {
        fCloseButton = new YFrameButton(this, fFrame, actionClose, actionKill);
        //fCloseButton->setWinGravity(NorthEastGravity);
        fCloseButton->__setToolTip("Close");
        fCloseButton->show();
    }

    if (!isButton('h'))
        fHideButton = 0;
    else {
        fHideButton = new YFrameButton(this, fFrame, actionHide, actionHide);
        //fHideButton->setWinGravity(NorthEastGravity);
        fHideButton->__setToolTip("Hide");
        fHideButton->show();
    }

    if (!isButton('r'))
        fRollupButton = 0;
    else {
        fRollupButton = new YFrameButton(this, fFrame, actionRollup, actionRollup);
        //fRollupButton->setWinGravity(NorthEastGravity);
        fRollupButton->__setToolTip("Rollup");
        fRollupButton->show();
    }

    if (!isButton('d'))
        fDepthButton = 0;
    else {
        fDepthButton = new YFrameButton(this, fFrame, actionDepth, actionDepth);
        //fDepthButton->setWinGravity(NorthEastGravity);
        fDepthButton->__setToolTip("Raise/Lower");
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

bool YFrameTitleBar::eventButton(const YButtonEvent &button) {
    if (button.type() == YEvent::etButtonPress) {
        if (getFrame()->shouldRaise(button) &&
            (button.getModifiers() == 0))
        {
            getFrame()->activate();
            if (gRaiseOnClickTitleBar.getBool())
                getFrame()->wmRaise();
        }
    }
#if 0
#ifdef CONFIG_WINLIST
    else if (button.type == ButtonRelease) {
        if (button.button == 1 &&
            IS_BUTTON(BUTTON_MODMASK(button.state), Button1Mask + Button3Mask))
        {
            fFrame->getRoot()->showWindowList(button.x_root, button.y_root);
        }
    }
#endif
#endif
    return YWindow::eventButton(button);
}

bool YFrameTitleBar::eventClick(const YClickEvent &up) {
    int mods = (up.getKeyModifiers() & ~YEvent::mCtrl);

    if (up.isDoubleClick()) {
        int maxb = gTitleMaximizeButton.getNum();
        int rolb = gTitleRollupButton.getNum();

        if (up.getButton() == maxb && mods == 0) {
            if (getFrame()->canMaximize())
                getFrame()->wmMaximize();
        } else if (up.getButton() == maxb && mods == YEvent::mShift) {
            if (getFrame()->canMaximize())
                getFrame()->wmMaximizeVert();
        } else if (up.getButton() == maxb && mods == YEvent::mAlt) {
            if (getFrame()->canMaximize())
                getFrame()->wmMaximizeHorz();
        } else if (up.getButton() == rolb && mods == 0) {
            if (getFrame()->canRollup())
                getFrame()->wmRollup();
        } else if (up.getButton() == rolb && mods == YEvent::mShift) {
            if (getFrame()->canMaximize())
                getFrame()->wmMaximizeHorz();
        }
    } else if (up.isSingleClick()) {
        if (up.getButton() == 3 && !(mods & YEvent::mAlt)) {
            getFrame()->popupSystemMenu(up.x_root(), up.y_root(), -1, -1,
                                        YPopupWindow::pfCanFlipVertical |
                                        YPopupWindow::pfCanFlipHorizontal);
        } else if (up.getButton() == 1 && mods == YEvent::mAlt) {
            if (getFrame()->canLower())
                getFrame()->wmLower();
        }
    }
    return YWindow::eventClick(up);
}

bool YFrameTitleBar::eventBeginDrag(const YButtonEvent &down, const YMotionEvent &motion) {
    if (getFrame()->canMove()) {
        getFrame()->startMoveSize(1, 1,
                                  0, 0,
                                  down.x() + x(), down.y() + y());
        return true;
    }
    return YWindow::eventBeginDrag(down, motion);
}

void YFrameTitleBar::activate() {
    repaint();
    if (menuButton()) menuButton()->activate();
    if (closeButton()) closeButton()->activate();
    if (maximizeButton()) maximizeButton()->activate();
    if (minimizeButton()) minimizeButton()->activate();
    if (hideButton()) hideButton()->activate();
    if (rollupButton()) rollupButton()->activate();
    if (depthButton()) depthButton()->activate();
}

void YFrameTitleBar::deactivate() {
    repaint();
    if (menuButton()) menuButton()->deactivate();
    if (closeButton()) closeButton()->deactivate();
    if (maximizeButton()) maximizeButton()->deactivate();
    if (minimizeButton()) minimizeButton()->deactivate();
    if (hideButton()) hideButton()->deactivate();
    if (rollupButton()) rollupButton()->deactivate();
    if (depthButton()) depthButton()->deactivate();
}

int YFrameTitleBar::titleLen() {
    const CStr *title = getFrame()->client()->windowTitle();
    YFont *font = gTitleFont.getFont();
    int tlen = (title && font) ? font->textWidth(title) : 0;
    return tlen;
}

void YFrameTitleBar::paint(Graphics &g, const YRect &/*er*/) {
    YColor *bg = getFrame()->focused() ? gTitleActiveBg.getColor() : gTitleNormalBg.getColor();
    YColor *fg = getFrame()->focused() ? gTitleActiveFg.getColor() : gTitleNormalFg.getColor();
    int onLeft = 0;
    int onRight = 0;

    if (!getFrame()->client())
        return;

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

    g.setColor(bg);
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
    g.setColor(fg);
    if (title && title->c_str()) {
        g.__drawCharsEllipsis(title->c_str(), title->length(),
                            stringOffset, yPos, (width() - onRight) - 1 - stringOffset);
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
    } else {
        int bw = b->getImage(0) ? b->getImage(0)->width() : titleY;

        if (onRight) xPos -= bw;
        b->setGeometry(xPos, 0, bw, titleY);
        if (!onRight) xPos += bw;
    }
}

void YFrameTitleBar::layoutButtons() {
    if (getFrame()->titleY() == 0)
        return;

    unsigned long decors = getFrame()->frameDecors();

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
        if ((decors & YFrameWindow::fdClose))
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
        int xPos = width();

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
