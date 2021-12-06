#ifndef ACLOCK_H
#define ACLOCK_H

#include "ywindow.h"
#include "ytimer.h"
#include "ypointer.h"
#include "yaction.h"
#include "applet.h"

class IAppletContainer;
class YSMListener;
class YMenu;

class YClock:
    public IApplet,
    private Picturer,
    private YTimerListener,
    private YActionListener
{
public:
    YClock(YSMListener* sml, IAppletContainer* iapp, YWindow* parent,
           const char* envTZ, const char* myTZ, const char* format);
    virtual ~YClock();

private:
    void autoSize();
    const char* strTimeFmt(const struct tm& t);

    virtual void actionPerformed(YAction action, unsigned int modifiers);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual void handleClick(const XButtonEvent &up, int count);

    virtual void updateToolTip();
    virtual bool handleTimer(YTimer *t);
    virtual bool picture();

    enum {
        TimeSize = 64,
        DateSize = 128,
    };

    lazy<YTimer> clockTimer;
    bool clockUTC;
    bool toolTipUTC;
    bool clockTicked;
    unsigned paintCount;
    int transparent;
    YSMListener *smActionListener;
    IAppletContainer* iapp;
    osmart<YMenu> fMenu;
    const char* fTimeFormat;
    const char* fAltFormat;
    long fPid;

    void changeTimeFormat(const char* format);
    using IApplet::getPixmap;
    ref<YPixmap> getPixmap(char ch);
    int calcWidth(const char* str, int count);
    bool hasTransparency();
    bool draw(Graphics& g);
    void fill(Graphics& g);
    void fill(Graphics& g, int x, int y, int w, int h);
    bool paintPretty(Graphics& g, const char* str, int len);
    bool paintPlain(Graphics& g, const char* str, int len);

    int negativePosition;
    int positions[TimeSize];
    char previous[TimeSize];
    char lastTime[TimeSize];
    YColorName clockBg;
    YColorName clockFg;
    YFont clockFont;

    friend class ClockSet;
    const char* defaultTimezone;
    const char* displayTimezone;
    const char* currentTimezone;
    bool timezone(bool restore = false);
};

class ClockSet {
public:
    ClockSet(YSMListener* sml, IAppletContainer* iapp, YWindow* parent);
    ~ClockSet();
    YClock** begin() const { return clocks.begin(); }
    YClock** end() const { return clocks.end(); }
    int count() const { return clocks.getCount(); }
    YClock* operator[](int index) const { return clocks[index]; }
private:
    YObjectArray<YClock> clocks;
    YArray<char*> specs;
};

#endif

// vim: set sw=4 ts=4 et:
