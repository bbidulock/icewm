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
#include <X11/keysym.h>
#include "ywordexp.h"

AddressBar::AddressBar(IApp *app, YWindow *parent):
    YInputLine(parent),
    app(app),
    location(0)
{
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
            return changeLocation(location - 1), true;
        }
        else if (k == XK_Down ||
                (k == XK_KP_Down && !(key.state & xapp->NumLockMask)))
        {
            return changeLocation(location + 1), true;
        }
    }
    return YInputLine::handleKey(key);
}

bool AddressBar::appendCommand(const char* cmd, class YStringArray& args) {
    const int count = args.getCount();
    if (nonempty(cmd)) {
        wordexp_t exp = {};
        if (wordexp(cmd, &exp, WRDE_NOCMD) == 0) {
            for (size_t i = 0; i < size_t(exp.we_wordc); ++i) {
                if (exp.we_wordv[i]) {
                    args += exp.we_wordv[i];
                }
            }
            wordfree(&exp);
        }
    }
    return count < args.getCount();
}

bool AddressBar::handleReturn(int mask) {
    const bool control(hasbit(mask, ControlMask));
    mstring line(getText());
    YStringArray args;

    if (line.nonempty()) {
        history.append(line);
        location = history.getCount();
    }

    if (control) {
        if ( ! appendCommand(terminalCommand, args))
            return false;
        args += "-e";
    }
    if ( ! appendCommand(addressBarCommand, args)) {
        args += "/bin/sh";
        args += "-c";
    }
    args += line;
    args += nullptr;

    if (line.isEmpty())
        args.replace(control, nullptr);
    if (args[0])
        app->runProgram(args[0], args.getCArray());
    selectAll();

    return true;
}

void AddressBar::changeLocation(int newLocation) {
    if (! inrange(newLocation, 0, history.getCount()))
        return;
    location = newLocation;
    setText(location == history.getCount() ? null : history[location], true);
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
