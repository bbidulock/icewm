#include "config.h"
#include "yapp.h"
#include "acpustatus.h"
#include "ytopwindow.h"
#include "ypaint.h"

//#define NO_KEYBIND
//#include "default.h"
//#define CFGDEF
//#include "default.h"

class YCPUStatusWindow: public YTopWindow {
public:
    YCPUStatusWindow(): YTopWindow() {}

    virtual void handleClose() {
        app->exit(0);
    }
};

int main(int argc, char **argv) {
    YApplication app("iceclock", &argc, &argv);

    YCPUStatusWindow *top = new YCPUStatusWindow();
    CPUStatus *cpu = new CPUStatus(top);
    cpu->show();

    top->setSize(cpu->width(), cpu->height());
    top->show();

    int rc = app.mainLoop();
    delete cpu;
    return rc;
}
