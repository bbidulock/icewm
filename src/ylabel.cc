/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#include "config.h"

#ifndef LITE

#include "ylabel.h"
#include "ymenu.h"  // !!! remove

#include "base.h"
#include "prefs.h"

YColor *YLabel::labelFg = 0;
YColor *YLabel::labelBg = 0;
YFont *YLabel::labelFont = 0;

YLabel::YLabel(const char *label, YWindow *parent): YWindow(parent) {
    setBitGravity(NorthWestGravity);

    if (labelFont == 0)
        labelFont = YFont::getFont(labelFontName);
    if (labelBg == 0)
        labelBg = new YColor(clrLabel);
    if (labelFg == 0)
        labelFg = new YColor(clrLabelText);

    fLabel = newstr(label);
    autoSize();
}

YLabel::~YLabel() {
    delete fLabel; fLabel = 0;
}

void YLabel::paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
    g.setColor(labelBg);
    if (menubackPixmap) 
        g.fillPixmap(menubackPixmap, 0, 0, width(), height());
    else
        g.fillRect(0, 0, width(), height());
    if (fLabel) {
        int y = 1 + labelFont->ascent();
        int x = 1;
        int h = labelFont->height();
        char *s = fLabel, *n;
        
        g.setColor(labelFg);
        g.setFont(labelFont);

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
    fLabel = newstr(label);
    autoSize();
}

void YLabel::autoSize() {
    int h = labelFont->height();
    int w = 0;
    if (fLabel) {
        int w1;
        char *s = fLabel, *n;
        int r = 0;

        while (*s) {
            n = s;
            while (*n && *n != '\n') n++;
            w1 = labelFont->textWidth(s, n - s);
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
#endif
