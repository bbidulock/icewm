#ifndef __YLABEL_H
#define __YLABEL_H

#include "ywindow.h"

class YLabel: public YWindow {
public:
    YLabel(const mstring &label, YWindow *parent = nullptr);
    virtual ~YLabel();

    virtual void paint(Graphics &g, const YRect &r);
    virtual void handleExpose(const XExposeEvent& expose);
    virtual void configure(const YRect2& r);
    virtual void repaint();

    void setText(const mstring &label);
    const mstring getText() const { return fLabel; }
private:
    mstring fLabel;
    bool fPainted;

    void autoSize();

    static YColorName labelFg;
    static YColorName labelBg;
    static ref<YFont> labelFont;
    static int labelObjectCount;
};

#endif

// vim: set sw=4 ts=4 et:
