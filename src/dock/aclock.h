#ifndef __CLOCK_H
#define __CLOCK_H

#include "ywindow.h"
#include "yconfig.h"
#include "ytimer.h"

class YClock: public YWindow, public YTimerListener {
public:
    YClock(YWindow *aParent = 0);
    virtual ~YClock();

    void autoSize();

    virtual void handleButton(const XButtonEvent &button);
    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);

    void updateToolTip();
    virtual bool handleTimer(YTimer *t);
private:
    YTimer clockTimer;
    bool clockUTC;
    bool toolTipUTC;

    YPixmap *getPixmap(char ch);
    int calcWidth(const char *s, int count);

    static YColorPrefProperty gClockBg;
    static YColorPrefProperty gClockFg;
    static YFontPrefProperty gClockFont;

    //static YColor *clockBg;
    //static YColor *clockFg;
    //static YFont *clockFont;

    YPref fFormatTime;
    YPref fFormatDate;

    YPixmap *PixNum[10];
    YPixmap *PixSpace;
    YPixmap *PixColon;
    YPixmap *PixSlash;
    YPixmap *PixA;
    YPixmap *PixP;
    YPixmap *PixM;
    YPixmap *PixDot;
};

#endif
