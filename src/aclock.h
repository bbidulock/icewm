#ifndef __CLOCK_H
#define __CLOCK_H

#include "ywindow.h"
#include "ytimer.h"

#ifdef CONFIG_APPLET_CLOCK

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
    YTimer *clockTimer;
    bool clockUTC;
    bool toolTipUTC;
    int transparent;
    YSMListener *smActionListener;

    ref<YPixmap> getPixmap(char ch);
    int calcWidth(const char *s, int count);
    bool hasTransparency();

    YColor *clockBg;
    YColor *clockFg;
    ref<YFont> clockFont;
};
#endif

#endif
