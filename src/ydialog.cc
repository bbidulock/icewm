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
    fOwner(owner),
    fGradient(dialogbackPixbuf),
    fSurface(dialogBg, dialogbackPixmap, getGradient())
{
    addStyle(wsNoExpose);
    setNetWindowType(_XA_NET_WM_WINDOW_TYPE_DIALOG);
}

YDialog::~YDialog() {
    fGradient = null;
}

void YDialog::paint(Graphics &g, const YRect& r) {
    if (width() > 2 && height() > 2)
        g.drawSurface(getSurface(), 1, 1, width() - 2, height() - 2);
    g.setColor(dialogBg);
    g.draw3DRect(0, 0, width() - 1, height() - 1, true);
}

void YDialog::configure(const YRect2& r) {
    if (r.resized()) {
        if (dialogbackPixbuf == null || r.width() <= 2 || r.height() <= 2)
            fGradient = null;
        else
            fGradient = dialogbackPixbuf->scale(r.width() - 2, r.height() - 2);
        repaint();
    }
}

void YDialog::repaint() {
    GraphicsBuffer(this).paint();
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
