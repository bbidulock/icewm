#ifndef __YPOINT_H
#define __YPOINT_H

class YPoint {
public:
    YPoint(): fX(0), fY(0) { }
    YPoint(int x, int y): fX(x), fY(y) { }
    ~YPoint() {}

    int x() const { return fX; }
    int y() const { return fY; }

#if 0
    void setX(int x) { fX = x; }
    void setY(int y) { fY = y; }

    void setPos(int x, int y) { fX = x; fY = y; }

    void setPoint(const YPoint &p) { fX = p.x(); fY = p.y(); }
#endif

    bool equals(const YPoint &p) {
        return ((fX == p.x()) &&
                (fY == p.y())) ? true : false;
    }

#if 0
    friend YPoint operator+(const YPoint &a, const YPoint &b);
    friend YPoint operator-(const YPoint &a, const YPoint &b);
    friend YPoint operator-(const YPoint &a);
#endif

private:
    int fX, fY;
};

#if 0
inline YPoint operator+(const YPoint &a, const YPoint &b) {
    return YPoint(a.fX + b.fX, a.fY + b.fX);
}

inline YPoint operator-(const YPoint &a, const YPoint &b) {
    return YPoint(a.fX - b.fX, a.fY - b.fX);
}

inline YPoint operator-(const YPoint &a) {
    return YPoint(-a.fX, -a.fY);
}
#endif

#endif

// vim: set sw=4 ts=4 et:
