/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 *
 * Dialogs
 */
#include "config.h"

#include "ypaths.h"
#include "wmdialog.h"
#include "wmaction.h"
#include "yactionbutton.h"
#include "wpixmaps.h"
#include "prefs.h"
#include "wmapp.h"
#include "wmmgr.h"
#include "yrect.h"
#include "ypointer.h"
#include "sysdep.h"

#include "intl.h"

bool couldRunCommand(const char *cmd) {
    if (isEmpty(cmd))
        return false;
    // else-case. Defined, but check whether it's executable first
    csmart copy(newstr(cmd));
    char *save = 0;
    char *tokn = strtok_r(copy, " \t\n", &save);
    return tokn && findPath(getenv("PATH"), X_OK, tokn) != null;
}

bool canLock()
{
    return couldRunCommand(lockCommand);
}

bool canShutdown(RebootShutdown reboot) {
    if (reboot == Shutdown && isEmpty(shutdownCommand))
            return false;
    if (reboot == Reboot && isEmpty(rebootCommand))
            return false;
    if (nonempty(logoutCommand))
        return false;
#ifdef CONFIG_SESSION
    if (smapp->haveSessionManager())
        return false;
#endif
    return true;
}

#define HORZ 10
#define MIDH 10
#define VERT 10
#define MIDV 6

static YColorName cadBg(&clrDialog);

CtrlAltDelete::CtrlAltDelete(IApp *app, YWindow *parent): YWindow(parent) {
    this->app = app;
    unsigned w = 0, h = 0;

    setStyle(wsOverrideRedirect);
    setPointer(YXApplication::leftPointer);
    setToplevel(true);

    /* Create the following buttons in an ordered sequence,
     * such that tabbing through them is in reading order
     * from left to right then top to bottom.
     */

    lockButton = addButton(_("Loc_k Workstation"), w, h);
    // TRANSLATORS: This means "energy saving mode" or "suspended system". Not "interrupt". Not "hibernate".
    suspendButton = addButton(_("_Sleep mode"), w, h);
    cancelButton = addButton(_("_Cancel"), w, h);
    logoutButton = addButton(_("_Logout..."), w, h);
    rebootButton = addButton(_("Re_boot"), w, h);
    shutdownButton = addButton(_("Shut_down"), w, h);
    windowListButton = addButton(_("_Window list"), w, h);
    restartButton = addButton(_("_Restart icewm"), w, h);
    aboutButton = addButton(_("_About"), w, h);

    if (!canShutdown(Reboot))
        rebootButton->setEnabled(false);
    if (!canShutdown(Shutdown))
        shutdownButton->setEnabled(false);
    if (!canLock())
        lockButton->setEnabled(false);
    if (!couldRunCommand(suspendCommand))
        suspendButton->setEnabled(false);

    setSize(HORZ + w + MIDH + w + MIDH + w + HORZ,
            VERT + h + MIDV + h + MIDV + h + VERT);

    int dx, dy;
    unsigned dw, dh;
    manager->getScreenGeometry(&dx, &dy, &dw, &dh);
    setPosition(dx + (dw - width()) / 2,
                dy + (dh - height()) / 2);

    lockButton->setGeometry(YRect(HORZ, VERT, w, h));
    suspendButton->setGeometry(YRect(HORZ + w + MIDH, VERT, w, h));
    cancelButton->setGeometry(YRect(HORZ + w + MIDH + w + MIDH, VERT, w, h));
    logoutButton->setGeometry(YRect(HORZ, VERT + h + MIDV, w, h));
    rebootButton->setGeometry(YRect(HORZ + w + MIDH, VERT + h + MIDV, w, h));
    shutdownButton->setGeometry(YRect(HORZ + w + MIDH + w + MIDH, VERT + h + MIDV, w, h));
    windowListButton->setGeometry(YRect(HORZ, VERT + 2 * (h + MIDV), w, h));
    restartButton->setGeometry(YRect(HORZ + w + MIDH, VERT + 2 * (h + MIDV), w, h));
    aboutButton->setGeometry(YRect(HORZ + w + MIDH + w + MIDH, VERT + 2 * (h + MIDV), w, h));
}

CtrlAltDelete::~CtrlAltDelete() {
    delete lockButton; lockButton = 0;
    delete suspendButton; suspendButton = 0;
    delete cancelButton; cancelButton = 0;
    delete logoutButton; logoutButton = 0;
    delete rebootButton; rebootButton = 0;
    delete shutdownButton; shutdownButton = 0;
    delete windowListButton; windowListButton = 0;
    delete restartButton; restartButton = 0;
    delete aboutButton; aboutButton = 0;
}

void CtrlAltDelete::paint(Graphics &g, const YRect &/*r*/) {
    YSurface surface(cadBg, logoutPixmap, logoutPixbuf);
    g.setColor(surface.color);
    g.drawSurface(surface, 1, 1, width() - 2, height() - 2);
    g.draw3DRect(0, 0, width() - 1, height() - 1, true);
}

void CtrlAltDelete::actionPerformed(YAction action, unsigned int /*modifiers*/) {
    deactivate();
    if (action == *lockButton) {
        if (lockCommand && lockCommand[0])
            app->runCommand(lockCommand);
    } else if (action == *logoutButton) {
        manager->doWMAction(ICEWM_ACTION_LOGOUT);
    } else if (action == *cancelButton) {
        // !!! side-effect, not really nice
        manager->doWMAction(ICEWM_ACTION_CANCEL_LOGOUT);
    } else if (action == *restartButton) {
        manager->doWMAction(ICEWM_ACTION_RESTARTWM);
    } else if (action == *shutdownButton) {
        manager->doWMAction(ICEWM_ACTION_SHUTDOWN);
    } else if (action == *rebootButton) {
        manager->doWMAction(ICEWM_ACTION_REBOOT);
    } else if (action == *suspendButton) {
        manager->doWMAction(ICEWM_ACTION_SUSPEND);
    } else if (action == *aboutButton) {
        manager->doWMAction(ICEWM_ACTION_ABOUT);
    } else if (action == *windowListButton) {
        manager->doWMAction(ICEWM_ACTION_WINDOWLIST);
    }
}

bool CtrlAltDelete::handleKey(const XKeyEvent &key) {
    KeySym k = keyCodeToKeySym(key.keycode);
    int m = KEY_MODMASK(key.state);

    if (key.type == KeyPress) {
        if (k == XK_Escape && m == 0) {
            deactivate();
            return true;
        }
        if ((k == XK_Left || k == XK_KP_Left) && m == 0) {
            prevFocus();
            return true;
        }
        if ((k == XK_Right || k == XK_KP_Right) && m == 0) {
            nextFocus();
            return true;
        }
        if ((k == XK_Down || k == XK_KP_Down) && m == 0) {
            nextFocus(); nextFocus(); nextFocus();
            return true;
        }
        if ((k == XK_Up || k == XK_KP_Up) && m == 0) {
            prevFocus(); prevFocus(); prevFocus();
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
        lockButton->requestFocus(true);
    }
}

void CtrlAltDelete::deactivate() {
    xapp->releaseEvents();
    hide();
    XSync(xapp->display(), False);
    //manager->setFocus(manager->getFocus());
}

YActionButton* CtrlAltDelete::addButton(const ustring& str, unsigned& maxW, unsigned& maxH)
{
        YActionButton* b = new YActionButton(this);
    b->setText(str, -2);
    if (b->width() > maxW) maxW = b->width();
    if (b->height() > maxH) maxH = b->height();
    b->setActionListener(this);
    b->show();
    return b;
}

// vim: set sw=4 ts=4 et:
