/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 *
 * Dialogs
 */
#pragma implementation
#include "config.h"

#include "yxkeydef.h"
#include "ydialog.h"
#include "ykeyevent.h"

#include "yapp.h"
#include "prefs.h"
#include "WinMgr.h"
#include "sysdep.h"

#include "ypaint.h"

YColorPrefProperty YDialog::gDialogBg("system", "ColorDialog", "rgb:C0/C0/C0");
YPixmapPrefProperty YDialog::gPixmapBackground("system", "PixmapDialog", 0, 0);

YDialog::YDialog(YWindow *owner): YTopWindow(), fOwner(owner) {
}

void YDialog::paint(Graphics &g, const YRect &/*er*/) {
    g.setColor(gDialogBg);
    g.draw3DRect(0, 0, width() - 1, height() - 1, true);
    YPixmap *bg = gPixmapBackground.getPixmap();
    if (bg)
        g.fillPixmap(bg, 1, 1, width() -2, height() - 2);
    else
        g.fillRect(1, 1, width() - 2, height() - 2);
}

bool YDialog::eventKey(const YKeyEvent &key) {
    if (key.type() == YEvent::etKeyPress) {
        if (key.getKey() == XK_Escape && key.getKeyModifiers() == 0) {
            handleClose();
            return true;
        }
    }
    return YWindow::eventKey(key);
}
