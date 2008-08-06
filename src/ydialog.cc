/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 *
 * Dialogs
 */
#include "config.h"

#ifndef LITE
#include "ypixbuf.h"
#include "ykey.h"
#include "ydialog.h"

#include "yxapp.h"
#include "prefs.h"
#include "WinMgr.h"
#include "wmframe.h"
#include "wmclient.h"
#include "wmmgr.h"
#include "wmdialog.h"
#include "wmabout.h"
#include "sysdep.h"

static YColor *dialogBg = 0;

YDialog::YDialog(YWindow *owner):
    YFrameClient(0, 0) INIT_GRADIENT(fGradient, NULL) {
    if (dialogBg == 0)
        dialogBg = new YColor(clrDialog);

    fOwner = owner;
}

YDialog::~YDialog() {
#ifdef CONFIG_GRADIENTS
    fGradient = null;
#endif
}

void YDialog::paint(Graphics &g, const YRect &/*r*/) {
    g.setColor(dialogBg);
    g.draw3DRect(0, 0, width() - 1, height() - 1, true);

#ifdef CONFIG_GRADIENTS
    if (dialogbackPixbuf != null
        && !(fGradient != null &&
             fGradient->width() == (width() - 2) &&
             fGradient->height() == (height() - 2)))
    {
        fGradient = dialogbackPixbuf->scale(width() - 2, height() - 2);
        repaint();
    }

    if (fGradient != null)
        g.drawImage(fGradient, 0, 0, width() - 2, height() - 2, 1, 1);
    else 
#endif    
    if (dialogbackPixmap != null)
        g.fillPixmap(dialogbackPixmap, 1, 1, width() -2, height() - 2);
    else
        g.fillRect(1, 1, width() - 2, height() - 2);
}

bool YDialog::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        KeySym k = XKeycodeToKeysym(xapp->display(), (KeyCode)key.keycode, 0);
        int m = KEY_MODMASK(key.state);

        if (k == XK_Escape && m == 0) {
            handleClose();
            return true;
        }
    }
    return YFrameClient::handleKey(key);
}
#endif
