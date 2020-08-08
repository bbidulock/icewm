/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "wmminiicon.h"
#include "wmframe.h"
#include "yxapp.h"
#include "prefs.h"

static ref<YFont> minimizedWindowFont;
static YColorName normalMinimizedWindowBg(&clrNormalMinimizedWindow);
static YColorName normalMinimizedWindowFg(&clrNormalMinimizedWindowText);
static YColorName activeMinimizedWindowBg(&clrActiveMinimizedWindow);
static YColorName activeMinimizedWindowFg(&clrActiveMinimizedWindowText);

MiniIcon::MiniIcon(YFrameWindow *frame):
    YWindow(),
    fFrame(frame)
{
    setStyle(wsOverrideRedirect | wsNoExpose);
    setSize(YIcon::hugeSize(), YIcon::hugeSize());
    setTitle("MiniIcon");

    if (minimizedWindowFont == null)
        minimizedWindowFont = YFont::getFont(XFA(minimizedWindowFontName));

    updateIcon();
}

MiniIcon::~MiniIcon() {
}

void MiniIcon::updateIcon() {
    ref<YIcon> icon(fFrame->clientIcon());
    if (icon != null && icon->huge() != null) {
        ref<YImage> image = icon->huge();
        ref<YPixmap> pixmap = image->renderToPixmap(depth());
        if (pixmap != null && pixmap->mask()) {
            XShapeCombineMask(xapp->display(), handle(), ShapeBounding,
                              0, 0, pixmap->mask(), ShapeSet);
            setBackgroundPixmap(pixmap->pixmap());
            return;
        }
    }
}

void MiniIcon::show() {
    beneath(manager->bottomLayer());
    YWindow::show();
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
        setToolTip(fFrame->getIconTitle());
    }
    YWindow::handleCrossing(crossing);
}

void MiniIcon::handleBeginDrag(const XButtonEvent& d, const XMotionEvent& m) {
    setToolTip(null);
    raise();
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
            if (fFrame->canClose())
                fFrame->wmClose();
        }
        else if (IS_WMKEY(k, vm, gKeyWinLower)) {
            if (fFrame->canLower())
                fFrame->wmLower();
        }
        else if (IS_WMKEY(k, vm, gKeyWinRestore)) {
            if (fFrame->canRestore())
                fFrame->wmRestore();
        }
        else if (k == XK_Return || k == XK_KP_Enter) {
            fFrame->activate();
        }
        else if ((k == XK_Menu) || (k == XK_F10 && m == ShiftMask)) {
            fFrame->popupSystemMenu(fFrame);
        }
    }
    return true;
}

// vim: set sw=4 ts=4 et:
