/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 *
 * ScrollBar
 */
#include "config.h"

#ifndef LITE

#include "ykey.h"
#include "yscrollbar.h"

#include "yapp.h"
#include "prefs.h"
#include "sysdep.h"

YNumPrefProperty YScrollBar::gScrollBarStartDelay("system", "ScrollBarStartDelay", 500);
YNumPrefProperty YScrollBar::gScrollBarDelay("system", "ScrollBarDelay", 30);
YColorPrefProperty YScrollBar::gScrollBarBg("system", "ColorScrollBar", "rgb:A0/A0/A0");
YColorPrefProperty YScrollBar::gScrollBarArrow("system", "ColorScrollBarArrow", "rgb:C0/C0/C0");
YColorPrefProperty YScrollBar::gScrollBarSlider("system", "ColorScrollBarSlider", "rgb:C0/C0/C0");

YScrollBar::YScrollBar(YWindow *aParent):
    YWindow(aParent), fScrollTimer(this, gScrollBarStartDelay.getNum())
{
    fOrientation = Vertical;
    fMinimum = fMaximum = fValue = fVisibleAmount = 0;
    fUnitIncrement = fBlockIncrement = 1;
    fListener = 0;
    fScrollTo = goNone;
    fDNDScroll = false;
}


YScrollBar::YScrollBar(Orientation anOrientation, YWindow *aParent):
    YWindow(aParent), fScrollTimer(this, gScrollBarStartDelay.getNum())
{
    fOrientation = anOrientation;

    fMinimum = fMaximum = fValue = fVisibleAmount = 0;
    fUnitIncrement = fBlockIncrement = 1;
    fListener = 0;
    fScrollTo = goNone;
    fDNDScroll = false;
}

YScrollBar::YScrollBar(Orientation anOrientation,
                       int aValue, int aVisibleAmount, int aMin, int aMax,
                       YWindow *aParent):
    YWindow(aParent), fScrollTimer(this, gScrollBarStartDelay.getNum())
{
    fOrientation = anOrientation;
    fMinimum = aMin;
    fMaximum = aMax;
    fVisibleAmount = aVisibleAmount;
    fValue = aValue;

    fUnitIncrement = fBlockIncrement = 1;
    fListener = 0;
    fScrollTo = goNone;
}

YScrollBar::~YScrollBar() {
    //if (fScrollTimer && fScrollTimer->getTimerListener() == this)
    //    fScrollTimer->setTimerListener(0);
}

void YScrollBar::setOrientation(Orientation anOrientation) {
    if (anOrientation != fOrientation) {
        fOrientation = anOrientation;
        repaint();
    }
}

void YScrollBar::setMaximum(int aMaximum) {
    if (aMaximum < fMinimum)
        aMaximum = fMinimum;

    if (aMaximum != fMaximum) {
        fMaximum = aMaximum;
        repaint();
    }
}

void YScrollBar::setMinimum(int aMinimum) {
    if (aMinimum > fMaximum)
        aMinimum = fMaximum;

    if (aMinimum != fMinimum) {
        fMinimum = aMinimum;
        repaint();
    }
}

void YScrollBar::setVisibleAmount(int aVisibleAmount) {
    if (fVisibleAmount > fMaximum - fMinimum)
        fVisibleAmount = fMaximum - fMinimum;
    if (fVisibleAmount < 0)
        fVisibleAmount = 0;

    if (aVisibleAmount != fVisibleAmount) {
        fVisibleAmount = aVisibleAmount;
        repaint();
    }
}

void YScrollBar::setUnitIncrement(int anUnitIncrement) {
    fUnitIncrement = anUnitIncrement;
}

void YScrollBar::setBlockIncrement(int aBlockIncrement) {
    fBlockIncrement = aBlockIncrement;
}

void YScrollBar::setValue(int aValue) {
    if (aValue > fMaximum - fVisibleAmount)
        aValue = fMaximum - fVisibleAmount;
    if (aValue < fMinimum)
        aValue = fMinimum;

    if (aValue != fValue) {
        fValue = aValue;
        repaint();
    }
}

void YScrollBar::setValues(int aValue, int aVisibleAmount, int aMin, int aMax) {
    int v = aValue;

    if (aMax < aMin)
        aMax = aMin;
    if (aVisibleAmount > aMax - aMin)
        aVisibleAmount = aMax - aMin;
    if (aVisibleAmount < 0)
        aVisibleAmount = 0;
    if (aValue > aMax - aVisibleAmount)
        aValue = aMax - aVisibleAmount;
    if (aValue < aMin)
        aValue = aMin;

    if (aMax != fMaximum ||
        aMin != fMinimum ||
        aValue != fValue ||
        aVisibleAmount != fVisibleAmount)
    {
        fMinimum = aMin;
        fMaximum = aMax;
        fValue = aValue;
        fVisibleAmount = aVisibleAmount;
        repaint();
        if (v != fValue)
            fListener->move(this, fValue);
    }
}

void YScrollBar::drawArrow(Graphics &g, bool forward) {
    if (fOrientation == Vertical) {
        if (forward) {
            int w = width();
            int m = 7;
            int d = 5;
            int t = w / 2 + d / 2 + ((fScrollTo == goDown) ? 1 : 0);
            t += height() - width();
            int b = t - d;

            if (fValue < fMaximum - fVisibleAmount)
                g.setColor(YColor::black);
            else
                g.setColor(YColor::white);

            g.drawLine(m - d, b, m, t);
            g.drawLine(m - d + 1, b, m + 1, t);
            g.drawLine(m + d + 1, b, m + 1, t);
            g.drawLine(m + d, b, m, t);
        } else {
            int w = width();
            int m = 7;
            int d = 5;
            int t = w / 2 - d / 2 - ((fScrollTo == goUp) ? 1 : 0);
            int b = t + d;

            if (fValue > fMinimum)
                g.setColor(YColor::black);
            else
                g.setColor(YColor::white);

            g.drawLine(m - d, b, m, t);
            g.drawLine(m - d + 1, b, m + 1, t);
            g.drawLine(m + d + 1, b, m + 1, t);
            g.drawLine(m + d, b, m, t);
        }
    } else {
        if (forward) {
            int h = height();
            int m = 7;
            int d = 5;
            int l = h / 2 + d / 2 + ((fScrollTo == goDown) ? 1 : 0);
            l += width() - height();
            int b = l - d;

            if (fValue < fMaximum - fVisibleAmount)
                g.setColor(YColor::black);
            else
                g.setColor(YColor::white);

            g.drawLine(b, m - d, l, m);
            g.drawLine(b, m - d + 1, l, m + 1);
            g.drawLine(b, m + d + 1, l, m + 1);
            g.drawLine(b, m + d, l, m);
        } else {
            int h = height();
            int m = 7;
            int d = 5;
            int l = h / 2 - d / 2 - ((fScrollTo == goUp) ? 1 : 0);
            int b = l + d;

            if (fValue > fMinimum)
                g.setColor(YColor::black);
            else
                g.setColor(YColor::white);

            g.drawLine(b, m - d, l, m);
            g.drawLine(b, m - d + 1, l, m + 1);
            g.drawLine(b, m + d + 1, l, m + 1);
            g.drawLine(b, m + d, l, m);
        }
    }
}

void YScrollBar::getCoord(int &beg, int &end, int &min, int &max, int &nn) {
    int dd = (fMaximum - fMinimum);

    if (fOrientation == Vertical) {
        beg = width();
        end = height() - width();
    } else {
        beg = height();
        end = width() - height();
    }
    nn = end - beg;
    if (dd <= 0) {
        min = 0;
        max = nn;
        return ;
    }
    int aa = nn;
    int vv = aa * fVisibleAmount / dd;
    if (vv < SCROLLBAR_MIN) {
        vv = SCROLLBAR_MIN;
        aa = nn - vv;
        if (aa < 0) aa = 0;
        dd -= fVisibleAmount;
        if (dd <= 0) {
            min = 0;
            max = nn;
            return ;
        }
    }
    if (vv > nn)
        vv = nn;
    min = aa * fValue / dd;
    max = min + vv;
}

void YScrollBar::paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
    int beg, end, min, max, nn;

    getCoord(beg, end, min, max, nn);

    /// !!! optimize this
    if (fOrientation == Vertical) {
        /* outside */
        g.setColor(gScrollBarArrow.getColor());
        g.draw3DRect(0, 0, width() - 1, width() - 1,
                     (fScrollTo == goUp) ? false : true);
        g.fillRect(1, 1, width() - 2, width() - 2);
        drawArrow(g, false);

        g.setColor(gScrollBarArrow.getColor());
        g.draw3DRect(0, height() - width(), width() - 1, width() - 1,
                     (fScrollTo == goDown) ? false : true);
        g.fillRect(1, height() - width() + 1, width() - 2, width() - 2);

        drawArrow(g, true);

        if (nn > 0) {
            g.setColor(gScrollBarBg.getColor());
            if (min > 0 && min < nn)
                g.fillRect(0, beg, width(), min);

            if (max > 0 && max < nn)
                g.fillRect(0, max + beg, width(), nn - max);

            g.setColor(gScrollBarSlider.getColor());
            if (max - min > 2) {
                g.draw3DRect(0, min + width(), width() - 1, max - min - 1,
                             (fScrollTo == goPosition) ? false : true);
                g.fillRect(1, min + width() + 1, width() - 2, max - min - 2);
            } else if (max - min == 2) {
                g.draw3DRect(0, min + width(), width() - 1, max - min - 1 ,
                             (fScrollTo == goPosition) ? false : true);
            } else {
                g.fillRect(0, min + width(), width(), max - min);
            }
        }
    } else {
        /* outside */
        g.setColor(gScrollBarArrow.getColor());
        g.draw3DRect(0, 0, height() - 1, height() - 1,
                     (fScrollTo == goUp) ? false : true);
        g.fillRect(1, 1, height() - 2, height() - 2);
        drawArrow(g, false);

        g.setColor(gScrollBarArrow.getColor());
        g.draw3DRect(width() - height(), 0, height() - 1, height() - 1,
                     (fScrollTo == goDown) ? false : true);
        g.fillRect(width() - height() + 1, 1, height() - 2, height() - 2);

        drawArrow(g, true);

        if (nn > 0) {
            g.setColor(gScrollBarBg.getColor());
            if (min > 0 && min < nn)
                g.fillRect(beg, 0, min, height());

            if (max > 0 && max < nn)
                g.fillRect(max + beg, 0, nn - max, height());

            g.setColor(gScrollBarSlider.getColor());
            if (max - min > 2) {
                g.draw3DRect(min + height(), 0, max - min - 1, height() - 1,
                             (fScrollTo == goPosition) ? false : true);
                g.fillRect(min + height() + 1, 1, max - min - 2, height() - 2);
            } else if (max - min == 2) {
                g.draw3DRect(min + height(), 0, max - min - 1, height() - 1,
                             (fScrollTo == goPosition) ? false : true);
            } else {
                g.fillRect(min + height(), 0, max - min, height());
            }
        }
    }
}

void YScrollBar::scroll(int delta) {
    int fNewPos = fValue;

    if (delta == 0)
        return ;

    fNewPos += delta;

    if (fNewPos >= fMaximum - fVisibleAmount)
        fNewPos = fMaximum - fVisibleAmount;
    if (fNewPos < fMinimum)
        fNewPos = fMinimum;
    if (fNewPos != fValue) {
        delta = fNewPos - fValue;
        fValue = fNewPos;
        repaint();
        fListener->scroll(this, delta);
    }
}

void YScrollBar::move(int pos) {
    int fNewPos = pos;

    if (fNewPos >= fMaximum - fVisibleAmount)
        fNewPos = fMaximum - fVisibleAmount;
    if (fNewPos < fMinimum)
        fNewPos = fMinimum;
    if (fValue != fNewPos) {
        fValue = fNewPos;
        repaint();
        fListener->move(this, fValue);
    }
}

void YScrollBar::handleButton(const XButtonEvent &button) {
    if (handleScrollMouse(button) == true)
        return ;

    if (button.button != 1)
        return ;

    if (fMinimum >= fMaximum)
        return ;

    if (button.type == ButtonPress) {
        fScrollTo = getOp(button.x, button.y);
        doScroll();
        //if (fScrollTimer == 0)
        //    fScrollTimer = new YTimer(this, scrollBarStartDelay);
        //if (fScrollTimer) {
        //    fScrollTimer->setTimerListener(this);
        fScrollTimer.setInterval(gScrollBarStartDelay.getNum());
        fScrollTimer.startTimer();
        //}
        repaint();
    } else if (button.type == ButtonRelease) {
        fScrollTo = goNone;
        //if (fScrollTimer && fScrollTimer->getTimerListener() == this) {
        //    fScrollTimer->setTimerListener(0);
        fScrollTimer.stopTimer();
        //}
        repaint();
    }
}

void YScrollBar::handleMotion(const XMotionEvent &motion) {
    if (motion.state != Button1Mask || fScrollTo != goPosition)
        return ;

    if (fOrientation == Vertical) {
        int h = height() - 2 * width();

        if (h <= 0 || fMinimum >= fMaximum)
            return ;

        int y = motion.y - fGrabDelta - width();
        if (y < 0) y = 0;
        int pos = y * (fMaximum - fMinimum) / h;
        move(pos);
    } else {
        int w = width() - 2 * height();

        if (w <= 0 || fMinimum >= fMaximum)
            return ;

        int x = motion.x - fGrabDelta - height();
        if (x < 0) x = 0;
        int pos = x * (fMaximum - fMinimum) / w;
        move(pos);
    }
}

bool YScrollBar::handleScrollKeys(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        KeySym k = XKeycodeToKeysym(app->display(), key.keycode, 0);
        int m = KEY_MODMASK(key.state);

        switch (k) {
        case XK_Prior:
            if (m & ControlMask)
                move(0);
            else
                scroll(-fBlockIncrement);
            return true;
        case XK_Next:
            if (m & ControlMask)
                move(fMaximum - fVisibleAmount);
            else
                scroll(+fBlockIncrement);
            return true;
        }
    }
    return false;
}

bool YScrollBar::handleScrollMouse(const XButtonEvent &button) {
    if (button.type == ButtonPress) {
        if (button.button == 4) {
            scroll(-fBlockIncrement);
            return true;
        } else if (button.button == 5) {
            scroll(+fBlockIncrement);
            return true;
        }
    }
    return false;
}

void YScrollBar::doScroll() {
    switch (fScrollTo) {
    case goNone:
        break;
    case goUp:
        scroll(-fUnitIncrement);
        break;
    case goDown:
        scroll(+fUnitIncrement);
        break;
    case goPageUp:
        scroll(-fBlockIncrement);
        break;
    case goPageDown:
        scroll(+fBlockIncrement);
        break;
    case goPosition:
        break;
    }
}

bool YScrollBar::handleTimer(YTimer *timer) {
    if (timer != &fScrollTimer)
        return false;
    doScroll();
    if (!fDNDScroll || (fScrollTo != goPageUp && fScrollTo != goPageDown))
        timer->setInterval(gScrollBarDelay.getNum());
    return true;
}

YScrollBar::ScrollOp YScrollBar::getOp(int xx, int yy) {
    int beg, end, min, max, nn;
    ScrollOp fScrollTo;
    int a;

    getCoord(beg, end, min, max, nn);

    if (fOrientation == Vertical)
        a = yy;
    else
        a = xx;

    fScrollTo = goNone;
    if (a > end) {
        fScrollTo = goDown;
    } else if (a < beg) {
        fScrollTo = goUp;
    } else {
        if (a < beg + min) {
            fScrollTo = goPageUp;
        } else if (a > beg + max) {
            fScrollTo = goPageDown;
        } else {
            fScrollTo = goPosition;
            fGrabDelta = a - min - beg;
        }
    }
    return fScrollTo;
}

void YScrollBar::handleDNDEnter(int /*nTypes*/, Atom * /*types*/) {
    puts("scroll enter");
    fScrollTo = goNone;
    fDNDScroll = true;
    //if (fScrollTimer && fScrollTimer->getTimerListener() == this) {
    //    fScrollTimer->setTimerListener(0);
    fScrollTimer.stopTimer();
    //}
}

void YScrollBar::handleDNDLeave() {
    puts("scroll leave");
    fScrollTo = goNone;
    fDNDScroll = false;
    repaint();
    //if (fScrollTimer && fScrollTimer->getTimerListener() == this) {
    //    fScrollTimer->setTimerListener(0);
    fScrollTimer.stopTimer();
    //}
}


bool YScrollBar::handleDNDPosition(int x, int y, Atom * /*action*/) {
    puts("scroll position");
    fScrollTo = getOp(x, y);
    //if (fScrollTimer == 0)
    //    fScrollTimer = new YTimer(this, scrollBarStartDelay);
    //if (fScrollTimer) {
    //    fScrollTimer->setTimerListener(this);
    fScrollTimer.setInterval(gScrollBarStartDelay.getNum());
    fScrollTimer.startTimer();
    //}
    repaint();
    return false;
}
#endif
