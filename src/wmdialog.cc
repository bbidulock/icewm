/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 *
 * Dialogs
 */
#include "config.h"

#ifndef LITE
#include "ykey.h"
#include "wmdialog.h"
#include "wmaction.h"
#include "yactionbutton.h"

#include "prefs.h"
#include "wmapp.h"
#include "wmmgr.h"

#include "intl.h"

#define HORZ 10
#define MIDH 10
#define VERT 10
#define MIDV 6

static YColor *cadBg = 0;

CtrlAltDelete *ctrlAltDelete = 0;

static bool canShutdown(bool reboot) {
    if (!reboot)
        if (shutdownCommand == 0 || shutdownCommand[0] == 0)
            return false;
    if (reboot)
        if (rebootCommand == 0 || rebootCommand[0] == 0)
            return false;
    if (logoutCommand && logoutCommand[0])
        return false;
#ifdef CONFIG_SESSION
    if (smapp->haveSessionManager())
        return false;
#endif
    return true;
}

CtrlAltDelete::CtrlAltDelete(YWindow *parent): YWindow(parent) {
    unsigned int w = 0, h = 0;
    YButton *b;

    if (cadBg == 0)
        cadBg = new YColor(clrDialog);

    setStyle(wsOverrideRedirect);
    setPointer(YApplication::leftPointer);
    setToplevel(true);
 
    b = lockButton = new YActionButton(this);
    b->setText(_("Lock _Workstation"), -2);
    if (b->width() > w) w = b->width();
    if (b->height() > h) h = b->height();
    b->setActionListener(this);
    b->show();

    b = logoutButton = new YActionButton(this);
    b->setText(_("_Logout..."), -2);
    if (b->width() > w) w = b->width();
    if (b->height() > h) h = b->height();
    b->setActionListener(this);
    b->show();

    b = cancelButton = new YActionButton(this);
    b->setText(_("_Cancel"), -2);
    if (b->width() > w) w = b->width();
    if (b->height() > h) h = b->height();
    b->setActionListener(this);
    b->show();

    b = restartButton = new YActionButton(this);
    b->setText(_("_Restart icewm"), -2);
    if (b->width() > w) w = b->width();
    if (b->height() > h) h = b->height();
    b->setActionListener(this);
    b->show();

    b = rebootButton = new YActionButton(this);
    b->setText(_("Re_boot"), -2);
    if (b->width() > w) w = b->width();
    if (b->height() > h) h = b->height();
    b->setActionListener(this);
    b->show();

    b = shutdownButton = new YActionButton(this);
    b->setText(_("Shut_down"), -2);
    if (b->width() > w) w = b->width();
    if (b->height() > h) h = b->height();
    b->setActionListener(this);
    b->show();

    if (!canShutdown(true))
        rebootButton->setEnabled(false);
    if (!canShutdown(false))
        shutdownButton->setEnabled(false);

    setSize(HORZ + w + MIDH + w + MIDH + w + HORZ,
            VERT + h + MIDV + h + VERT);
    setPosition((desktop->width() - width()) / 2,
                (desktop->height() - height()) / 2);

    lockButton->setGeometry(HORZ, VERT, w, h);
    logoutButton->setGeometry(HORZ + w + MIDH, VERT, w, h);
    cancelButton->setGeometry(HORZ + w + MIDH + w + MIDH, VERT, w, h);
    restartButton->setGeometry(HORZ, VERT + h + MIDV, w, h);
    rebootButton->setGeometry(HORZ + w + MIDH, VERT + h + MIDV, w, h);
    shutdownButton->setGeometry(HORZ + w + MIDH + w + MIDH, VERT + h + MIDV, w, h);
}

CtrlAltDelete::~CtrlAltDelete() {
    delete lockButton; lockButton = 0;
    delete logoutButton; logoutButton = 0;
    delete restartButton; restartButton = 0;
    delete cancelButton; cancelButton = 0;
    delete rebootButton; rebootButton = 0;
    delete shutdownButton; shutdownButton = 0;
}

void CtrlAltDelete::paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
#ifdef CONFIG_GRADIENTS    
    YSurface surface(cadBg, logoutPixmap, logoutPixbuf);
#else
    YSurface surface(cadBg, logoutPixmap);
#endif
    g.setColor(surface.color);
    g.drawSurface(surface, 1, 1, width() - 2, height() - 2);
    g.draw3DRect(0, 0, width() - 1, height() - 1, true);
}

void CtrlAltDelete::actionPerformed(YAction *action, unsigned int /*modifiers*/) {
    deactivate();
    if (action == lockButton) {
        if (lockCommand && lockCommand[0])
            app->runCommand(lockCommand);
    } else if (action == logoutButton) {
        //app->exit(0);
        wmapp->actionPerformed(actionLogout, 0);
    } else if (action == cancelButton) {
        // !!! side-effect, not really nice
        wmapp->actionPerformed(actionCancelLogout, 0);
    } else if (action == restartButton) {
        wmapp->restartClient(0, 0);
    } else if (action == shutdownButton) {
        rebootOrShutdown = 2;
        wmapp->actionPerformed(actionLogout, 0);
    } else if (action == rebootButton) {
        rebootOrShutdown = 1;
        wmapp->actionPerformed(actionLogout, 0);
    }
}

bool CtrlAltDelete::handleKey(const XKeyEvent &key) {
    KeySym k = XKeycodeToKeysym(app->display(), key.keycode, 0);
    int m = KEY_MODMASK(key.state);
        
    if (key.type == KeyPress) {
        if (k == XK_Escape && m == 0) {
            deactivate();
            return true;
        }
    }
    return YWindow::handleKey(key);
}

void CtrlAltDelete::activate() {
    raise();
    show();
    if (!app->grabEvents(this, None,
                         ButtonPressMask |
                         ButtonReleaseMask |
                         ButtonMotionMask |
                         EnterWindowMask |
                         LeaveWindowMask, 1, 1, 1))
        hide();
    else {
        requestFocus();
        lockButton->requestFocus();
        lockButton->setWindowFocus();
    }
}

void CtrlAltDelete::deactivate() {
    app->releaseEvents();
    hide();
    manager->setFocus(manager->getFocus());
}
#endif
