/*
 * IceWM
 *
 * Copyright (C) 1998-2001 Marko Macek
 *
 * AddressBar
 */
#include "config.h"
#include "ykey.h"
#include "aaddressbar.h"

#ifdef CONFIG_ADDRESSBAR
#include "yxapp.h"
#include "wmmgr.h"
#include "sysdep.h"
#include "default.h"


AddressBar::AddressBar(YWindow *parent): YInputLine(parent) {
}

AddressBar::~AddressBar() {
}

bool AddressBar::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        KeySym k = XKeycodeToKeysym(xapp->display(), key.keycode, 0);
        int m = KEY_MODMASK(key.state);

        if (k == XK_KP_Enter || k == XK_Return) {
            const char *t = getText();
            const char *args[7];
            int i = 0;

            hideNow();

            if (m & ControlMask) {
                args[i++] = terminalCommand;
                args[i++] = "-e";
            }
            if (addressBarCommand && addressBarCommand[0]) {
                args[i++] = addressBarCommand;
            } else {
/// TODO #warning calling /bin/sh is considered to be bloat
                args[i++] = "/bin/sh";
                args[i++] = "-c";
            }
            args[i++] = t;
            args[i++] = 0;
            if (m & ControlMask)
                if (t == 0 || t[0] == 0)
                    args[1] = 0;
            app->runProgram(args[0], args);
            selectAll();
            return true;
        } else if (k == XK_Escape) {
            hideNow();
            return true;
        }
    }
    return YInputLine::handleKey(key);
}

void AddressBar::showNow() {
    if (!showAddressBar || (taskBarShowWindows && !taskBarDoubleHeight) ) {
        raise();
        show();
    }
    setWindowFocus();
}

void AddressBar::hideNow() {
    manager->focusLastWindow();
    if (!showAddressBar || (taskBarShowWindows && !taskBarDoubleHeight) ) {
        hide();
    }
}

#endif
