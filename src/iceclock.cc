#include "config.h"
#include "yxapp.h"
#include "ylocale.h"
#include "aclock.h"
#include "intl.h"

const char *ApplicationName = "iceclock";

YColorName taskBarBg("rgb:C0/C0/C0");

int main(int argc, char **argv) {
    YLocale locale;

    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);

    YXApplication app(&argc, &argv);

    YClock clock(nullptr, nullptr, nullptr);
    clock.show();

    return app.mainLoop();
}

// vim: set sw=4 ts=4 et:
