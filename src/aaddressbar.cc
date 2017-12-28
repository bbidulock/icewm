/*
 * IceWM
 *
 * Copyright (C) 1998-2001 Marko Macek
 *
 * AddressBar
 */
#include "config.h"
#include "aaddressbar.h"
#include "yxapp.h"
#include "wmmgr.h"
#include "default.h"

AddressBar::AddressBar(IApp *app, YWindow *parent):
    YInputLine(parent),
    location(0)
{
    this->app = app;
}

AddressBar::~AddressBar() {
}

bool AddressBar::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        KeySym k = keyCodeToKeySym(key.keycode);

        if (k == XK_KP_Enter || k == XK_Return) {
            hideNow();
            return handleReturn(KEY_MODMASK(key.state));
        }
        else if (k == XK_Escape) {
            hideNow();
            return true;
        }
        else if (k == XK_Up ||
                (k == XK_KP_Up && !(key.state & xapp->NumLockMask)))
        {
            return changeLocation(location - 1);
        }
        else if (k == XK_Down ||
                (k == XK_KP_Down && !(key.state & xapp->NumLockMask)))
        {
            return changeLocation(location + 1);
        }
    }
    return YInputLine::handleKey(key);
}

bool AddressBar::handleReturn(int mask) {
    mstring line(getText());
    cstring text(line);
    const char *args[7];
    int i = 0;

    if (line.nonempty()) {
        history.append(line);
        location = history.getCount();
    }

    if (mask & ControlMask) {
        args[i++] = terminalCommand;
        args[i++] = "-e";
    }
    if (addressBarCommand && addressBarCommand[0]) {
        args[i++] = addressBarCommand;
    } else {
        args[i++] = "/bin/sh";
        args[i++] = "-c";
    }
    args[i++] = text.c_str();
    args[i++] = 0;

    if (line.isEmpty())
        args[hasbit(mask, ControlMask)] = 0;
    if (args[0])
        app->runProgram(args[0], args);
    selectAll();

    return true;
}

bool AddressBar::changeLocation(int newLocation) {
    if (inrange(newLocation, 0, history.getCount())) {
        location = newLocation;
        if (location == history.getCount()) {
            setText(null);
        }
        else {
            setText(history[location]);
        }
    }
    return true;
}

void AddressBar::showNow() {
    if (!showAddressBar || (taskBarShowWindows && !taskBarDoubleHeight) ) {
        raise();
        show();
    }
    setWindowFocus();
}

void AddressBar::hideNow() {
    if (!showAddressBar || (taskBarShowWindows && !taskBarDoubleHeight) ) {
        hide();
    }
    manager->focusLastWindow();
}

void AddressBar::handleFocus(const XFocusChangeEvent &focus) {
    if (focus.type == FocusOut) {
        if (focus.mode == NotifyNormal || focus.mode == NotifyWhileGrabbed) {
            if (!showAddressBar || (taskBarShowWindows && !taskBarDoubleHeight) ) {
                hide();
            }
        }
    }
    YInputLine::handleFocus(focus);
}

// vim: set sw=4 ts=4 et:
