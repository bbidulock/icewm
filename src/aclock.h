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
    virtual void handleExpose(const XExposeEvent &expose);
    virtual void handleVisibility(const XVisibilityEvent& visib);
    virtual void paint(Graphics &g, const YRect &r);

    virtual void updateToolTip();
    virtual bool handleTimer(YTimer *t);

private:
    enum {
        TimeSize = 64,
        DateSize = 128,
    };

    lazy<YTimer> clockTimer;
    bool clockUTC;
    bool toolTipUTC;
    bool isVisible;
    bool clockTicked;
    int transparent;
    YSMListener *smActionListener;

    ref<YPixmap> getPixmap(char ch);
    int calcWidth(const char *s, int count);
    bool hasTransparency();
    void repaint();
    void picture();
    void draw(Graphics& g);
    void fill(Graphics& g);
    void fill(Graphics& g, int x, int y, int w, int h);
    void paintPretty(Graphics& g, const char* s, int len);
    void paintPlain(Graphics& g, const char* s, int len);

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
