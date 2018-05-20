/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "ykey.h"
#include "ybutton.h"
#include "yaction.h"
#include "ymenu.h"
#include "yrect.h"
#include "yicon.h"
#include "wpixmaps.h"
#include "yxapp.h" // !!! remove (AltMask)
#include "yprefs.h"
#include "prefs.h"
#include "ascii.h"
#include "wmtaskbar.h"

#include <string.h>

#include "intl.h"

YColorName YButton::normalButtonBg(&clrNormalButton);
YColorName YButton::normalButtonFg(&clrNormalButtonText);

YColorName YButton::activeButtonBg(&clrActiveButton);
YColorName YButton::activeButtonFg(&clrActiveButtonText);

ref<YFont> YButton::normalButtonFont;
ref<YFont> YButton::activeButtonFont;

YButton::YButton(YWindow *parent, YAction action, YMenu *popup) :
    YWindow(parent),
    fOver(false),
    fAction(action), fPopup(popup),
    fIcon(null),
    fIconSize(0),
    fImage(null),
    fText(null),
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
}

YButton::~YButton() {
    if (hotKey != -1) {
        removeAccelerator(hotKey, 0, this);
        if (xapp->AltMask != 0)
            removeAccelerator(hotKey, xapp->AltMask, this);
    }
    popdown();
    if (fPopup && fPopup->isShared() == false) {
        delete fPopup;
    }
}

void YButton::paint(Graphics &g, int const d, const YRect &r) {
    int x = r.x(), y = r.y(), w = r.width(), h = r.height();
    YSurface surface(getSurface());
    g.drawSurface(surface, x, y, w, h);

    if (fIcon != null)
        fIcon->draw(g,
                    x + (w - fIconSize) / 2,
                    y + (h - fIconSize) / 2,
                    fIconSize);
    else
    if (fImage != null)
        g.drawImage(fImage,
                    x + (w - fImage->width()) / 2,
                    y + (h - fImage->height()) / 2);
    else if (fText != null) {
        ref<YFont> font = fPressed ? activeButtonFont : normalButtonFont;

        int const w(font->textWidth(fText));
        int const p((width() - w) / 2);
        int yp((height() - font->height()) / 2
               + font->ascent() + d);

        g.setFont(font);
        g.setColor(getColor());
        if (!fEnabled) {
            g.setColor(YColor::white);
            g.drawChars(fText, d + p + 1, yp + 1);
            g.setColor(YColor::white->darker().darker());
            g.drawChars(fText, d + p, yp);
        } else {
            g.drawChars(fText, d + p, yp);
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
        short x1(dp), x2(dp), x3(dp+width()-ds-1), x4(dp);
        short y1(dp), y2(dp+1), y3(dp+1), y4(dp+height()-ds-1);
        unsigned short w1(width()-ds), w2(1), w3(1), w4(width()-ds);
        unsigned short h1(1), h2(height()-ds-2), h3(height()-ds-2), h4(1);
        XRectangle focus[] = {
            { x1, y1, w1, h1 },
            { x2, y2, w2, h2 },
            { x3, y3, w3, h3 },
            { x4, y4, w4, h4 }
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
        if (fPopup) {
            if (fArmed)
                popup(mouseDown);
            else
                popdown();
        }
    }
}

bool YButton::handleKey(const XKeyEvent &key) {
    KeySym k = keyCodeToKeySym(key.keycode);
    unsigned m = KEY_MODMASK(key.state);
    int uk = (k < 256) ? ASCII::toUpper((char)k) : k;

    if (fEnabled) {
        if (key.type == KeyPress) {
            if (!fSelected) {
                if (((k == XK_Return || k == 32) && m == 0) ||
                    (uk == hotKey && (m & ~xapp->AltMask) == 0))
                {
                    requestFocus(false);
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
                    XEvent xev = {};

                    if (XCheckTypedWindowEvent(xapp->display(), handle(),
                                               KeyPress, &xev) &&
                        xev.type == KeyPress &&
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
        requestFocus(false);
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
            requestFocus(false);
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

void YButton::updateSize() {
    int w = 72;
    int h = 18;
    if (fIcon != null) {
        w = h = fIconSize;
    } else if (fImage != null) {
        w = fImage->width();
        h = fImage->height();
    } else if (fText != null) {
        w = activeButtonFont->textWidth(fText);
        h = activeButtonFont->ascent();
    }
    setSize(w + 3 + 2 - ((wmLook == lookMetal || wmLook == lookFlat) ? 1 : 0),
            h + 3 + 2 - ((wmLook == lookMetal || wmLook == lookFlat) ? 1 : 0));
}

void YButton::setIcon(ref<YIcon> icon, int iconSize) {
    fIcon = icon;
    fIconSize = iconSize;
    fImage = null;

    updateSize();
}

void YButton::setImage(ref<YImage> image) {
    fImage = image;
    fIconSize = 0;
    fIcon = null;

    updateSize();
}

void YButton::setText(const ustring &str, int hotChar) {
    if (hotKey != -1) {
        removeAccelerator(hotKey, 0, this);
        if (xapp->AltMask != 0)
            removeAccelerator(hotKey, xapp->AltMask, this);
    }
    fText = str;
    if (fText != null) {
        fHotCharPos = hotChar;

        if (fHotCharPos == -2) {
            int i = fText.indexOf('_');
            if (i != -1) {
                fHotCharPos = i;
                fText = fText.remove(i, 1);
            }
        }

        hotKey = (fHotCharPos != -1) ? fText.charAt(fHotCharPos) : -1;
        hotKey = ASCII::toUpper(hotKey);

        if (hotKey != -1) {
            installAccelerator(hotKey, 0, this);
            if (xapp->AltMask != 0)
                installAccelerator(hotKey, xapp->AltMask, this);
        }
    } else
        hotKey = -1;
    updateSize();
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

void YButton::setAction(YAction action) {
    fAction = action;
}

void YButton::actionPerformed(YAction action, unsigned modifiers) {
    if (fListener && action != actionNull && fEnabled)
        fListener->actionPerformed(action, modifiers);
}

ref<YFont> YButton::getFont() {
    return (fPressed ? activeButtonFont : normalButtonFont);
}

YColor YButton::getColor() {
    return (fPressed ? activeButtonFg : normalButtonFg);
}

YSurface YButton::getSurface() {
    return (fPressed ? YSurface(activeButtonBg, buttonAPixmap, buttonAPixbuf)
                     : YSurface(normalButtonBg, buttonIPixmap, buttonIPixbuf));
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

// vim: set sw=4 ts=4 et:
