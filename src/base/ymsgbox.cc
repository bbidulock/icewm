/*
 * IceWM
 *
 * Copyright (C) 1999 Marko Macek
 *
 * MessageBox
 */
#pragma implementation
#include "config.h"

#include "base.h"
#include "ymsgbox.h"

YMsgBox::YMsgBox(int buttons, YWindow *owner): YDialog(owner),
    fLabel(0), fButtonOK(0), fButtonCancel(0), fListener(0)
{
    fLabel = new YLabel(0, this);
    fLabel->show();

    setToplevel(true);

    if (buttons & mbOK) {
        fButtonOK = new YActionButton(this);
        if (fButtonOK) {
            fButtonOK->__setText("OK");
            fButtonOK->setActionListener(this);
            fButtonOK->show();
        }
    }
    if (buttons & mbCancel) {
        fButtonCancel = new YActionButton(this);
        if (fButtonCancel) {
            fButtonCancel->__setText("Cancel");
            fButtonCancel->setActionListener(this);
            fButtonCancel->show();
        }
    }
    setResizeable(false);
    autoSize();
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
    int lw = fLabel ? fLabel->width() : 0;
    int w = lw + 40, h;

    if (w < 300)
        w = 300;
    
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
    if (fButtonOK)
        h += fButtonOK->height();
    else if (fButtonCancel)
        h += fButtonCancel->height();

    h += 20;
    
    setSize(w, h);

    /*if (fLabel)
        fLabel->setPosition(20, 20);
    if (fButtonOK)
        fButtonOK->setPosition(100, 60);
    if (fButtonCancel)
        fButtonCancel->setPosition(300, 60);*/
}

void YMsgBox::__setText(const char *text) {
    if (fLabel)
        fLabel->setText(text);
    autoSize();
}

void YMsgBox::setPixmap(YPixmap * /*pixmap*/) {
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

bool YMsgBox::eventFocus(const YFocusEvent &focus) {
    return YWindow::eventFocus(focus);
}

#warning "fix YMsgBox::showFocused, add beforeShow event"
void YMsgBox::showFocused() {
    warn("YMsgBox::showFocused");
#if 1
    setPosition(desktop->width() / 2 - width() / 2,
                desktop->height() / 2 - height() / 2);
    show();
#if 0
    requestFocus();
#endif
    //if (getFrame() == 0)
    //    desktop->manageWindow(this, false);
    //if (getFrame()) {
    //    getFrame()->activate(true);
    //    if (fButtonCancel) {
    //        fButtonCancel->requestFocus();
    //    }
    //}
#endif
}
