#ifndef __YLABEL_H
#define __YLABEL_H

#include "ywindow.h"
#include "yconfig.h"

class CStr;

class YLabel: public YWindow {
public:
    YLabel(const char *label = 0, YWindow *parent = 0);
    virtual ~YLabel();

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);

    void setText(const char *label);
    const CStr *getText() const { return fLabel; }
private:
    CStr *fLabel;

    void autoSize();

    static YColorPrefProperty gLabelFg;
    static YColorPrefProperty gLabelBg;
    static YFontPrefProperty gLabelFont;
};

#endif
