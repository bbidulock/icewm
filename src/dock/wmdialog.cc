/*
 * IceWM
 *
 * Copyright (C) 1997,1998,1999 Marko Macek
 *
 * Dialogs
 */
#include "config.h"

#include "ykey.h"
#include "wmdialog.h"
//#include "wmaction.h"
#include "yactionbutton.h"

#include "prefs.h"
//#include "wmapp.h"
//#include "wmmgr.h"
#include "yapp.h"

#define NO_KEYBIND
//#include "bindkey.h"
#include "prefs.h"
#define CFGDEF
//#include "bindkey.h"
#include "default.h"


#define HORZ 10
#define MIDH 10
#define VERT 10
#define MIDV 6

static YColorPrefProperty CtrlAltDelete::gBackgroundColor("sysdlg", "ColorBackground", "rgb:C0/C0/C0");

bool CtrlAltDelete::canShutdown(bool reboot) {
    const char *shutdownCommand = fShutdownCommand.getStr(0);
    const char *rebootCommand = fRebootCommand.getStr(0);
    const char *logoutCommand = fLogoutCommand.getStr(0);
    if (!reboot)
        if (shutdownCommand == 0 || shutdownCommand[0] == 0)
            return false;
    if (reboot)
        if (rebootCommand == 0 || rebootCommand[0] == 0)
            return false;
    if (logoutCommand && logoutCommand[0])
        return false;
#ifdef SM
    if (app->haveSessionManager())
        return false;
#endif
    return true;
}

CtrlAltDelete::CtrlAltDelete(YWindow *parent):
    YWindow(parent),
    fShutdownCommand("sysdlg", "ShutdownCommand"),
    fRebootCommand("sysdlg", "RebootCommand"),
    fLogoutCommand("sysdlg", "LogoutCommand"),
    fLockCommand("sysdlg", "LockCommand")
{
    unsigned int w = 0, h = 0;
    YButton *b;

    setStyle(wsOverrideRedirect);
    //setToplevel(true);
 
    b = lockButton = new YActionButton(this);
    b->setText("Lock Workstation", 5);
    if (b->width() > w) w = b->width();
    if (b->height() > h) h = b->height();
    b->setActionListener(this);
    b->show();

    b = logoutButton = new YActionButton(this);
    b->setText("Logout...", 0);
    if (b->width() > w) w = b->width();
    if (b->height() > h) h = b->height();
    b->setActionListener(this);
    b->show();

    b = cancelButton = new YActionButton(this);
    b->setText("Cancel", 0);
    if (b->width() > w) w = b->width();
    if (b->height() > h) h = b->height();
    b->setActionListener(this);
    b->show();

    b = restartButton = new YActionButton(this);
    b->setText("Restart icewm", 0);
    if (b->width() > w) w = b->width();
    if (b->height() > h) h = b->height();
    b->setActionListener(this);
    b->show();

    b = rebootButton = new YActionButton(this);
    b->setText("Reboot", 2);
    if (b->width() > w) w = b->width();
    if (b->height() > h) h = b->height();
    b->setActionListener(this);
    b->show();

    b = shutdownButton = new YActionButton(this);
    b->setText("Shutdown", 4);
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
    g.setColor(gBackgroundColor);
    g.draw3DRect(0, 0, width() - 1, height() - 1, true);
    if (logoutPixmap)
        g.fillPixmap(logoutPixmap, 1, 1, width() - 2, height() - 2);
    else
        g.fillRect(1, 1, width() - 2, height() - 2);
}

void CtrlAltDelete::actionPerformed(YAction *action, unsigned int /*modifiers*/) {
    deactivate();
    if (action == lockButton) {
        const char *lockCommand = fLockCommand.getStr("xlock");
        if (lockCommand && lockCommand[0])
            app->runCommand(lockCommand);
#if 0
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
#endif
    }
}

bool CtrlAltDelete::handleKeySym(const XKeyEvent &key, KeySym ksym, int vmod) {
    //KeySym k = XKeycodeToKeysym(app->display(), key.keycode, 0);
    //int m = KEY_MODMASK(key.state);
        
    if (key.type == KeyPress) {
        if (ksym == XK_Escape && vmod == 0) {
            deactivate();
            return true;
        }
    }
    return YWindow::handleKeySym(key, ksym, vmod);
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
    //???!!!manager->setFocus(manager->getFocus());
}

// ---


int main(int argc, char **argv) {
    YApplication app("wmdialog", &argc, &argv);

    CtrlAltDelete *ctl = new CtrlAltDelete(0);

    ctl->show();

    return app.mainLoop();
}

