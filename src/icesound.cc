/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 * EsounD Hack by CW Zuckschwerdt
 */

#include "config.h"
#include "intl.h"

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
static char *soundPath = NULL;

char *displayName = 0;
Display *display = 0;
Window root = 0;
int counter = 0;

Atom _GUI_EVENT;;

int main(int argc, char *argv[]) {
    program_name = argv [0];
    if(argc == 2){
    	soundPath = argv[1];
    }
    
    if (!(display = XOpenDisplay(displayName))) {
        fprintf(stderr,
                _("Can't open display: %s. "
                  "X must be running and $DISPLAY set."),
                displayName ? displayName : _("<none>"));
        fputc('\n', stderr);
        exit(1);
    }

    root = RootWindow(display, DefaultScreen(display));

    _GUI_EVENT = XInternAtom(display, XA_GUI_EVENT_NAME, False);

    XSelectInput(display, root, PropertyChangeMask);

    signal(SIGCHLD, SIG_IGN);

    for (;;) {
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
                                           _GUI_EVENT, 0, 3, True, _GUI_EVENT,
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
                                char s[1024];
                                if(soundPath != NULL){
                                	sprintf(s, "%s/sounds/%s.wav", soundPath, gui_events[i].name);
                                } else {
                                	sprintf(s, "%s/sounds/%s.wav", LIBDIR, gui_events[i].name);
                                }

                                if (access(s, R_OK) == 0) {
#ifdef ESD
				    char prg[1024];
				    sprintf(prg,"%s-%i",program_name,counter);
				    counter++;
				    if(counter >= 32767) counter = 0;
                                    esd_play_file(program_name, s, 1);
#else
                                    int ifd = open(s, O_RDONLY);
                                    if (ifd == -1)
                                        exit(0);
                                    int ofd = open("/dev/dsp", O_WRONLY);
                                    if (ofd == -1)
                                        exit(0);
					
				    int n; 
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
