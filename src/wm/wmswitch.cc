/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 *
 * Windows/OS2 like Alt{+Shift}+Tab window switching
 */
#include "config.h"

#ifndef LITE
#include "ykey.h"
#include "wmswitch.h"

#include "wmmgr.h"
#include "wmframe.h"
#include "yapp.h"
#include "yconfig.h"
#include "yrect.h"
#include "default.h"

#include <string.h>
#include <stdio.h>
#include "ycstring.h"
#include "yresource.h"

//YColor *SwitchWindow::switchFg = 0;
//YColor *SwitchWindow::switchBg = 0;

//YFont *SwitchWindow::switchFont = 0;

YBoolPrefProperty SwitchWindow::gSwitchToAllWorkspaces("icewm", "QuickSwitchToAllWorkspaces", false);
YBoolPrefProperty SwitchWindow::gSwitchToMinimized("icewm", "QuickSwitchToMinimized", true);
YBoolPrefProperty SwitchWindow::gSwitchToHidden("icewm", "QuickSwitchToHidden", true);

YFontPrefProperty SwitchWindow::gSwitchFont("icewm", "QuickSwitchFont", FONT(100));
YColorPrefProperty SwitchWindow::gSwitchBg("icewm", "ColorQuickSwitch", "rgb:C0/C0/C0");
YColorPrefProperty SwitchWindow::gSwitchFg("icewm", "ColorQuickSwitchText", "rgb:00/00/00");
YPixmapPrefProperty SwitchWindow::gPixmapBackground("icewm", "QuickSwitchBackgroundPixmap", 0, 0); //"switchbg.xpm", LIBDIR);

SwitchWindow::SwitchWindow(YWindowManager *root, YWindow *parent): YPopupWindow(parent) {
    fActiveWindow = 0;
    fLastWindow = 0;
    modsDown = 0;
    isUp = false;
    fRoot = root;
    zCount = 0;
    zList = 0;
    zTarget = 0;

    int sW = 4 + fRoot->width() / 5 * 3;
    int sH = 4 + 32;
        //statusFont->max_bounds.ascent +
        //statusFont->max_bounds.descent;

    setGeometry(fRoot->width() / 2 - sW / 2, fRoot->height() / 2 - sH / 2,
                sW, sH);

    setStyle(wsOverrideRedirect);
}

SwitchWindow::~SwitchWindow() {
    freeZList();
    if (isUp) {
        cancelPopup();
        isUp = false;
    }
}

void SwitchWindow::paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
    g.setColor(gSwitchBg.getColor());
    g.drawBorderW(0, 0, width() - 1, height() - 1, true);
    YPixmap *switchbackPixmap = gPixmapBackground.getPixmap();
    if (switchbackPixmap)
        g.fillPixmap(switchbackPixmap, 1, 1, width() - 3, height() - 3);
    else
        g.fillRect(1, 1, width() - 3, height() - 3);

    if (fActiveWindow) {
        int ofs = 0;//, pos;
        if (fActiveWindow->clientIcon() && fActiveWindow->clientIcon()->large()) {
            g.drawPixmap(fActiveWindow->clientIcon()->large(), 2, 2);
            ofs = fActiveWindow->clientIcon()->large()->width() + 2;
        }
        g.setColor(gSwitchFg.getColor());
        g.setFont(gSwitchFont.getFont());
        const CStr *str = fActiveWindow->client()->windowTitle();
        if (str && str->c_str()) {
            g.drawText(YRect(ofs, 0, width() - ofs, height()),
                       str, DrawText_VCenter + DrawText_HCenter);
        }
    }
}

int SwitchWindow::getZListCount() {
    int count = 0;

    YFrameWindow *w = fRoot->topLayer();
    while (w) {
        count++;
        w = w->nextLayer();
    }
    return count;
}

int SwitchWindow::getZList(YFrameWindow **list, int max) {
    int count = 0;

    for (int pass = 0; pass <= 7; pass++) {
        YFrameWindow *w = fRoot->topLayer();

        while (w) {
            // pass 0: focused window
            // pass 1: normal windows
            // pass 2: rollup windows
            // pass 3: minimized windows
            // pass 4: hidden windows
            // pass 5: unfocusable windows
            // pass 6: anything else?
            // pass 7: windows on other workspaces

            if (w == fRoot->getFocus()) {
                if (pass == 0) list[count++] = w;

            } else if (!w->isFocusable() || (w->frameOptions() & YFrameWindow::foIgnoreQSwitch)) {
                if (pass == 7) list[count++] = w;

            } else if (!w->isSticky() && w->getWorkspace() != fRoot->activeWorkspace()) {
                if (pass == 5) list[count++] = w;

            } else if (w->isHidden()) {
                if (pass == 4) list[count++] = w;

            } else if (w->isMinimized()) {
                if (pass == 3) list[count++] = w;

            } else if (w->isRollup()) {
                if (pass == 2) list[count++] = w;

            } else if (w->visibleNow()) {
                if (pass == 1) list[count++] = w;

            } else {
                if (pass == 6) list[count++] = w;
            }

            w = w->nextLayer();

            if (count > max) {
                fprintf(stderr, "icewm: wmswitch BUG: limit=%d pass=%d\n", max, pass);
                return max;
            }
        }
    }
    return count;
}

void SwitchWindow::updateZList() {
    freeZList();

    zCount = getZListCount();

    zList = new YFrameWindow *[zCount + 1]; // for bug hunt
    if (zList == 0)
        zCount = 0;
    else
        zCount = getZList(zList, zCount);
}

void SwitchWindow::freeZList() {
    if (zList)
        delete [] zList;
    zCount = 0;
    zList = 0;

}
/*
YFrameWindow *SwitchWindow::nextWindow(YFrameWindow *from, bool zdown, bool next) {
    if (from == 0) {
        next = false;
        from = zdown ? fRoot->topLayer() : fRoot->bottomLayer();
    }
    int flags =
        YFrameWindow::fwfFocusable |
        (gSwitchToAllWorkspaces.getBool() ? 0 : YFrameWindow::fwfWorkspace) |
        YFrameWindow::fwfLayers |
        YFrameWindow::fwfSwitchable |
        (next ? YFrameWindow::fwfNext: 0) |
        (zdown ? 0 : YFrameWindow::fwfBackward);

    YFrameWindow *n = 0;

    if (from == 0)
        return 0;

    if (zdown) {
        n = from->findWindow(flags | YFrameWindow::fwfUnminimized | YFrameWindow::fwfNotHidden);
        if (n == 0 && gSwitchToMinimized.getBool())
            n = from->findWindow(flags | YFrameWindow::fwfMinimized | YFrameWindow::fwfNotHidden);
        if (n == 0 && gSwitchToHidden.getBool())
            n = from->findWindow(flags | YFrameWindow::fwfHidden);
        if (n == 0) {
            flags |= YFrameWindow::fwfCycle;
            n = from->findWindow(flags | YFrameWindow::fwfUnminimized | YFrameWindow::fwfNotHidden);
            if (n == 0 && gSwitchToMinimized.getBool())
                n = from->findWindow(flags | YFrameWindow::fwfMinimized | YFrameWindow::fwfNotHidden);
            if (n == 0 && gSwitchToHidden.getBool())
                n = from->findWindow(flags | YFrameWindow::fwfHidden);
        }
    } else {
        if (n == 0 && gSwitchToHidden.getBool())
            n = from->findWindow(flags | YFrameWindow::fwfHidden);
        if (n == 0 && gSwitchToMinimized.getBool())
            n = from->findWindow(flags | YFrameWindow::fwfMinimized | YFrameWindow::fwfNotHidden);
        if (n == 0)
            n = from->findWindow(flags | YFrameWindow::fwfUnminimized | YFrameWindow::fwfNotHidden);
        if (n == 0) {
            flags |= YFrameWindow::fwfCycle;
            if (n == 0 && gSwitchToHidden.getBool())
                n = from->findWindow(flags | YFrameWindow::fwfHidden);
            if (n == 0 && gSwitchToMinimized.getBool())
                n = from->findWindow(flags | YFrameWindow::fwfMinimized | YFrameWindow::fwfNotHidden);
            if (n == 0)
                n = from->findWindow(flags | YFrameWindow::fwfUnminimized | YFrameWindow::fwfNotHidden);
        }
    }
    if (n == 0)
        n = from->findWindow(flags |
                             YFrameWindow::fwfVisible |
                             YFrameWindow::fwfSame);
    if (n == 0)
        n = fLastWindow;
    return n;
}
*/

YFrameWindow *SwitchWindow::nextWindow(bool zdown) {
    if (zdown) {
        zTarget = zTarget + 1;
        if (zTarget >= zCount) zTarget = 0;
    } else {
        zTarget = zTarget - 1;
        if (zTarget < 0) zTarget = zCount - 1;
    }
    if (zTarget >= zCount || zTarget < 0)
        zTarget = -1;

    if (zTarget == -1)
        return 0;
    else
        return zList[zTarget];
}

void SwitchWindow::begin(bool zdown, int mods) {
    modsDown = mods & (kfAlt | kfMeta | kfHyper | kfSuper | kfCtrl);
    if (isUp == true) {
        cancelPopup();
        isUp = false;
        return ;
    }
    fLastWindow = fActiveWindow = fRoot->getFocus();
    updateZList();
    zTarget = 0;
    fActiveWindow = nextWindow(zdown);
    if (fActiveWindow) {
        displayFocus(fActiveWindow);
        isUp = popup(0, 0, 0);
    }
}

void SwitchWindow::activatePopup() {
}

void SwitchWindow::deactivatePopup() {
}

void SwitchWindow::cancel() {
    if (isUp) {
        cancelPopup();
        isUp = false;
    }
    if (fLastWindow) {
        displayFocus(fLastWindow);
    } else if (fActiveWindow) {
        fActiveWindow->wmRaise();
        fRoot->activate(fActiveWindow, true);
    }
    freeZList();
    fLastWindow = fActiveWindow = 0;
}

void SwitchWindow::accept() {
    if (fActiveWindow == 0)
        cancel();
    else {
        fRoot->activate(fActiveWindow, true);
        if (isUp) {
            cancelPopup();
            isUp = false;
        }
        fActiveWindow->wmRaise();
        //manager->activate(fActiveWindow, true);
    }
    freeZList();
    fLastWindow = fActiveWindow = 0;
}

void SwitchWindow::displayFocus(YFrameWindow *frame) {
    fRoot->switchFocusTo(frame);
    repaint();
}

void SwitchWindow::destroyedFrame(YFrameWindow *frame) {
    if (frame == fLastWindow)
        fLastWindow = 0;
    if (frame == fActiveWindow) {
        zTarget = -1;
        updateZList();
        fActiveWindow = nextWindow(true);
        displayFocus(fActiveWindow);
    }
}

bool SwitchWindow::handleKeySym(const XKeyEvent &key, KeySym ksym, int vmod) {
    if (key.type == KeyPress) {
        if ((ksym == XK_Tab) && !(vmod & kfShift)) {
            fActiveWindow = nextWindow(true);
            displayFocus(fActiveWindow);
            return true;
        } else if ((ksym == XK_Tab) && (vmod & kfShift)) {
            fActiveWindow = nextWindow(false);
            displayFocus(fActiveWindow);
            return true;
        } else if (ksym == XK_Escape) {
            cancel();
            return true;
        }
        if (ksym == XK_Tab && !modDown(vmod)) {
            accept();
            return true;
        }
    } else if (key.type == KeyRelease) {
        if (ksym == XK_Tab && !modDown(vmod)) {
            accept();
            return true;
        } else if (isModKey(key.keycode)) {
            accept();
            return true;
        }
    }
    return YPopupWindow::handleKeySym(key, ksym, vmod);
}

bool SwitchWindow::isModKey(KeyCode c) {
    KeySym k = XKeycodeToKeysym(app->display(), c, 0);

    if (k == XK_Control_L || k == XK_Control_R ||
        k == XK_Alt_L     || k == XK_Alt_R     ||
        k == XK_Meta_L    || k == XK_Meta_R    ||
        k == XK_Super_L   || k == XK_Super_R   ||
        k == XK_Hyper_L   || k == XK_Hyper_R)
        return true;

    return false;
}

bool SwitchWindow::modDown(int mod) {
    int m = mod & (app->getAltMask() | app->getWinMask() /*| app->getHyperMask() | app->getSuperMask()*/ | ControlMask);

    if ((m & modsDown) != modsDown)
        return false;
    return true;
}

void SwitchWindow::handleButton(const XButtonEvent &button) {
    if (button.type == ButtonPress) {
        if (button.button == 5) {
            fActiveWindow = nextWindow(false);
            displayFocus(fActiveWindow);
            return ;
        } else if (button.button == 4) {
            fActiveWindow = nextWindow(true);
            displayFocus(fActiveWindow);
            return ;
        }
    } else {
        if (button.button == 5 || button.button == 4)
            return;
    }
    YPopupWindow::handleButton(button);
}
#endif
