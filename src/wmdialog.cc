/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 *
 * Dialogs
 */
#include "config.h"

#include "ykey.h"
#include "wmdialog.h"
#include "wmaction.h"
#include "yactionbutton.h"

#include "prefs.h"
#include "wmapp.h"
#include "wmmgr.h"
#include "yrect.h"
#include "sysdep.h"

#include "intl.h"

bool canLock() {
    if (lockCommand == 0 || lockCommand[0] == 0)
        return false;
    // else-case. Defined, but check whether it's executable first
    char *copy = strdup(lockCommand);
    char *term = strchr(copy, ' ');
    if (term)
        *term = 0x0;
    term = strchr(copy, '\t');
    if (term)
        *term = 0x0;
    char *whereis = findPath(getenv("PATH"), X_OK, copy);
    if (whereis != 0) {
        free(copy);
        return true;
    }
    free(copy);
    return false;
}

bool canShutdown(bool reboot) {
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

#ifndef LITE

#define HORZ 10
#define MIDH 10
#define VERT 10
#define MIDV 6

static YColor *cadBg = 0;

CtrlAltDelete *ctrlAltDelete = 0;

CtrlAltDelete::CtrlAltDelete(YWindow *parent): YWindow(parent) {
    int w = 0, h = 0;
    YButton *b;

    if (cadBg == 0)
        cadBg = new YColor(clrDialog);

    setStyle(wsOverrideRedirect);
    setPointer(YXApplication::leftPointer);
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
    if (!canLock())
        lockButton->setEnabled(false);

    setSize(HORZ + w + MIDH + w + MIDH + w + HORZ,
            VERT + h + MIDV + h + VERT);

    int dx, dy, dw, dh;
    manager->getScreenGeometry(&dx, &dy, &dw, &dh);
    setPosition(dx + (dw - width()) / 2,
                dy + (dh - height()) / 2);

    lockButton->setGeometry(YRect(HORZ, VERT, w, h));
    logoutButton->setGeometry(YRect(HORZ + w + MIDH, VERT, w, h));
    cancelButton->setGeometry(YRect(HORZ + w + MIDH + w + MIDH, VERT, w, h));
    restartButton->setGeometry(YRect(HORZ, VERT + h + MIDV, w, h));
    rebootButton->setGeometry(YRect(HORZ + w + MIDH, VERT + h + MIDV, w, h));
    shutdownButton->setGeometry(YRect(HORZ + w + MIDH + w + MIDH, VERT + h + MIDV, w, h));
}

CtrlAltDelete::~CtrlAltDelete() {
    delete lockButton; lockButton = 0;
    delete logoutButton; logoutButton = 0;
    delete restartButton; restartButton = 0;
    delete cancelButton; cancelButton = 0;
    delete rebootButton; rebootButton = 0;
    delete shutdownButton; shutdownButton = 0;
}

void CtrlAltDelete::paint(Graphics &g, const YRect &/*r*/) {
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
        manager->doWMAction(ICEWM_ACTION_LOGOUT);
    } else if (action == cancelButton) {
        // !!! side-effect, not really nice
        manager->doWMAction(ICEWM_ACTION_CANCEL_LOGOUT);
    } else if (action == restartButton) {
        wmapp->restartClient(0, 0);
    } else if (action == shutdownButton) {
        manager->doWMAction(ICEWM_ACTION_SHUTDOWN);
    } else if (action == rebootButton) {
        manager->doWMAction(ICEWM_ACTION_REBOOT);
    }
}

bool CtrlAltDelete::handleKey(const XKeyEvent &key) {
    KeySym k = XKeycodeToKeysym(xapp->display(), key.keycode, 0);
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
    if (!xapp->grabEvents(this, None,
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
    xapp->releaseEvents();
    hide();
    XSync(xapp->display(), False);
    //manager->setFocus(manager->getFocus());
}
#endif
