#include "config.h"
#include "yapp.h"
#include "aclock.h"

const char *ApplicationName = "iceclock";

YColor* getTaskBarBg() {
    static YColor* taskBarBg;
    if (taskBarBg == 0) {
        taskBarBg = new YColor("rgb:C0/C0/C0");
    }
    return taskBarBg;
}

int main(int argc, char **argv) {
    YApplication app(&argc, &argv);

    YClock *clock = new YClock(0);
    clock->show();

    return app.mainLoop();
}

// vim: set sw=4 ts=4 et:
