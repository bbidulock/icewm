#include "config.h"
#include "ybuttonborder.h"
#include "yrect.h"
#include "ypaint.h"

void YButtonBorder::drawBorder(int style, Graphics &g, const YRect &r, bool press) {
    switch (style) {
    case bsNone:
        break;
    case bsLine:
        {
            YColor *c = g.getColor();

            if (press)
                g.setColor(c->brighter());
            else
                g.setColor(c->darker());

            g.drawRect(r.x(), r.y(), r.width() - 1, r.height() - 1);
            if (press)
                g.setColor(c);
            else
                g.setColor(c);
        }
        break;
    case bsRaised:
        g.draw3DRect(r.x(), r.y(), r.width() - 1, r.height() - 1, press ? false : true);
        break;
    case bsGtkRaised:
        g.drawBorderG(r.x(), r.y(), r.width() - 1, r.height() - 1, press ? false : true);
        break;
    case bsWinRaised:
        g.drawBorderW(r.x(), r.y(), r.width() - 1, r.height() - 1, press ? false : true);
        break;
    case bsEtched:
        g.drawBorderM(r.x(), r.y(), r.width() - 1, r.height() - 1, press ? false : true);
        break;
    }
}

void YButtonBorder::getInside(int style, const YRect &r, YRect &inside, bool press) {
    switch (style) {
    case bsNone:
        inside.setRect(r);
        break;
    case bsLine:
    case bsRaised:
        inside.setLeft(r.left() + 1);
        inside.setRight(r.right() - 1);
        inside.setTop(r.top() + 1);
        inside.setBottom(r.bottom() - 1);
        break;
    case bsGtkRaised:
    case bsWinRaised:
        if (press) {
            inside.setLeft(r.left() + 2);
            inside.setRight(r.right() - 1);
            inside.setTop(r.top() + 2);
            inside.setBottom(r.bottom() - 1);
        } else {
            inside.setLeft(r.left() + 1);
            inside.setRight(r.right() - 2);
            inside.setTop(r.top() + 1);
            inside.setBottom(r.bottom() - 2);
        }
        break;
    case bsEtched:
        inside.setLeft(r.left() + 2);
        inside.setRight(r.right() - 2);
        inside.setTop(r.top() + 2);
        inside.setBottom(r.bottom() - 2);
        break;
    }
}

void YButtonBorder::getSize(int style, const YPoint &interior, YPoint &exterior) {
    switch (style) {
    case bsNone:
        exterior.setPoint(interior);
        break;
    case bsLine:
    case bsRaised:
        exterior.setPoint(interior + YPoint(2, 2));
        break;
    case bsGtkRaised:
    case bsWinRaised:
        exterior.setPoint(interior + YPoint(3, 3));
        break;
    case bsEtched:
        exterior.setPoint(interior + YPoint(4, 4));
        break;
    }
}
