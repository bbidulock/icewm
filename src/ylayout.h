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
};

/*
 * Create internal space in a dialog.
 */
class Spacer : public Sizeable {
public:
    Spacer(unsigned width, unsigned height) :
        fW(width), fH(height), fX(0), fY(0) { }
    virtual unsigned width() { return fW; }
    virtual unsigned height() { return fH; }
    virtual int x() { return fX; }
    virtual int y() { return fY; }
    virtual void setPosition(int x, int y) {
        fX = x;
        fY = y;
    }
    virtual void realize() { }
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
    virtual unsigned width() { return fWin->width(); }
    virtual unsigned height() { return fWin->height(); }
    virtual int x() { return fWin->x(); }
    virtual int y() { return fWin->y(); }
    virtual void setPosition(int x, int y) {
        fWin->setPosition(x, y);
    }
    virtual void realize() { fWin->show(); }
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
    virtual unsigned width() { return fWin->width() + 2 * fHori; }
    virtual unsigned height() { return fWin->height() + 2 * fVert; }
    virtual int x() { return fWin->x() - fHori; }
    virtual int y() { return fWin->y() - fVert; }
    virtual void setPosition(int x, int y) {
        fWin->setPosition(x + fHori, y + fVert);
    }
    virtual void realize() { fWin->realize(); }
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
    virtual unsigned width() {
        unsigned w = 1;
        for (Sizeable* step : fSteps)
            w = max(w, step->width());
        return w;
    }
    virtual unsigned height() {
        unsigned h = 0;
        for (Sizeable* step : fSteps)
            h += step->height();
        return h;
    }
    virtual int x() { return empty() ? 0 : fSteps[0]->x(); }
    virtual int y() { return empty() ? 0 : fSteps[0]->y(); }
    virtual void setPosition(int x, int y) {
        for (Sizeable* step : fSteps) {
            step->setPosition(x, y);
            y += step->height();
        }
    }
    virtual void realize() {
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
    virtual unsigned width() {
        return fRight ? offset() + fRight->width() : fLeft->width();
    }
    virtual unsigned height() {
        return max(fLeft->height(), fRight ? fRight->height() : 0);
    }
    virtual int x() { return fLeft->x(); }
    virtual int y() { return fLeft->y(); }
    virtual void setPosition(int x, int y) {
        fLeft->setPosition(x, y);
        if (fRight)
            fRight->setPosition(x + offset(), y);
    }
    virtual void realize() {
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
    virtual unsigned width() {
        unsigned w = 1;
        for (Sizeable* row : fRows)
            w = max(w, row->width());
        return w;
    }
    virtual unsigned height() {
        unsigned h = 1;
        for (Sizeable* row : fRows)
            h = max(h, row->y() + row->height());
        return h - y();
    }
    virtual int x() {
        return fRows.nonempty() ? fRows[0]->x() : 0;
    }
    virtual int y() {
        return fRows.nonempty() ? fRows[0]->y() : 0;
    }
    virtual void setPosition(int x, int y) {
        for (Sizeable* row : fRows) {
            row->setPosition(x, y);
            y += row->height();
        }
    }
    virtual void realize() {
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

#endif

// vim: set sw=4 ts=4 et:
