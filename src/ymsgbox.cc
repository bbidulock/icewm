/*
 * IceWM
 *
 * Copyright (C) 1999-2001 Marko Macek
 *
 * MessageBox
 */
#include "config.h"

#ifndef LITE

#include "ylib.h"
#include "ymsgbox.h"

#include "WinMgr.h"
#include "yapp.h"
#include "wmframe.h"
#include "sysdep.h"
#include "yprefs.h"
#include "prefs.h"

#include "intl.h"

YMsgBox::YMsgBox(int buttons, YWindow *owner): YDialog(owner) {
    fListener = 0;
    fButtonOK = 0;
    fButtonCancel = 0;
    fLabel = new YLabel(0, this);
    fLabel->show();

    setToplevel(true);

    if (buttons & mbOK) {
        fButtonOK = new YActionButton(this);
        if (fButtonOK) {
            fButtonOK->setText(_("OK"));
            fButtonOK->setActionListener(this);
            fButtonOK->show();
        }
    }
    if (buttons & mbCancel) {
        fButtonCancel = new YActionButton(this);
        if (fButtonCancel) {
            fButtonCancel->setText(_("Cancel"));
            fButtonCancel->setActionListener(this);
            fButtonCancel->show();
        }
    }
    autoSize();
    setWinLayerHint(WinLayerAboveDock);
    setWinStateHint(WinStateAllWorkspaces, WinStateAllWorkspaces);
    setWinHintsHint(WinHintsSkipWindowMenu);
    {
        MwmHints mwm;

        memset(&mwm, 0, sizeof(mwm));
        mwm.flags =
            MWM_HINTS_FUNCTIONS |
            MWM_HINTS_DECORATIONS;
        mwm.functions =
            MWM_FUNC_MOVE | MWM_FUNC_CLOSE;
        mwm.decorations =
            MWM_DECOR_BORDER | MWM_DECOR_TITLE | MWM_DECOR_MENU;

        setMwmHints(mwm);
    }
}

YMsgBox::~YMsgBox() {
    delete fLabel; fLabel = 0;
    delete fButtonOK; fButtonOK = 0;
    delete fButtonCancel; fButtonCancel = 0;
}

void YMsgBox::autoSize() {
    int lw = fLabel ? fLabel->width() : 0;
    int w = lw + 24, h;

    w = clamp(w, 240, desktop->width());
    
    h = 12;
    if (fLabel) {
        fLabel->setPosition((w - lw) / 2, h);
        h += fLabel->height();
    }
    h += 18;
    
    unsigned const hh(max(fButtonOK ? fButtonOK->height() : 0,
                          fButtonCancel ? fButtonCancel->height() : 0));
    unsigned const ww(max(fButtonOK ? fButtonOK->width() : 0,
                          fButtonCancel ? fButtonCancel->width() : 0) + 3);

    if (fButtonOK) {
        fButtonOK->setSize(ww, hh);
        fButtonOK->setPosition((w - hh)/2 - fButtonOK->width(), h);
    }
    if (fButtonCancel) {
        fButtonCancel->setSize(ww, hh);
        fButtonCancel->setPosition((w + hh)/2, h);
    }

    h += fButtonOK ? fButtonOK->height() :
        fButtonCancel ? fButtonCancel->height() : 0;
    h += 12;
    
    setSize(w, h);
}

void YMsgBox::setTitle(const char *title) {
    setWindowTitle(title);
    autoSize();
}

void YMsgBox::setText(const char *text) {
    if (fLabel)
        fLabel->setText(text);
    autoSize();
}

void YMsgBox::setPixmap(ref<YPixmap>/*pixmap*/) {
}

void YMsgBox::actionPerformed(YAction *action, unsigned int /*modifiers*/) {
    if (fListener) {
        if (action == fButtonOK) {
            fListener->handleMsgBox(this, mbOK);
        } else if (action == fButtonCancel) {
            fListener->handleMsgBox(this, mbCancel);
        }
    }
}

void YMsgBox::handleClose() {
    fListener->handleMsgBox(this, 0);
}

void YMsgBox::handleFocus(const XFocusChangeEvent &/*focus*/) {
}

void YMsgBox::showFocused() {
    if (getFrame() == 0)
        manager->manageClient(handle(), false);
    if (getFrame()) {
        int dx, dy, dw, dh;
        desktop->getScreenGeometry(&dx, &dy, &dw, &dh);
        getFrame()->setNormalPositionOuter(
            dx + dw / 2 - getFrame()->width() / 2,
            dy + dh / 2 - getFrame()->height() / 2);
        getFrame()->activate(true);

        switch (msgBoxDefaultAction) {
        case 0:
            if (fButtonCancel) fButtonCancel->requestFocus();
            break;
        case 1:
            if (fButtonOK) fButtonOK->requestFocus();
            break;
        }
    }
}
#endif
