/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#include "config.h"

#ifndef LITE

#include "wmminiicon.h"
#include "ybuttonevent.h"
#include "ycrossingevent.h"
#include "ymotionevent.h"

#include "wmframe.h"
#include "yapp.h"
#include "deffonts.h"
#include "yconfig.h"
#include "ypaint.h"

#include <string.h>
#include "ycstring.h"

YFontPrefProperty MiniIcon::gMinimizedWindowFont("icewm", "MinimizedWindowFontName", FONT(120));

YColorPrefProperty MiniIcon::gNormalBg("icewm", "ColorNormalMinimizedWindow", "rgb:C0/C0/C0");
YColorPrefProperty MiniIcon::gNormalFg("icewm", "ColorNormalMinimizedWindowText", "rgb:C0/C0/C0");
YColorPrefProperty MiniIcon::gActiveBg("icewm", "ColorActiveMinimizedWindow", "rgb:C0/C0/C0");
YColorPrefProperty MiniIcon::gActiveFg("icewm", "ColorActiveMinimizedWindowText", "rgb:C0/C0/C0");


MiniIcon::MiniIcon(YWindowManager *root, YWindow *aParent, YFrameWindow *frame): YWindow(aParent) {
    fRoot = root;
    fFrame = frame;
    selected = 0;
    setGeometry(0, 0, 120, 24);
}

MiniIcon::~MiniIcon() {
}

void MiniIcon::paint(Graphics &g, const YRect &/*er*/) {
    //#ifdef CONFIG_TASKBAR
    bool focused = getFrame()->focused();
    YColor *bg = focused ? gActiveBg.getColor() : gNormalBg.getColor();
    YColor *fg = focused ? gActiveFg.getColor() : gNormalBg.getColor();
    int tx = 2;
    int x, y, w, h;

    g.setColor(bg);
    g.draw3DRect(0, 0, width() - 1, height() - 1, true);
    g.fillRect(1, 1, width() - 2, height() - 2);

    x = tx; y = 2;
    w = width() - 6;
    h = height() - 6;

    if (selected == 2) {
        g.setColor(bg->darker());
        g.draw3DRect(x, y, w, h, false);
        g.fillRect(x + 1, y + 1, w - 1, h - 1);
    } else {
        g.setColor(bg->brighter());
        g.drawRect(x + 1, y + 1, w, h);
        g.setColor(bg->darker());
        g.drawRect(x, y, w, h);
        g.setColor(bg);
        g.fillRect(x + 2, y + 2, w - 2, h - 2);
    }

    if (getFrame()->clientIcon() && getFrame()->clientIcon()->small()) {
        //int y = (height() - 3 - frame()->clientIcon()->small()->height()) / 2;
        g.drawPixmap(getFrame()->clientIcon()->small(), 2 + tx + 1, 4);
    }

    const CStr *str = getFrame()->client()->iconTitle();
    if (!str || str->isWhitespace())
        str = getFrame()->client()->windowTitle();
    if (str && str->c_str()) {
        g.setColor(fg);
        YFont *font = gMinimizedWindowFont.getFont();
        if (font) {
            g.setFont(font);
            int ty = (height() - 1 + font->height()) / 2 - font->descent();
            if (ty < 2)
                ty = 2;
            g.drawChars(str->c_str(), 0, str->length(),
                        tx + 4 + 16 + 2,
                        ty);
        }
        //(yheight() - font->height()) / 2 - titleFont->descent() - 4);
    }
    //#endif
}

bool MiniIcon::eventButton(const YButtonEvent &button) {
    if (button.type() == YEvent::etButtonPress) {
        if (!(button.isCtrl()) &&
            getFrame()->shouldRaise(button))
        {
            getFrame()->wmRaise();
        }
        fRoot->setFocus(getFrame(), false);
        if (button.getButton() == 1) {
            selected = 2;
            repaint();
        }
    } else if (button.type() == YEvent::etButtonRelease) {
        if (button.getButton() == 1) {
            if (selected == 2) {
                if (button.isAlt()) {
                    getFrame()->wmLower();
                } else {
                    if (!(button.isCtrl()))
                        getFrame()->wmRaise();
                    getFrame()->activate();
                }
            }
            selected = 0;
            repaint();
        }
    }
    return YWindow::eventButton(button);
}

bool MiniIcon::eventClick(const YClickEvent &up) {
    if (up.getButton() == 3) {
        getFrame()->popupSystemMenu(up.x_root(), up.y_root(), -1, -1,
                                    YPopupWindow::pfCanFlipVertical |
                                    YPopupWindow::pfCanFlipHorizontal);
        return true;
    }
    return YWindow::eventClick(up);
}

bool MiniIcon::eventCrossing(const YCrossingEvent &crossing) {
    if (selected > 0) {
        if (crossing.type() == YEvent::etPointerIn) {
            selected = 2;
            repaint();
        } else if (crossing.type() == YEvent::etPointerOut) {
            selected = 1;
            repaint();
        }
    }
    return YWindow::eventCrossing(crossing);
}

bool MiniIcon::eventDrag(const YButtonEvent &down, const YMotionEvent &motion) {
    if (!down.leftButton()) {
        int x = motion.x_root() - down.x();
        int y = motion.y_root() - down.y();

        //x += down.x;
        //y += down.y;

        long l = getFrame()->getLayer();
        int mx = fRoot->minX(l), Mx = fRoot->maxX(l);
        int my = fRoot->minY(l), My = fRoot->maxY(l);

        if (x + int(width()) >= Mx) x = Mx - width();
        if (y + int(height()) >= My) y = My - height();
        if (x < mx) x = mx;
        if (y < my) y = my;

        getFrame()->setPosition(x, y);
        return true;
    }
    return YWindow::eventDrag(down, motion);
}
#endif
