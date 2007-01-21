/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"

#ifndef LITE

#include "ylabel.h"
#include "ydialog.h"  // !!! remove

#include "base.h"
#include "prefs.h"

YColor *YLabel::labelFg = 0;
YColor *YLabel::labelBg = 0;
ref<YFont> YLabel::labelFont;

YLabel::YLabel(const ustring &label, YWindow *parent):
    YWindow(parent), fLabel(label)
{
    setBitGravity(NorthWestGravity);

    if (labelFont == null)
        labelFont = YFont::getFont(XFA(labelFontName));
    if (labelBg == 0)
        labelBg = new YColor(clrLabel);
    if (labelFg == 0)
        labelFg = new YColor(clrLabelText);

    autoSize();
}

YLabel::~YLabel() {
}

void YLabel::paint(Graphics &g, const YRect &/*r*/) {
#ifdef CONFIG_GRADIENTS
    ref<YImage> gradient(parent() ? parent()->getGradient() : null);

    if (gradient != null)
        g.drawImage(gradient, x() - 1, y() - 1, width(), height(), 0, 0);
    else 
#endif    
    if (dialogbackPixmap != null)
        g.fillPixmap(dialogbackPixmap, 0, 0, width(), height(), x() - 1, y() - 1);
    else {
        g.setColor(labelBg);
        g.fillRect(0, 0, width(), height());
    }

    if (fLabel != null) {
        int y = 1 + labelFont->ascent();
        int x = 1;
        int h = labelFont->height();
        ustring s(null), r(null);
        
        g.setColor(labelFg);
        g.setFont(labelFont);

        for (s = fLabel; s.splitall('\n', &s, &r); s = r) {
            g.drawChars(s, x, y);
            y += h;
        }
    }
}

void YLabel::setText(const ustring &label) {
    fLabel = label;
    autoSize();
}

void YLabel::autoSize() {
    int h = labelFont->height();
    int w = 0;
    if (fLabel != null) {
        int w1;
        ustring s(null), r(null);
        int n = 0;

        for (s = fLabel; s.splitall('\n', &s, &r); s = r) {
            w1 = labelFont->textWidth(s);
            if (w1 > w)
                w = w1;
            n++;
        }
        h *= n;
    }
    setSize(1 + w + 1, 1 + h + 1);
}
#endif
