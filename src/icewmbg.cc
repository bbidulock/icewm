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
#include "appnames.h"

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

    Atom _XA_NET_WORKAREA;
    int desktopWidth, desktopHeight;
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

    desktopWidth = desktop->width();
    desktopHeight = desktop->height();
    _XA_NET_WORKAREA = XInternAtom(xapp->display(), "_NET_WORKAREA", False);
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
    ref<YPixmap> scaled = null;

    if (*filename == '/') {
        if (access(filename, R_OK) == 0)
            back = YPixmap::load(filename);
    } else
        back = paths->loadPixmap(0, filename);

        //printf("Background: center %d scaled %d\n", centerBackground, desktopBackgroundScaled);

#ifndef NO_CONFIGURE
    if (back != null) {
        ref<YPixmap> cBack = YPixmap::create(desktop->width(), desktop->height());
        Graphics g(cBack, 0, 0);

        g.setColor(color);
        g.fillRect(0, 0, desktop->width(), desktop->height());

        if (centerBackground || desktopBackgroundScaled) {
            int x, y, w, h, zerowidth = 0, zeroheight = 0;
	    for (int i = 0; i < desktop->getScreenCount(); i++) {
                desktop->getScreenGeometry(&x, &y, &w, &h, i);
                /* WA: only draw biggest image on coordinate (0,0) */
                if (x == 0 && y == 0) {
                    zerowidth < w  ? zerowidth = w  : w = zerowidth;
                    zeroheight < h ? zeroheight = h : h = zeroheight;
                }
                //printf("geometry%d: x %d y %d w %d h %d\n", i, x, y, w, h);
                int bw, bh;
                if (desktopBackgroundScaled && !centerBackground) {
                    bw = w;
                    bh = h;
                } else {
                    bw = back->width();
                    bh = back->height();
                    if (bw >= bh && (bw > w || desktopBackgroundScaled)) {
                        bh = (double)w*(double)bh/(double)bw;
                        bw = w;
                    } else if (bh > h || desktopBackgroundScaled) {
                        bw = (double)h*(double)bw/(double)bh;
                        bh = h;
                    }
                }
                if (bw != back->width() || bh != back->height()) {
                    //printf("scale\n");
                    scaled = back->scale(bw,bh);
                }
                g.drawPixmap(scaled != null ? scaled : back,
                             x + (w - (scaled != null ? scaled->width()  : back->width())) / 2,
                             y + (h - (scaled != null ? scaled->height() : back->height())) / 2);
                scaled = null;
	    }
        } else {
            g.fillPixmap(back, 0, 0, desktop->width(), desktop->height(), 0, 0);
        }

        back = cBack;
    }
#endif
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
    }
    if (!handleBackground && (DesktopBackgroundColor && DesktopBackgroundColor[0])) {
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
        if (xev.xproperty.window == desktop->handle() &&
            xev.xproperty.atom == _XA_NET_WORKAREA &&
            (desktopWidth != desktop->width() || desktopHeight != desktop->height()))
        {
            int w, h;
            desktop->updateXineramaInfo(w, h);
            desktopWidth = w;
            desktopHeight= h;
            update();
        }
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

static const char* get_help_text() {
    return _(
    "Usage: icewmbg [OPTIONS]\n"
    "\n"
    "Options:\n"
    "  -r, --restart       Restart icewmbg\n"
    "  -q, --quit          Quit icewmbg\n"
    "\n"
    "  -c, --config=FILE   Load preferences from FILE.\n"
    "  -t, --theme=FILE    Load theme from FILE.\n"
    "\n"
    "  --display=NAME      Use NAME to connect to the X server.\n"
    "  --sync              Synchronize communication with X11 server.\n"
    "\n"
    "  -h, --help          Print this usage screen and exit.\n"
    "  -V, --version       Prints version information and exits.\n"
    "\n"
    "Loads desktop background according to preferences file:\n"
    " DesktopBackgroundCenter  - Display desktop background centered\n"
    " DesktopBackgroundScaled  - Display desktop background scaled\n"
    " SupportSemitransparency  - Support for semitransparent terminals\n"
    " DesktopBackgroundColor   - Desktop background color\n"
    " DesktopBackgroundImage   - Desktop background image\n"
    " DesktopTransparencyColor - Color to announce for semi-transparent windows\n"
    " DesktopTransparencyImage - Image to announce for semi-transparent windows\n"
    "\n"
    " center:0 scaled:0 = tiled\n"
    " center:1 scaled:1 = keep aspect ratio\n"
    "\n");
}

static void print_help_xit() {
    fputs(get_help_text(), stderr);
    exit(1);
}

static DesktopBackgroundManager *bg;

void addBgImage(const char * /*name*/, const char *value, bool) {
    bg->addImage(value);
}

int main(int argc, char **argv) {
    ApplicationName = my_basename(*argv);

    bool sendRestart = false;
    bool sendQuit = false;
    const char *overrideTheme = 0;
    const char *configFile = 0;

    for (char **arg = argv + 1; arg < argv + argc; ++arg) {
        if (**arg == '-') {
            char *value(0);
            if (is_switch(*arg, "r", "restart")) {
                sendRestart = true;
            }
            else if (is_switch(*arg, "q", "quit")) {
                sendQuit = true;
            }
            else if (is_help_switch(*arg)) {
                print_help_xit();
            }
            else if (is_version_switch(*arg)) {
                print_version_exit(VERSION);
            }
            else if (GetLongArgument(value, "theme", argv, argv + argc)
                ||  GetShortArgument(value, "t", argv, argv + argc)) {
                overrideTheme = value;
            }
            else if (GetLongArgument(value, "config", argv, argv + argc)
                ||  GetShortArgument(value, "c", argv, argv + argc)) {
                configFile = value;
            }
        }
    }

    nice(5);

    bg = new DesktopBackgroundManager(&argc, &argv);

    if (sendRestart) {
        bg->sendRestart();
        delete bg;
        return 0;
    }
    else if (sendQuit) {
        bg->sendQuit();
        delete bg;
        return 0;
    }

#ifndef NO_CONFIGURE
    if (configFile == 0 || *configFile == 0)
        configFile = "preferences";
    if (overrideTheme && *overrideTheme)
        themeName = overrideTheme;
    else
    {
        cfoption theme_prefs[] = {
            OSV("Theme", &themeName, "Theme name"),
            OK0()
        };

        YConfig::findLoadConfigFile(bg, theme_prefs, configFile);
        YConfig::findLoadConfigFile(bg, theme_prefs, "theme");
    }
    YConfig::findLoadConfigFile(bg, icewmbg_prefs, configFile);
    if (themeName != 0) {
        MSG(("themeName=%s", themeName));

        YConfig::findLoadThemeFile(bg, icewmbg_prefs,
                upath("themes").child(themeName));
    }
    YConfig::findLoadConfigFile(bg, icewmbg_prefs, "prefoverride");
#endif

    bg->update();

    return bg->mainLoop();
}

