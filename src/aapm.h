
#include "ywindow.h"
#include "ytimer.h"

#ifdef CONFIG_APPLET_APM
class YApm: public YWindow, public YTimerListener {
public:
    YApm(YWindow *aParent = 0);
    virtual ~YApm();

    void autoSize();

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);

    void updateToolTip();
    virtual bool handleTimer(YTimer *t);

private:
    YTimer *apmTimer;

    YPixmap *getPixmap(char ch);
    int calcWidth(const char *s, int count);

    static YColor *apmBg;
    static YColor *apmFg;
    static YFont *apmFont;
};

#endif
