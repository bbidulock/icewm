#ifndef __YTOOLTIP_H
#define __YTOOLTIP_H

#ifdef CONFIG_TOOLTIP
#include "ywindow.h"
#include "ytimer.h"

class YToolTip:
public YWindow, 
public YTimer::Listener {
public:
    YToolTip(YWindow *aParent = 0);
    virtual ~YToolTip();
    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);

    void setText(const char *tip);
    virtual bool handleTimer(YTimer *t);
    void locate(YWindow *w, const XCrossingEvent &crossing);

private:
    void display();

    char *fText;

    static YColor *toolTipBg;
    static YColor *toolTipFg;
    static YFont *toolTipFont;
    static YTimer *fToolTipVisibleTimer;
};
#endif

#endif
