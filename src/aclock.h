#ifndef __CLOCK_H
#define __CLOCK_H

#include "ywindow.h"
#include "ytimer.h"

#ifdef CONFIG_APPLET_CLOCK
class YClock: public YWindow, public YTimerListener {
public:
    YClock(YWindow *aParent = 0);
    virtual ~YClock();

    void autoSize();

    virtual void handleButton(const XButtonEvent &button);
    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void paint(Graphics &g, const YRect &r);

    void updateToolTip();
    virtual bool handleTimer(YTimer *t);

private:
    YTimer *clockTimer;
    bool clockUTC;
    bool toolTipUTC;
    int transparent;

    ref<YPixmap> getPixmap(char ch);
    int calcWidth(const char *s, int count);
    bool hasTransparency();


    static YColor *clockBg;
    static YColor *clockFg;
    static ref<YFont> clockFont;
};
#endif

// !!! remove this
#ifdef CONFIG_APPLET_CLOCK
extern ref<YPixmap> PixNum[10];
extern ref<YPixmap> PixSpace;
extern ref<YPixmap> PixColon;
extern ref<YPixmap> PixSlash;
extern ref<YPixmap> PixA;
extern ref<YPixmap> PixP;
extern ref<YPixmap> PixM;
extern ref<YPixmap> PixDot;
extern ref<YPixmap> PixPercent;
#endif

#endif
