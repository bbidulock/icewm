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

    virtual bool eventButton(const YButtonEvent &button);
    virtual bool eventCrossing(const YCrossingEvent &crossing);
    virtual bool eventClick(const YClickEvent &up);
    virtual void paint(Graphics &g, const YRect &er);

    void updateToolTip();
    virtual bool handleTimer(YTimer *t);
private:
    YTimer clockTimer;
    bool clockUTC;
    bool toolTipUTC;

    YPixmap *getPixmap(char ch);
    int calcWidth(const char *s, int count);

    YPref fFormatTime;
    YPref fFormatDate;

    static YColorPrefProperty gClockBg;
    static YColorPrefProperty gClockFg;
    static YFontPrefProperty gClockFont;
    static YPixmapPrefProperty gPixNum0;
    static YPixmapPrefProperty gPixNum1;
    static YPixmapPrefProperty gPixNum2;
    static YPixmapPrefProperty gPixNum3;
    static YPixmapPrefProperty gPixNum4;
    static YPixmapPrefProperty gPixNum5;
    static YPixmapPrefProperty gPixNum6;
    static YPixmapPrefProperty gPixNum7;
    static YPixmapPrefProperty gPixNum8;
    static YPixmapPrefProperty gPixNum9;
    static YPixmapPrefProperty gPixSpace;
    static YPixmapPrefProperty gPixColon;
    static YPixmapPrefProperty gPixSlash;
    static YPixmapPrefProperty gPixDot;
    static YPixmapPrefProperty gPixA;
    static YPixmapPrefProperty gPixP;
    static YPixmapPrefProperty gPixM;
};

#endif
