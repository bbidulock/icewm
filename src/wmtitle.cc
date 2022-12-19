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
#include "wmmgr.h"
#include "wpixmaps.h"
#include "yprefs.h"
#include "prefs.h"
#include "intl.h"

static YFont titleFont;
bool YFrameTitleBar::swapTitleButtons;

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
        titleFont = titleFontName;
        if (rightToLeft) {
            swapTitleButtons = true;
            int pn = 0;
            ref<YPixmap> old;
            for (char c : YRange<const char>(titleButtonsSupported,
                                      strlen(titleButtonsSupported))) {
                ref<YPixmap> pix;
                switch (c) {
                    case Depth : pix = depthPixmap[pn]; break;
                    case Hide  : pix = hidePixmap[pn]; break;
                    case Mini  : pix = minimizePixmap[pn]; break;
                    case Maxi  : pix = maximizePixmap[pn]; break;
                    case Roll  : pix = rollupPixmap[pn]; break;
                    case Menu  : pix = ::menuButton[pn]; break;
                    case Close : pix = closePixmap[pn]; break;
                }
                if (pix != null) {
                    if (old != null) {
                        swapTitleButtons &= (pix->width() == old->width());
                    }
                    old = pix;
                }
            }
        }
    }
}

void freeTitleColorsFonts() {
    titleFont = null;
}

YColor YFrameTitleBar::background(bool active) {
    return titleBarBackground[active];
}

YFrameTitleBar::YFrameTitleBar(YFrameWindow *frame):
    YWindow(frame),
    fFrame(frame),
    fPartner(nullptr),
    fDragX(0),
    fDragY(0),
    fRoom(0),
    fLeftTabX(0),
    fLeftTabLen(0),
    fRightTabX(0),
    fRightTabLen(0),
    wasCanRaise(false),
    fVisible(false),
    fToggle(false)
{
    initTitleColorsFonts();
    addStyle(wsNoExpose);
    setWinGravity(NorthGravity);
    setBitGravity(NorthWestGravity);
    addEventMask(VisibilityChangeMask);
    setTitle("TitleBar");

    memset(fButtons, 0, sizeof fButtons);
    relayout();
}

YFrameTitleBar::~YFrameTitleBar() {
    for (auto b : fButtons)
        delete b;
    if (fPartner) {
        fPartner->fPartner = nullptr;
    }
}

void YFrameTitleBar::setPartner(YFrameTitleBar* partner) {
    if (fPartner != partner) {
        if (fPartner) {
            fPartner->fPartner = nullptr;
        }
        fPartner = partner;
        if (partner) {
            if (fTimer == nullptr || fTimer->isRunning() == false) {
                fTimer->setTimer(0, this, true);
                fToggle = true;
            }
            if (partner->fPartner != this)
                partner->setPartner(this);
        }
    }
}

const char* YFrameTitleBar::titleButtons(Locate locate) {
    return locate == swapTitleButtons ? titleButtonsLeft : titleButtonsRight;
}

bool YFrameTitleBar::isRight(const YFrameButton* b) {
    return isRight(b->getKind());
}

bool YFrameTitleBar::isRight(char c) {
    return strchr(titleButtons(Distant), c);
}

bool YFrameTitleBar::supported(char c) {
    return strchr(titleButtonsSupported, c);
}

YFrameButton *YFrameTitleBar::getButton(char c) {
    unsigned index, flag;
    switch (c) {
    case Depth: index = 1; flag = YFrameWindow::fdDepth;    break;
    case Hide:  index = 2; flag = YFrameWindow::fdHide;     break;
    case Mini:  index = 3; flag = YFrameWindow::fdMinimize; break;
    case Maxi:  index = 4; flag = YFrameWindow::fdMaximize; break;
    case Roll:  index = 5; flag = YFrameWindow::fdRollup;   break;
    case Menu:  index = 6; flag = YFrameWindow::fdSysMenu;  break;
    case Close: index = 7; flag = YFrameWindow::fdClose;    break;
    default:    index = 0; flag = 0; break;
    }
    if (index) {
        if (hasbit(decors(), flag)) {
            if (fButtons[index] == nullptr && supported(c)) {
                fButtons[index] = new YFrameButton(this, c);
            }
        } else if (fButtons[index]) {
            delete fButtons[index];
            fButtons[index] = nullptr;
        }
    }
    return fButtons[index];
}

void YFrameTitleBar::handleButton(const XButtonEvent &button) {
    if (button.type == ButtonPress) {
        if ((buttonRaiseMask & (1 << (button.button - Button1))) &&
           !(button.state & (xapp->AltMask | ControlMask | xapp->ButtonMask))) {
            if ( !getFrame()->isRollup())
                getFrame()->activate();
            wasCanRaise = getFrame()->canRaise();
            if (raiseOnClickTitleBar)
                getFrame()->wmRaise();
        }
    }
    else if (button.type == ButtonRelease) {
        if ((button.button == Button1 || button.button == Button3) &&
            xapp->isButton(button.state, Button1Mask + Button3Mask))
        {
            windowList->showFocused(button.x_root, button.y_root);
        }
    }
    YWindow::handleButton(button);
}

bool YFrameTitleBar::hasMask(unsigned state, unsigned bits, unsigned ignore) {
    return (KEY_MODMASK(state) & ~ignore) == bits;
}

void YFrameTitleBar::handleClick(const XButtonEvent &up, int count) {
    YAction action(actionNull);
    if (count >= 2 && (count % 2 == 0)) {
        if (up.button == titleMaximizeButton && hasMask(up.state, 0))
        {
            action = actionMaximize;
        }
        else if (up.button == titleMaximizeButton && hasMask(up.state, ShiftMask))
        {
            action = actionMaximizeVert;
        }
        else if (up.button == titleMaximizeButton && xapp->AltMask &&
                 hasMask(up.state, xapp->AltMask + ShiftMask))
        {
            action = actionMaximizeHoriz;
        }
        else if (up.button == titleRollupButton && up.button <= Button3 &&
                 hasMask(up.state, 0))
        {
            action = actionRollup;
        }
        else if (up.button == titleRollupButton && up.button <= Button3 &&
                 hasMask(up.state, ShiftMask))
        {
            action = actionMaximizeHoriz;
        }
    }
    else if (count == 1) {
        if (up.button == Button3 && notbit(KEY_MODMASK(up.state), xapp->AltMask)) {
            getFrame()->popupSystemMenu(this, up.x_root, up.y_root,
                                        YPopupWindow::pfCanFlipVertical |
                                        YPopupWindow::pfCanFlipHorizontal);
        }
        else if (up.button == Button1) {
            if (KEY_MODMASK(up.state) == xapp->AltMask) {
                action = actionLower;
            }
            else if (lowerOnClickWhenRaised &&
                   (buttonRaiseMask & (1 << (up.button - Button1))) &&
                   ((up.state & (ControlMask | xapp->ButtonMask)) ==
                    Button1Mask))
            {
                if (!wasCanRaise) {
                    action = actionLower;
                    wasCanRaise = true;
                }
            }
            if (action == actionNull && KEY_MODMASK(up.state) == None) {
                if (inrange(up.x, fLeftTabX, fLeftTabX + fLeftTabLen - 1)) {
                    getFrame()->changeTab(-1);
                }
                if (inrange(up.x, fRightTabX, fRightTabX + fRightTabLen - 1)) {
                    getFrame()->changeTab(+1);
                }
            }
        }
        else if (inrange<unsigned>(up.button, Button4, Button5) &&
                 inrange<unsigned>(titleRollupButton, Button4, Button5))
        {
            if (up.button == Button4 && !getFrame()->isRollup())
                action = actionRollup;
            if (up.button == Button5 && getFrame()->isRollup())
                action = actionRollup;
        }
    }
    if (action != actionNull) {
        getFrame()->actionPerformed(action, up.state);
    }
}

int YFrameTitleBar::isTabbingButton(unsigned button) {
    return button + XK_Pointer_Button1 - 1 == gMouseWinTabbing.key;
}

int YFrameTitleBar::isTabbingModifier(unsigned state) {
    return VMod(KEY_MODMASK(state)) == gMouseWinTabbing.mod;
}

bool YFrameTitleBar::handleBeginDrag(
        const XButtonEvent &down,
        const XMotionEvent &motion)
{
    // check for a drag on the reparented resize handles
    if (down.subwindow &&
        getFrame()->hasIndicators() &&
        (down.subwindow == getFrame()->topSideIndicator() ||
         down.subwindow == getFrame()->topLeftIndicator() ||
         down.subwindow == getFrame()->topRightIndicator()))
    {
        return getFrame()->handleBeginDrag(down, motion);
    }
    else if (getFrame()->canMove()) {
        if (isTabbingButton(getClickButton())) {
            fDragX = down.x_root;
            fDragY = down.y_root;
            setPointer(YWMApp::movePointer);
            handleDrag(down, motion);
        } else {
            getFrame()->startMoveSize(true, true,
                                      0, 0,
                                      down.x + x(), down.y + y());
        }
        return true;
    }
    return false;
}

bool YFrameTitleBar::isPartner(YFrameTitleBar* other) {
    bool partner = false;
    if (other->hasRoom() && other->getClient()->adopted()) {
        YRect geo(other->geometry());
        geo.xx += other->parent()->x();
        geo.yy += other->parent()->y();
        partner = geo.contains(fDragX, fDragY);
    }
    return partner;
}

YFrameTitleBar* YFrameTitleBar::findPartner() {
    YFrameTitleBar* partner = nullptr;
    bool one = false;
    for (YFrameWindow* f = manager->topLayer(); f; f = f->nextLayer()) {
        if (f->visible()) {
            if (f == getFrame()) {
                one = true;
            }
            else if (one && f->hasTitleBar() && isPartner(f->titlebar())) {
                partner = f->titlebar();
                break;
            }
            else {
                YRect geo(f->geometry());
                geo.yy += topSideVerticalOffset;
                geo.hh -= topSideVerticalOffset;
                if (geo.contains(fDragX, fDragY)) {
                    break;
                }
            }
        }
    }
    return partner;
}

void YFrameTitleBar::handleDrag(const XButtonEvent& down,
                                const XMotionEvent& motion)
{
    if (isTabbingButton(getClickButton())) {
        int dx = motion.x_root - fDragX;
        int dy = motion.y_root - fDragY;
        if (dx | dy) {
            getFrame()->setCurrentPositionOuter(getFrame()->x() + dx,
                                                getFrame()->y() + dy);
            fDragX = motion.x_root;
            fDragY = motion.y_root;

            if (getClient() && getClient()->adopted()) {
                setPartner(isTabbingModifier(motion.state)
                            ? findPartner() : nullptr);
            }
            getFrame()->checkEdgeSwitch(fDragX, fDragY);
        }
    }
}

void YFrameTitleBar::handleEndDrag(const XButtonEvent& d, const XButtonEvent& u) {
    if (isTabbingButton(u.button)) {
        setPointer(None);
        if (fPartner) {
            if (fPartner == findPartner()) {
                fPartner->getFrame()->mergeTabs(getFrame());
            } else {
                setPartner(nullptr);
            }
        }
    }
}

void YFrameTitleBar::activate() {
    repaint();
    if (LOOK(lookPixmap | lookMetal | lookGtk | lookFlat)) {
        for (auto b : fButtons)
            if (b)
                b->repaint();
    }
    else if (LOOK(lookWin95)) {
        if (menuButton())
            menuButton()->repaint();
    }
}

void YFrameTitleBar::layoutButtons() {
    bool const pi(focused());
    int left(titleJ[pi] != null ? int(titleJ[pi]->width()) : 0);
    int right(int(getFrame()->width()) - 2 * getFrame()->borderX() -
              (titleQ[pi] != null ? int(titleQ[pi]->width()) : 0));
    bool hid = false;

    const char* nearby = titleButtons(Nearby);
    if (nearby) {
        for (const char *bc = nearby; *bc; bc++) {
            if (*bc == ' ')
                left++;
            else {
                YFrameButton *b = getButton(*bc);
                if (b && isRight(b) == false) {
                    if (left + int(b->width()) >= right) {
                        b->hide();
                        hid = true;
                    } else {
                        b->setPosition(left, 0);
                        left += b->width();
                        b->show();
                    }
                }
            }
        }
    }

    const char* distant = titleButtons(Distant);
    if (distant) {
        for (const char *bc = distant; *bc; bc++) {
            if (*bc == ' ')
                right--;
            else {
                YFrameButton *b = getButton(*bc);
                if (b && isRight(b)) {
                    if (right - int(b->width()) <= left) {
                        b->hide();
                        hid = true;
                    } else {
                        right -= b->width();
                        b->setPosition(right, 0);
                        b->show();
                    }
                }
            }
        }
    }

    fRoom = (hid || right < left) ? 0 : (right - left);
}

void YFrameTitleBar::deactivate() {
    activate(); // for now
}

void YFrameTitleBar::refresh() {
    repaint();
    for (auto b : fButtons)
        if (b)
            b->repaint();
}

void YFrameTitleBar::repaint() {
    if (fVisible && width() > 1 && height() > 1 && getFrame()->tabCount()) {
        GraphicsBuffer(this).paint();
    }
}

bool YFrameTitleBar::handleTimer(YTimer* timer) {
    if (timer == fTimer) {
        if (fPartner == nullptr) {
            if (fToggle)
                fToggle = false;
            else
                repaint();
            return false;
        } else {
            repaint();
            fToggle ^= true;
            timer->setInterval(150);
            return true;
        }
    }
    return false;
}

void YFrameTitleBar::handleVisibility(const XVisibilityEvent& visib) {
    bool prev = fVisible;
    fVisible = (visib.state != VisibilityFullyObscured);
    if (prev < fVisible)
        repaint();
}

void YFrameTitleBar::configure(const YRect2& r) {
}

void YFrameTitleBar::relayout() {
    int h = getFrame()->titleY();
    if (h == 0) {
        hide();
    } else {
        int x = getFrame()->borderX();
        int y = getFrame()->borderY();
        int w = max(1, int(getFrame()->width()) - 2 * x);
        setGeometry(YRect(x, y, w, h));
        layoutButtons();
        repaint();
        show();
    }
}

void YFrameTitleBar::paint(Graphics &g, const YRect &/*r*/) {
    if (getClient() == nullptr || visible() == false)
        return;

    bool foci = focused() ^ (fToggle && fPartner);
    YColor bg = titleBarBackground[foci];
    YColor fg = titleBarForeground[foci];

    int onLeft(0);
    int onRight((int) width());

    if (titleQ[foci] != null)
        onRight -= int(titleQ[foci]->width());

    const char* nearby = titleButtons(Nearby);
    if (nearby) {
        for (const char *bc = nearby; *bc; bc++) {
            if (*bc == ' ')
                ++onLeft;
            else {
                YFrameButton const *b(getButton(*bc));
                if (b && b->visible() && isRight(b) == false)
                    onLeft = max(onLeft, (int)(b->x() + b->width()));
            }
        }
    }

    const char* distant = titleButtons(Distant);
    if (distant) {
        for (const char *bc = distant; *bc; bc++) {
            if (*bc == ' ')
                --onRight;
            else {
                YFrameButton const *b(getButton(*bc));
                if (b && b->visible() && isRight(b))
                    onRight = max(onRight - (int) b->width(), b->x());
            }
        }
    }

    mstring title = getFrame()->getTitle();
    int const fontHeight = titleFont ? titleFont->height() : 8;
    int const fontAscent = titleFont ? titleFont->ascent() : 6;
    int const yPos(int(height() - fontHeight) / 2 +
                   fontAscent + titleBarVertOffset);
    int tlen = title != null && titleFont ? titleFont->textWidth(title) : 0;

    const char* lstr = nullptr;
    const char* rstr = nullptr;
    int llen = 0;
    int rlen = 0;
    if (1 < getFrame()->tabCount()) {
        YFrameWindow* frame = getFrame();
        lstr = frame->lessTabs() ? "... | " : nullptr;
        rstr = frame->moreTabs() ? " | ..." : nullptr;
        llen = lstr && titleFont ? titleFont->textWidth(lstr) : 0;
        rlen = rstr && titleFont ? titleFont->textWidth(rstr) : 0;
        if (onRight - onLeft <= 3 * max(llen, rlen)) {
            llen = rlen = 0;
            lstr = rstr = nullptr;
        }
    }
    int stringOffset = onLeft + (onRight - onLeft - tlen - llen - rlen)
                              * titleBarJustify / 100;

    g.setColor(bg);
    g.setFont(titleFont);

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
            g.setColor(foci ? fg.darker() : bg.darker());
            g.drawLine(0, y2, width(), y2);
        }
        break;
    case lookWarp4:
        if (titleBarJustify == 0)
            stringOffset++;
        else if (titleBarJustify == 100)
            stringOffset--;

        if (foci) {
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
        bool const pi(foci);

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
        if ((llen | rlen) && lRight - lLeft <= 3 * max(llen, rlen)) {
            llen = rlen = 0;
            lstr = rstr = nullptr;
        }
        tlen = clamp(lRight - lLeft - llen - rlen, 0, tlen);
        stringOffset = lLeft + (lRight - lLeft - tlen - llen - rlen)
                     * titleBarJustify / 100;

        lLeft = stringOffset;
        lRight = stringOffset + tlen + llen + rlen;

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

        if (rgbTitleB[pi] != null || rgbTitleS[pi] != null ||
            rgbTitleT[pi] != null)
        {
            g.maxOpacity();
        }
        break;
    }
    }

    if (title != null && tlen && onLeft + 16 < onRight) {
        stringOffset += titleBarHorzOffset;

        if (lstr && llen) {
            if (titleBarShadowText[0]) {
                g.setColor(titleBarShadowText[0]);
                g.drawStringEllipsis(stringOffset + 1, yPos + 1, lstr, llen);
            }

            g.setColor(titleBarForeground[0]);
            g.drawStringEllipsis(stringOffset, yPos, lstr, llen);

            fLeftTabX = stringOffset;
            fLeftTabLen = llen;
            stringOffset += llen;
        } else {
            fLeftTabX = fLeftTabLen = 0;
        }

        if (titleBarShadowText[foci]) {
            g.setColor(titleBarShadowText[foci]);
            g.drawStringEllipsis(stringOffset + 1, yPos + 1, title, tlen);
        }

        g.setColor(fg);
        g.drawStringEllipsis(stringOffset, yPos, title, tlen);

        if (rstr && rlen) {
            stringOffset += tlen;
            fRightTabX = stringOffset;
            fRightTabLen = rlen;

            if (titleBarShadowText[0]) {
                g.setColor(titleBarShadowText[0]);
                g.drawStringEllipsis(stringOffset + 1, yPos + 1, rstr, rlen);
            }

            g.setColor(titleBarForeground[0]);
            g.drawStringEllipsis(stringOffset, yPos, rstr, rlen);
        } else {
            fRightTabX = fRightTabLen = 0;
        }
    }
}

void YFrameTitleBar::renderShape(Graphics& g) {
#ifdef CONFIG_SHAPE
    if (LOOK(lookPixmap | lookMetal | lookGtk | lookFlat))
    {
        int onLeft(0);
        int onRight((int) width());

        if (titleQ[focused()] != null)
            onRight -= int(titleQ[focused()]->width());

        const char* nearby = titleButtons(Nearby);
        if (nearby)
            for (const char *bc = nearby; *bc; bc++) {
                if (*bc == ' ')
                    ++onLeft;
                else {
                    YFrameButton const *b(getButton(*bc));
                    if (b && b->visible() && isRight(b) == false) {
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

        const char* distant = titleButtons(Distant);
        if (distant)
            for (const char *bc = distant; *bc; bc++) {
                if (*bc == ' ')
                    --onRight;
                else {
                    YFrameButton const *b(getButton(*bc));
                    if (b && b->visible() && isRight(b)) {
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

        mstring title = getFrame()->getTitle();
        int tlen = title != null && titleFont ? titleFont->textWidth(title) : 0;

        const char* lstr = nullptr;
        const char* rstr = nullptr;
        int llen = 0;
        int rlen = 0;
        if (1 < getFrame()->tabCount()) {
            YFrameWindow* frame = getFrame();
            lstr = frame->lessTabs() ? "... | " : nullptr;
            rstr = frame->moreTabs() ? " | ..." : nullptr;
            llen = lstr && titleFont ? titleFont->textWidth(lstr) : 0;
            rlen = rstr && titleFont ? titleFont->textWidth(rstr) : 0;
            if (onRight - onLeft <= 3 * max(llen, rlen)) {
                llen = rlen = 0;
                lstr = rstr = nullptr;
            }
        }

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

        if ((llen | rlen) && lRight - lLeft <= 3 * max(llen, rlen)) {
            llen = rlen = 0;
            lstr = rstr = nullptr;
        }
        tlen = clamp(lRight - lLeft - llen - rlen, 0, tlen);
        int stringOffset = lLeft + (lRight - lLeft - tlen - llen - rlen)
                         * titleBarJustify / 100;
        lLeft = stringOffset;
        lRight = stringOffset + tlen + llen + rlen;

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
#endif
}

// vim: set sw=4 ts=4 et:
