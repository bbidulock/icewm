/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"

#include "ylib.h"
#include "wmminiicon.h"

#include "wmframe.h"
#include "yxapp.h"
#include "yrect.h"
#include "yicon.h"
#include "prefs.h"

#include <string.h>

static ref<YFont> minimizedWindowFont;
static YColorName normalMinimizedWindowBg(&clrNormalMinimizedWindow);
static YColorName normalMinimizedWindowFg(&clrNormalMinimizedWindowText);
static YColorName activeMinimizedWindowBg(&clrActiveMinimizedWindow);
static YColorName activeMinimizedWindowFg(&clrActiveMinimizedWindowText);

MiniIcon::MiniIcon(YWindow *aParent, YFrameWindow *frame):
    YWindow(desktop) {
    reparent(aParent, 0, 0);
    if (minimizedWindowFont == null)
        minimizedWindowFont = YFont::getFont(XFA(minimizedWindowFontName));

    fFrame = frame;
    selected = 0;
    setGeometry(YRect(0, 0, 120, 24));
}

MiniIcon::~MiniIcon() {
}

void MiniIcon::paint(Graphics &g, const YRect &/*r*/) {
    bool focused = getFrame()->focused();
    YColor bg = focused ? activeMinimizedWindowBg : normalMinimizedWindowBg;
    YColor fg = focused ? activeMinimizedWindowFg : normalMinimizedWindowFg;
    int tx = 2;
    int x, y, w, h;

    g.setColor(bg);
    g.draw3DRect(0, 0, width() - 1, height() - 1, true);
    g.fillRect(1, 1, width() - 2, height() - 2);

    x = tx; y = 2;
    w = width() - 6;
    h = height() - 6;

    if (selected == 2) {
        g.setColor(bg.darker());
        g.draw3DRect(x, y, w, h, false);
        g.fillRect(x + 1, y + 1, w - 1, h - 1);
    } else {
        g.setColor(bg.brighter());
        g.drawRect(x + 1, y + 1, w, h);
        g.setColor(bg.darker());
        g.drawRect(x, y, w, h);
        g.setColor(bg);
        g.fillRect(x + 2, y + 2, w - 2, h - 2);
    }

    if (getFrame()->clientIcon() != null) {
        //int y = (height() - 3 - frame()->clientIcon()->small()->height()) / 2;
        getFrame()->clientIcon()->draw(g, 2 + tx + 1, 4, YIcon::smallSize());
    }

    ustring str = getFrame()->client()->iconTitle();

    if (str == null || str.length() == 0)
        str = getFrame()->client()->windowTitle();

    if (str != null) {
        g.setColor(fg);
        ref<YFont> font = minimizedWindowFont;
        if (font != null) {
            g.setFont(font);
            int ty = (height() - 1 + font->height()) / 2 - font->descent();
            if (ty < 2) ty = 2;

            g.drawStringEllipsis(tx + 4 + YIcon::smallSize() + 2, ty,
                                 str, w - 4 - YIcon::smallSize() - 4);
        }
    }
}

void MiniIcon::handleButton(const XButtonEvent &button) {
    if (button.type == ButtonPress) {
        if (!(button.state & ControlMask) &&
            (buttonRaiseMask & (1 << (button.button - 1))))
            getFrame()->wmRaise();
        manager->setFocus(getFrame(), false);
        if (button.button == 1) {
            selected = 2;
            repaint();
        }
    } else if (button.type == ButtonRelease) {
        if (button.button == 1) {
            if (selected == 2) {
                if (button.state & xapp->AltMask) {
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
        getFrame()->popupSystemMenu(this, up.x_root, up.y_root,
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

        int mx, my, Mx, My;
        manager->getWorkArea(getFrame(), &mx, &my, &Mx, &My);

        if (x + int(width()) >= Mx) x = Mx - width();
        if (y + int(height()) >= My) y = My - height();
        if (x < mx) x = mx;
        if (y < my) y = my;

        getFrame()->setCurrentGeometryOuter(YRect(x, y, width(), height()));
    }
}

// vim: set sw=4 ts=4 et:
