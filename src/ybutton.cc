/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "ykey.h"
#include "ybutton.h"
#include "yaction.h"
#include "ymenu.h"

#include "yapp.h" // !!! remove (AltMask)
#include "prefs.h"

#include "wmtaskbar.h"

#include <string.h>

#include "intl.h"

YColor *YButton::normalButtonBg = 0;
YColor *YButton::normalButtonFg = 0;

YColor *YButton::activeButtonBg = 0;
YColor *YButton::activeButtonFg = 0;

YFont *YButton::normalButtonFont = 0;
YFont *YButton::activeButtonFont = 0;

// !!! needs to go away
YPixmap *taskbuttonPixmap = 0;
YPixmap *taskbuttonactivePixmap = 0;
YPixmap *taskbuttonminimizedPixmap = 0;

YButton::YButton(YWindow *parent, YAction *action, YMenu *popup): YWindow(parent) {
    if (normalButtonFont == 0)
        normalButtonFont = YFont::getFont(normalButtonFontName);
    if (activeButtonFont == 0)
        activeButtonFont = YFont::getFont(activeButtonFontName);
    if (normalButtonBg == 0)
        normalButtonBg = new YColor(clrNormalButton);
    if (normalButtonFg == 0)
        normalButtonFg = new YColor(clrNormalButtonText);
    if (activeButtonBg == 0)
        activeButtonBg = new YColor(clrActiveButton);
    if (activeButtonFg == 0)
        activeButtonFg = new YColor(clrActiveButtonText);

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

#ifndef CONFIG_TASKBAR
void YButton::paint(Graphics &/*g*/, int /*x*/, int /*y*/, unsigned /*w*/, unsigned /*h*/) {
#else
void YButton::paint(Graphics &g, int /*x*/, int /*y*/, unsigned /*w*/, unsigned /*h*/) {
    int d((fPressed || fArmed) ? 1 : 0);
    int x(0), y(0), w(width()), h(height());
    
    g.setColor(fPressed ? activeButtonBg : normalButtonBg);

    if (wmLook == lookMetal) {
        g.drawBorderM(x, y, w - 1, h - 1, !d);
        d = 0;
        x += 2;
        y += 2;
        w -= 4;
        h -= 4;
    } else if (wmLook == lookGtk) {
        g.drawBorderG(x, y, w - 1, h - 1, !d);
        x += 1 + d;
        y += 1 + d;
        w -= 3;
        h -= 3;
    } else {
        g.drawBorderW(x, y, w - 1, h - 1, !d);
        x += 1 + d;
        y += 1 + d;
        w -= 3;
        h -= 3;
    }
    
    YPixmap *bgPix(fPressed ? buttonAPixmap : buttonIPixmap);
#ifdef CONFIG_GRADIENTS    
    YPixbuf *bgGrad(fPressed ? buttonAPixbuf : buttonIPixbuf);

    if (bgGrad)
	g.drawGradient(*bgGrad, x, y, w, h);
    else 
#endif    
    if (bgPix)
	g.fillPixmap(bgPix, x, y, w, h);
    else
	g.fillRect(x, y, w, h);

    if (fPixmap) // !!! fix drawing
        g.drawPixmap(fPixmap,
                     x + (w - fPixmap->width()) / 2,
                     y + (h - fPixmap->height()) / 2);
    else if (fText) {
        YFont *font(fPressed ? activeButtonFont : normalButtonFont);

	int const w(font->textWidth(fText));
	int const p((width() - w) / 2);
	int yp((height() - 1 - font->height()) / 2
                	     + font->ascent() + d);

        g.setFont(font);
	g.setColor(fPressed ? activeButtonFg : normalButtonFg);
        g.drawChars(fText, 0, strlen(fText), d + p, yp);
        if (fHotCharPos != -1)
            g.drawCharUnderline(d + p, yp, fText, fHotCharPos);
    }

    paintFocus(g, x, y, w, h);
#endif
}

#ifndef CONFIG_TASKBAR
void YButton::paintFocus(Graphics &/*g*/, int /*x*/, int /*y*/, unsigned /*w*/, unsigned /*h*/) {
#else
void YButton::paintFocus(Graphics &g, int /*x*/, int /*y*/, unsigned /*w*/, unsigned /*h*/) {
    int const d = (fPressed || fArmed) ? 1 : 0;
    int const dp(wmLook == lookMetal ? 2 : 2 + d);
    int const ds(wmLook == lookMetal ? 5 : 4);

    if (isFocused()) {
        g.setPenStyle(true);
        g.setColor(YColor::black);
	g.drawRect(dp, dp, width() - ds - 1, height() - ds - 1);
	g.setPenStyle(false);
    } else {
	XRectangle focus[] = {
            { dp, dp, width() - ds, 1 }, 
	    { dp, dp + 1, 1, height() - ds - 2 },
	    { dp + width() - ds - 1, dp + 1, 1, height() - ds - 2 },
	    { dp, dp + height() - ds - 1, width() - ds, 1 }
        };
    
        XSetClipRectangles(app->display(), g.handle(), 0, 0, focus, 4, YXSorted);

	YPixmap *bgPix(fPressed ? buttonAPixmap : buttonIPixmap);
#ifdef CONFIG_GRADIENTS    
	YPixbuf *bgGrad(fPressed ? buttonAPixbuf : buttonIPixbuf);

	if (bgGrad)
	    g.drawGradient(*bgGrad, dp, dp, width() - ds, height() - ds);
	else 
#endif    
	if (bgPix)
	    g.fillPixmap(bgPix, dp, dp, width() - ds, height() - ds);
	else {
	    g.setColor(fPressed ? activeButtonBg : normalButtonBg);
	    g.drawRect(dp, dp, width() - ds - 1, height() - ds - 1);
	}

	XSetClipMask(app->display(), g.handle(), None);
    }
#endif
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

bool YButton::handleKey(const XKeyEvent &key) {
    KeySym k = XKeycodeToKeysym(app->display(), key.keycode, 0);
    unsigned m = KEY_MODMASK(key.state);
    int uk = TOUPPER(k);

    if (key.type == KeyPress) {
        if (!fSelected) {
            if (((k == XK_Return || k == 32) && m == 0) ||
                (uk == hotKey && (m & ~app->AltMask) == 0))
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
            if (((k == XK_Return || k == 32) && m == 0) ||
                (uk == hotKey && (m & ~app->AltMask) == 0))
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
    return YWindow::handleKey(key);
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

#ifndef CONFIG_TASKBAR
void YButton::setText(const char *str, int /*hotChar*/) {
#else
void YButton::setText(const char *str, int hotChar) {
#endif
    if (hotKey != -1) {
        removeAccelerator(hotKey, 0, this);
        if (app->AltMask != 0)
            removeAccelerator(hotKey, app->AltMask, this);
    }
    fText = newstr(str);
#ifdef CONFIG_TASKBAR
    /// fix
    if (fText) {
        int w = activeButtonFont->textWidth(fText);
        int h = activeButtonFont->ascent();
        fHotCharPos = hotChar;

        if (fHotCharPos == -2) {
            char *hotChar = strchr(fText, '_');
            if (hotChar != NULL) {
                fHotCharPos = (hotChar - fText);
                memmove(hotChar, hotChar + 1, strlen(hotChar));
            } else
                fHotCharPos = -1;
        }

        hotKey = (fHotCharPos != -1) ? fText[fHotCharPos] : -1;
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

void YButton::actionPerformed(YAction *action, unsigned modifiers) {
    if (fListener && action)
        fListener->actionPerformed(action, modifiers);
}
