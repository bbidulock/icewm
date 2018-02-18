#ifndef __CLOCK_H
#define __CLOCK_H

#include "ywindow.h"
#include "ytimer.h"

class YSMListener;

class YClock: public YWindow, public YTimerListener {
public:
    YClock(YSMListener *smActionListener, YWindow *aParent = 0);
    virtual ~YClock();

    void autoSize();

    virtual void handleButton(const XButtonEvent &button);
    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void paint(Graphics &g, const YRect &r);

    virtual void updateToolTip();
    virtual bool handleTimer(YTimer *t);

private:
    lazy<YTimer> clockTimer;
    bool clockUTC;
    bool toolTipUTC;
    int transparent;
    YSMListener *smActionListener;

    ref<YPixmap> getPixmap(char ch);
    int calcWidth(const char *s, int count);
    bool hasTransparency();

    YColorName clockBg;
    YColorName clockFg;
    ref<YFont> clockFont;
};
#endif

// vim: set sw=4 ts=4 et:
