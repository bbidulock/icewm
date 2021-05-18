/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "wmminiicon.h"
#include "wmframe.h"
#include "wmmgr.h"
#include "yxapp.h"
#include "prefs.h"

static ref<YFont> minimizedWindowFont;
static YColorName normalMinimizedWindowBg(&clrNormalMinimizedWindow);
static YColorName normalMinimizedWindowFg(&clrNormalMinimizedWindowText);
static YColorName activeMinimizedWindowBg(&clrActiveMinimizedWindow);
static YColorName activeMinimizedWindowFg(&clrActiveMinimizedWindowText);

static bool acceptableDimensions(unsigned w, unsigned h) {
    unsigned lower = YIcon::hugeSize() / 2;
    unsigned upper = 3 * YIcon::hugeSize() / 2;
    return inrange(w, lower, upper) && inrange(h, lower, upper);
}

inline YFrameClient* MiniIcon::client() const {
    return fFrame->client();
}

Window MiniIcon::iconWindow() {
    return Elvis(fIconWindow, handle());
}

MiniIcon::MiniIcon(YFrameWindow *frame):
    YWindow(),
    fFrame(frame),
    fIconWindow(client()->iconWindowHint())
{
    setStyle(wsOverrideRedirect | wsBackingMapped);
    setSize(YIcon::hugeSize(), YIcon::hugeSize());
    setTitle("MiniIcon");
    setPosition(-1, -1);

    if (minimizedWindowFont == null)
        minimizedWindowFont = YFont::getFont(XFA(minimizedWindowFontName));

    if (fIconWindow) {
        Window root, parent;
        xsmart<Window> child;
        unsigned border, depth, count;
        if (XGetGeometry(xapp->display(), fIconWindow, &root,
                         &fIconGeometry.xx, &fIconGeometry.yy,
                         &fIconGeometry.ww, &fIconGeometry.hh,
                         &border, &depth) == False) {
            fIconWindow = None;
        }
        else if (acceptableDimensions(fIconGeometry.ww, fIconGeometry.hh)) {
            int x = (int(YIcon::hugeSize()) - int(fIconGeometry.ww)) / 2
                  - int(border);
            int y = (int(YIcon::hugeSize()) - int(fIconGeometry.hh)) / 2
                  - int(border);
            XAddToSaveSet(xapp->display(), fIconWindow);
            XReparentWindow(xapp->display(), fIconWindow, handle(), x, y);
            if (XQueryTree(xapp->display(), handle(), &root, &parent, &child,
                   &count) == True && count == 1 && child[0] == fIconWindow)
            {
                XMapWindow(xapp->display(), fIconWindow);
                XWMHints* hints = XGetWMHints(xapp->display(), fIconWindow);
                if (hints) {
                    if ((hints->flags & StateHint) &&
                        hints->initial_state != WithdrawnState) {
                        // Fix initial_state for icewm restart to succeed.
                        hints->initial_state = WithdrawnState;
                        XSetWMHints(xapp->display(), fIconWindow, hints);
                    }
                    XFree(hints);
                }
            }
            else {
                fIconWindow = None;
            }
        }
        else {
            fIconWindow = None;
        }
    }

    updateIcon();
}

MiniIcon::~MiniIcon() {
    if (fIconWindow && client() && !client()->destroyed()) {
        XUnmapWindow(xapp->display(), fIconWindow);
        XReparentWindow(xapp->display(), fIconWindow, xapp->root(),
                        fIconGeometry.xx, fIconGeometry.hh);
        XRemoveFromSaveSet(xapp->display(), fIconWindow);
    }
}

void MiniIcon::handleExpose(const XExposeEvent& expose) {
    if (expose.count == 0) {
        repaint();
    }
}

void MiniIcon::repaint() {
    if (fIconWindow == None) {
        Graphics g(*this);
        paint(g, geometry());
    }
}

void MiniIcon::paint(Graphics &g, const YRect &r) {
    if (fIconWindow == None) {
        ref<YIcon> icon(fFrame->clientIcon());
        if (icon != null && icon->huge() != null) {
            int x = (YIcon::hugeSize() - icon->huge()->width()) / 2;
            int y = (YIcon::hugeSize() - icon->huge()->height()) / 2;
            if (xapp->alpha()) {
               icon->draw(g, x, y, YIcon::hugeSize());
            }
            // g.drawImage(icon->huge(), x, y);
            else {
                icon->huge()->copy(g, x, y);
            }
        }
    }
}

void MiniIcon::updateIcon() {
    if (fIconWindow)
        return;
#ifdef CONFIG_SHAPE
    ref<YIcon> icon(fFrame->clientIcon());
    if (icon != null && icon->huge() != null) {
        ref<YImage> image = icon->huge();
        ref<YPixmap> pixmap = image->renderToPixmap(depth());
        if (pixmap != null && pixmap->mask()) {
            int x = (YIcon::hugeSize() - pixmap->width()) / 2;
            int y = (YIcon::hugeSize() - pixmap->height()) / 2;
            XShapeCombineMask(xapp->display(), handle(), ShapeBounding,
                              x, y, pixmap->mask(), ShapeSet);
        }
    }
#endif
}

void MiniIcon::updatePosition() {
    int xpos = x();
    int ypos = y();
    manager->getIconPosition(fFrame, &xpos, &ypos);
    if (xpos != x() || ypos != y()) {
        setPosition(xpos, ypos);
    }
}

void MiniIcon::show() {
    updatePosition();
    if (x() != -1 || y() != -1) {
        beneath(manager->bottomLayer());
        YWindow::show();
    }
}

void MiniIcon::handleButton(const XButtonEvent &button) {
    if (button.type == ButtonPress) {
        if (!(button.state & ControlMask) &&
            (buttonRaiseMask & (1 << (button.button - Button1))))
        {
            raise();
        }
        YWindow::handleButton(button);
    }
    else if (button.type == ButtonRelease) {
        YWindow::handleButton(button);
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
        if (up.state & xapp->AltMask) {
            lower();
        } else {
            if (!(up.state & ControlMask))
                getFrame()->wmRaise();
            getFrame()->activate();
        }
    }
}

void MiniIcon::handleCrossing(const XCrossingEvent &crossing) {
    if (crossing.type == EnterNotify && !dragging()) {
        mstring title(fFrame->getIconTitle());
        if (title.isEmpty())
            title = fFrame->getTitle();
        setToolTip(title);
    }
    YWindow::handleCrossing(crossing);
}

bool MiniIcon::handleBeginDrag(const XButtonEvent& d, const XMotionEvent& m) {
    setToolTip(null);
    raise();
    return true;
}

void MiniIcon::handleEndDrag(const XButtonEvent& d, const XButtonEvent& u) {
    beneath(manager->bottomLayer());
}

void MiniIcon::handleDrag(const XButtonEvent &down, const XMotionEvent &motion) {
    if (down.button) {
        int mx, my, Mx, My;
        manager->getWorkArea(fFrame, &mx, &my, &Mx, &My);
        int x = clamp(motion.x_root - down.x, mx, Mx - int(width()));
        int y = clamp(motion.y_root - down.y, my, My - int(height()));
        setPosition(x, y);
    }
}

bool MiniIcon::handleKey(const XKeyEvent& key) {
    if (key.type == KeyPress) {
        KeySym k = keyCodeToKeySym(key.keycode);
        unsigned int m = KEY_MODMASK(key.state);
        unsigned int vm = VMod(m);
        if (IS_WMKEY(k, vm, gKeyWinClose)) {
            fFrame->actionPerformed(actionClose);
        }
        else if (IS_WMKEY(k, vm, gKeyWinLower)) {
            fFrame->actionPerformed(actionLower);
        }
        else if (IS_WMKEY(k, vm, gKeyWinRestore)) {
            fFrame->actionPerformed(actionRestore);
        }
        else if (k == XK_Return || k == XK_KP_Enter) {
            fFrame->activate();
        }
        else if (k == XK_Menu || (k == XK_F10 && m == ShiftMask)) {
            fFrame->popupSystemMenu(fFrame);
        }
    }
    return true;
}

// vim: set sw=4 ts=4 et:
