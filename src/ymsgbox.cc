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

void YMsgBox::getMaxButton(unsigned int &w, unsigned int &h) {
    w = 0;
    h = 0;
    if (fButtonOK) {
        w = fButtonOK->width();
        h = fButtonOK->height();
    }
    if (fButtonCancel) {
        if (fButtonCancel->width() > w)
            w = fButtonCancel->width();
        if (fButtonCancel->height() > h)
            h = fButtonCancel->height();
    }
}

void YMsgBox::autoSize() {
    unsigned lw = fLabel ? fLabel->width() : 0;
    unsigned w = lw + 40, h;

    w = clamp(w, 300U, desktop->width());
    
    h = 20;
    if (fLabel) {
        fLabel->setPosition(20, h);
        h += fLabel->height();
    }
    h += 20;
    if (fButtonOK) {
        unsigned int ww, hh;

        getMaxButton(ww, hh);
        fButtonOK->setSize(ww, hh);
        fButtonOK->setPosition(w / 4 - fButtonOK->width() / 2, h);
    }
    if (fButtonCancel) {
        unsigned int ww, hh;

        getMaxButton(ww, hh);
        fButtonCancel->setSize(ww, hh);
        fButtonCancel->setPosition(3 * w / 4 - fButtonCancel->width() / 2, h);
    }
    h += fButtonOK ? fButtonOK->height() :
        fButtonCancel ? fButtonCancel->height() : 0;
    h += 20;
    
    setSize(w, h);

    /*if (fLabel)
        fLabel->setPosition(20, 20);
    if (fButtonOK)
        fButtonOK->setPosition(100, 60);
    if (fButtonCancel)
        fButtonCancel->setPosition(300, 60);*/
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

void YMsgBox::setPixmap(YPixmap */*pixmap*/) {
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
        getFrame()->setPosition(desktop->width() / 2 - getFrame()->width() / 2,
                                desktop->height() / 2 - getFrame()->height() / 2);
        getFrame()->activate(true);
	
	switch(msgBoxDefaultAction) {
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
