/*
 * IceWM
 *
 * Copyright (C) 1998,1999 Marko Macek
 *
 * AddressBar
 */
#include "config.h"
#include "ykey.h"
#include "aaddressbar.h"

#include "yapp.h"
#include "yconfig.h"
#include "sysdep.h"
#include "default.h"
//#include "bindkey.h"

#include <stdlib.h>

AddressBar::AddressBar(YWindow *parent): YInputLine(parent) {
}

AddressBar::~AddressBar() {
}

bool AddressBar::handleKeySym(const XKeyEvent &key, KeySym ksym, int vmod) {
    if (key.type == KeyPress) {
        if (ksym == XK_KP_Enter || ksym == XK_Return) {
            const char *t = getText();
            const char *args[7];
            int i = 0;

            if (vmod & kfCtrl) {
                YPref prefTerminalCommand("system", "TerminalCommand"); // !! fix domain
                const char *pvTerminalCommand = prefTerminalCommand.getStr(0);


                args[i++] = pvTerminalCommand;
                args[i++] = "-e";
            }
            YPref prefAddressBarCommand("addressbar_applet", "AddressBarCommand");
            const char *pvAddressBarCommand = prefAddressBarCommand.getStr(0);
            if (pvAddressBarCommand && pvAddressBarCommand[0]) {
                args[i++] = pvAddressBarCommand;
            } else {
                args[i++] = getenv("SHELL");;
                args[i++] = "-c";
            }
            args[i++] = t;
            args[i++] = 0;
            if (vmod & kfCtrl)
                if (t == 0 || t[0] == 0)
                    args[1] = 0;
            app->runProgram(args[0], args);
            selectAll();
            return true;
        }
    }
    return YInputLine::handleKeySym(key, ksym, vmod);
}
