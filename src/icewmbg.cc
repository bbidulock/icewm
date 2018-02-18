#include "config.h"

#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "yfull.h"
#include "yxapp.h"
#include "yprefs.h"
#include "ypaths.h"
#include "udir.h"
#include "intl.h"
#include "appnames.h"

#define CFGDEF
#include "icewmbg_prefs.h"

char const *ApplicationName = NULL;

// used in YXFont but declared only in icewm sources, not in icewmbg special set.
// Declaring it here for now.
XIV(bool, fontPreferFreetype,                   true)

class PixFile {
public:
    typedef ref<YResourcePaths> Paths;
    typedef ref<YPixmap> Pixmap;
private:
    mstring name;
    Pixmap pix;
    time_t last, check;
public:
    explicit PixFile(mstring file, Pixmap pixmap = null)
        : name(file), pix(pixmap), last(0L), check(0L)
    {
    }
    ~PixFile() {
        name = null;
        pix = null;
    }
    mstring file() const { return name; }
    Pixmap pixmap(Paths paths) {
        time_t now = time(NULL);
        if (pix == null) {
            if (last == 0 || last + 2 < now) {
                pix = load(paths);
                last = check = now;
            }
        }
        else if (check + 10 < now) {
            check = now;
            upath path(name);
            struct stat st;
            if (path.stat(&st) == 0) {
                if (last < st.st_mtime) {
                    Pixmap tmp = load(paths);
                    if (tmp != null) {
                        pix = tmp;
                        last = now;
                    }
                }
            }
        }
        return pix;
    }
    Pixmap load(Paths paths) {
        Pixmap image;
        upath path(name);
        if (false == path.isAbsolute()) {
            image = paths->loadPixmap(null, path);
        }
        if (image == null) {
            image = YPixmap::load(path);
        }
        if (image == null) {
            tlog(_("Failed to load image '%s'."), cstring(name).c_str());
        }
        return image;
    }
};

class PixCache {
public:
    typedef ref<YResourcePaths> Paths;
    typedef ref<YPixmap> Pixmap;
private:
    Paths paths;
    YObjectArray<PixFile> pixes;
    int find(mstring name) {
        for (int i = 0; i < pixes.getCount(); ++i) {
            if (name == pixes[i]->file()) {
                return i;
            }
        }
        return -1;
    }
    Paths getPaths() {
        if (paths == null) {
            paths = YResourcePaths::subdirs(null);
        }
        return paths;
    }
public:
    ~PixCache() {
        clear();
        paths = null;
    }
    void add(mstring file, Pixmap pixmap = null) {
        if (-1 == find(file)) {
            pixes.append(new PixFile(file, pixmap));
        }
    }
    Pixmap get(mstring name) {
        int k = find(name);
        return k == -1 ? null : pixes[k]->pixmap(getPaths());
    }
    void clear() {
        pixes.clear();
        paths = null;
    }
};

class Background: public YXApplication {
public:
    Background(int *argc, char ***argv, bool verbose = false);
    ~Background();

    void add(const char* name, const char* value, bool append);
    bool become(bool replace = false);
    void update(bool force = false);
    void sendQuit()    { sendClientMessage(_XA_ICEWMBG_QUIT);    }
    void sendRestart() { sendClientMessage(_XA_ICEWMBG_RESTART); }

    void clearBackgroundImages()   { backgroundImages.clear();   }
    void clearBackgroundColors()   { backgroundColors.clear();   }
    void clearTransparencyImages() { transparencyImages.clear(); }
    void clearTransparencyColors() { transparencyColors.clear(); }
    void enableSemitransparency(bool enable = true) {
        supportSemitransparency = enable;
    }

private:
    virtual bool filterEvent(const XEvent& xev);
    virtual void handleSignal(int sig);

    void restart() const;
    void changeBackground(bool force);
    void sendClientMessage(Atom message) const;
    long* getLongProperties(Atom property, int n) const;
    int getLongProperty(Atom property) const;
    int getWorkspace() const;
    void getDesktopGeometry();
    int getBgPid() const {
        return getLongProperty(_XA_ICEWMBG_PID);
    }
    void deleteProperty(Atom property) const {
        XDeleteProperty(display(), window(), property);
    }
    void changeProperty(Atom property, int format, unsigned char* data) const {
        XChangeProperty(display(), window(), property, format,
                        32, PropModeReplace, data, 1);
    }
    void changeLongProperty(Atom property, unsigned char* data) const {
        changeProperty(property, XA_CARDINAL, data);
    }

