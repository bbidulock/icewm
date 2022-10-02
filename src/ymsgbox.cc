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
#include "wmmgr.h"
#include "ylabel.h"
#include "yinputline.h"
#include "prefs.h"
#include "intl.h"

const unsigned margin = 18;

YMsgBox::YMsgBox(int buttons,
                 const char* title,
                 const char* text,
                 YMsgBoxListener* listener,
                 const char* iconName):
    YDialog(),
    fLabel(nullptr),
    fInput(nullptr),
    fButtonOK(nullptr),
    fButtonCancel(nullptr),
    fListener(listener),
    fPixmap(null)
{
    setToplevel(true);
    if (title) {
        setWindowTitle(title);
    }
    if (text) {
        fLabel = new YLabel(text, this);
        fLabel->show();
    }
    if (buttons & mbInput) {
        fInput = new YInputLine(this, this);
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
    if (nonempty(iconName)) {
        ref<YIcon> icon(YIcon::getIcon(iconName));
        if (icon != null) {
            if (text && !strchr(text, '\n') && icon->small() != null) {
                fPixmap = icon->small()->renderToPixmap(xapp->depth());
            }
            else if (icon->large() != null) {
                fPixmap = icon->large()->renderToPixmap(xapp->depth());
            }
        }
    }
    autoSize();
    setLayerHint(WinLayerAboveDock);
    setWorkspaceHint(AllWorkspaces);
    setWinHintsHint(WinHintsSkipWindowMenu);
    setNetWindowType(_XA_NET_WM_WINDOW_TYPE_DIALOG);
    Atom protocols[] = { _XA_WM_DELETE_WINDOW, _XA_WM_TAKE_FOCUS };
    XSetWMProtocols(xapp->display(), handle(), protocols, 2);
    getProtocols(true);
    setMwmHints(MwmHints(
       MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS,
       MWM_FUNC_MOVE | MWM_FUNC_CLOSE,
       MWM_DECOR_BORDER | MWM_DECOR_TITLE | MWM_DECOR_MENU));
    if (fListener) {
        showFocused();
    }
}

YMsgBox::~YMsgBox() {
    delete fLabel; fLabel = nullptr;
    delete fInput; fInput = nullptr;
    delete fButtonOK; fButtonOK = nullptr;
    delete fButtonCancel; fButtonCancel = nullptr;
}

void YMsgBox::autoSize() {
    unsigned pw = (fPixmap != null) ? fPixmap->width() + margin : 0;
    unsigned ph = (fPixmap != null) ? fPixmap->height() : 0;
    unsigned lw = fLabel ? fLabel->width() : 0;
    unsigned lh = fLabel ? fLabel->height() : 0;
    unsigned w = clamp(lw + pw + 2*margin, 240U, desktop->width());
    unsigned h = margin;

    if (fLabel) {
        fLabel->setPosition(pw + (w - pw - lw) / 2, h);
    }
    h += 18 + max(lh, ph);

    if (fInput) {
        fInput->setSize(w - 2*margin, fInput->height());
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

    h += max(fButtonOK ? fButtonOK->height() : 0,
             fButtonCancel ? fButtonCancel->height() : 0);
    h += margin;

    setSize(w, h);
}

void YMsgBox::setText(const char* text) {
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

void YMsgBox::setPixmap(ref<YPixmap> pixmap) {
    if (fPixmap != pixmap) {
        fPixmap = pixmap;
        autoSize();
    }
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
        fListener->handleMsgBox(this, mbClose);
    else
        delete this;
}

void YMsgBox::handleFocus(const XFocusChangeEvent& focus) {
    if (fInput) {
        fInput->handleFocus(focus);
    }
}

void YMsgBox::showFocused() {
    if (fInput) {
        fInput->requestFocus(false);
    }
    else if (msgBoxDefaultAction == 0 && fButtonCancel) {
        fButtonCancel->requestFocus(false);
    }
    else if (msgBoxDefaultAction == 1 && fButtonOK) {
        fButtonOK->requestFocus(false);
    }

    center();
    become();
}

void YMsgBox::inputReturn(YInputLine* input) {
    if (fListener) {
        fListener->handleMsgBox(this, mbOK);
    }
}

void YMsgBox::inputEscape(YInputLine* input) {
    if (fListener) {
        fListener->handleMsgBox(this, mbCancel);
    }
}

void YMsgBox::inputLostFocus(YInputLine* input) {
}

void YMsgBox::paint(Graphics &g, const YRect& r) {
    YDialog::paint(g, r);
    if (fPixmap != null) {
        int dy = 0;
        if (fLabel && fLabel->height() > fPixmap->height()) {
            dy = (fLabel->height() - fPixmap->height()) / 2;
        }
        g.drawPixmap(fPixmap, margin, margin + dy);
    }
}

// vim: set sw=4 ts=4 et:
