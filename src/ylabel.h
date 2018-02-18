#ifndef __YLABEL_H
#define __YLABEL_H

#include "ywindow.h"

class YLabel: public YWindow {
public:
    YLabel(const ustring &label, YWindow *parent = 0);
    virtual ~YLabel();

    virtual void paint(Graphics &g, const YRect &r);

    void setText(const ustring &label);
    const ustring getText() const { return fLabel; }
private:
    ustring fLabel;

    void autoSize();

    static YColorName labelFg;
    static YColorName labelBg;
    static ref<YFont> labelFont;
};

#endif

// vim: set sw=4 ts=4 et:
