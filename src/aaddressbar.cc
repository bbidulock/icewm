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
#include <sys/stat.h>
#include <X11/keysym.h>
#include "ywordexp.h"

AddressBar::AddressBar(IApp *app, YWindow *parent):
    YInputLine(parent, this),
    app(app),
    location(0),
    restored(0),
    curdir(upath::cwd()),
    olddir(curdir)
{
    loadHistoryTimer->setTimer(500L, this, true);
}

AddressBar::~AddressBar() {
    saveHistory();
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
        if (saveHistoryTimer == nullptr) {
            saveHistoryTimer->setTimer(minute(), this, true);
        }
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
        bool change = false;
        upath cwd;
        if (curdir != null && curdir != (cwd = upath::cwd()))
            change = true;
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
            arg = (olddir != null) ? olddir.path() : upath::cwd();
        upath newdir((arg[0] == '/') ? upath(arg) :
                     (curdir != null) ? curdir + arg :
                     upath(upath::cwd()) + arg);
        upath realdir = upath(newdir).real();
        if (realdir.isEmpty()) {
            arg.fmt("%s: %m", newdir.string());
            return setText(arg, true), true;
        }
        else if ( !realdir.dirExists() || !realdir.isExecutable()) {
            arg.fmt("%s: %m", realdir.string());
            return setText(arg, true), true;
        } else {
            olddir = (curdir != null) ? curdir : upath(upath::cwd());
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

bool AddressBar::handleTimer(YTimer* timer) {
    if (timer == loadHistoryTimer) {
        loadHistory();
        loadHistoryTimer = null;
    }
    else if (timer == saveHistoryTimer) {
        saveHistory();
        saveHistoryTimer = null;
    }
    else {
        return YInputLine::handleTimer(timer);
    }
    return false;
}

upath AddressBar::historyFile() {
    return YApplication::getPrivConfFile("ahistory");
}

void AddressBar::loadHistory() {
    upath path(historyFile());
    FILE* fp = path.fopen("r");
    if (fp) {
        char buf[4096];
        int n = 0;
        while (fgets(buf, sizeof buf, fp))
            ++n;
        rewind(fp);
        for (int i = saveLines; i < n && fgets(buf, sizeof buf, fp); ++i)
            ;
        for (int i = 0; i < saveLines && fgets(buf, sizeof buf, fp); ++i) {
            char* nl = strchr(buf, '\n');
            if (nl)
                *nl = '\0';
            history.insert(i, buf);
            location++;
            restored++;
        }
        fclose(fp);
    }
}

void AddressBar::saveHistory() {
    if (restored < history.getCount()) {
        upath path(historyFile());
        FILE* fp = path.fopen("w");
        if (fp) {
            fchmod(fileno(fp), 0600);
            const int first = max(0, history.getCount() - saveLines);
            for (int i = first; i < history.getCount(); ++i) {
                fputs(history[i], fp);
                fputc('\n', fp);
            }
            fclose(fp);
            restored = history.getCount();
        }
    }
}

// vim: set sw=4 ts=4 et:
