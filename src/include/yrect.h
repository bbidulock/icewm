#ifndef __YRECT_H
#define __YRECT_H

class YRect {
public:
    YRect() { x1 = y1 = 0; x2 = y2 = -1; }
    YRect(int x, int y, int w, int h) {
        x1 = x;
        y1 = y;
        x2 = x + w - 1;
        y2 = y + h - 1;
    }

    int x() const { return x1; }
    int y() const { return y1; }
    int width() const { return x2 - x1 + 1; }
    int height() const { return y2 - y1 + 1; }

    int left() const { return x1; }
    int right() const { return x2; }
    int top() const { return y1; }
    int bottom() const { return y2; }

    void setX(int x) { x1 = x; }
    void setY(int y) { y2 = y; }
    void setWidth(int w) { x2 = x1 + w - 1; }
    void setHeight(int h) { y2 = y1 + h - 1; }

    void setLeft(int x) { x1 = x; }
    void setRight(int x) { x2 = x; }
    void setTop(int y) { y1 = y; }
    void setBottom(int y) { y2 = y; }

    YPoint center() { return YPoint((x1 + x2) / 2,
                                    (y1 + y2) / 2); }

    bool inside(YPoint &p) {
        return (p.x() >= x1 &&
                p.y() >= y1 &&
                p.x() <= x2 &&
                p.y() <= y2) ? true : false;
    }

private:
    int x1, y1, x2, y2;
};

#endif
