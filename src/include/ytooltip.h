#ifndef __YTOOLTIP_H
#define __YTOOLTIP_H

#include "ywindow.h"
#include "ytimer.h"
#include "yconfig.h"

#pragma interface

class CStr;

class YToolTip: public YWindow, public YTimerListener {
public:
    YToolTip(YWindow *aParent = 0);
    virtual ~YToolTip();
    virtual void paint(Graphics &g, const YRect &er);

    void __setText(const char *tip);
    void setText(const CStr *tip);
    virtual bool handleTimer(YTimer *t);
    void locate(YWindow *w, int x_root, int y_root);

private:
    void display();

    CStr *fText;

    static YColorPrefProperty gToolTipBg;
    static YColorPrefProperty gToolTipFg;
    static YFontPrefProperty gToolTipFont;
    static YTimer *fToolTipVisibleTimer;
    static unsigned int ToolTipTime;
private: // not-used
    YToolTip(const YToolTip &);
    YToolTip &operator=(const YToolTip &);
};

#endif
