/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 *
 * Dialogs
 */
#include "config.h"

#include "ydialog.h"
#include "wpixmaps.h"
#include "yxapp.h"
#include "prefs.h"

static YColorName dialogBg(&clrDialog);

YDialog::YDialog(YWindow *owner):
    YFrameClient(0, 0),
    fGradient(null)
{
    fOwner = owner;
}

YDialog::~YDialog() {
    fGradient = null;
}

void YDialog::paint(Graphics &g, const YRect &/*r*/) {
    g.setColor(dialogBg);
    g.draw3DRect(0, 0, width() - 1, height() - 1, true);

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
    if (dialogbackPixmap != null)
        g.fillPixmap(dialogbackPixmap, 1, 1, width() -2, height() - 2);
    else
        g.fillRect(1, 1, width() - 2, height() - 2);
}

bool YDialog::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        KeySym k = keyCodeToKeySym(key.keycode);
        int m = KEY_MODMASK(key.state);

        if (k == XK_Escape && m == 0) {
            handleClose();
            return true;
        }
    }
    return YFrameClient::handleKey(key);
}

// vim: set sw=4 ts=4 et:
