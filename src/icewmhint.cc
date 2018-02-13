/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include "intl.h"
#include "base.h"

char *displayName = 0;
Display *display = 0;
Window root = 0;

Atom XA_IcewmWinOptHint;
char const *ApplicationName = "icewmhint";

static const char* get_hint_usage() {
    return _("Usage: icewmhint class.instance option arg\n");
}

static void print_usage()
{
    puts(get_hint_usage());
    exit(0);
}

static void initDisplay() {
    display = XOpenDisplay(displayName);
    if (display == 0) {
        fprintf(stderr,
                _("Can't open display: %s. "
                  "X must be running and $DISPLAY set."),
                displayName ? displayName : _("<none>"));
        fputs("\n", stderr);
        exit(1);
    }
    else {
        root = DefaultRootWindow(display);
        XA_IcewmWinOptHint = XInternAtom(display, "_ICEWM_WINOPTHINT", False);
    }
}

static void setHint(const char* clsin, const char* option, const char* arg) {
    int clsin_len = strlen(clsin) + 1;
    int option_len = strlen(option) + 1;
    int arg_len = strlen(arg) + 1;

    int hint_len = clsin_len + option_len + arg_len;
    unsigned char *hint = (unsigned char *)malloc(hint_len);

    if (hint == 0) {
        fprintf(stderr, _("Out of memory (len=%d)."), hint_len);
        fputs("\n", stderr);
        exit(1);
    }

    memcpy(hint, clsin, clsin_len);
    memcpy(hint + clsin_len, option, option_len);
    memcpy(hint + clsin_len + option_len, arg, arg_len);

    XChangeProperty(display, root,
                    XA_IcewmWinOptHint, XA_IcewmWinOptHint,
                    8, PropModeAppend,
                    hint, hint_len);

    free(hint);
}

int main(int argc, char **argv) {

    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);

    int k = 1;
    for (char** arg = argv + 1; k < argc && **arg == '-'; k = ++arg - argv) {
        char* value(0);
        if (GetArgument(value, "d", "display", arg, argv + argc)) {
            displayName = value;
        }
        else {
            check_help_version(*arg, get_hint_usage(), VERSION);
        }
    }

    if (argc - k < 3 || (argc - k) % 3 != 0) {
        print_usage();
    }

    initDisplay();

    for (; k + 3 <= argc; k += 3) {
        setHint(argv[k + 0], argv[k + 1], argv[k + 2]);
    }

    XCloseDisplay(display);

    return 0;
}

// vim: set sw=4 ts=4 et:
