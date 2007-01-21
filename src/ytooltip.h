#ifndef __YTOOLTIP_H
#define __YTOOLTIP_H

#ifdef CONFIG_TOOLTIP
#include "ywindow.h"
#include "ytimer.h"

class YToolTip: public YWindow, public YTimerListener {
public:
    YToolTip(YWindow *aParent = 0);
    virtual ~YToolTip();
    virtual void paint(Graphics &g, const YRect &r);

    void setText(const ustring &tip);
    virtual bool handleTimer(YTimer *t);
    void locate(YWindow *w, const XCrossingEvent &crossing);

private:
    void display();

    ustring fText;

    static YColor *toolTipBg;
    static YColor *toolTipFg;
    static ref<YFont> toolTipFont;
    static YTimer *fToolTipVisibleTimer;
};
#endif

#endif
