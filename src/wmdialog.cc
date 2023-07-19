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

bool couldRunCommand(const char* commandline) {
    if (nonempty(commandline)) {
        csmart cmd(newstr(commandline));
        bool init = true;
        for (tokens tok(cmd, " \t\n"); tok; ++tok) {
            if (init) {
                init = false;
                csmart path(path_lookup(tok));
                if (path)
                    return true;
            }
            else if (tok == "||") {
                init = true;
            }
        }
    }
    return false;
}

bool canLock() {
    return couldRunCommand(lockCommand);
}

bool canSuspend() {
    return couldRunCommand(suspendCommand);
}

bool canHibernate() {
    return couldRunCommand(hibernateCommand);
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

CtrlAltDelete::CtrlAltDelete(IApp* app, YWindow* parent)
    : YWindow(parent)
    , app(app)
{
    unsigned w = 140, h = 22;

    setStyle(wsOverrideRedirect);
    setPointer(YWMApp::leftPointer);
    setToplevel(true);
    addEventMask(VisibilityChangeMask);

    struct {
        const char* text;
        EAction act;
        WMAction wma;
    } data[Count] = {
        { _("Loc_k Workstation"), actionLock, ICEWM_ACTION_LOCK },
        { _("_Sleep mode"), actionSuspend, ICEWM_ACTION_SUSPEND },
        { _("_Cancel"), actionCancelLogout, ICEWM_ACTION_CANCEL_LOGOUT },
        { _("_Logout..."), actionLogout, ICEWM_ACTION_LOGOUT },
        { _("Re_boot"), actionReboot, ICEWM_ACTION_REBOOT },
        { _("Shut_down"), actionShutdown, ICEWM_ACTION_SHUTDOWN },
        { _("_Hibernate"), actionHibernate, ICEWM_ACTION_HIBERNATE },
        { _("_Window list"), actionWindowList, ICEWM_ACTION_WINDOWLIST },
        { _("_Restart icewm"), actionRestart, ICEWM_ACTION_RESTARTWM },
        { _("_About"), actionAbout, ICEWM_ACTION_ABOUT },
        { _("Reload win_options"), actionWinOptions, ICEWM_ACTION_WINOPTIONS },
        { _("Reload ke_ys"), actionReloadKeys, ICEWM_ACTION_RELOADKEYS },
        // { _("Clos_e"), actionClose, ICEWM_ACTION_NOP },
    };
    for (int i = 0; i < Count; ++i) {
        buttons[i] = new YActionButton(this, data[i].text, -2,
                                       this, data[i].act, data[i].wma);
        if (w < buttons[i]->width())
            w = buttons[i]->width();
        if (h < buttons[i]->height() + 2)
            h = buttons[i]->height() + 2;
    }

    if (!canLock())
        buttons[0]->setEnabled(false);
    if (!canSuspend())
        buttons[1]->setEnabled(false);
    if (!canShutdown(Reboot))
        buttons[4]->setEnabled(false);
    if (!canShutdown(Shutdown))
        buttons[5]->setEnabled(false);
    if (!canHibernate())
        buttons[6]->setEnabled(false);

    setSize(HORZ + w + MIDH + w + MIDH + w + HORZ,
            VERT + (h + MIDV) * ((Count + 2) / 3) - MIDV + VERT);

    for (int i = 0; i < Count; ++i) {
        int x = HORZ + (i % 3) * (w + MIDH);
        int y = VERT + (i / 3) * (h + MIDV);
        buttons[i]->setGeometry(YRect(x, y, w, h));
    }

    setNetWindowType(_XA_NET_WM_WINDOW_TYPE_DIALOG);
    setClassHint("icecad", "IceWM");
    setTitle("IceCAD");
}

CtrlAltDelete::~CtrlAltDelete() {
    for (int i = 0; i < Count; ++i)
        delete buttons[i];
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
    for (int i = 0; i < Count; ++i) {
        if (action == *buttons[i]) {
            if (inrange<int>(buttons[i]->wmAction, 2, 14))
                manager->doWMAction(buttons[i]->wmAction);
            else if (action == actionLock && canLock())
                app->runCommand(lockCommand);
            break;
        }
    }
}

int CtrlAltDelete::indexFocus() {
    YWindow* w = getFocusWindow();
    int i = Count;
    while (i-- > 0 && w != buttons[i]);
    return i;
}

bool CtrlAltDelete::handleKey(const XKeyEvent &key) {
    KeySym k = keyCodeToKeySym(key.keycode);
    int m = KEY_MODMASK(key.state);

    if (key.type == KeyPress) {
        if ((k == XK_Left || k == XK_KP_Left) && m == 0) {
            prevFocus();
            return true;
        }
        if ((k == XK_Right || k == XK_KP_Right) && m == 0) {
            nextFocus();
            return true;
        }
        if ((k == XK_Down || k == XK_KP_Down) && m == 0) {
            int i = indexFocus();
            if (i >= 0) {
                for (int k = 3; k < Count; k += 3) {
                    if (buttons[(i + k) % Count]->isFocusTraversable()) {
                        setFocus(buttons[(i + k) % Count]);
                        break;
                    }
                }
            }
            return true;
        }
        if ((k == XK_Up || k == XK_KP_Up) && m == 0) {
            int i = indexFocus();
            if (i >= 0) {
                for (int k = Count - 3; 0 < k; k -= 3) {
                    if (buttons[(i + k) % Count]->isFocusTraversable()) {
                        setFocus(buttons[(i + k) % Count]);
                        break;
                    }
                }
            }
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
        if ((k == XK_Prior || k == XK_KP_Prior) && m == 0) {
            int i = indexFocus();
            if (i >= 3) {
                setFocus(buttons[i % 3]);
            }
            return true;
        }
        if ((k == XK_Next || k == XK_KP_Next) && m == 0) {
            int i = indexFocus();
            if (0 <= i && i < Count - 3) {
                setFocus(buttons[Count - 3 + i % 3]);
            }
            return true;
        }
    }
    else if (key.type == KeyRelease) {
        if (k == XK_Escape && m == 0) {
            deactivate();
            return true;
        }
    }
    return YWindow::handleKey(key);
}

void CtrlAltDelete::activate() {
    YRect geo(desktop->getScreenGeometry());
    setPosition(geo.xx + (geo.ww - width()) / 2,
                geo.yy + (geo.hh - height()) / 2);
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
        manager->switchFocusFrom(manager->getFocus());
        manager->lockFocus();
        buttons[0]->requestFocus(true);
    }
}

void CtrlAltDelete::deactivate() {
    xapp->releaseEvents();
    hide();
    xapp->sync();
    if (manager->focusLocked())
        manager->unlockFocus();
    manager->focusLastWindow();
}

void CtrlAltDelete::handleVisibility(const XVisibilityEvent& vis) {
    if (vis.state > VisibilityUnobscured) {
        raise();
        xapp->sync();
    }
}

void CtrlAltDelete::handleButton(const XButtonEvent &button) {
    if (button.type == ButtonPress &&
        button.button == Button1 &&
        xapp->grabWindow() == this &&
        YRect(0, 0, width(), height()).contains(button.x, button.y) == false) {
        deactivate();
    }
}

YActionButton::YActionButton(YWindow* parent, const mstring& text, int hotkey,
                             YActionListener* listener, EAction action,
                             WMAction wmAction):
    YButton(parent, action ? action : YAction()),
    wmAction(wmAction)
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
    YFont font(getActiveFont());
    return YDimension(
            max(72, font ? font->textWidth(getText()) + 12 : 0),
            max(18, font ? font->height() : 0));
}

YSurface YActionButton::getSurface() {
    YSurface surf(normalButtonBg, buttonIPixmap, buttonIPixbuf);
    if (surf.gradient == null && surf.pixmap != null) {
        unsigned h = surf.pixmap->height();
        unsigned w = surf.pixmap->width();
        if (h >= 19 && h < height()) {
            h = height();
            if (w >= 100 && w < width())
                w = width();
            surf.pixmap = surf.pixmap->scale(w, h);
        }
    }
    return surf;
}

// vim: set sw=4 ts=4 et:
