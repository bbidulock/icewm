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
#include "prefs.h"

#include <string.h>

YColor *SwitchWindow::switchFg = 0;
YColor *SwitchWindow::switchBg = 0;

YFont *SwitchWindow::switchFont = 0;

//SwitchWindow *switchWindow = 0;

SwitchWindow::SwitchWindow(YWindowManager *root, YWindow *parent): YPopupWindow(parent) {
    if (switchBg == 0)
        switchBg = new YColor(clrQuickSwitch);
    if (switchFg == 0)
        switchFg = new YColor(clrQuickSwitchText);
    if (switchFont == 0)
        switchFont = YFont::getFont(switchFontName);

    fActiveWindow = 0;
    fLastWindow = 0;
    modsDown = 0;
    isUp = false;
    fRoot = root;

    int sW = 4 + fRoot->width() / 5 * 3;
    int sH = 4 + 32;
        //statusFont->max_bounds.ascent +
        //statusFont->max_bounds.descent;

    setGeometry(fRoot->width() / 2 - sW / 2, fRoot->height() / 2 - sH / 2,
                sW, sH);

    setStyle(wsOverrideRedirect);
}

SwitchWindow::~SwitchWindow() {
    if (isUp) {
        cancelPopup();
        isUp = false;
    }
}

void SwitchWindow::paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
    g.setColor(switchBg);
    g.drawBorderW(0, 0, width() - 1, height() - 1, true);
    if (switchbackPixmap)
        g.fillPixmap(switchbackPixmap, 1, 1, width() - 3, height() - 3);
    else
        g.fillRect(1, 1, width() - 3, height() - 3);

    if (fActiveWindow) {
        int ofs = 0, pos;
        if (fActiveWindow->clientIcon() && fActiveWindow->clientIcon()->large()) {
            g.drawPixmap(fActiveWindow->clientIcon()->large(), 2, 2);
            ofs = fActiveWindow->clientIcon()->large()->width() + 2;
        }
        g.setColor(switchFg);
        g.setFont(switchFont);
        const char *str = fActiveWindow->client()->windowTitle();
        if (str) {
            pos = ofs + (width() - ofs) / 2 - switchFont->textWidth(str) / 2;
            if (pos < ofs)
                pos = ofs;
            g.drawChars(str, 0, strlen(str),
                        pos,
                        height() / 2 + (switchFont->height()) / 2
                        - switchFont->descent());
        }
    }
}

YFrameWindow *SwitchWindow::nextWindow(YFrameWindow *from, bool zdown, bool next) {
    if (from == 0) {
        next = false;
        from = zdown ? fRoot->topLayer() : fRoot->bottomLayer();
    }
    int flags =
        YFrameWindow::fwfFocusable |
        (quickSwitchToAllWorkspaces ? 0 : YFrameWindow::fwfWorkspace) |
        YFrameWindow::fwfLayers |
        YFrameWindow::fwfSwitchable |
        (next ? YFrameWindow::fwfNext: 0) |
        (zdown ? 0 : YFrameWindow::fwfBackward);

    YFrameWindow *n = 0;

    if (from == 0)
        return 0;

    if (zdown) {
        n = from->findWindow(flags | YFrameWindow::fwfUnminimized | YFrameWindow::fwfNotHidden);
        if (n == 0 && quickSwitchToMinimized)
            n = from->findWindow(flags | YFrameWindow::fwfMinimized | YFrameWindow::fwfNotHidden);
        if (n == 0 && quickSwitchToHidden)
            n = from->findWindow(flags | YFrameWindow::fwfHidden);
        if (n == 0) {
            flags |= YFrameWindow::fwfCycle;
            n = from->findWindow(flags | YFrameWindow::fwfUnminimized | YFrameWindow::fwfNotHidden);
            if (n == 0 && quickSwitchToMinimized)
                n = from->findWindow(flags | YFrameWindow::fwfMinimized | YFrameWindow::fwfNotHidden);
            if (n == 0 && quickSwitchToHidden)
                n = from->findWindow(flags | YFrameWindow::fwfHidden);
        }
    } else {
        if (n == 0 && quickSwitchToHidden)
            n = from->findWindow(flags | YFrameWindow::fwfHidden);
        if (n == 0 && quickSwitchToMinimized)
            n = from->findWindow(flags | YFrameWindow::fwfMinimized | YFrameWindow::fwfNotHidden);
        if (n == 0)
            n = from->findWindow(flags | YFrameWindow::fwfUnminimized | YFrameWindow::fwfNotHidden);
        if (n == 0) {
            flags |= YFrameWindow::fwfCycle;
            if (n == 0 && quickSwitchToHidden)
                n = from->findWindow(flags | YFrameWindow::fwfHidden);
            if (n == 0 && quickSwitchToMinimized)
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

void SwitchWindow::begin(bool zdown, int mods) {
    modsDown = mods & (kfAlt | kfMeta | kfHyper | kfSuper | kfCtrl);
    if (isUp) {
        cancelPopup();
        isUp = false;
        return ;
    }
    fLastWindow = fActiveWindow = fRoot->getFocus();
    fActiveWindow = nextWindow(fActiveWindow, zdown, true);
    if (fActiveWindow) {
        displayFocus(fActiveWindow);
        isUp = popup(0, 0, YPopupWindow::pfNoPointerChange);
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
        fActiveWindow = nextWindow(0, true, false);
        displayFocus(fActiveWindow);
    }
}

bool SwitchWindow::handleKeySym(const XKeyEvent &key, KeySym ksym, int vmod) {
    if (key.type == KeyPress) {
        if ((ksym == XK_Tab) && !(vmod & kfShift)) {
            fActiveWindow = nextWindow(fActiveWindow, true, true);
            displayFocus(fActiveWindow);
            return true;
        } else if ((ksym == XK_Tab) && (vmod & kfShift)) {
            fActiveWindow = nextWindow(fActiveWindow, false, true);
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
    int m = mod & (app->AltMask | app->MetaMask | app->HyperMask | app->SuperMask | ControlMask);

    if ((m & modsDown) != modsDown)
        return false;
    return true;
}

void SwitchWindow::handleButton(const XButtonEvent &button) {
    YPopupWindow::handleButton(button);
}
#endif
