/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 *
 * Dialogs
 */
#include "config.h"

#include "ykey.h"
#include "ydialog.h"

#include "yapp.h"
#include "prefs.h"
#include "WinMgr.h"
#include "sysdep.h"

static YColor *dialogBg = 0;

YDialog::YDialog(YWindow *owner): YWindow(0) {
    if (dialogBg == 0)
        dialogBg = new YColor(clrDialog);

    fOwner = owner;
}

void YDialog::paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
    g.setColor(dialogBg);
    g.draw3DRect(0, 0, width() - 1, height() - 1, true);
#if 0
    if (menubackPixmap)
        g.fillPixmap(menubackPixmap, 1, 1, width() -2, height()-2);
    else
#endif
        g.fillRect(1, 1, width() - 2, height() - 2);
}

bool YDialog::handleKeySym(const XKeyEvent &key, KeySym ksym, int vmod) {
    if (key.type == KeyPress) {
        if (ksym == XK_Escape && vmod == 0) {
            handleClose();
            return true;
        }
    }
    return YWindow::handleKeySym(key, ksym, vmod);
}
