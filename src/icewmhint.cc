/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include "intl.h"
#include "base.h"

char const *ApplicationName = "icewmhint";

static const char* get_hint_usage() {
    return _("Usage: icewmhint class.instance option arg\n");
}

static void print_usage()
{
    puts(get_hint_usage());
    exit(0);
}

class Hinter {
    Display *display;

public:
    Hinter() :
        display(XOpenDisplay(nullptr))
    {
        if (display == nullptr)
            die(1,  _("Can't open display: %s. "
                      "X must be running and $DISPLAY set."),
                    XDisplayName(nullptr));

        XSynchronize(display, True);
    }

    ~Hinter() {
        XCloseDisplay(display);
        // let icewm take hint
        usleep(42*1000);
    }

    void setHints(char** args, int count) {
        size_t size = 0;
        for (int i = 0; i < count; ++i)
            size += 1 + strlen(args[i]);

        unsigned char *hint = new unsigned char [size];
        if (hint == nullptr)
            die(1, _("Out of memory (len=%d)."), int(size));

        size_t copy = 0;
        for (int i = 0; i < count; ++i) {
            size_t len = 1 + strlen(args[i]);
            memcpy(hint + copy, args[i], len);
            copy += len;
        }

        Atom icehint = XInternAtom(display, "_ICEWM_WINOPTHINT", False);
        XChangeProperty(display, DefaultRootWindow(display),
                        icehint, icehint,
                        8, PropModeAppend,
                        hint, int(size));

        delete[] hint;
    }
};

int main(int argc, char **argv) {

    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);

    char** arg = &argv[1];
    for (; arg < &argv[argc] && **arg == '-'; ++arg) {
        char* value = nullptr;
        if (GetArgument(value, "d", "display", arg, argv + argc)) {
            setenv("DISPLAY", value, True);
        }
        else {
            check_help_version(*arg, get_hint_usage(), VERSION);
        }
    }

    int count = static_cast<int>(&argv[argc] - arg);
    if (count < 3 || count % 3 != 0)
        print_usage();

    Hinter().setHints(arg, count);

    return 0;
}

// vim: set sw=4 ts=4 et:
