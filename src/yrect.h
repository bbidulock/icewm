#ifndef __YRECT_H
#define __YRECT_H

#ifndef INT_MAX
#include <limits.h>
#endif

class YRect {
public:
    YRect() : xx(0), yy(0), ww(0), hh(0) { }
    YRect(int x, int y, unsigned w, unsigned h)
        :xx(x), yy(y), ww(w), hh(h)
    {
        PRECONDITION(ww < INT_MAX);
        PRECONDITION(hh < INT_MAX);
    }
    YRect(const XRectangle& r) : xx(r.x), yy(r.y), ww(r.width), hh(r.height) { }

    int x() const { return xx; }
    int y() const { return yy; }
    unsigned width() const { return ww; }
    unsigned height() const { return hh; }
    unsigned pixels() const { return ww * hh; }

    void setRect(int x, int y, unsigned w, unsigned h) {
        xx = x;
        yy = y;
        ww = w;
        hh = h;
    }

    // does the same as gdk_rectangle_union
    void unionRect(int x, int y, unsigned width, unsigned height) {
        int mx = min(xx, x), w = int(max(xx + int(ww), x + int(width)));
        int my = min(yy, y), h = int(max(yy + int(hh), y + int(height)));
        setRect(mx, my, unsigned(w - mx), unsigned(h - my));
    }

    YRect intersect(const YRect& r) const {
        int x = max(xx, r.xx), w = int(min(xx + int(ww), r.xx + int(r.ww)));
        int y = max(yy, r.yy), h = int(min(yy + int(hh), r.yy + int(r.hh)));
        if (x < w && y < h)
            return YRect(x, y, unsigned(w - x), unsigned(h - y));
        return YRect();
    }

    unsigned overlap(const YRect& r) const {
        return intersect(r).pixels();
    }

    bool contains(const YRect& r) const {
        return overlap(r) == r.pixels();
    }

    bool operator==(YRect const& r) const {
        return xx == r.xx && yy == r.yy && ww == r.ww && hh == r.hh;
    }
    bool operator!=(YRect const& r) const {
        return !(*this == r);
    }

    int xx, yy;
    unsigned ww, hh;
};

class YRect2 : public YRect {
public:
    YRect2(const YRect& r1, const YRect& r2) :
        YRect(r1),
        old(r2)
    { }

    const YRect old;

    int deltaX() const { return x() - old.x(); }
    int deltaY() const { return y() - old.y(); }
    bool moved() const { return deltaX() | deltaY(); }
    int deltaWidth() const { return int(width()) - int(old.width()); }
    int deltaHeight() const { return int(height()) - int(old.height()); }
    bool resized() const { return deltaWidth() | deltaHeight(); }
    bool enlarged() const { return 0 < deltaWidth() || 0 < deltaHeight(); }
};

#endif

// vim: set sw=4 ts=4 et:
