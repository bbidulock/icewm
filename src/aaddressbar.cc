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
#include "yapp.h"
#include "sysdep.h"
#include "default.h"

AddressBar::AddressBar(YWindow *parent):
    YInputLine(parent, addressBarHistory ? "AddressBar" : NULL) {
}

AddressBar::~AddressBar() {
}

bool AddressBar::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        KeySym k = XKeycodeToKeysym(app->display(), key.keycode, 0);
        int m = KEY_MODMASK(key.state);

        if (k == XK_KP_Enter || k == XK_Return) {
            const char *t = getText();
            const char *args[7];
            int i = 0;

            if (m & ControlMask) {
                args[i++] = terminalCommand;
                args[i++] = "-e";
            }
            if (addressBarCommand && addressBarCommand[0]) {
                args[i++] = addressBarCommand;
            } else {
                args[i++] = getenv("SHELL");;
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
        }
    }
    return YInputLine::handleKey(key);
}

#endif
