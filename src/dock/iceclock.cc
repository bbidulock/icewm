#include "config.h"
#include "yapp.h"
#include "aclock.h"
#include "ytopwindow.h"

//#define NO_KEYBIND
//#include "default.h"
//#define CFGDEF
//#include "default.h"

class YClockWindow: public YTopWindow {
public:
    YClockWindow(): YTopWindow() {}

    virtual void handleClose() {
        app->exit(0);
    }
};

int main(int argc, char **argv) {
    YApplication app("iceclock", &argc, &argv);

    YClockWindow *top = new YClockWindow();
    YClock *clock = new YClock(top);
    clock->show();

    top->setSize(clock->width(), clock->height());
    top->show();

    int rc = app.mainLoop();
    delete clock;
    return rc;
}
