/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 *
 * Dialogs
 */
#include "config.h"
#include "wmdialog.h"
#include "wpixmaps.h"
#include "prefs.h"
#include "wmapp.h"
#include "wmmgr.h"
#include "ypointer.h"
#include "intl.h"

#define HORZ 10
#define MIDH 10
#define VERT 10
#define MIDV 6

static YColorName cadBg(&clrDialog);

bool couldRunCommand(const char *cmd) {
    if (isEmpty(cmd))
        return false;
    // else-case. Defined, but check whether it's executable first
    csmart copy(newstr(cmd));
    char *save = nullptr;
    char *tokn = strtok_r(copy, " \t\n", &save);
    csmart path(path_lookup(tokn));
    return path;
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

CtrlAltDelete::CtrlAltDelete(IApp *app, YWindow *parent): YWindow(parent) {
    this->app = app;
    unsigned w = 140, h = 22;

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
    desktop->getScreenGeometry(&dx, &dy, &dw, &dh);
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

    setNetWindowType(_XA_NET_WM_WINDOW_TYPE_DIALOG);
    setClassHint("icecad", "IceWM");
    setTitle("IceCAD");
}

CtrlAltDelete::~CtrlAltDelete() {
    delete lockButton; lockButton = nullptr;
    delete suspendButton; suspendButton = nullptr;
    delete cancelButton; cancelButton = nullptr;
    delete logoutButton; logoutButton = nullptr;
    delete rebootButton; rebootButton = nullptr;
    delete shutdownButton; shutdownButton = nullptr;
    delete windowListButton; windowListButton = nullptr;
    delete restartButton; restartButton = nullptr;
    delete aboutButton; aboutButton = nullptr;
}

void CtrlAltDelete::configure(const YRect2& r) {
    if (r.resized()) {
        repaint();
    }
}

void CtrlAltDelete::repaint() {
    GraphicsBuffer(this).paint();
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
        if ((k == XK_End || k == XK_KP_End) && m == 0) {
            setFocus(firstWindow());
            return true;
        }
        if ((k == XK_Home || k == XK_KP_Home) && m == 0) {
            setFocus(lastWindow());
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

YActionButton* CtrlAltDelete::addButton(const mstring& str, unsigned& maxW, unsigned& maxH)
{
    YActionButton* b = new YActionButton(this, str, -2, this);
    maxW = max(maxW, b->width());
    maxH = max(maxH, b->height());
    return b;
}

YActionButton::YActionButton(YWindow* parent, const mstring& text, int hotkey,
                             YActionListener* listener):
    YButton(parent, YAction())
{
    addStyle(wsNoExpose);
    setText(text, hotkey);
    setActionListener(listener);
    show();
}

void YActionButton::repaint() {
    GraphicsBuffer(this).paint();
}

void YActionButton::configure(const YRect2& r) {
    if (r.resized() && r.width() > 1 && r.height() > 1) {
        repaint();
    }
}

YDimension YActionButton::getTextSize() {
    return YDimension(
            max(72, getActiveFont()->textWidth(getText()) + 12),
            max(18, getActiveFont()->height()));
}

// vim: set sw=4 ts=4 et:
