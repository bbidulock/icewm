#include "config.h"

#include "yfull.h"
#include "yapp.h"
#include "yarray.h"

#warning duplicates lots of prefs
#include "default.h"
#include "wmconfig.h"


#if 1
#include <stdio.h>
#include "intl.h"
#include <stdlib.h>
#include <unistd.h>
#endif

#if 0
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdarg.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
///#include <signal.h>


#include "base.h"
#include "WinMgr.h"
#endif
#if 0
#ifdef CONFIG_IMLIB
#include <Imlib.h>

static ImlibData *hImlib = 0;
#else
#include <X11/xpm.h>
#endif

#endif

char const * ApplicationName(NULL);

class DesktopBackgroundManager: public YApplication {
public:
    DesktopBackgroundManager(int *argc, char ***argv);

    virtual void handleSignal(int sig);

    void addImage(const char *imageFileName);

    void update();
private:
    long getWorkspace();
    void changeBackground(long workspace);
    YPixmap *loadImage(const char *imageFileName);

    bool filterEvent(const XEvent &xev);

private:
    YPixmap *defaultBackground;
    YPixmap *currentBackground;
    YObjectArray<YPixmap> backgroundPixmaps;
    long activeWorkspace;

    Atom _XA_XROOTPMAP_ID;
    Atom _XA_XROOTCOLOR_PIXEL;
    Atom _XA_NET_CURRENT_DESKTOP;
};

DesktopBackgroundManager::DesktopBackgroundManager(int *argc, char ***argv):
    YApplication(argc, argv),
    defaultBackground(0),
    currentBackground(0),
    activeWorkspace(-1),
    _XA_XROOTPMAP_ID(None),
    _XA_XROOTCOLOR_PIXEL(None)

{
    desktop->setStyle(YWindow::wsDesktopAware);
    catchSignal(SIGTERM);
    catchSignal(SIGINT);
    catchSignal(SIGQUIT);

    _XA_NET_CURRENT_DESKTOP =
        XInternAtom(app->display(), "_NET_CURRENT_DESKTOP", False);

#warning "I don't see a reason for this to be conditional...? maybe only as an #ifdef"
    if (supportSemitransparency) {
	_XA_XROOTPMAP_ID = XInternAtom(app->display(), "_XROOTPMAP_ID", False);
	_XA_XROOTCOLOR_PIXEL = XInternAtom(app->display(), "_XROOTCOLOR_PIXEL", False);
    }
}

void DesktopBackgroundManager::handleSignal(int sig) {
    switch (sig) {
    case SIGINT:
    case SIGTERM:
    case SIGQUIT:
        if (supportSemitransparency) {
            if (_XA_XROOTPMAP_ID)
                XDeleteProperty(app->display(), desktop->handle(), _XA_XROOTPMAP_ID);
            if (_XA_XROOTCOLOR_PIXEL)
                XDeleteProperty(app->display(), desktop->handle(), _XA_XROOTCOLOR_PIXEL);
        }

        ///XCloseDisplay(display);
        exit(1);

        break;

    default:
        YApplication::handleSignal(sig);
        break;
    }
}

void DesktopBackgroundManager::addImage(const char *imageFileName) {
    YPixmap *image = loadImage(imageFileName);

    backgroundPixmaps.append(image);
    if (defaultBackground == 0)
        defaultBackground = image;
}

YPixmap *DesktopBackgroundManager::loadImage(const char *imageFileName) {
    if (access(imageFileName, 0) == 0)
        return new YPixmap(imageFileName);
    else
        return 0;
}

void DesktopBackgroundManager::update() {
    long w = getWorkspace();
    if (w != activeWorkspace) {
        activeWorkspace = w;
        changeBackground(activeWorkspace);
    }
}

long DesktopBackgroundManager::getWorkspace() {
    long w;
    Atom r_type;
    int r_format;
    unsigned long nitems, lbytes;
    unsigned char *prop;

    if (XGetWindowProperty(app->display(), desktop->handle(),
                           _XA_NET_CURRENT_DESKTOP,
                           0, 1, False, XA_CARDINAL,
                           &r_type, &r_format,
                           &nitems, &lbytes, &prop) == Success && prop)
    {
        if (r_type == XA_CARDINAL && r_format == 32 && nitems == 1) {
            w = ((long *)prop)[0];
            XFree(prop);
            return w;
        }
        XFree(prop);
    }
    return -1;
}

void DesktopBackgroundManager::changeBackground(long workspace) {
    YPixmap *pixmap = defaultBackground;

    if (workspace >= 0 && workspace < (long)backgroundPixmaps.getCount() &&
        backgroundPixmaps[workspace])
    {
        pixmap = backgroundPixmaps[workspace];
    }

    if (pixmap != currentBackground) {
        XSetWindowBackgroundPixmap(app->display(), desktop->handle(), pixmap->pixmap());
        XClearWindow(app->display(), desktop->handle());

        if (supportSemitransparency) {
	    if (_XA_XROOTPMAP_ID)
		XChangeProperty(app->display(), desktop->handle(), _XA_XROOTPMAP_ID,
				XA_PIXMAP, 32, PropModeReplace,
				(const unsigned char*) &pixmap, 1);
	    if (_XA_XROOTCOLOR_PIXEL) {
		unsigned long black(BlackPixel(app->display(),
				    DefaultScreen(app->display())));

		XChangeProperty(app->display(), desktop->handle(), _XA_XROOTCOLOR_PIXEL,
				XA_CARDINAL, 32, PropModeReplace,
				(const unsigned char*) &black, 1);
            }
	}
    }
    XFlush(app->display());

    if (backgroundPixmaps.getCount() <= 1)
        exit(0);
}

bool DesktopBackgroundManager::filterEvent(const XEvent &xev) {
    if (xev.type == PropertyNotify &&
        xev.xproperty.window == desktop->handle() &&
        xev.xproperty.atom == _XA_NET_CURRENT_DESKTOP)
    {
        puts("switch");
        update();
    }
    return YApplication::filterEvent(xev);
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
    ApplicationName = basename(*argv);

    {
        int n;
        int gotOpts = 0;

        for (n = 1; n < argc; ++n) if (argv[n][0] == '-')
            if (argv[n][1] == 's' ||
                strcmp(argv[n] + 1, "-semitransparency") == 0 &&
                !supportSemitransparency)
            {
                supportSemitransparency = true;
                gotOpts++;
            } else if (argv[n][1] == 'h' ||
                     strcmp(argv[n] + 1, "-help") == 0)
                printUsage(0);
            else
                invalidArgument("icewmbg", argv[n]);

        if (argc < 1 + gotOpts + 1)
            printUsage();
    }

    DesktopBackgroundManager bg(&argc, &argv);

    {
        char *configFile = 0;

        if (configFile == 0)
            configFile = app->findConfigFile("background");
        if (configFile)
            loadConfiguration(configFile);
        delete configFile; configFile = 0;
    }

#warning "TODO: move to config file"
    for (int n = 1; n < argc; n++) {
        if (*argv[n] != '-') {
            bg.addImage(argv[n]);
        }
    }

    ///XSelectInput(app->display(), desktop->handle(), PropertyChangeMask);
    bg.update();

    return bg.mainLoop();
}

#if 0
Pixmap loadPixmap(const char *filename) {
    Pixmap pixmap = 0;
#ifdef CONFIG_IMLIB
    if(!hImlib) hImlib=Imlib_init(display);

    ImlibImage *im = Imlib_load_image(hImlib, (char *)filename);
    if(im) {
        Imlib_render(hImlib, im, im->rgb_width, im->rgb_height);
        pixmap = (Pixmap)Imlib_move_image(hImlib, im);
        Imlib_destroy_image(hImlib, im);
    } else {
        fprintf(stderr, _("Loading image %s failed"), filename);
        fputs("\n", stderr);
    }
#else
    XpmAttributes xpmAttributes;
    xpmAttributes.colormap  = defaultColormap;
    xpmAttributes.closeness = 65535;
    xpmAttributes.valuemask = XpmSize|XpmReturnPixels|XpmColormap|XpmCloseness;

    Pixmap mask;
    int const rc(XpmReadFileToPixmap(display, root, (char *)filename,
				     &pixmap, &mask, &xpmAttributes));

    if (rc != XpmSuccess)
        warn(_("Loading of pixmap \"%s\" failed: %s"),
	       filename, XpmGetErrorString(rc));
    else
	if (mask != None) XFreePixmap(display, mask);
#endif
    return pixmap;
}
#endif

