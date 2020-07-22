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
    YWindow(aParent),
    fFrame(frame),
    selected(0)
{
    if (minimizedWindowFont == null)
        minimizedWindowFont = YFont::getFont(XFA(minimizedWindowFontName));

    setGeometry(YRect(0, 0, 120, 24));
    setTitle("MiniIcon");
}

MiniIcon::~MiniIcon() {
}

void MiniIcon::setSelected(int state) {
    if (state != selected) {
        selected = state;
        repaint();
    }
}

void MiniIcon::repaint() {
    paint(getGraphics(), geometry());
}

void MiniIcon::handleExpose(const XExposeEvent& expose) {
    if (expose.count == 0) {
        repaint();
    }
}

void MiniIcon::paint(Graphics &g, const YRect &/*r*/) {
    bool focused = (getFrame()->focused() && getFrame() == manager->getFocus());
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
        XRectangle rect = {
            (short) (2 + tx + 1),
            (short) 4,
            (unsigned short) YIcon::smallSize(),
            (unsigned short) YIcon::smallSize(),
        };
        g.setClipRectangles(&rect, 1);
        //int y = (height() - 3 - frame()->clientIcon()->small()->height()) / 2;
        getFrame()->clientIcon()->draw(g, rect.x, rect.y, YIcon::smallSize());
        g.resetClip();
    }

    mstring str(getFrame()->getIconTitle());
    if (str.isEmpty())
        str = getFrame()->getTitle();

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
            (buttonRaiseMask & (1 << (button.button - Button1))))
            getFrame()->wmRaise();
        // manager->setFocus(getFrame(), false);
        if (button.button == Button1) {
            setSelected(2);
        }
        YWindow::handleButton(button);
    }
    else if (button.type == ButtonRelease) {
        YWindow::handleButton(button);
        setSelected(0);
    }
}

void MiniIcon::handleClick(const XButtonEvent &up, int /*count*/) {
    if (up.button == Button3) {
        getFrame()->popupSystemMenu(this, up.x_root, up.y_root,
                                    YPopupWindow::pfCanFlipVertical |
                                    YPopupWindow::pfCanFlipHorizontal |
                                    YPopupWindow::pfPopupMenu);
    }
    else if (up.button == Button1) {
        if (selected == 2) {
            if (up.state & xapp->AltMask) {
                getFrame()->wmLower();
            } else {
                if (!(up.state & ControlMask))
                    getFrame()->wmRaise();
                getFrame()->activate();
            }
        }
    }
}

void MiniIcon::handleCrossing(const XCrossingEvent &crossing) {
    if (selected > 0) {
        if (crossing.type == EnterNotify) {
            setSelected(2);
        } else if (crossing.type == LeaveNotify) {
            setSelected(1);
        }
    }
    if (crossing.type == EnterNotify &&
        (crossing.mode == NotifyNormal ||
         (strongPointerFocus && crossing.mode == NotifyUngrab)) &&
        crossing.window == handle() &&
        !clickFocus &&
        (strongPointerFocus ||
         (crossing.serial != YWindow::getLastEnterNotifySerial() &&
          crossing.serial != YWindow::getLastEnterNotifySerial() + 1)))
    {
        setSelected(2);
        manager->setFocus(getFrame(), false);
    }
    else if (crossing.type == LeaveNotify && crossing.mode == NotifyNormal)
    {
        if (manager->getFocus() == getFrame())
            manager->focusLastWindow();
    }
    YWindow::handleCrossing(crossing);
}

void MiniIcon::handleDrag(const XButtonEvent &down, const XMotionEvent &motion) {
    if (down.button) {
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
