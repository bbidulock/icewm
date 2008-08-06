#include "config.h"

#include "yfull.h"
#include "yxapp.h"
#include "yarray.h"

#if 1
#include <stdio.h>
#include "intl.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#endif

#include "yconfig.h"
#include "yprefs.h"

#define CFGDEF

#include "icewmbg_prefs.h"

char const *ApplicationName = NULL;

class DesktopBackgroundManager: public YXApplication {
public:
    DesktopBackgroundManager(int *argc, char ***argv);

    virtual void handleSignal(int sig);

    void addImage(const char *imageFileName);

    void update();
    void sendQuit();
    void sendRestart();
private:
    long getWorkspace();
    void changeBackground(long workspace);
    ref<YPixmap> loadImage(const char *imageFileName);

    bool filterEvent(const XEvent &xev);

private:
    ref<YPixmap> defaultBackground;
    ref<YPixmap> currentBackground;
    YArray<ref<YPixmap> > backgroundPixmaps;
    long activeWorkspace;

    Atom _XA_XROOTPMAP_ID;
    Atom _XA_XROOTCOLOR_PIXEL;
    Atom _XA_NET_CURRENT_DESKTOP;

    Atom _XA_ICEWMBG_QUIT;
    Atom _XA_ICEWMBG_RESTART;
};

DesktopBackgroundManager::DesktopBackgroundManager(int *argc, char ***argv):
    YXApplication(argc, argv),
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
        XInternAtom(xapp->display(), "_NET_CURRENT_DESKTOP", False);
    _XA_ICEWMBG_QUIT =
        XInternAtom(xapp->display(), "_ICEWMBG_QUIT", False);
    _XA_ICEWMBG_RESTART =
        XInternAtom(xapp->display(), "_ICEWMBG_RESTART", False);

/// TODO #warning "I don't see a reason for this to be conditional...? maybe only as an #ifdef"
/// TODO #warning "XXX I see it now, the process needs to hold on to the pixmap to make this work :("
#ifndef NO_CONFIGURE
    if (supportSemitransparency) {
        _XA_XROOTPMAP_ID = XInternAtom(xapp->display(), "_XROOTPMAP_ID", False);
        _XA_XROOTCOLOR_PIXEL = XInternAtom(xapp->display(), "_XROOTCOLOR_PIXEL", False);
    }
#endif
}

void DesktopBackgroundManager::handleSignal(int sig) {
    switch (sig) {
    case SIGINT:
    case SIGTERM:
    case SIGQUIT:
#ifndef NO_CONFIGURE
        if (supportSemitransparency) {
            if (_XA_XROOTPMAP_ID)
                XDeleteProperty(xapp->display(), desktop->handle(), _XA_XROOTPMAP_ID);
            if (_XA_XROOTCOLOR_PIXEL)
                XDeleteProperty(xapp->display(), desktop->handle(), _XA_XROOTCOLOR_PIXEL);
        }
#endif

        ///XCloseDisplay(display);
        exit(1);

        break;

    default:
        YApplication::handleSignal(sig);
        break;
    }
}

void DesktopBackgroundManager::addImage(const char *imageFileName) {
    ref<YPixmap> image = loadImage(imageFileName);

    backgroundPixmaps.append(image);
    if (defaultBackground == null)
        defaultBackground = image;
}

ref<YPixmap> DesktopBackgroundManager::loadImage(const char *imageFileName) {
    if (access(imageFileName, 0) == 0) {
        ref<YPixmap> r = YPixmap::load(imageFileName);
        return r;
    } else
        return null;
}

void DesktopBackgroundManager::update() {
    //    long w = getWorkspace();
    //    if (w != activeWorkspace) {
    //        activeWorkspace = w;
    changeBackground(activeWorkspace);
    //    }
}

long DesktopBackgroundManager::getWorkspace() {
    long w;
    Atom r_type;
    int r_format;
    unsigned long nitems, lbytes;
    unsigned char *prop;

    if (XGetWindowProperty(xapp->display(), desktop->handle(),
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

#if 1
// should be a separate program to reduce memory waste
static ref<YPixmap> renderBackground(ref<YResourcePaths> paths,
                                     char const * filename, YColor * color)
{
    ref<YPixmap> back;

    if (*filename == '/') {
        if (access(filename, R_OK) == 0)
            back = YPixmap::load(filename);
    } else
        back = paths->loadPixmap(0, filename);

#ifndef NO_CONFIGURE
    if (back != null && (centerBackground || desktopBackgroundScaled)) {
        ref<YPixmap> cBack = YPixmap::create(desktop->width(), desktop->height());
        Graphics g(cBack, 0, 0);

        g.setColor(color);
        g.fillRect(0, 0, desktop->width(), desktop->height());
        if (desktopBackgroundScaled &&
	    (desktop->width() != back->width() ||
	     desktop->height() != back->height()))
	{
            int aw = back->width();
            int ah = back->height();
            if (aw < desktop->width()) {
                ah = (int)((long long)desktop->width() * ah / aw);
                aw = desktop->width();
                if (ah * 11 / 10 > desktop->height()) {
                    aw = (int)((long long)desktop->height() * aw / ah);
                    ah = desktop->height();
                }
            } else {
                aw = (int)((long long)desktop->height() * aw / ah);
                ah = desktop->height();
                if (aw * 11 / 10 > desktop->width()) {
                    ah = (int)((long long)desktop->width() * ah / aw);
                    aw = desktop->width();
                }
            }
            ref<YPixmap> scaled =
		back->scale(aw, ah);
            if (scaled != null) {
                g.drawPixmap(scaled, (desktop->width() -  scaled->width()) / 2,
                             (desktop->height() - scaled->height()) / 2);
                scaled = null;
            }
        } else
        {
            g.drawPixmap(back, (desktop->width() -  back->width()) / 2,
                         (desktop->height() - back->height()) / 2);
        }

        back = cBack;
    }
#endif
/// TODO #warning "TODO: implement scaled background"
    return back;
}
#endif

void DesktopBackgroundManager::changeBackground(long /*workspace*/) {
/// TODO #warning "fixme: add back handling of multiple desktop backgrounds"
#if 0
    ref<YPixmap> pixmap = defaultBackground;

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

#endif
#if 1
    ref<YResourcePaths> paths = YResourcePaths::subdirs(null, true);
    YColor * bColor((DesktopBackgroundColor && DesktopBackgroundColor[0])
                    ? new YColor(DesktopBackgroundColor)
                    : 0);

    if (bColor == 0)
        bColor = YColor::black;

    unsigned long const bPixel(bColor->pixel());
    bool handleBackground(false);
    Pixmap bPixmap(None);

    if (DesktopBackgroundPixmap && DesktopBackgroundPixmap[0]) {
        ref<YPixmap> back =
            renderBackground(paths, DesktopBackgroundPixmap, bColor);

        if (back != null) {
            bPixmap = back->pixmap();
            XSetWindowBackgroundPixmap(xapp->display(), desktop->handle(),
                                       bPixmap);
            currentBackground = back;
            handleBackground = true;
        }
    } else if (DesktopBackgroundColor && DesktopBackgroundColor[0]) {
        XSetWindowBackgroundPixmap(xapp->display(), desktop->handle(), 0);
        XSetWindowBackground(xapp->display(), desktop->handle(), bPixel);
        handleBackground = true;
    }

    if (handleBackground) {
#ifndef NO_CONFIGURE
        if (supportSemitransparency &&
            _XA_XROOTPMAP_ID && _XA_XROOTCOLOR_PIXEL)
        {
            if (DesktopBackgroundPixmap &&
                DesktopTransparencyPixmap &&
                !strcmp (DesktopBackgroundPixmap,
                         DesktopTransparencyPixmap)) {
                delete[] DesktopTransparencyPixmap;
                DesktopTransparencyPixmap = NULL;
            }

            YColor * tColor(DesktopTransparencyColor &&
                            DesktopTransparencyColor[0]
                            ? new YColor(DesktopTransparencyColor)
                            : bColor);

            ref<YPixmap> root =
                DesktopTransparencyPixmap &&
                DesktopTransparencyPixmap[0] != 0
                ? renderBackground(paths, DesktopTransparencyPixmap,
                                   tColor) : null;
            if (root != null) currentBackground = root;

            unsigned long const tPixel(tColor->pixel());
            Pixmap const tPixmap(root != null ? root->pixmap() : bPixmap);

            XChangeProperty(xapp->display(), desktop->handle(),
                            _XA_XROOTPMAP_ID, XA_PIXMAP, 32,
                            PropModeReplace, (unsigned char const*)&tPixmap, 1);
            XChangeProperty(xapp->display(), desktop->handle(),
                            _XA_XROOTCOLOR_PIXEL, XA_CARDINAL, 32,
                            PropModeReplace, (unsigned char const*)&tPixel, 1);
        }
#endif
    }
#endif
    XClearWindow(xapp->display(), desktop->handle());
    XFlush(xapp->display());
    //    if (backgroundPixmaps.getCount() <= 1)
#ifndef NO_CONFIGURE
    if (!supportSemitransparency) {
        exit(0);
    }
#endif
}

bool DesktopBackgroundManager::filterEvent(const XEvent &xev) {
    if (xev.type == PropertyNotify) {
/// TODO #warning "leak needs to be fixed when multiple background desktops are enabled again"
#if 0
        if (xev.xproperty.window == desktop->handle() &&
            xev.xproperty.atom == _XA_NET_CURRENT_DESKTOP)
        {
            update();
        }
#endif
    } else if (xev.type == ClientMessage) {
        if (xev.xclient.window == desktop->handle() &&
            xev.xproperty.atom == _XA_ICEWMBG_QUIT)
        {
            exit(0);
        }
        if (xev.xclient.window == desktop->handle() &&
            xev.xproperty.atom == _XA_ICEWMBG_RESTART)
        {
            execlp(ICEWMBGEXE, ICEWMBGEXE, (void *)NULL);
        }
    }

    return YXApplication::filterEvent(xev);
}

void DesktopBackgroundManager::sendQuit() {
    XClientMessageEvent xev;

    memset(&xev, 0, sizeof(xev));
    xev.type = ClientMessage;
    xev.window = desktop->handle();
    xev.message_type = _XA_ICEWMBG_QUIT;
    xev.format = 32;
    xev.data.l[0] = getpid();
    XSendEvent(xapp->display(), desktop->handle(), False, StructureNotifyMask, (XEvent *) &xev);
    XSync(xapp->display(), False);
}

void DesktopBackgroundManager::sendRestart() {
    XClientMessageEvent xev;

    memset(&xev, 0, sizeof(xev));
    xev.type = ClientMessage;
    xev.window = desktop->handle();
    xev.message_type = _XA_ICEWMBG_RESTART;
    xev.format = 32;
    xev.data.l[0] = getpid();
    XSendEvent(xapp->display(), desktop->handle(), False, StructureNotifyMask, (XEvent *) &xev);
    XSync(xapp->display(), False);
}

void printUsage(int rc = 1) {
    fputs (_("Usage: icewmbg [ -r | -q ]\n"
             " -r  Restart icewmbg\n"
             " -q  Quit icewmbg\n"
             "Loads desktop background according to preferences file\n"
             " DesktopBackgroundCenter  - Display desktop background centered, not tiled\n"
             " SupportSemitransparency  - Support for semitransparent terminals\n"
             " DesktopBackgroundColor   - Desktop background color\n"
             " DesktopBackgroundImage   - Desktop background image\n"
             " DesktopTransparencyColor - Color to announce for semi-transparent windows\n"
             " DesktopTransparencyImage - Image to announce for semi-transparent windows\n"),
           stderr);
    exit(rc);
}

void invalidArgument(const char *appName, const char *arg) {
    fprintf(stderr, _("%s: unrecognized option `%s'\n"
                      "Try `%s --help' for more information.\n"),
            appName, arg, appName);
    exit(1);
}

DesktopBackgroundManager *bg;

void addBgImage(const char */*name*/, const char *value, bool) {
    bg->addImage(value);
}

int main(int argc, char **argv) {
    ApplicationName = my_basename(*argv);

    nice(5);

#if 0
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
#endif

    bg = new DesktopBackgroundManager(&argc, &argv);

    if (argc > 1) {
        if ( strcmp(argv[1], "-r") == 0) {
            bg->sendRestart();
            return 0;
        } else if (strcmp(argv[1], "-q") == 0) {
            bg->sendQuit();
            return 0;
        } else
            printUsage();
    }

#ifndef NO_CONFIGURE
    {
        cfoption theme_prefs[] = {
            OSV("Theme", &themeName, "Theme name"),
            OK0()
        };

        YConfig::findLoadConfigFile(theme_prefs, "preferences");
        YConfig::findLoadConfigFile(theme_prefs, "theme");
    }
    YConfig::findLoadConfigFile(icewmbg_prefs, "preferences");
    if (themeName != 0) {
        MSG(("themeName=%s", themeName));

        YConfig::findLoadConfigFile(icewmbg_prefs, upath("themes").child(themeName));
    }
    YConfig::findLoadConfigFile(icewmbg_prefs, "prefoverride");
#endif

#if 0
    {
        char *configFile = 0;

        if (configFile == 0)
            configFile = app->findConfigFile("preferences");
        if (configFile)
            loadConfig(icewmbg_prefs, configFile);
        delete configFile; configFile = 0;

        if (themeName) {
            if (*themeName == '/')
                loadConfig(icewmbg_prefs, themeName);
            else {
                char *theme(strJoin("themes/", themeName, NULL));
                char *themePath(app->findConfigFile(theme));

                if (themePath)
                    loadConfig(icewmbg_prefs, themePath);

                delete[] themePath;
                delete[] theme;
            }
        }
    }
#endif

    ///XSelectInput(app->display(), desktop->handle(), PropertyChangeMask);
    bg->update();

    return bg->mainLoop();
}


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#if 0
Pixmap loadPixmap(const char *filename) {
    Pixmap pixmap = 0;
#ifdef CONFIG_IMLIB
    if (!hImlib) hImlib = Imlib_init(display);

    ImlibImage *im = Imlib_load_image(hImlib, (char *)filename);
    if (im) {
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

#if 0
#ifdef CONFIG_IMLIB
#include <Imlib.h>

static ImlibData *hImlib = 0;
#else
#include <X11/xpm.h>
#endif
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

///#warning duplicates lots of prefs
///#include "default.h"
///#include "wmconfig.h"


