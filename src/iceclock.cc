#include "config.h"
#include "yapp.h"
#include "aclock.h"

const char *ApplicationName = "iceclock";

int main(int argc, char **argv) {
    YApplication app(&argc, &argv);

    YClock *clock = new YClock();
    clock->show();

    return app.mainLoop();
}
