#ifndef __CLOCK_H
#define __CLOCK_H

#include "ywindow.h"
#include "ytimer.h"

#ifdef CONFIG_APPLET_CLOCK
class YClock:
public YWindow,
public YTimer::Listener {
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
    YTimer *clockTimer;
    bool clockUTC;
    bool toolTipUTC;

    YPixmap *getPixmap(char ch);
    int calcWidth(const char *s, int count);

    static YColor *clockBg;
    static YColor *clockFg;
    static YFont *clockFont;
};
#endif

// !!! remove this
#ifdef CONFIG_APPLET_CLOCK
extern YPixmap *PixNum[10];
extern YPixmap *PixSpace;
extern YPixmap *PixColon;
extern YPixmap *PixSlash;
extern YPixmap *PixA;
extern YPixmap *PixP;
extern YPixmap *PixM;
extern YPixmap *PixDot;
#endif

#endif
