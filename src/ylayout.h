#ifndef YLAYOUT_H
#define YLAYOUT_H

/*
 * Layout widgets.
 * Use in dialogs.
 */

#include "yarray.h"

class YWindow;

/*
 * Layout base interface.
 */
class Sizeable {
public:
    virtual ~Sizeable() { }
    virtual unsigned width() = 0;
    virtual unsigned height() = 0;
    virtual int x() = 0;
    virtual int y() = 0;
    virtual void setPosition(int x, int y) = 0;
    virtual void realize() = 0;

    void layout(YWindow* outside);
};

/*
 * Create internal space in a dialog.
 */
class Spacer : public Sizeable {
public:
    Spacer(unsigned width, unsigned height) :
        fW(width), fH(height), fX(0), fY(0) { }
    unsigned width() override { return fW; }
    unsigned height() override { return fH; }
    int x() override { return fX; }
    int y() override { return fY; }
    void setPosition(int x, int y) override {
        fX = x;
        fY = y;
    }
    void realize() override { }
protected:
    unsigned fW, fH;
    int fX, fY;
};

/*
 * Hold a YWindow.
 */
class Cell : public Sizeable {
public:
    Cell(YWindow* win) : fWin(win) { }
    ~Cell() { delete fWin; }
    unsigned width() override { return fWin->width(); }
    unsigned height() override { return fWin->height(); }
    int x() override { return fWin->x(); }
    int y() override { return fWin->y(); }
    void setPosition(int x, int y) override {
        fWin->setPosition(x, y);
    }
    void realize() override { fWin->show(); }
protected:
    YWindow* fWin;
};

/*
 * Create space around others.
 */
class Padder : public Sizeable {
public:
    Padder(Sizeable* win, int hpad, int vpad) :
        fWin(win), fHori(hpad), fVert(vpad) { }
    ~Padder() { delete fWin; }
    unsigned width() override { return fWin->width() + 2 * fHori; }
    unsigned height() override { return fWin->height() + 2 * fVert; }
    int x() override { return fWin->x() - fHori; }
    int y() override { return fWin->y() - fVert; }
    void setPosition(int x, int y) override {
        fWin->setPosition(x + fHori, y + fVert);
    }
    void realize() override { fWin->realize(); }
protected:
    Sizeable* fWin;
    int fHori, fVert;
};

/*
 * Group sizeables vertically.
 */
class Ladder : public Sizeable {
public:
    Ladder& operator+=(Sizeable* step) {
        fSteps += step;
        return *this;
    }
    Ladder& operator+=(YWindow* win) {
        return operator+=(new Cell(win));
    }
    bool empty() const {
        return fSteps.isEmpty();
    }
    unsigned width() override {
        unsigned w = 1;
        for (Sizeable* step : fSteps)
            w = max(w, step->width());
        return w;
    }
    unsigned height() override {
        unsigned h = 0;
        for (Sizeable* step : fSteps)
            h += step->height();
        return h;
    }
    int x() override { return empty() ? 0 : fSteps[0]->x(); }
    int y() override { return empty() ? 0 : fSteps[0]->y(); }
    void setPosition(int x, int y) override {
        for (Sizeable* step : fSteps) {
            step->setPosition(x, y);
            y += step->height();
        }
    }
    void realize() override {
        for (Sizeable* step : fSteps)
            step->realize();
    }

protected:
    YObjectArray<Sizeable> fSteps;
};

/*
 * Group sizeables horizontally.
 */
class Row : public Sizeable {
public:
    Row(Sizeable* left, Sizeable* right = nullptr) :
        fLeft(left), fRight(right),
        fOffset(fRight ? 20 + left->width() : 0) { }
    Row(YWindow* left, YWindow* right = nullptr) :
        Row(new Cell(left), right ? new Cell(right) : nullptr) { }
    ~Row() {
        delete fLeft;
        delete fRight;
    }
    unsigned offset() {
        return fRight ? max(fOffset, fLeft->width()) : 0;
    }
    void setOffset(unsigned newOffset) {
        if (fOffset != newOffset) {
            fOffset = newOffset;
            if (fRight)
                fRight->setPosition(x() + offset(), y());
        }
    }
    unsigned width() override {
        return fRight ? offset() + fRight->width() : fLeft->width();
    }
    unsigned height() override {
        return max(fLeft->height(), fRight ? fRight->height() : 0);
    }
    int x() override { return fLeft->x(); }
    int y() override { return fLeft->y(); }
    void setPosition(int x, int y) override {
        fLeft->setPosition(x, y);
        if (fRight)
            fRight->setPosition(x + offset(), y);
    }
    void realize() override {
        fLeft->realize();
        if (fRight)
            fRight->realize();
    }

protected:
    Sizeable* fLeft;
    Sizeable* fRight;
    unsigned fOffset;
};

/*
 * A grid of rows.
 * The second column is aligned.
 */
class Table : public Sizeable {
public:
    Table() { }
    ~Table() { }
    Table& operator+=(Row* row) {
        if (row) fRows += row;
        return *this;
    }
    unsigned width() override {
        unsigned w = 1;
        for (Sizeable* row : fRows)
            w = max(w, row->width());
        return w;
    }
    unsigned height() override {
        unsigned h = 1;
        for (Sizeable* row : fRows)
            h = max(h, row->y() + row->height());
        return h - y();
    }
    int x() override {
        return fRows.nonempty() ? fRows[0]->x() : 0;
    }
    int y() override {
        return fRows.nonempty() ? fRows[0]->y() : 0;
    }
    void setPosition(int x, int y) override {
        for (Sizeable* row : fRows) {
            row->setPosition(x, y);
            y += row->height();
        }
    }
    void realize() override {
        unsigned offset = 0;
        for (Row* row : fRows) {
            offset = max(offset, row->offset());
        }
        for (Row* row : fRows) {
            row->setOffset(offset);
            row->realize();
        }
    }
protected:
    YObjectArray<Row> fRows;
};

/*
 * Communicate a layout to the outside world.
 */
inline void Sizeable::layout(YWindow* outside) {
    setPosition(0, 0);
    realize();
    outside->setSize(width(), height());
}

#endif

// vim: set sw=4 ts=4 et:
