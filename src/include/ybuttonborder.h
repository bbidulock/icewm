#ifndef __YBORDER_H
#define __YBORDER_H

class Graphics;
class YRect;
class YPoint;

class YButtonBorder {
public:
    enum {
        bsNone = 0,
        bsLine = 1,
        bsRaised = 2,
        bsGtkRaised = 3,
        bsWinRaised = 4,
        bsEtched = 5
    } BorderStyle;

    static void drawBorder(int style, Graphics &g, const YRect &r, bool press);
    static void getInside(int style, const YRect &r, YRect &inside, bool press);
    static void getSize(int style, const YPoint &interior, YPoint &exterior);
};

#endif
