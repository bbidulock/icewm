/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 *
 * Dialogs
 */
#include "config.h"

#ifndef LITE
#include "ykey.h"
#include "ydialog.h"

#include "yapp.h"
#include "prefs.h"
#include "WinMgr.h"
#include "wmframe.h"
#include "wmclient.h"
#include "wmmgr.h"
#include "wmdialog.h"
#include "wmabout.h"
#include "sysdep.h"

static YColor *dialogBg = 0;

YDialog::YDialog(YWindow *owner): YFrameClient(0, 0) {
    if (dialogBg == 0)
        dialogBg = new YColor(clrDialog);

    fOwner = owner;
}

void YDialog::paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
    g.setColor(dialogBg);
    g.draw3DRect(0, 0, width() - 1, height() - 1, true);
    if (menubackPixmap)
        g.fillPixmap(menubackPixmap, 1, 1, width() -2, height()-2);
    else
        g.fillRect(1, 1, width() - 2, height() - 2);
}

bool YDialog::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        KeySym k = XKeycodeToKeysym(app->display(), key.keycode, 0);
        int m = KEY_MODMASK(key.state);

        if (k == XK_Escape && m == 0) {
            handleClose();
            return true;
        }
    }
    return YFrameClient::handleKey(key);
}
#endif
