#include "config.h"
#include "yapp.h"
#include "aclock.h"

const char *ApplicationName = "iceclock";

YColorName taskBarBg("rgb:C0/C0/C0");

int main(int argc, char **argv) {
    YApplication app(&argc, &argv);

    YClock clock(0);
    clock.show();

    return app.mainLoop();
}

// vim: set sw=4 ts=4 et:
