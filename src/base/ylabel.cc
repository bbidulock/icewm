/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#include "config.h"

#include "ylabel.h"
#include "ymenu.h"  // !!! remove
#include "ycstring.h"

#include "base.h"
#include "prefs.h"

YColorPrefProperty YLabel::gLabelBg("system", "ColorLabel", "rgb:C0/C0/C0");
YColorPrefProperty YLabel::gLabelFg("system", "ColorLabelText", "rgb:00/00/00");
YFontPrefProperty YLabel::gLabelFont("system", "LabelFontName", FONT(140));

YLabel::YLabel(const char *label, YWindow *parent): YWindow(parent) {
    setBitGravity(NorthWestGravity);

    fLabel = CStr::newstr(label);
    autoSize();
}

YLabel::~YLabel() {
    delete fLabel; fLabel = 0;
}

void YLabel::paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
    g.setColor(gLabelBg.getColor());
    if (menubackPixmap) 
        g.fillPixmap(menubackPixmap, 0, 0, width(), height());
    else
        g.fillRect(0, 0, width(), height());
    if (fLabel) {
        int y = 1 + gLabelFont.getFont()->ascent();
        int x = 1;
        int h = gLabelFont.getFont()->height();
        const char *s = fLabel->c_str(), *n;
        
        g.setColor(gLabelFg.getColor());
        g.setFont(gLabelFont.getFont());

        while (*s) {
            n = s;
            while (*n && *n != '\n') n++;
            g.drawChars(s, 0, n - s, x, y);
            if (*n == '\n')
                n++;
            s = n;
            y += h;
        }
    }
}

void YLabel::setText(const char *label) {
    delete fLabel;
    fLabel = CStr::newstr(label);
    autoSize();
}

void YLabel::autoSize() {
    int h = gLabelFont.getFont()->height();
    int w = 0;
    if (fLabel) {
        int w1;
        const char *s = fLabel->c_str(), *n;
        int r = 0;

        while (*s) {
            n = s;
            while (*n && *n != '\n') n++;
            w1 = gLabelFont.getFont()->textWidth(s, n - s);
            if (*n == '\n')
                n++;
            s = n;

            if (w1 > w)
                w = w1;
            r++;
        }
        h *= r;
    }
    setSize(1 + w + 1, 1 + h + 1);
}
