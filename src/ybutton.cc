/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "ykey.h"
#include "ypixbuf.h"
#include "ybutton.h"
#include "yaction.h"
#include "ymenu.h"
#include "yrect.h"

#include "yxapp.h" // !!! remove (AltMask)
#include "yprefs.h"
#include "prefs.h"
#include "ascii.h"
#include "wmtaskbar.h"

#include <string.h>

#include "intl.h"

YColor *YButton::normalButtonBg = 0;
YColor *YButton::normalButtonFg = 0;

YColor *YButton::activeButtonBg = 0;
YColor *YButton::activeButtonFg = 0;

ref<YFont> YButton::normalButtonFont;
ref<YFont> YButton::activeButtonFont;

// !!! needs to go away
ref<YPixmap> taskbuttonPixmap;
ref<YPixmap> taskbuttonactivePixmap;
ref<YPixmap> taskbuttonminimizedPixmap;

YButton::YButton(YWindow *parent, YAction *action, YMenu *popup) :
    YWindow(parent),
    fOver(false),
    fAction(action), fPopup(popup),
    fImage(null), fText(NULL),
    fPressed(false),
    fEnabled(true),
    fHotCharPos(-1), hotKey(-1),
    fListener(NULL),
    fSelected(false), fArmed(false),
    wasPopupActive(false),
    fPopupActive(false)
{
    if (normalButtonFont == null)
        normalButtonFont = YFont::getFont(XFA(normalButtonFontName));
    if (activeButtonFont == null)
        activeButtonFont = YFont::getFont(XFA(activeButtonFontName));
    if (normalButtonBg == 0)
        normalButtonBg = new YColor(clrNormalButton);
    if (normalButtonFg == 0)
        normalButtonFg = new YColor(clrNormalButtonText);
    if (activeButtonBg == 0)
        activeButtonBg = new YColor(clrActiveButton);
    if (activeButtonFg == 0)
        activeButtonFg = new YColor(clrActiveButtonText);
}

YButton::~YButton() {
    if (hotKey != -1) {
        removeAccelerator(hotKey, 0, this);
        if (xapp->AltMask != 0)
            removeAccelerator(hotKey, xapp->AltMask, this);
    }
    popdown();
    delete[] fText; fText = 0;
}

void YButton::paint(Graphics &g, int const d, const YRect &r) {
    int x = r.x(), y = r.y(), w = r.width(), h = r.height();
    YSurface surface(getSurface());
    g.drawSurface(surface, x, y, w, h);

    if (fImage != null)
        g.drawImage(fImage, x + (w - fImage->width()) / 2,
                    y + (h - fImage->height()) / 2);
    else if (fText) {
        ref<YFont> font = fPressed ? activeButtonFont : normalButtonFont;

        int const w(font->textWidth(fText));
        int const p((width() - w) / 2);
        int yp((height() - font->height()) / 2
               + font->ascent() + d);

        g.setFont(font);
        g.setColor(getColor());
        if (!fEnabled) {
            g.setColor(YColor::white);
            g.drawChars(fText, 0, strlen(fText), d + p + 1, yp + 1);
            g.setColor(YColor::white->darker()->darker());
            g.drawChars(fText, 0, strlen(fText), d + p, yp);
        } else {
            g.drawChars(fText, 0, strlen(fText), d + p, yp);
        }
        if (fHotCharPos != -1)
            g.drawCharUnderline(d + p, yp, fText, fHotCharPos);
    }
}

void YButton::paint(Graphics &g, const YRect &/*r*/) {
    int d((fPressed || fArmed) ? 1 : 0);
    int x(0), y(0), w(width()), h(height());

    if (w > 1 && h > 1) {
        YSurface surface(getSurface());
        g.setColor(surface.color);

        if (wmLook == lookMetal) {
            g.drawBorderM(x, y, w - 1, h - 1, !d);
            d = 0; x += 2; y += 2; w -= 4; h -= 4;
        } else if (wmLook == lookGtk) {
            g.drawBorderG(x, y, w - 1, h - 1, !d);
            x += 1 + d; y += 1 + d; w -= 3; h -= 3;
	} else if (wmLook == lookFlat){
	    d = 0;
        } else {
            g.drawBorderW(x, y, w - 1, h - 1, !d);
            x += 1 + d; y += 1 + d; w -= 3; h -= 3;
        }

        paint(g, d, YRect(x, y, w, h));

        if (wmLook != lookFlat) {
            paintFocus(g, YRect(x, y, w, h));
        }
    }
}

void YButton::paintFocus(Graphics &g, const YRect &/*r*/) {
    int const d = (fPressed || fArmed ? 1 : 0);
    int const dp(wmLook == lookMetal ? 2 : 2 + d);
    int const ds(4);

    if (isFocused()) {
        g.setPenStyle(true);
        g.setFunction(GXxor);
        g.setColor(YColor::white);
        g.drawRect(dp, dp, width() - ds - 1, height() - ds - 1);
        g.setFunction(GXcopy);
        g.setPenStyle(false);
    } else {
        XRectangle focus[] = {
            { dp, dp, width() - ds, 1 },
            { dp, dp + 1, 1, height() - ds - 2 },
            { dp + width() - ds - 1, dp + 1, 1, height() - ds - 2 },
            { dp, dp + height() - ds - 1, width() - ds, 1 }
        };

        g.setClipRectangles(focus, 4);

        if (wmLook == lookMetal)
            paint(g, 0, YRect(dp, dp, width() - ds, height() - ds));
        else
            paint(g, d, YRect(dp - 1, dp - 1, width() - ds + 1, height() - ds + 1));
        g.setClipMask(None);
    }
}

void YButton::setOver(bool over) {
    if (fOver != over) {
       fOver= over;
       repaint();
    }
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
    KeySym k = XKeycodeToKeysym(xapp->display(), key.keycode, 0);
    unsigned m = KEY_MODMASK(key.state);
    int uk = ASCII::toUpper(k);

    if (fEnabled) {
        if (key.type == KeyPress) {
            if (!fSelected) {
                if (((k == XK_Return || k == 32) && m == 0) ||
                    (uk == hotKey && (m & ~xapp->AltMask) == 0))
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
                    (uk == hotKey && (m & ~xapp->AltMask) == 0))
                {
                    bool wasArmed = fArmed;

                    // !!! is this guaranteed to work? (skip autorepeated keys)
                    XEvent xev;

                    XCheckTypedWindowEvent(xapp->display(), handle(), KeyPress, &xev);
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
    if (fEnabled) {
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
    }
    YWindow::handleButton(button);
}

void YButton::handleCrossing(const XCrossingEvent &crossing) {
    if (fSelected && fEnabled) {
        if (crossing.type == EnterNotify) {
            if (!fPopup)
                setArmed(true, true);
        } else if (crossing.type == LeaveNotify) {
            if (!fPopup)
                setArmed(false, true);
        }
    }

    if (fEnabled) {
        if (crossing.type == EnterNotify) {
            setOver(true);
        } else if (crossing.type == LeaveNotify) {
            setOver(false);
        }
    }

    YWindow::handleCrossing(crossing);
}

void YButton::setImage(ref<YIconImage> image) {
    fImage = image;

    if (image != null)
        setSize(image->width() + 3 + 2 - ((wmLook == lookMetal || wmLook == lookFlat) ? 1 : 0),
                image->height() + 3 + 2 - ((wmLook == lookMetal || wmLook == lookFlat) ? 1 : 0));
}

void YButton::setText(const char *str, int hotChar) {
    if (hotKey != -1) {
        removeAccelerator(hotKey, 0, this);
        if (xapp->AltMask != 0)
            removeAccelerator(hotKey, xapp->AltMask, this);
    }
    if (fText != 0) { delete[] fText; fText = 0; }
    fText = newstr(str);
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
        hotKey = ASCII::toUpper(hotKey);

        if (hotKey != -1) {
            installAccelerator(hotKey, 0, this);
            if (xapp->AltMask != 0)
                installAccelerator(hotKey, xapp->AltMask, this);
        }

        if (fImage == null)
            setSize(3 + w + 4 + 2, 3 + h + 4 + 2);
    } else
        hotKey = -1;
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

        int xiscreen = desktop->getScreenForRect(x, y, width(), height());
        if (fPopup->popup(this, this, 0,
                          x - 2,
                          y + height(),
                          0,
                          height(),
                          xiscreen,
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
    if (fListener && action && fEnabled)
        fListener->actionPerformed(action, modifiers);
}

ref<YFont> YButton::getFont() {
    return (fPressed ? activeButtonFont : normalButtonFont);
}

YColor * YButton::getColor() {
    return (fPressed ? activeButtonFg : normalButtonFg);
}

YSurface YButton::getSurface() {
#ifdef CONFIG_GRADIENTS
    return (fPressed ? YSurface(activeButtonBg, buttonAPixmap, buttonAPixbuf)
                     : YSurface(normalButtonBg, buttonIPixmap, buttonIPixbuf));
#else
    return (fPressed ? YSurface(activeButtonBg, buttonAPixmap)
                     : YSurface(normalButtonBg, buttonIPixmap));
#endif
}

void YButton::setEnabled(bool enabled) {
    if (fEnabled != enabled) {
        fEnabled = enabled;
        if (!fEnabled) {
            popdown();
            fOver = false;
            fArmed = false;
        }
        repaint();
    }
}
