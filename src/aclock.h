#ifndef __CLOCK_H
#define __CLOCK_H

#include "ywindow.h"
#include "ytimer.h"
#include "ypointer.h"
#include "yaction.h"

class IAppletContainer;
class YSMListener;
class YMenu;

class YClock: public YWindow, private YTimerListener, private YActionListener {
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
    virtual void handleExpose(const XExposeEvent &expose);
    virtual void handleMapNotify(const XMapEvent &map);
    virtual void handleUnmapNotify(const XUnmapEvent &map);
    virtual void handleVisibility(const XVisibilityEvent& visib);
    virtual void paint(Graphics &g, const YRect &r);

    virtual void updateToolTip();
    virtual bool handleTimer(YTimer *t);

    enum {
        TimeSize = 64,
        DateSize = 128,
    };

    lazy<YTimer> clockTimer;
    bool clockUTC;
    bool toolTipUTC;
    bool isMapped;
    bool isVisible;
    bool clockTicked;
    unsigned paintCount;
    int transparent;
    YSMListener *smActionListener;
    IAppletContainer* iapp;
    osmart<YMenu> fMenu;

    ref<YPixmap> getPixmap(char ch);
    int calcWidth(const char *s, int count);
    bool hasTransparency();
    void repaint();
    bool picture();
    bool draw(Graphics& g);
    void fill(Graphics& g);
    void fill(Graphics& g, int x, int y, int w, int h);
    bool paintPretty(Graphics& g, const char* s, int len);
    bool paintPlain(Graphics& g, const char* s, int len);

    int negativePosition;
    int positions[TimeSize];
    char previous[TimeSize];
    Drawable clockPixmap;
    YColorName clockBg;
    YColorName clockFg;
    ref<YFont> clockFont;
};
#endif

// vim: set sw=4 ts=4 et:
