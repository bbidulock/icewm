#include "config.h"
#include "yapp.h"
#include "aclock.h"

#define NO_KEYBIND
//#include "bindkey.h"
#include "prefs.h"
#define CFGDEF
//#include "bindkey.h"
#include "default.h"

int main(int argc, char **argv) {
    YApplication app(&argc, &argv);

    YClock *clock = new YClock();
    clock->show();

    return app.mainLoop();
}
