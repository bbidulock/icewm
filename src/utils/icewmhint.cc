/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
//#include <X11/xpm.h>
#include <signal.h>

#define GUI_EVENT_NAMES
#include "guievent.h"

char *displayName = 0;
Display *display = 0;
Window root = 0;

Atom XA_IcewmWinOptHint;

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: icewmhint [class.instance] option arg\n");
        exit(1);
    }

    char *clsin = argv[1];
    char *option = argv[2];
    char *arg = argv[3];

    int clsin_len = strlen(clsin) + 1;;
    int option_len = strlen(option) + 1;
    int arg_len = strlen(arg) + 1;
    
    int hint_len = clsin_len + option_len + arg_len;
    unsigned char *hint = (unsigned char *)malloc(hint_len);

    if (hint == 0) {
        fprintf(stderr, "Out of memory (len=%d).", hint_len);
        exit(1);
    }

    memcpy(hint, clsin, clsin_len);
    memcpy(hint + clsin_len, option, option_len);
    memcpy(hint + clsin_len + option_len, arg, arg_len);

    if (!(display = XOpenDisplay(displayName))) {
        fprintf(stderr, "Can't open display: %s", displayName);
        exit(1);
    }

    root = RootWindow(display, DefaultScreen(display));
    XA_IcewmWinOptHint = XInternAtom(display, "_ICEWM_WINOPTHINT", False);
    XChangeProperty(display, root,
                    XA_IcewmWinOptHint, XA_IcewmWinOptHint,
                    8, PropModeAppend,
                    hint, hint_len);
    XCloseDisplay(display);
}
