
#include "ywindow.h"
#include "yconfig.h"
#include "ytimer.h"

class YApm: public YWindow, public YTimerListener {
public:
    YApm(YWindow *aParent = 0);
    virtual ~YApm();

    void autoSize();

    virtual void paint(Graphics &g, const YRect &er);

    void updateToolTip();
    virtual bool handleTimer(YTimer *t);

private:
    YTimer apmTimer;

    YPixmap *getPixmap(char ch);
    int calcWidth(const char *s, int count);

    static YColorPrefProperty gApmBg;
    static YColorPrefProperty gApmFg;
    static YFontPrefProperty gApmFont;
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
