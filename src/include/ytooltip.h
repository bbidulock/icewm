#ifndef __YTOOLTIP_H
#define __YTOOLTIP_H

#include "ywindow.h"
#include "ytimer.h"
#include "yconfig.h"

class CStr;

class YToolTip: public YWindow, public YTimerListener {
public:
    YToolTip(YWindow *aParent = 0);
    virtual ~YToolTip();
    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);

    void _setText(const char *tip);
    void setText(const CStr *tip);
    virtual bool handleTimer(YTimer *t);
    void locate(YWindow *w, const XCrossingEvent &crossing);

private:
    void display();

    CStr *fText;

    static YColorPrefProperty gToolTipBg;
    static YColorPrefProperty gToolTipFg;
    static YFontPrefProperty gToolTipFont;
    static YTimer *fToolTipVisibleTimer;
    static unsigned int ToolTipTime;
};

#endif
