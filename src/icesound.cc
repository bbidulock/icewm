/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 * EsounD Hack by CW Zuckschwerdt
 */

#include "config.h"

#define ESD

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
#include <X11/xpm.h>
#include <signal.h>
#include <audiofile.h>
#include <esd.h>

#define GUI_EVENT_NAMES
#include "guievent.h"

static char *program_name = NULL;

char *displayName = 0;
Display *display = 0;
Window root = 0;

Atom _GUI_EVENT;;

int main(int argc, char *argv[]) {

    program_name = argv [0];

    if (!(display = XOpenDisplay(displayName))) {
        fprintf(stderr,
                _("Can't open display: %s. "
                  "X must be running and $DISPLAY set."),
                displayName ? displayName : _("<none>"));
        puts("\n", stderr);
        exit(1);
    }

    root = RootWindow(display, DefaultScreen(display));

    _GUI_EVENT = XInternAtom(display, XA_GUI_EVENT_NAME, False);

    XSelectInput(display, root, PropertyChangeMask);

    signal(SIGCHLD, SIG_IGN);

    while (1) {
        XEvent xev;

        XNextEvent(display, &xev);

        switch (xev.type) {
        case PropertyNotify:
            if (xev.xproperty.atom == _GUI_EVENT) {
                if (xev.xproperty.state == PropertyNewValue) {
                    Atom type;
                    int format;
                    unsigned long nitems, lbytes;
                    unsigned char *propdata;
                    int d = -1;

                    if (XGetWindowProperty(display, root,
                                           _GUI_EVENT, 0, 3, False, _GUI_EVENT,
                                           &type, &format, &nitems, &lbytes,
                                           &propdata) == Success)
                        if (propdata) {
                            d = *(char *)propdata;
                            XFree(propdata);
                        }
                    int pid = -1;
                    if (pid > 0)
                        kill(pid, SIGKILL);
                    if (d != -1 && (pid = fork()) == 0) {
                        for (unsigned int i = 0; i < sizeof(gui_events)/sizeof(gui_events[0]); i++) {
                            if (gui_events[i].type == d) {
                                puts(gui_events[i].name);
                                char s[1024];
                                int ifd, ofd, n;
                                // !!! TODO: search in (~/.icewm/)
                                sprintf(s, "%s/sounds/%s.wav", LIBDIR, gui_events[i].name);

                                if (access(s, R_OK) == 0) {
#ifdef ESD
                                    esd_play_file(program_name, s, 1);
#else
                                    ifd = open(s, O_RDONLY);
                                    if (ifd == -1)
                                        exit(0);
                                    ofd = open("/dev/dsp", O_WRONLY);
                                    if (ofd == -1)
                                        exit(0);
                                    while ((n = read(ifd, s, sizeof(s))) > 0)
                                        write(ofd, s, n);
#endif
                                }
                                break;
                            }
                        }
                        exit(0);
                    }
                }
            }
            break;
        }
    }
}
