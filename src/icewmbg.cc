#include "config.h"

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
#include <signal.h>

#include "intl.h"

#ifdef IMLIB
#include <Imlib.h>

static ImlibData *hImlib = 0;
#else
#include <X11/xpm.h>
#endif

#include "base.h"
#include "WinMgr.h"

char *displayName = 0;
Display *display = 0;
Window root = 0;
Colormap defaultColormap;
bool supportSemitransparency = false;

Atom _XA_WIN_WORKSPACE = None;
Atom _XA_XROOTPMAP_ID = None;
Atom _XA_XROOTCOLOR_PIXEL = None;

long activeWorkspace = WinWorkspaceInvalid;

bool getWorkspace() {
    long w;
    Atom r_type;
    int r_format;
    unsigned long nitems, lbytes;
    unsigned char *prop;

    if (XGetWindowProperty(display, root,
                           _XA_WIN_WORKSPACE,
                           0, 1, False, XA_CARDINAL,
                           &r_type, &r_format,
                           &nitems, &lbytes, &prop) == Success && prop)
    {
        if (r_type == XA_CARDINAL && r_format == 32 && nitems == 1) {
            w = ((long *)prop)[0];
            //printf("%ld\n", w);
            if (w != activeWorkspace) {
                activeWorkspace = w;
                //printf("active=%ld\n", activeWorkspace);
                XFree(prop);
                return true;
            }
        }
        XFree(prop);
    }
    return false;
}

long bgCount;
Pixmap defbg, bg[64] = { 0 };

Pixmap loadPixmap(const char *fileName) {
    Pixmap pixmap = 0;
#ifdef IMLIB
    if(!hImlib) hImlib=Imlib_init(display);

    ImlibImage *im = Imlib_load_image(hImlib, (char *)fileName);
    if(im) {
        Imlib_render(hImlib, im, im->rgb_width, im->rgb_height);
        pixmap = (Pixmap)Imlib_move_image(hImlib, im);
        Imlib_destroy_image(hImlib, im);
    } else {
        fprintf(stderr, _("Loading image %s failed"), fileName);
        fputs("\n", stderr);
    }
#else
    XpmAttributes xpmAttributes;
    Pixmap fake;
    int rc;

    xpmAttributes.colormap  = defaultColormap;
    xpmAttributes.closeness = 65535;
    xpmAttributes.valuemask = XpmSize|XpmReturnPixels|XpmColormap|XpmCloseness;

    rc = XpmReadFileToPixmap(display, root,
                             (char *)fileName,
                             &pixmap, &fake,
                             &xpmAttributes);

    if (rc == 0) {
        if (fake != None)
            XFreePixmap(display, fake);
    } else {
        fprintf(stderr, _("Load pixmap %s failed with rc=%d"), fileName, rc);
        fputs("\n", stderr);
    }
#endif
    return pixmap;
}

void updateBg(long workspace) {
    Pixmap pixmap = defbg;

    if (workspace < bgCount && bg[workspace])
        pixmap = bg[workspace];

    if (pixmap != None) {
        XSetWindowBackgroundPixmap(display, root, pixmap);
        XClearWindow(display, root);

	if (supportSemitransparency) {
	    if (_XA_XROOTPMAP_ID)
		XChangeProperty(display, root, _XA_XROOTPMAP_ID,
				XA_PIXMAP, 32, PropModeReplace,
				(const unsigned char*) &pixmap, 1);
	    if (_XA_XROOTCOLOR_PIXEL) {
		unsigned long black(BlackPixel(display,
				    DefaultScreen(display)));

		XChangeProperty(display, root, _XA_XROOTCOLOR_PIXEL,
				XA_CARDINAL, 32, PropModeReplace,
				(const unsigned char*) &black, 1);
	    }

	    XFlush(display);
	}
    }
}

void signal_handler(int sig) {
    if (supportSemitransparency) {
	if (_XA_XROOTPMAP_ID)
            XDeleteProperty(display, root, _XA_XROOTPMAP_ID);
	if (_XA_XROOTCOLOR_PIXEL)
            XDeleteProperty(display, root, _XA_XROOTCOLOR_PIXEL);
    }

    XCloseDisplay(display);
    exit(sig);
}

void printUsage(int rc = 1) {
    fputs (_("Usage: icewmbg [OPTION]... pixmap1 [pixmap2]...\n"
	     "Changes desktop background on workspace switches.\n"
	     "The first pixmap is used as a default one.\n\n"
	     "-s, --semitransparency    Enable support for "
				       "semi-transparent terminals\n"),
	     stderr);
    exit(rc);
}

void invalidArgument(const char *appName, const char *arg) {
    fprintf(stderr, _("%s: unrecognized option `%s'\n"
		      "Try `%s --help' for more information.\n"),
		      appName, arg, appName);
    exit(1);
}

int main(int argc, char **argv) {
#ifdef ENABLE_NLS
    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);
#endif

    for (int n = 1; n < argc; ++n) if (argv[n][0] == '-')
	if (argv[n][1] == 's' ||
	    strcmp(argv[n] + 1, "-semitransparency") == 0 &&
	    !supportSemitransparency)
	    supportSemitransparency = true;
	else if (argv[n][1] == 'h' ||
		 strcmp(argv[n] + 1, "-help") == 0)
	    printUsage(0);
	else
	    invalidArgument("icewmbg", argv[n]);

    if (argc <= (supportSemitransparency ? 2 : 1))
	printUsage();

    if (!(display = XOpenDisplay(displayName))) {
        fprintf(stderr, _("Can't open display: `%s'. "
                          "X must be running and $DISPLAY set.\n"),
                displayName ? displayName : _("<none>"));
        exit(1);
    }

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);

    root = RootWindow(display, DefaultScreen(display));
    defaultColormap = DefaultColormap(display, DefaultScreen(display));
    _XA_WIN_WORKSPACE = XInternAtom(display, XA_WIN_WORKSPACE, False);
    
    if (supportSemitransparency) {
	_XA_XROOTPMAP_ID = XInternAtom(display, "_XROOTPMAP_ID", False);
	_XA_XROOTCOLOR_PIXEL = XInternAtom(display, "_XROOTCOLOR_PIXEL", False);
    }

    XSelectInput(display, root, PropertyChangeMask);

//  if (getWorkspace())
//      updateBg(activeWorkspace);

    // could be optimized
    bgCount = 0;
    for (int n = 1; n < argc; n++) if (*argv[n] != '-') {
	bg[bgCount++] = loadPixmap(argv[n]);
	if (!defbg) defbg = bg[bgCount - 1];
    }

//     Figment: moved here ...
    if (getWorkspace())
        updateBg(activeWorkspace);


    for (;;) {
        XEvent xev;

        XNextEvent(display, &xev);

        switch (xev.type) {
        case PropertyNotify:
            if (xev.xproperty.window == root &&
                xev.xproperty.atom == _XA_WIN_WORKSPACE)
            {
                if (getWorkspace())
                    updateBg(activeWorkspace);
            }
            break;
        }
    }
}
