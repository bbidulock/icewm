#ifndef __YLABEL_H
#define __YLABEL_H

#include "ywindow.h"

class YLabel: public YWindow {
public:
    YLabel(const char *label = 0, YWindow *parent = 0);
    virtual ~YLabel();

    virtual void paint(Graphics &g, const YRect &r);

    void setText(const char *label);
    const char *getText() const { return fLabel; }
private:
    char *fLabel;

    void autoSize();

    static YColor *labelFg;
    static YColor *labelBg;
    static ref<YFont> labelFont;
};

#endif
