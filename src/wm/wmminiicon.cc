/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#include "config.h"

#ifndef LITE

#include "ylib.h"
#include "wmminiicon.h"

#include "wmframe.h"
#include "yapp.h"
#include "yconfig.h"

#include <string.h>
#include "ycstring.h"

YFontPrefProperty MiniIcon::gMinimizedWindowFont("icewm", "MinimizedWindowFontName", FONT(120));;
static YColor *normalMinimizedWindowBg = 0;
static YColor *normalMinimizedWindowFg = 0;
static YColor *activeMinimizedWindowBg = 0;
static YColor *activeMinimizedWindowFg = 0;


MiniIcon::MiniIcon(YWindowManager *root, YWindow *aParent, YFrameWindow *frame): YWindow(aParent) {
    if (normalMinimizedWindowBg == 0) {
        YPref prefColorNormalMinimizedWindow("icewm", "ColorNormalMinimizedWindow");
        const char *pvColorNormalMinimizedWindow = prefColorNormalMinimizedWindow.getStr("rgb:C0/C0/C0");
        normalMinimizedWindowBg = new YColor(pvColorNormalMinimizedWindow);
    }
    if (normalMinimizedWindowFg == 0) {
        YPref prefColorNormalMinimizedWindowText("icewm", "ColorNormalMinimizedWindowText");
        const char *pvColorNormalMinimizedWindowText = prefColorNormalMinimizedWindowText.getStr("rgb:C0/C0/C0");
        normalMinimizedWindowFg = new YColor(pvColorNormalMinimizedWindowText);
    }
    if (activeMinimizedWindowBg == 0) {
        YPref prefColorActiveMinimizedWindow("icewm", "ColorActiveMinimizedWindow");
        const char *pvColorActiveMinimizedWindow = prefColorActiveMinimizedWindow.getStr("rgb:C0/C0/C0");
        activeMinimizedWindowBg = new YColor(pvColorActiveMinimizedWindow);
    }
    if (activeMinimizedWindowFg == 0) {
        YPref prefColorActiveMinimizedWindowText("icewm", "ColorActiveMinimizedWindowText");
        const char *pvColorActiveMinimizedWindowText = prefColorActiveMinimizedWindowText.getStr("rgb:C0/C0/C0");
        activeMinimizedWindowFg = new YColor(pvColorActiveMinimizedWindowText);
    }

    fRoot = root;
    fFrame = frame;
    selected = 0;
    setGeometry(0, 0, 120, 24);
}

MiniIcon::~MiniIcon() {
}

void MiniIcon::paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
//#ifdef CONFIG_TASKBAR
    bool focused = getFrame()->focused();
    YColor *bg = focused ? activeMinimizedWindowBg : normalMinimizedWindowBg;;
    YColor *fg = focused ? activeMinimizedWindowFg : normalMinimizedWindowFg;;
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

void MiniIcon::handleButton(const XButtonEvent &button) {
    if (button.type == ButtonPress) {
        if (!(button.state & ControlMask) &&
            getFrame()->shouldRaise(button))
        {
            getFrame()->wmRaise();
        }
        fRoot->setFocus(getFrame(), false);
        if (button.button == 1) {
            selected = 2;
            repaint();
        }
    } else if (button.type == ButtonRelease) {
        if (button.button == 1) {
            if (selected == 2) {
                if (button.state & app->AltMask) {
                    getFrame()->wmLower();
                } else {
                    if (!(button.state & ControlMask))
                        getFrame()->wmRaise();
                    getFrame()->activate();
                }
            }
            selected = 0;
            repaint();
        }
    }
    YWindow::handleButton(button);
}

void MiniIcon::handleClick(const XButtonEvent &up, int /*count*/) {
    if (up.button == 3) {
        getFrame()->popupSystemMenu(up.x_root, up.y_root, -1, -1,
                                    YPopupWindow::pfCanFlipVertical |
                                    YPopupWindow::pfCanFlipHorizontal |
                                    YPopupWindow::pfPopupMenu);
    }
}

void MiniIcon::handleCrossing(const XCrossingEvent &crossing) {
    if (selected > 0) {
        if (crossing.type == EnterNotify) {
            selected = 2;
            repaint();
        } else if (crossing.type == LeaveNotify) {
            selected = 1;
            repaint();
        }
    }

}

void MiniIcon::handleDrag(const XButtonEvent &down, const XMotionEvent &motion) {
    if (down.button != 1) {
        int x = motion.x_root - down.x;
        int y = motion.y_root - down.y;

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
    }
}
#endif
