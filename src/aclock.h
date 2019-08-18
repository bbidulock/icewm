#ifndef __CLOCK_H
#define __CLOCK_H

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
    YClock(YSMListener *smActionListener, IAppletContainer* iapp, YWindow *aParent);
    virtual ~YClock();

private:
    void autoSize();
    char const * strTimeFmt(struct tm const & t);

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
    long fPid;

    void changeTimeFormat(const char* format);
    using IApplet::getPixmap;
    ref<YPixmap> getPixmap(char ch);
    int calcWidth(const char *s, int count);
    bool hasTransparency();
    bool draw(Graphics& g);
    void fill(Graphics& g);
    void fill(Graphics& g, int x, int y, int w, int h);
    bool paintPretty(Graphics& g, const char* s, int len);
    bool paintPlain(Graphics& g, const char* s, int len);

    int negativePosition;
    int positions[TimeSize];
    char previous[TimeSize];
    YColorName clockBg;
    YColorName clockFg;
    ref<YFont> clockFont;
};
#endif

// vim: set sw=4 ts=4 et:
