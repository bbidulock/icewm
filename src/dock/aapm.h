
#include "ywindow.h"
#include "ytimer.h"

class YApm: public YWindow, public YTimerListener {
public:
    YApm(YWindow *aParent = 0);
    virtual ~YApm();

    void autoSize();

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);

    void updateToolTip();
    virtual bool handleTimer(YTimer *t);

private:
    YTimer apmTimer;

    YPixmap *getPixmap(char ch);
    int calcWidth(const char *s, int count);

    static YColor *apmBg;
    static YColor *apmFg;
    static YFont *apmFont;

    YPixmap *PixNum[10];
    YPixmap *PixSpace;
    YPixmap *PixColon;
    YPixmap *PixSlash;
    YPixmap *PixA;
    YPixmap *PixP;
    YPixmap *PixM;
    YPixmap *PixDot;
};
