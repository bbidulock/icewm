/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#include "config.h"
#include "ykey.h"
#include "ybutton.h"
#include "yaction.h"
#include "ymenu.h"
#include "ycstring.h"
#include "yrect.h"

#include "yapp.h" // !!! remove (AltMask)
#include "prefs.h"

#include <string.h>

YColorPrefProperty YButton::gNormalButtonBg("system", "ColorNormalButton", "rgb:C0/C0/C0");
YColorPrefProperty YButton::gNormalButtonFg("system", "ColorNormalButtonText", "rgb:00/00/00");
YColorPrefProperty YButton::gActiveButtonBg("system", "ColorActiveButton", "rgb:E0/E0/E0");
YColorPrefProperty YButton::gActiveButtonFg("system", "ColorActiveButtonText", "rgb:00/00/00");
YFontPrefProperty YButton::gNormalButtonFont("system", "NormalButtonFontName", FONT(120));
YFontPrefProperty YButton::gActiveButtonFont("system", "ActiveButtonFontName", BOLDFONT(120));

YButton::YButton(YWindow *parent, YAction *action, YMenu *popup): YWindow(parent) {
    fSelected = false;
    fArmed = false;
    fPopupActive = false;
    wasPopupActive = false;
    fPixmap = 0;
    fPressed = 0;
    fListener = 0;
    fHotCharPos = -1;
    fText = 0;
    hotKey = -1;

    fAction = action;
    fPopup = popup;
}

YButton::~YButton() {
    if (hotKey != -1) {
        removeAccelerator(hotKey, 0, this);
        if (app->AltMask != 0)
            removeAccelerator(hotKey, app->AltMask, this);
    }
    popdown();
    delete fText; fText = 0;
}

void YButton::paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*w*/, unsigned int /*h*/) {
    int d = (fPressed || fArmed) ? 1 : 0;
    int x, y, w, h;
    YPixmap *bgPix = 0;

    if (fPressed) {
        g.setColor(gActiveButtonBg);
#if 0
        ////bgPix = taskbuttonactivePixmap;
#endif
    } else {
        g.setColor(gNormalButtonBg);
#if 0
        ////bgPix = taskbuttonPixmap;
#endif
    }

    x = 0;
    y = 0;
    w = width();
    h = height();

    if (wmLook == lookMetal) {
        g.drawBorderM(x, y, w - 1, h - 1, d ? false : true);
        d = 0;
        x += 2;
        y += 2;
        w -= 4;
        h -= 4;
    } else if (wmLook == lookGtk) {
        g.drawBorderG(x, y, w - 1, h - 1, d ? false : true);
        x += 1 + d;
        y += 1 + d;
        w -= 3;
        h -= 3;
    } else {
        g.drawBorderW(x, y, w - 1, h - 1, d ? false : true);
        x += 1 + d;
        y += 1 + d;
        w -= 3;
        h -= 3;
    }

    if (fPixmap) { // !!! fix drawing
        if (fPixmap->mask()) {
            if (bgPix)
                g.fillPixmap(bgPix, x, y, w, h);
            else
                g.fillRect(x, y, w, h);
            g.drawPixmap(fPixmap,
                         x + (w - fPixmap->width()) / 2,
                         y + (h - fPixmap->height()) / 2);
        } else {
            if (bgPix)
                g.fillPixmap(bgPix, x, y, w, h);
            else
                g.fillRect(x, y, w, h);
            g.drawPixmap(fPixmap,
                         x + (w - fPixmap->width()) / 2,
                         y + (h - fPixmap->height()) / 2);
        }
    } else {
        if (bgPix)
            g.fillPixmap(bgPix, x, y, w, h);
        else
            g.fillRect(x, y, w, h);

        YFont *font;

        if (fPressed)
            font = gActiveButtonFont.getFont();
        else
            font = gNormalButtonFont.getFont();

        if (fText) {
            if (fPressed)
                g.setColor(gActiveButtonFg);
            else
                g.setColor(gNormalButtonFg);
            g.setFont(font);

            g.drawText(YRect(x, y, w, h),
                       fText,
                       DrawText_HCenter + DrawText_VCenter,
                       fHotCharPos);
        }
    }
    paintFocus(g, x, y, w, h);
}

void YButton::paintFocus(Graphics &g, int /*x*/, int /*y*/, unsigned int /*w*/, unsigned int /*h*/) {
    int d = (fPressed || fArmed) ? 1 : 0;

    if (isFocused())
        g.setColor(YColor::black);
    else if (fPressed)
        g.setColor(gActiveButtonBg);
    else
        g.setColor(gNormalButtonBg);

    if (isFocused())
        g.setPenStyle(true);
    if (wmLook == lookMetal)
        g.drawRect(2, 2, width() - 5, height() - 5);
    else
        g.drawRect(1 + d, 1 + d, width() - 4, height() - 4);
    g.setPenStyle(false);
}

void YButton::setPressed(int pressed) {
    if (fPressed != pressed) {
        fPressed = pressed;
        repaint();
    }
}

void YButton::setSelected(bool selected) {
    if (selected != fSelected) {
        fSelected = selected;
        //repaint();
    }
}

void YButton::setArmed(bool armed, bool mouseDown) {
    if (armed != fArmed) {
        fArmed = armed;
        repaint();
        if (fPopup)
            if (fArmed)
                popup(mouseDown);
            else
                popdown();
    }
}

bool YButton::handleKeySym(const XKeyEvent &key, KeySym ksym, int vmod) {
    int uk = TOUPPER(ksym);
    bool rightKey =
        ((ksym == XK_Return || ksym == 32) && vmod == 0) ||
        (uk == hotKey && (vmod & ~kfAlt) == 0);

    if (key.type == KeyPress) {
        if (!fSelected) {
            if (rightKey)
            {
                requestFocus();
                wasPopupActive = fArmed;
                setSelected(true);
                setArmed(true, false);
                return true;
            }

        }
    } else if (key.type == KeyRelease) {

        if (fSelected) {
            if (rightKey)
            {
                bool wasArmed = fArmed;

                // !!! is this guaranteed to work? (skip autorepeated keys)
                XEvent xev;

                XCheckTypedWindowEvent(app->display(), handle(), KeyPress, &xev);
                if (xev.type == KeyPress &&
                    xev.xkey.time == key.time &&
                    xev.xkey.keycode == key.keycode &&
                    xev.xkey.state == key.state)
                    return true;


                setArmed(false, false);
                setSelected(false);
                if (!fPopup && wasArmed)
                    actionPerformed(fAction, key.state);
                return true;
            }
        }
    }
    return YWindow::handleKeySym(key, ksym, vmod);
}

void YButton::popupMenu() {
    if (fPopup) {
        requestFocus();
        wasPopupActive = fArmed;
        setSelected(true);
        setArmed(true, false);
    }
}

void YButton::updatePopup() {
}

void YButton::handleButton(const XButtonEvent &button) {
    if (button.type == ButtonPress && button.button == 1) {
        requestFocus();
        wasPopupActive = fArmed;
        setSelected(true);
        setArmed(true, true);
    } else if (button.type == ButtonRelease) {
        if (fPopup) {
            int inWindow = (button.x >= 0 &&
                            button.y >= 0 &&
                            button.x < int (width()) &&
                            button.y < int (height()));

            if ((!inWindow || wasPopupActive) && fArmed) {
                setArmed(false, false);
                setSelected(false);
            }
        } else {
            bool wasArmed = fArmed;

            setArmed(false, false);
            setSelected(false);
            if (wasArmed) {
                actionPerformed(fAction, button.state);
                return ;
            }
        }
    }
    YWindow::handleButton(button);
}

void YButton::handleCrossing(const XCrossingEvent &crossing) {
    if (fSelected) {
        if (crossing.type == EnterNotify) {
            if (!fPopup)
                setArmed(true, true);
        } else if (crossing.type == LeaveNotify) {
            if (!fPopup)
                setArmed(false, true);
        }
    }
    YWindow::handleCrossing(crossing);
}

void YButton::setPixmap(YPixmap *pixmap) {
    fPixmap = pixmap;
    if (pixmap) {
        setSize(pixmap->width() + 3 + 2 - ((wmLook == lookMetal) ? 1 : 0),
                pixmap->height() + 3 + 2 - ((wmLook == lookMetal) ? 1 : 0));
    }
}

void YButton::setText(const char *str, int hotChar) {
    if (hotKey != -1) {
        removeAccelerator(hotKey, 0, this);
        if (app->AltMask != 0)
            removeAccelerator(hotKey, app->AltMask, this);
    }
    fText = CStr::newstr(str);
#if 1 //CONFIG_TASKBAR
    /// fix
    if (fText && fText->c_str()) {
        int w = gActiveButtonFont.getFont()->textWidth(fText);
        int h = gActiveButtonFont.getFont()->ascent();
        fHotCharPos = hotChar;

        hotKey = (fHotCharPos != -1) ? fText->c_str()[fHotCharPos] : -1;
        hotKey = TOUPPER(hotKey);

        if (hotKey != -1) {
            installAccelerator(hotKey, 0, this);
            if (app->AltMask != 0)
                installAccelerator(hotKey, app->AltMask, this);
        }

        setSize(3 + w + 4 + 2, 3 + h + 4 + 2);
    } else
        hotKey = -1;
#endif
}

void YButton::setPopup(YMenu *popup) {
    if (fPopup) {
        popdown();
        delete fPopup;
    }
    fPopup = popup;
}

void YButton::donePopup(YPopupWindow *popup) {
    if (popup != fPopup) {
        MSG(("popup different?"));
        return ;
    }
    popdown();
    fArmed = 0;
    fSelected = 0;
    repaint();
}

void YButton::popup(bool mouseDown) {
    if (fPopup) {
        int x = 0;
        int y = 0;
        mapToGlobal(x, y);
        updatePopup();
        fPopup->setActionListener(getActionListener());
        if (fPopup->popup(this, 0,
                          x - 2,
                          y + height(),
                          0,
                          height(),
                          YPopupWindow::pfCanFlipVertical |
                          (mouseDown ? YPopupWindow::pfButtonDown : 0)))
            fPopupActive = true;
    }
}

void YButton::popdown() {
    if (fPopup && fPopupActive) {
        fPopup->popdown();
        fPopupActive = false;
    }
}

bool YButton::isFocusTraversable() {
    return true;
}

void YButton::setAction(YAction *action) {
    fAction = action;
}

void YButton::actionPerformed(YAction *action, unsigned int modifiers) {
    if (fListener && action)
        fListener->actionPerformed(action, modifiers);
}
