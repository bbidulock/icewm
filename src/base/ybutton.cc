/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#pragma implementation
#include "config.h"

#include "yxkeydef.h"
#include "ybutton.h"

#include "yaction.h"
#include "ymenu.h"
#include "ycstring.h"
#include "yrect.h"
#include "ybuttonborder.h"
#include "ykeyevent.h"
#include "ybuttonevent.h"
#include "ycrossingevent.h"
#include "yapp.h"
#include "deffonts.h"
#include "ypaint.h"
#include "base.h"

#include <string.h>

#define ISLOWER(c) ((c) >= 'a' && (c) <= 'z')
#define TOUPPER(c) (ISLOWER(c) ? (c) - 'a' + 'A' : (c))

YColorPrefProperty YButton::gNormalButtonBg("system", "ColorNormalButton", "rgb:C0/C0/C0");
YColorPrefProperty YButton::gNormalButtonFg("system", "ColorNormalButtonText", "rgb:00/00/00");
YColorPrefProperty YButton::gActiveButtonBg("system", "ColorActiveButton", "rgb:E0/E0/E0");
YColorPrefProperty YButton::gActiveButtonFg("system", "ColorActiveButtonText", "rgb:00/00/00");
YFontPrefProperty YButton::gNormalButtonFont("system", "NormalButtonFontName", FONT(120));
YFontPrefProperty YButton::gActiveButtonFont("system", "ActiveButtonFontName", BOLDFONT(120));
YPixmapPrefProperty YButton::gPixmapNormalButton("system", "PixmapNormalButton", 0, 0);
YPixmapPrefProperty YButton::gPixmapActiveButton("system", "PixmapActiveButton", 0, 0);
YNumPrefProperty YButton::gBorderStyle("system", "ButtonBorderStyle", YButtonBorder::bsRaised);

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
        removeAccelerator(hotKey, YEvent::mAlt, this);
    }
    popdown();
    delete fText; fText = 0;
}


void YButton::paint(Graphics &g, const YRect &/*er*/) {
    int d = (fPressed || fArmed) ? 1 : 0;
    YPixmap *bgPix = 0;

    if (fPressed) {
        g.setColor(gActiveButtonBg);
        bgPix = gPixmapActiveButton.getPixmap();
    } else {
        g.setColor(gNormalButtonBg);
        bgPix = gPixmapNormalButton.getPixmap();
    }

    int style = gBorderStyle.getNum(); //YButtonBorder::bsWinRaised;
    YRect border(0, 0, width(), height());
    YButtonBorder::drawBorder(style, g, border, d ? true : false);
    YRect inside;
    YButtonBorder::getInside(style, border, inside, d ? true : false);

    if (bgPix)
        g.fillPixmap(bgPix,
                     inside.x(),
                     inside.y(),
                     inside.width(),
                     inside.height());
    else
        g.fillRect(inside);

    if (fPixmap) {
        if (fPixmap->mask()) {
            g.drawPixmap(fPixmap,
                         inside.x() + (inside.width() - fPixmap->width()) / 2,
                         inside.x() + (inside.height() - fPixmap->height()) / 2);
        } else {
            g.drawPixmap(fPixmap,
                         inside.x() + (inside.width() - fPixmap->width()) / 2,
                         inside.y() + (inside.height() - fPixmap->height()) / 2);
        }
    } else {
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

            g.drawText(inside,
                       fText,
                       DrawText_HCenter + DrawText_VCenter,
                       fHotCharPos);
        }
    }
#warning "fix YButton paintFocus hack"
    repaintFocus();
}

void YButton::paintFocus(Graphics &g, const YRect &/*er*/) {
    int d = (fPressed || fArmed) ? 1 : 0;

    if (isFocused())
        g.setColor(YColor::black);
    else if (fPressed)
        g.setColor(gActiveButtonBg);
    else
        g.setColor(gNormalButtonBg);

    if (isFocused())
        g.setPenStyle(Graphics::psDotted);

    int style = gBorderStyle.getNum(); //YButtonBorder::bsWinRaised;
    YRect border(0, 0, width(), height());
    YRect inside;
    YButtonBorder::getInside(style, border, inside, d ? true : false);

    g.drawRect(inside.x(), inside.y(), inside.width() - 1, inside.height() - 1);

    g.setPenStyle();
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

bool YButton::eventKey(const YKeyEvent &key) {
    int ksym = key.getKey();
    int vmod = key.getKeyModifiers();
    int uk = TOUPPER(key.getKey());
    bool rightKey =
        ((ksym == XK_Return || ksym == 32) && vmod == 0) ||
        (uk == hotKey && (vmod & ~YKeyEvent::mAlt) == 0);

    if (key.type() == YEvent::etKeyPress) {
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
    } else if (key.type() == YEvent::etKeyRelease) {

        if (fSelected) {
            if (rightKey)
            {
                bool wasArmed = fArmed;

                if (key.isAutoRepeat())
                    return true;

                setArmed(false, false);
                setSelected(false);
                if (!fPopup && wasArmed)
                    actionPerformed(fAction, key.getModifiers());
                return true;
            }
        }
    }
    return YWindow::eventKey(key);
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

bool YButton::eventButton(const YButtonEvent &button) {
    if (button.type() == YEvent::etButtonPress && button.getButton() == 1) {
        requestFocus();
        wasPopupActive = fArmed;
        setSelected(true);
        setArmed(true, true);
    } else if (button.type() == YEvent::etButtonRelease) {
        if (fPopup) {
            int inWindow = (button.x() >= 0 &&
                            button.y() >= 0 &&
                            button.x() < int (width()) &&
                            button.y() < int (height()));

            if ((!inWindow || wasPopupActive) && fArmed) {
                setArmed(false, false);
                setSelected(false);
            }
        } else {
            bool wasArmed = fArmed;

            setArmed(false, false);
            setSelected(false);
            if (wasArmed) {
                actionPerformed(fAction, button.getModifiers());
                return true;
            }
        }
    }
    return YWindow::eventButton(button);
}

bool YButton::eventCrossing(const YCrossingEvent &crossing) {
    if (fSelected) {
        if (crossing.type() == YEvent::etPointerIn) {
            if (!fPopup)
                setArmed(true, true);
        } else if (crossing.type() == YEvent::etPointerOut) {
            if (!fPopup)
                setArmed(false, true);
        }
    }
    return YWindow::eventCrossing(crossing);
}

void YButton::setPixmap(YPixmap *pixmap) {
    fPixmap = pixmap;
    if (pixmap) {
        int style = gBorderStyle.getNum(); //YButtonBorder::bsWinRaised;
        YPoint ps(pixmap->width(), pixmap->height());
        YPoint bs;
        YButtonBorder::getSize(style, ps, bs);
        setSize(bs.x(), bs.y());
    }
}

void YButton::__setText(const char *str, int hotChar) {
    if (hotKey != -1) {
        removeAccelerator(hotKey, 0, this);
        removeAccelerator(hotKey, YEvent::mAlt, this);
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
            installAccelerator(hotKey, YEvent::mAlt, this);
        }

        int style = gBorderStyle.getNum(); //YButtonBorder::bsWinRaised;
        YPoint ps(w, h);
        YPoint bs;
        YButtonBorder::getSize(style, ps, bs);

        setSize(bs.x() + 5, bs.y() + 5);
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
        return;
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
