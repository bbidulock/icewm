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
#include <unistd.h>
#include <errno.h>
#include <X11/keysym.h>
#include "ywordexp.h"

AddressBar::AddressBar(IApp *app, YWindow *parent):
    YInputLine(parent, this),
    app(app),
    location(0),
    curdir(upath::cwd()),
    olddir(curdir)
{
}

AddressBar::~AddressBar() {
}

bool AddressBar::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        KeySym k = keyCodeToKeySym(key.keycode);
        if (k == XK_Up ||
           (k == XK_KP_Up && !(key.state & xapp->NumLockMask)))
        {
            return changeLocation(location - 1), true;
        }
        if (k == XK_Down ||
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

void AddressBar::handleReturn(bool control) {
    mstring line(getText());
    YStringArray args;

    if (line.nonempty()) {
        history.append(line);
        location = history.getCount();
    }
    if (internal(line))
        return;

    hideNow();
    if (control) {
        if ( ! appendCommand(terminalCommand, args))
            return;
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
    if (args[0]) {
        upath cwd(upath::cwd());
        bool change = (cwd != curdir);
        if (change)
            curdir.chdir();
        app->runProgram(args[0], args.getCArray());
        if (change)
            cwd.chdir();
    }
    selectAll();
}

bool AddressBar::internal(mstring line) {
    mstring cmd(line.trim());
    if (cmd == "pwd")
        return setText(upath::cwd(), true), true;
    if (cmd == "cd") {
        olddir = curdir;
        curdir = YApplication::getHomeDir();
        return setText(curdir, true), true;
    }
    mstring arg;
    if (cmd.startsWith("cd") && cmd.split(' ', &cmd, &arg) && cmd == "cd") {
        arg = arg.trim();
        if (strchr("$~", arg[0]))
            arg = upath(arg).expand();
        else if (arg == "-")
            arg = olddir;
        upath newdir((arg[0] == '/') ? upath(arg) : curdir + arg);
        upath realdir = upath(newdir).real();
        if (realdir.isEmpty()) {
            arg.fmt("%s: %m", newdir.string());
            return setText(arg, true), true;
        }
        else if ( !realdir.dirExists() || !realdir.isExecutable()) {
            arg.fmt("%s: %m", realdir.string());
            return setText(arg, true), true;
        } else {
            olddir = curdir;
            curdir = realdir;
            return setText(curdir, true), true;
        }
    }
    return false;
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

void AddressBar::inputReturn(YInputLine* input, bool control) {
    handleReturn(control);
}

void AddressBar::inputEscape(YInputLine* input) {
    hideNow();
}

void AddressBar::inputLostFocus(YInputLine* input) {
}

// vim: set sw=4 ts=4 et:
