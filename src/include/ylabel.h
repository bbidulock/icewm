#ifndef __YLABEL_H
#define __YLABEL_H

#include "ywindow.h"
#include "yconfig.h"

#pragma interface

class CStr;

class YLabel: public YWindow {
public:
    YLabel(const char *label = 0, YWindow *parent = 0);
    virtual ~YLabel();

    virtual void paint(Graphics &g, const YRect &er);

    void setText(const char *label);
    const CStr *getText() const { return fLabel; }
private:
    CStr *fLabel;

    void autoSize();

    static YColorPrefProperty gLabelFg;
    static YColorPrefProperty gLabelBg;
    static YFontPrefProperty gLabelFont;
    static YPixmapPrefProperty gPixmapBackground;
private: // not-used
    YLabel(const YLabel &);
    YLabel &operator=(const YLabel &);
};

#endif