    typedef YObjectArray<mstring> Strings;
    typedef YArray<YColor> YColors;

    ref<YPixmap> renderBackground(ref<YPixmap> back, YColor color);
    ref<YPixmap> getBackgroundPixmap();
    YColor getBackgroundColor();
    ref<YPixmap> getTransparencyPixmap();
    YColor getTransparencyColor();
    Atom atom(const char* name) const;
    static Window window() { return desktop->handle(); }

private:
    char** mainArgv;
    bool verbose;
    Strings backgroundImages;
    YColors backgroundColors;
    Strings transparencyImages;
    YColors transparencyColors;
    PixCache cache;
    int activeWorkspace;
    unsigned desktopWidth, desktopHeight;
    ref<YPixmap> currentBackgroundPixmap;
    ref<YPixmap> currentTransparencyPixmap;
    YColor currentBackgroundColor;

    Atom _XA_XROOTPMAP_ID;
    Atom _XA_XROOTCOLOR_PIXEL;
    Atom _XA_NET_CURRENT_DESKTOP;
    Atom _XA_NET_DESKTOP_GEOMETRY;
    Atom _XA_NET_WORKAREA;
    Atom _XA_ICEWMBG_QUIT;
    Atom _XA_ICEWMBG_RESTART;
    Atom _XA_ICEWMBG_PID;
};

Background::Background(int *argc, char ***argv, bool verb):
    YXApplication(argc, argv),
    mainArgv(*argv),
    verbose(verb),
    activeWorkspace(0),
    desktopWidth(desktop->width()),
    desktopHeight(desktop->height()),
    currentBackgroundColor(),
    _XA_XROOTPMAP_ID(atom("_XROOTPMAP_ID")),
    _XA_XROOTCOLOR_PIXEL(atom("_XROOTCOLOR_PIXEL")),
    _XA_NET_CURRENT_DESKTOP(atom("_NET_CURRENT_DESKTOP")),
    _XA_NET_DESKTOP_GEOMETRY(atom("_NET_DESKTOP_GEOMETRY")),
    _XA_NET_WORKAREA(atom("_NET_WORKAREA")),
    _XA_ICEWMBG_QUIT(atom("_ICEWMBG_QUIT")),
    _XA_ICEWMBG_RESTART(atom("_ICEWMBG_RESTART"))
{
    char abuf[42];
    snprintf(abuf, sizeof abuf, "_ICEWMBG_PID_S%d", xapp->screen());
    _XA_ICEWMBG_PID = atom(abuf);

    desktop->setStyle(YWindow::wsDesktopAware);

    catchSignal(SIGTERM);
    catchSignal(SIGINT);
    catchSignal(SIGQUIT);
    catchSignal(SIGUSR2);
}

Background::~Background() {
    int pid = getBgPid();
    if (supportSemitransparency) {
        if (pid <= 0 || pid == getpid()) {
            if (_XA_XROOTPMAP_ID)
                deleteProperty(_XA_XROOTPMAP_ID);
            if (_XA_XROOTCOLOR_PIXEL)
                deleteProperty(_XA_XROOTCOLOR_PIXEL);
        }
    }
    if (pid == getpid()) {
        deleteProperty(_XA_ICEWMBG_PID);
    }
    clearBackgroundImages();
    clearBackgroundColors();
    clearTransparencyImages();
    clearTransparencyColors();
    cache.clear();
}

Atom Background::atom(const char* name) const {
    return XInternAtom(display(), name, False);
}

static bool hasImageExt(const upath& path) {
    mstring ext(path.getExtension());
    return ext.length() == 4 || ext.length() == 5;
}

void Background::add(const char* name, const char* value, bool append) {
    if (value == 0 || *value == 0) {
        warn("Empty value for '%s'.", name);
    }
    else if (0 == strcmp(name, "DesktopBackgroundImage")) {
        if (append == false) {
            clearBackgroundImages();
        }
        if (backgroundImages.getCount() < MAX_WORKSPACES) {
            mstring name(value);
            upath path(name);
            if (path.dirExists()) {
                for (sdir dir(path); dir.next(); ) {
                    if (backgroundImages.getCount() < MAX_WORKSPACES) {
                        upath image(path + dir.entry());
                        if (hasImageExt(image)) {
                            backgroundImages.append(new mstring(image));
                            cache.add(image);
                        }
                    }
                }
            } else {
                backgroundImages.append(new mstring(name));
                cache.add(name);
            }
        }
    }
    else if (0 == strcmp(name, "DesktopBackgroundColor")) {
        if (append == false) {
            clearBackgroundColors();
        }
        if (backgroundColors.getCount() < MAX_WORKSPACES) {
            backgroundColors.append(YColor(value));
        }
    }
    else if (0 == strcmp(name, "DesktopTransparencyImage")) {
        if (append == false) {
            clearTransparencyImages();
        }
        if (transparencyImages.getCount() < MAX_WORKSPACES) {
            mstring name(value);
            upath path(name);
            if (path.dirExists()) {
                for (sdir dir(path); dir.next(); ) {
                    if (transparencyImages.getCount() < MAX_WORKSPACES) {
                        upath image(path + dir.entry());
                        if (hasImageExt(image)) {
                            transparencyImages.append(new mstring(image));
                            cache.add(image);
                        }
                    }
                }
            } else {
                transparencyImages.append(new mstring(name));
                cache.add(name);
            }
        }
    }
    else if (0 == strcmp(name, "DesktopTransparencyColor")) {
        if (append == false) {
            clearTransparencyColors();
        }
        if (transparencyColors.getCount() < MAX_WORKSPACES) {
            transparencyColors.append(YColor(value));
        }
    }
    else {
        warn("Unknown name '%s'.", name);
    }
}

void Background::handleSignal(int sig) {
    switch (sig) {
    case SIGINT:
    case SIGTERM:
    case SIGQUIT:
        this->exit(1);
        break;

    case SIGUSR2:
        tlog("logEvents %s", boolstr(toggleLogEvents()));
        break;

    default:
        YApplication::handleSignal(sig);
        break;
    }
}

ref<YPixmap> Background::getBackgroundPixmap() {
    ref<YPixmap> pixmap;
    int count = backgroundImages.getCount();
    if (count > 0 && activeWorkspace >= 0) {
        for (int i = 0; i < count; ++i) {
            int k = (i + activeWorkspace) % count;
            pixmap = cache.get(*backgroundImages[k]);
            if (pixmap != null)
                break;
        }
    }
    return pixmap;
}

YColor Background::getBackgroundColor() {
    int count = backgroundColors.getCount();
    return count > 0
        ? backgroundColors[activeWorkspace % count]
        : YColor::black;
}

ref<YPixmap> Background::getTransparencyPixmap() {
    ref<YPixmap> pixmap;
    int count = transparencyImages.getCount();
    if (count > 0 && activeWorkspace >= 0) {
        for (int i = 0; i < count; ++i) {
            int k = (i + activeWorkspace) % count;
            pixmap = cache.get(*transparencyImages[k]);
            if (pixmap != null)
                break;
        }
    }
    return pixmap;
}

YColor Background::getTransparencyColor() {
    int count = transparencyColors.getCount();
    return count > 0
        ? transparencyColors[activeWorkspace % count]
        : getBackgroundColor();
}

void Background::update(bool force) {
    activeWorkspace = getWorkspace();
    if (verbose) tlog("update %s %d", boolstr(force), activeWorkspace);
    changeBackground(force);
}

long* Background::getLongProperties(Atom property, int n) const {
    Atom r_type;
    int r_format;
    unsigned long nitems, lbytes;
    unsigned char *prop(0);

    if (XGetWindowProperty(display(), window(), property,
                           0, n, False, XA_CARDINAL,
                           &r_type, &r_format,
                           &nitems, &lbytes, &prop) == Success && prop)
    {
        if (r_type == XA_CARDINAL && r_format == 32 &&
                n >= 0 && nitems == (unsigned long) n) {
            return (long *) prop;
        }
        XFree(prop);
    }
    return 0;
}

int Background::getLongProperty(Atom property) const {
    long* prop = getLongProperties(property, 1);
    int value = prop ? (int) *prop : 0;
    XFree(prop);
    return value;
}

void Background::getDesktopGeometry() {
    long* prop = getLongProperties(_XA_NET_DESKTOP_GEOMETRY, 2);
    if (prop) {
        desktopWidth = (int) prop[0];
        desktopHeight = (int) prop[1];
        XFree(prop);
    }
}

int Background::getWorkspace() const {
    return getLongProperty(_XA_NET_CURRENT_DESKTOP);
}

ref<YPixmap> Background::renderBackground(ref<YPixmap> back, YColor color) {
    if (verbose) tlog("rendering...");
    unsigned width = desktopWidth;
    unsigned height = desktopHeight;
    if (back == null || width == 0 || height == 0) {
        if (verbose) tlog("%s null return", __func__);
        return back;
    }

    int numScreens = multiheadBackground ? 1 : desktop->getScreenCount();
    if (numScreens == 1) {
        if (width == back->width() && height == back->height()) {
            if (verbose) tlog("%s numScreens == 1 return", __func__);
            return back;
        }
    }

    ref<YPixmap> cBack = YPixmap::create(width, height, xapp->depth());
    Graphics g(cBack, 0, 0);
    g.setColor(color);
    g.fillRect(0, 0, width, height);

    if (centerBackground == false && scaleBackground == false) {
        g.fillPixmap(back, 0, 0, width, height, 0, 0);
        back = cBack;
        if (verbose) tlog("%s fillPixmap false return", __func__);
        return back;
    }

    unsigned zeroWidth(0), zeroHeight(0);
    if (numScreens > 1) {
        for (int screen = 0; screen < numScreens; ++screen) {
            int x(0), y(0);
            unsigned w(0), h(0);
            desktop->getScreenGeometry(&x, &y, &w, &h, screen);
            if (x == 0 && y == 0) {
                if (zeroWidth < w)
                    zeroWidth = w;
                if (zeroHeight < h)
                    zeroHeight = h;
            }
        }
    } else {
        zeroWidth = width;
        zeroHeight = height;
    }
    bool zeroDone(false);

    for (int screen = 0; screen < numScreens; ++screen) {
        int x(0), y(0);
        if (numScreens > 1) {
            desktop->getScreenGeometry(&x, &y, &width, &height, screen);
            if (x == 0 && y == 0) {
                if (zeroDone) {
                    continue;
                }
                else {
                    zeroDone = true;
                    width = zeroWidth;
                    height = zeroHeight;
                }
            }
        }
        if (verbose) tlog("%s (%d, %d) (%dx%d+%d+%d)", __func__,
                desktopWidth, desktopHeight, width, height, x, y);
        unsigned bw = back->width();
        unsigned bh = back->height();
        if (scaleBackground && false == centerBackground) {
            if (bh * width < bw * height) {
                bw = height * bw / bh;
                bh = height;
            } else {
                bh = width * bh / bw;
                bw = width;
            }
        }
        else if (centerBackground) {
            if (bw >= bh && (bw > width || scaleBackground)) {
                bh = width * bh / bw;
                bw = width;
            } else if (bh > height || scaleBackground) {
                bw = height * bw / bh;
                bh = height;
            }
        }

        ref<YPixmap> scaled;
        if (bw != back->width() || bh != back->height()) {
            scaled = back->scale(bw, bh);
        }
        if (scaled == null) {
            scaled = back;
        }
        g.drawPixmap(scaled,
                     max(0, int(bw - width) / 2),
                     max(0, int(bh - height) / 2),
                     min(width, bw),
                     min(height, bh),
                     max(x, x + int(width - bw) / 2),
                     max(y, y + int(height - bh) / 2));
    }
    back = cBack;

    return back;
}

void Background::changeBackground(bool force) {
    ref<YPixmap> backgroundPixmap = getBackgroundPixmap();
    YColor backgroundColor(getBackgroundColor());
    if (force == false) {
        if (backgroundPixmap == currentBackgroundPixmap &&
            backgroundColor == currentBackgroundColor) {
            return;
        }
    }
    unsigned long const bPixel(backgroundColor.pixel());
    bool handleBackground(false);
    Pixmap bPixmap(None);
    ref<YPixmap> back = renderBackground(backgroundPixmap, backgroundColor);

    if (back != null) {
        bPixmap = back->pixmap();
        XSetWindowBackgroundPixmap(display(), window(), bPixmap);
        handleBackground = true;
    }
    if (false == handleBackground) {
        XSetWindowBackgroundPixmap(display(), window(), None);
        XSetWindowBackground(display(), window(), bPixel);
        handleBackground = true;
    }

    if (handleBackground) {
        if (supportSemitransparency &&
            _XA_XROOTPMAP_ID &&
            _XA_XROOTCOLOR_PIXEL)
        {
            YColor tColor(getTransparencyColor());
            ref<YPixmap> trans = getTransparencyPixmap();
            bool samePixmap = trans == backgroundPixmap;
            bool sameColor = tColor == backgroundColor;
            currentTransparencyPixmap =
                trans == null || (samePixmap && sameColor) ? back :
                renderBackground(trans, tColor);

            unsigned long tPixel(tColor.pixel());
            Pixmap tPixmap(currentTransparencyPixmap != null
                           ? currentTransparencyPixmap->pixmap()
                           : bPixmap);

            changeProperty(_XA_XROOTPMAP_ID, XA_PIXMAP,
                           (unsigned char *) &tPixmap);
            changeProperty(_XA_XROOTCOLOR_PIXEL, XA_CARDINAL,
                           (unsigned char *) &tPixel);
        }
    }
    XClearWindow(display(), window());
    XFlush(display());
    if (false == supportSemitransparency &&
        backgroundImages.getCount() <= 1 &&
        backgroundColors.getCount() <= 1)
    {
        this->exit(0);
    }
    currentBackgroundPixmap = backgroundPixmap;
    currentBackgroundColor = backgroundColor;
}

void Background::restart() const {
    if (mainArgv[0][0] == '/' ||
        (strchr(mainArgv[0], '/') != 0 &&
         access(mainArgv[0], X_OK) == 0))
    {
        execv(mainArgv[0], mainArgv);
        fail("execv %s", mainArgv[0]);
    }
    execvp(ICEWMBGEXE, mainArgv);
    fail("execlp %s", ICEWMBGEXE);
}

bool Background::filterEvent(const XEvent &xev) {
    if (xev.xany.window != window()) {
        /*ignore*/;
    }
    else if (xev.type == PropertyNotify) {
        if (xev.xproperty.atom == _XA_NET_CURRENT_DESKTOP) {
            update();
        }
        if (xev.xproperty.atom == _XA_NET_DESKTOP_GEOMETRY) {
            unsigned w = desktopWidth, h = desktopHeight;
            getDesktopGeometry();
            update(w != desktopWidth || h != desktopHeight);
        }
        if (xev.xproperty.atom == _XA_NET_WORKAREA) {
            unsigned w = desktopWidth, h = desktopHeight;
            desktop->updateXineramaInfo(desktopWidth, desktopHeight);
            update(w != desktopWidth || h != desktopHeight);
        }
        if (xev.xproperty.atom == _XA_ICEWMBG_PID) {
            int pid = getBgPid();
            if (pid > 0 && pid != getpid()) {
                this->exit(0);
            }
            return true;
        }
    }
    else if (xev.type == ClientMessage) {
        if (xev.xclient.message_type == _XA_ICEWMBG_QUIT) {
            if (xev.xclient.data.l[0] != getpid()) {
                this->exit(0);
            }
            return true;
        }
        if (xev.xclient.message_type == _XA_ICEWMBG_RESTART) {
            restart();
            return true;
        }
    }

    return YXApplication::filterEvent(xev);
}

void Background::sendClientMessage(Atom message) const {
    XClientMessageEvent xev = {};
    xev.type = ClientMessage;
    xev.window = window();
    xev.message_type = message;
    xev.format = 32;
    xev.data.l[0] = getpid();
    XSendEvent(display(), window(), False,
               StructureNotifyMask, (XEvent *) &xev);
    XSync(display(), False);
}

bool Background::become(bool replace) {
    long mypid = (long) getpid();
    int pid = getBgPid();

    if (pid > 0 && pid != mypid && (replace || kill(pid, 0) != 0)) {
        sendQuit();
        int waited = 0;
        int period = 1000;
        int atmost = 1000*1000;
        while (pid > 0 && pid != mypid && waited < atmost) {
            if (kill(pid, 0) != 0) {
                pid = 0;
            } else {
                usleep(period);
                waited += period;
                period += 1000;
            }
        }
    }
    if (pid <= 0) {
        changeLongProperty(_XA_ICEWMBG_PID, (unsigned char *) &mypid);
        XSync(display(), False);
        pid = (int) mypid;
    }

    return pid == (int) mypid;
}

static const char* get_help_text() {
    return _(
    "Usage: icewmbg [OPTIONS]\n"
    "Where multiple values can be given they are separated by commas.\n"
    "When image is a directory then all images from that directory are used.\n"
    "\n"
    "Options:\n"
    "  -p, --replace        Replace an existing icewmbg.\n"
    "  -q, --quit           Tell the running icewmbg to quit.\n"
    "  -r, --restart        Tell the running icewmbg to restart itself.\n"
    "\n"
    "  -c, --config=FILE    Load preferences from FILE.\n"
    "  -t, --theme=NAME     Load the theme with name NAME.\n"
    "\n"
    "  -i, --image=FILE(S)  Load background image(s) from FILE(S).\n"
    "  -k, --color=NAME(S)  Use background color(s) from NAME(S).\n"
    "\n"
    "  -s, --semis=FILE(S)  Load transparency image(s) from FILE(S).\n"
    "  -x, --trans=NAME(S)  Use transparency color(s) from NAME(S).\n"
    "\n"
    "  -e, --center=0/1     Disable/Enable centering background.\n"
    "  -a, --scaled=0/1     Disable/Enable scaling background.\n"
    "  -m, --multi=0/1      Disable/Enable multihead background.\n"
    "\n"
    "  --display=NAME       Use NAME to connect to the X server.\n"
    "  --sync               Synchronize communication with X11 server.\n"
    "\n"
    "  -h, --help           Print this usage screen and exit.\n"
    "  -V, --version        Prints version information and exits.\n"
    "\n"
    "Loads desktop background according to preferences file:\n"
    " DesktopBackgroundCenter  - Display desktop background centered\n"
    " DesktopBackgroundScaled  - Display desktop background scaled\n"
    " DesktopBackgroundColor   - Desktop background color(s)\n"
    " DesktopBackgroundImage   - Desktop background image(s)\n"
    " SupportSemitransparency  - Support for semitransparent terminals\n"
    " DesktopTransparencyColor - Semitransparency background color(s)\n"
    " DesktopTransparencyImage - Semitransparency background image(s)\n"
    " DesktopBackgroundMultihead - One background over all monitors\n"
    "\n"
    " center:0 scaled:0 = tiled\n"
    " center:1 scaled:0 = centered\n"
    " center:1 scaled:1 = fill one dimension and keep aspect ratio\n"
    " center:0 scaled:1 = fill both dimensions and keep aspect ratio\n"
    "\n");
}

static void print_help_xit() {
    fputs(get_help_text(), stdout);
    exit(0);
}

static Background *globalBg;

void addBgImage(const char* name, const char* value, bool append) {
    if (globalBg) {
        globalBg->add(name, value, append);
    } else {
        warn("null globalBg");
    }
}

static void bgLoadConfig(const char *overrideTheme, const char *configFile)
{
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

        YConfig::findLoadConfigFile(globalBg, theme_prefs, configFile);
        YConfig::findLoadConfigFile(globalBg, theme_prefs, "theme");
    }
    YConfig::findLoadConfigFile(globalBg, icewmbg_prefs, configFile);
    if (themeName != 0) {
        MSG(("themeName=%s", themeName));

        YConfig::findLoadThemeFile(globalBg, icewmbg_prefs,
                upath("themes").child(themeName));
    }
    YConfig::findLoadConfigFile(globalBg, icewmbg_prefs, "prefoverride");
}

static void bgParse(const char* name, const char* value) {
    mstring opt, str(value);
    for (int i = 0; str.splitall(',', &opt, &str); ++i) {
        if (i + 1 < MAX_WORKSPACES) {
            if ((opt = opt.trim()).nonempty()) {
                globalBg->add(name, cstring(opt), 0 < i);
            }
        }
    }
}

static void testFlag(bool* flag, const char* value) {
    if (value && *value &&
            (value[1] == 0 || (*value == '=' && (++value)[1] == 0)))
    {
        if (*value == '0') *flag = false;
        if (*value == '1') *flag = true;
    }
}

int main(int argc, char **argv) {
    ApplicationName = my_basename(*argv);

    bool sendRestart = false;
    bool sendQuit = false;
    bool replace = false;
    bool verbose = false;
    const char* overrideTheme = 0;
    const char* configFile = 0;
    const char* image = 0;
    const char* color = 0;
    const char* trans = 0;
    const char* semis = 0;
    const char* center = 0;
    const char* scaled = 0;
    const char* multi = 0;

    for (char **arg = argv + 1; arg < argv + argc; ++arg) {
        if (**arg == '-') {
            char *value(0);
            if (is_switch(*arg, "r", "restart")) {
                sendRestart = true;
            }
            else if (is_switch(*arg, "q", "quit")) {
                sendQuit = true;
            }
            else if (is_switch(*arg, "p", "replace")) {
                replace = true;
            }
            else if (is_help_switch(*arg)) {
                print_help_xit();
            }
            else if (is_version_switch(*arg)) {
                print_version_exit(VERSION);
            }
            else if (is_long_switch(*arg, "verbose")) {
                verbose = true;
            }
            else if (GetArgument(value, "t", "theme", arg, argv + argc)) {
                overrideTheme = value;
            }
            else if (GetArgument(value, "c", "config", arg, argv + argc)) {
                configFile = value;
            }
            else if (GetArgument(value, "i", "image", arg, argv + argc)) {
                image = value;
            }
            else if (GetArgument(value, "k", "color", arg, argv + argc)) {
                color = value;
            }
            else if (GetArgument(value, "s", "semis", arg, argv + argc)) {
                semis = value;
            }
            else if (GetArgument(value, "x", "trans", arg, argv + argc)) {
                trans = value;
            }
            else if (GetArgument(value, "e", "center", arg, argv + argc)) {
                center = value;
            }
            else if (GetArgument(value, "a", "scaled", arg, argv + argc)) {
                scaled = value;
            }
            else if (GetArgument(value, "m", "multi", arg, argv + argc)) {
                multi = value;
            }
        }
    }

    if (nice(5)) {
        /*ignore*/;
    }

    Background bg(&argc, &argv, verbose);

    if (sendRestart) {
        bg.sendRestart();
        return 0;
    }

    if (sendQuit) {
        bg.sendQuit();
        return 0;
    }

    if (bg.become(replace) == false) {
        warn(_("Cannot start, because another icewmbg is still running."));
        return 1;
    }

    globalBg = &bg;
    bgLoadConfig(configFile, overrideTheme);
    if (image) {
        bg.clearBackgroundImages();
        bgParse("DesktopBackgroundImage", image);
    }
    if (color) {
        bg.clearBackgroundColors();
        bgParse("DesktopBackgroundColor", color);
    }
    if (semis) {
        bg.clearTransparencyImages();
        bgParse("DesktopTransparencyImage", semis);
        bg.enableSemitransparency();
    }
    if (trans) {
        bg.clearTransparencyColors();
        bgParse("DesktopTransparencyColor", trans);
        bg.enableSemitransparency();
    }
    testFlag(&centerBackground, center);
    testFlag(&scaleBackground, scaled);
    testFlag(&multiheadBackground, multi);
    globalBg = NULL;
    bg.update();

    return bg.mainLoop();
}

// vim: set sw=4 ts=4 et:
