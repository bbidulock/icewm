/*
 * IceWM
 *
 * Copyright (C) 1999-2001 Marko Macek
 *
 * MessageBox
 */
#include "config.h"
#include "ymsgbox.h"
#include "yxapp.h"
#include "wmframe.h"
#include "ylabel.h"
#include "yinputline.h"
#include "prefs.h"
#include "intl.h"

YMsgBox::YMsgBox(int buttons):
    YDialog(),
    fLabel(nullptr),
    fInput(nullptr),
    fButtonOK(nullptr),
    fButtonCancel(nullptr),
    fListener(nullptr)
{
    setToplevel(true);
    if (buttons & mbInput) {
        fInput = new YInputLine(this);
        if (fInput) {
            fInput->show();
        }
    }
    if (buttons & mbOK) {
        fButtonOK = new YActionButton(this, _("_OK"), -2, this);
    }
    if (buttons & mbCancel) {
        fButtonCancel = new YActionButton(this, _("_Cancel"), -2, this);
    }
    autoSize();
    setWinLayerHint(WinLayerAboveDock);
    setWinWorkspaceHint(AllWorkspaces);
    setWinHintsHint(WinHintsSkipWindowMenu);
    Atom protocols[] = { _XA_WM_DELETE_WINDOW, _XA_WM_TAKE_FOCUS };
    XSetWMProtocols(xapp->display(), handle(), protocols, 2);
    getProtocols(true);
    setMwmHints(MwmHints(
       MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS,
       MWM_FUNC_MOVE | MWM_FUNC_CLOSE,
       MWM_DECOR_BORDER | MWM_DECOR_TITLE | MWM_DECOR_MENU));
}

YMsgBox::~YMsgBox() {
    delete fLabel; fLabel = nullptr;
    delete fButtonOK; fButtonOK = nullptr;
    delete fButtonCancel; fButtonCancel = nullptr;
}

void YMsgBox::autoSize() {
    unsigned lw = fLabel ? fLabel->width() : 0;
    unsigned w = clamp(lw + 24, 240U, desktop->width());
    unsigned h = 12;

    if (fLabel) {
        fLabel->setPosition((w - lw) / 2, h);
        h += fLabel->height();
    }
    h += 18;

    if (fInput) {
        fInput->setPosition((w - fInput->width()) / 2, h);
        h += 18 + fInput->height();
    }

    unsigned const hh(max(fButtonOK ? fButtonOK->height() : 0,
                          fButtonCancel ? fButtonCancel->height() : 0));
    unsigned const ww(max(fButtonOK ? fButtonOK->width() : 0,
                          fButtonCancel ? fButtonCancel->width() : 3));

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

void YMsgBox::setTitle(mstring title) {
    setWindowTitle(title);
}

void YMsgBox::setText(mstring text) {
    if (fLabel) {
        fLabel->hide();
        fLabel->setText(text);
    }
    else {
        fLabel = new YLabel(text, this);
    }
    if (fLabel) {
        autoSize();
        fLabel->show();
        fLabel->repaint();
    }
}

void YMsgBox::setPixmap(ref<YPixmap>/*pixmap*/) {
}

void YMsgBox::actionPerformed(YAction action, unsigned int /*modifiers*/) {
    if (fListener) {
        if (fButtonOK && action == *fButtonOK) {
            fListener->handleMsgBox(this, mbOK);
        }
        else if (fButtonCancel && action == *fButtonCancel) {
            fListener->handleMsgBox(this, mbCancel);
        }
        else {
            TLOG(("unknown action %d for msgbox", action.ident()));
        }
    }
}

void YMsgBox::handleClose() {
    if (fListener)
        fListener->handleMsgBox(this, 0);
    else {
        manager->unmanageClient(this);
        manager->focusTopWindow();
    }
}

void YMsgBox::handleFocus(const XFocusChangeEvent &/*focus*/) {
}

void YMsgBox::showFocused() {
    YRect r(desktop->getScreenGeometry());
    setPosition(r.x() + int(r.width() / 2) - int(width() / 2),
                r.y() + int(r.height() / 2) - int(height() / 2));

    switch (msgBoxDefaultAction) {
    case 0:
        if (fButtonCancel) fButtonCancel->requestFocus(false);
        break;
    case 1:
        if (fButtonOK) fButtonOK->requestFocus(false);
        break;
    }
    if (getFrame() == nullptr) {
        manager->manageClient(handle(), false);
    }
    if (getFrame()) {
        getFrame()->activateWindow(true);
    }
}

// vim: set sw=4 ts=4 et:
