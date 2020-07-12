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
#include "ytimer.h"
#include "udir.h"
#include "intl.h"
#include "appnames.h"

#define CFGDEF
#include "icewmbg_prefs.h"

char const *ApplicationName = nullptr;

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
        time_t now = time(nullptr);
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
            tlog(_("Failed to load image '%s'."), name.c_str());
        }
        return image;
    }
    void unload() {
        pix = null;
        last = check = 0L;
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
    void unload() {
        for (int k = pixes.getCount(); --k >= 0; ) {
            pixes[k]->unload();
        }
    }
};

class Background: public YXApplication, private YTimerListener {
public:
    Background(int *argc, char ***argv, bool verbose = false);
    ~Background();

    void add(const char* name, const char* value, bool append);
    bool become(bool replace = false);
    void update(bool force = false);
    void sendQuit()    { sendClientMessage(_XA_ICEWMBG_QUIT);    }
    void sendRestart() { sendClientMessage(_XA_ICEWMBG_RESTART); }
    void sendShuffle() { sendClientMessage(_XA_ICEWMBG_SHUFFLE); }
    int mainLoop();

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
    virtual bool handleTimer(YTimer* timer);

    void restart() const;
    void changeBackground(bool force);
    void sendClientMessage(Atom message) const;
    long* getLongProperties(Atom property, int n) const;
    int getLongProperty(Atom property) const;
    int getWorkspace() const;
    int getNumberOfDesktops() const;
    void getDesktopGeometry();
    void reshuffle();
    void randinit(long seed);
    void startShuffle();
    int randmax(int upper);
    int shuffle(int index);
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
    void changeStringProperty(Atom property, char* data) const {
        XChangeProperty(display(), window(), property, XA_STRING, 8,
                        PropModeReplace, (unsigned char *) data, strlen(data));
    }

    typedef MStringArray Strings;
    typedef YArray<YColor> YColors;

    void addImage(Strings& images, const char* name, bool append);
    ref<YPixmap> renderBackground(ref<YPixmap> back, YColor color);
    ref<YPixmap> getBackgroundPixmap();
    YColor getBackgroundColor();
    ref<YPixmap> getTransparencyPixmap();
    YColor getTransparencyColor();
    Atom atom(const char* name) const;
    static Window window() { return desktop->handle(); }
    upath getThemeDir();

private:
    char** mainArgv;
    bool verbose;
    bool randInited;
    Strings backgroundImages;
    YColors backgroundColors;
    Strings transparencyImages;
    YColors transparencyColors;
    PixCache cache;
    upath themeDir;
    int activeWorkspace;
    int cycleOffset;
    int desktopCount;
    unsigned desktopWidth, desktopHeight;
    ref<YPixmap> currentBackgroundPixmap;
    ref<YPixmap> currentTransparencyPixmap;
    YColor currentBackgroundColor;
    mstring pixmapName;
    YArray<int> sequence;
    lazy<YTimer> cycleTimer;

    Atom _XA_XROOTPMAP_ID;
    Atom _XA_XROOTCOLOR_PIXEL;
    Atom _XA_NET_CURRENT_DESKTOP;
    Atom _XA_NET_DESKTOP_GEOMETRY;
    Atom _XA_NET_NUMBER_OF_DESKTOPS;
    Atom _XA_NET_WORKAREA;
    Atom _XA_ICEWMBG_QUIT;
    Atom _XA_ICEWMBG_IMAGE;
    Atom _XA_ICEWMBG_SHUFFLE;
    Atom _XA_ICEWMBG_RESTART;
    Atom _XA_ICEWMBG_PID;
};

Background::Background(int *argc, char ***argv, bool verb):
    YXApplication(argc, argv),
    mainArgv(*argv),
    verbose(verb),
    randInited(false),
    activeWorkspace(0),
    cycleOffset(0),
    desktopCount(MAX_WORKSPACES),
    desktopWidth(desktop->width()),
    desktopHeight(desktop->height()),
    currentBackgroundColor(),
    _XA_XROOTPMAP_ID(atom("_XROOTPMAP_ID")),
    _XA_XROOTCOLOR_PIXEL(atom("_XROOTCOLOR_PIXEL")),
    _XA_NET_CURRENT_DESKTOP(atom("_NET_CURRENT_DESKTOP")),
    _XA_NET_DESKTOP_GEOMETRY(atom("_NET_DESKTOP_GEOMETRY")),
    _XA_NET_NUMBER_OF_DESKTOPS(atom("_NET_NUMBER_OF_DESKTOPS")),
    _XA_NET_WORKAREA(atom("_NET_WORKAREA")),
    _XA_ICEWMBG_QUIT(atom("_ICEWMBG_QUIT")),
    _XA_ICEWMBG_IMAGE(atom("_ICEWMBG_IMAGE")),
    _XA_ICEWMBG_SHUFFLE(atom("_ICEWMBG_SHUFFLE")),
    _XA_ICEWMBG_RESTART(atom("_ICEWMBG_RESTART"))
{
    char abuf[42];
    snprintf(abuf, sizeof abuf, "_ICEWMBG_PID_S%d", xapp->screen());
    _XA_ICEWMBG_PID = atom(abuf);

    desktop->setStyle(YWindow::wsDesktopAware);
    desktopCount = Elvis(getNumberOfDesktops(), desktopCount);

    catchSignal(SIGHUP);
    catchSignal(SIGTERM);
    catchSignal(SIGINT);
    catchSignal(SIGQUIT);
    catchSignal(SIGUSR1);
    catchSignal(SIGUSR2);
}

upath Background::getThemeDir() {
    if (themeDir == null && nonempty(themeName)) {
        upath path(themeName);
        if (path.isAbsolute() == false) {
            if (strchr(themeName, '/') == nullptr) {
                path = upath("themes") + path + "default.theme";
            }
            else {
                path = upath("themes") + path;
            }
            path = findConfigFile(path);
        }
        if (path.fileExists()) {
            themeDir = path.parent();
        }
        else if (path.dirExists()) {
            themeDir = path;
        }
        if (themeDir == null) {
            tlog("Could not find theme dir");
            themeDir = "";
        }
    }
    return themeDir;
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
        deleteProperty(_XA_ICEWMBG_IMAGE);
    }
    clearBackgroundImages();
    clearBackgroundColors();
    clearTransparencyImages();
    clearTransparencyColors();
    cache.clear();
}

int Background::mainLoop() {
    update();
    if (0 < cycleBackgroundsPeriod) {
        cycleTimer->setTimer(cycleBackgroundsPeriod * 1000L, this, true);
    }
    return YXApplication::mainLoop();
}

Atom Background::atom(const char* name) const {
    return XInternAtom(display(), name, False);
}

static bool hasImageExt(const char* name) {
    size_t len = strlen(name);
    if (len >= 5 && name[len - 4] == '.') {
        const char* ext = &name[len - 3];
        if (3 == strspn(ext, "jpngxmJPNGXM"))
            return true;
#ifdef CONFIG_LIBRSVG
        if (0 == strcasecmp(ext, "svg"))
            return true;
#endif
    }
    else if (len >= 6 && name[len - 5] == '.') {
        const char* ext = &name[len - 4];
        if (0 == strcasecmp(ext, "jpeg"))
            return true;
    }
    return false;
}

void Background::addImage(Strings& images, const char* name, bool append) {
    if (append == false) {
        images.clear();
    }
    if (images.getCount() < ICEBG_MAX_ARGS) {
        const bool permute = (name[0] == '!');
        if (permute || (name[0] == '\\' && name[1] == '!')) {
            ++name;
        }
        upath path(name);
        bool globbing = path.hasglob();
        if (globbing == false && path.dirExists()) {
            path += "*";
            globbing = true;
        }
        if (globbing) {
            YStringArray list;
            if (path.glob(list, "/")) {
                if (permute) {
                    int k = list.getCount();
                    randinit(k);
                    while (0 < --k) {
                        list.swap(k, randmax(k));
                    }
                }
                YStringArray::IterType iter = list.iterator();
                while (++iter && images.getCount() < ICEBG_MAX_ARGS) {
                    if (hasImageExt(*iter)) {
                        upath image(*iter);
                        images.append(image);
                        cache.add(image);
                    }
                }
            }
        }
        else {
            bool isAbs = path.isAbsolute();
            path = path.expand();
            bool xist = (isAbs && path.fileExists());
            if (xist == false && isAbs == false) {
                upath p(getThemeDir() + path);
                if (p.fileExists()) {
                    path = p;
                    xist = true;
                }
            }
            if (xist || (isAbs == false && path.fileExists())) {
                images.append(path);
                cache.add(path);
            }
        }
    }
}

void Background::add(const char* name, const char* value, bool append) {
    if (value == nullptr || *value == 0) {
    }
    else if (0 == strcmp(name, "DesktopBackgroundImage")) {
        addImage(backgroundImages, value, append);
    }
    else if (0 == strcmp(name, "DesktopBackgroundColor")) {
        if (append == false) {
            clearBackgroundColors();
        }
        if (backgroundColors.getCount() < ICEBG_MAX_ARGS) {
            backgroundColors.append(YColor(value));
        }
    }
    else if (0 == strcmp(name, "DesktopTransparencyImage")) {
        addImage(transparencyImages, value, append);
    }
    else if (0 == strcmp(name, "DesktopTransparencyColor")) {
        if (append == false) {
            clearTransparencyColors();
        }
        if (transparencyColors.getCount() < ICEBG_MAX_ARGS) {
            transparencyColors.append(YColor(value));
        }
    }
    else {
        warn("Unknown name '%s'.", name);
    }
}

void Background::handleSignal(int sig) {
    switch (sig) {
    case SIGHUP:
        restart();
        break;

    case SIGINT:
    case SIGTERM:
    case SIGQUIT:
        this->exit(1);
        break;

    case SIGUSR1:
        startShuffle();
        break;

    case SIGUSR2:
        tlog("logEvents %s", boolstr(toggleLogEvents()));
        break;

    default:
        YApplication::handleSignal(sig);
        break;
    }
}

bool Background::handleTimer(YTimer* timer) {
    if (timer == cycleTimer) {
        cycleOffset += desktopCount;
        cache.unload();
        update(true);
    }
    return true;
}

ref<YPixmap> Background::getBackgroundPixmap() {
    ref<YPixmap> pixmap;
    int count = backgroundImages.getCount();
    if (count > 0 && activeWorkspace >= 0) {
        for (int i = 0; i < count; ++i) {
            int k = shuffle((i + activeWorkspace + cycleOffset) % count);
            pixmap = cache.get(backgroundImages[k]);
            if (pixmap != null) {
                pixmapName = backgroundImages[k];
                break;
            }
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
    int numbg = backgroundImages.getCount();
    if (count > 0 && numbg > 0 && activeWorkspace >= 0) {
        for (int i = 0; i < count; ++i) {
            int k = shuffle((i + activeWorkspace + cycleOffset) % numbg) % count;
            pixmap = cache.get(transparencyImages[k]);
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

void Background::randinit(long seed) {
    if (randInited == false) {
        timeval now = walltime();
        srand(unsigned((seed + getpid()) * now.tv_sec + now.tv_usec));
        randInited = true;
    }
}

int Background::randmax(int upper) {
    return rand() / (RAND_MAX / (upper + 1) + 1);
}

void Background::reshuffle() {
    const int count = backgroundImages.getCount();
    sequence.getCount() ? sequence.clear() : randinit(count);
    sequence.setCapacity(count);
    for (int i = 0; i < count; ++i) {
        sequence.append(i);
    }
    for (int k = count; --k > 0; ) {
        sequence.swap(k, randmax(k));
    }
}

int Background::shuffle(int index) {
    if (shuffleBackgroundImages) {
        if (backgroundImages.getCount() != sequence.getCount()) {
            reshuffle();
            cache.unload();
        }
        return sequence[index];
    }
    else {
        return index;
    }
}

void Background::startShuffle() {
    if (verbose) tlog("shuffle");
    shuffleBackgroundImages = true;
    reshuffle();
    update(true);
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
    unsigned char *prop(nullptr);

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
    return nullptr;
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

int Background::getNumberOfDesktops() const {
    return getLongProperty(_XA_NET_NUMBER_OF_DESKTOPS);
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
                bw = height * bw / non_zero(bh);
                bh = height;
            } else {
                bh = width * bh / non_zero(bw);
                bw = width;
            }
        }
        else if (centerBackground) {
            if (bw >= bh && (bw > width || scaleBackground)) {
                bh = width * bh / non_zero(bw);
                bw = width;
            } else if (bh > height || scaleBackground) {
                bw = height * bw / non_zero(bh);
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
        changeStringProperty(_XA_ICEWMBG_IMAGE, (char *) pixmapName.c_str());
        handleBackground = true;
    }
    if (false == handleBackground) {
        XSetWindowBackgroundPixmap(display(), window(), None);
        XSetWindowBackground(display(), window(), bPixel);
        deleteProperty(_XA_ICEWMBG_IMAGE);
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
    if (verbose) tlog("restart");
    if (mainArgv[0][0] == '/' ||
        (strchr(mainArgv[0], '/') != nullptr &&
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
        if (xev.xproperty.atom == _XA_NET_NUMBER_OF_DESKTOPS) {
            getNumberOfDesktops();
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
                if (verbose) tlog("quit");
                this->exit(0);
            }
            return true;
        }
        if (xev.xclient.message_type == _XA_ICEWMBG_RESTART) {
            restart();
            return true;
        }
        if (xev.xclient.message_type == _XA_ICEWMBG_SHUFFLE) {
            startShuffle();
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
    "  -u, --shuffle        Shuffle/reshuffle the list of background images.\n"
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
    "  -y, --cycle=SECONDS  Cycle backgrounds every SECONDS.\n"
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
    " ShuffleBackgroundImages  - Shuffle the list of background images\n"
    " SupportSemitransparency  - Support for semitransparent terminals\n"
    " DesktopTransparencyColor - Semitransparency background color(s)\n"
    " DesktopTransparencyImage - Semitransparency background image(s)\n"
    " DesktopBackgroundMultihead - One background over all monitors\n"
    " CycleBackgroundsPeriod   - Seconds between cycling over backgrounds\n"
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

static void bgLoadConfig(const char *configFile, const char *overrideTheme)
{
    if (configFile == nullptr || *configFile == 0)
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
    if (themeName != nullptr) {
        MSG(("themeName=%s", themeName));

        YConfig::findLoadThemeFile(globalBg, icewmbg_prefs,
                *themeName == '/' ? themeName :
                upath("themes").child(themeName));
    }
    YConfig::findLoadConfigFile(globalBg, icewmbg_prefs, "prefoverride");
}

static void bgParse(const char* name, const char* value) {
    mstring opt, str(value);
    for (int i = 0; str.splitall(',', &opt, &str); ++i) {
        if (i + 1 < ICEBG_MAX_ARGS) {
            if ((opt = opt.trim()).nonempty()) {
                globalBg->add(name, opt, 0 < i);
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
    bool shuffle = false;
    bool verbose = false;
    const char* overrideTheme = nullptr;
    const char* configFile = nullptr;
    const char* image = nullptr;
    const char* color = nullptr;
    const char* trans = nullptr;
    const char* semis = nullptr;
    const char* center = nullptr;
    const char* scaled = nullptr;
    const char* multi = nullptr;
    const char* cycle = nullptr;
    const char* shuffleArg = nullptr;

    for (char **arg = argv + 1; arg < argv + argc; ++arg) {
        if (**arg == '-') {
            char *value(nullptr);
            if (is_switch(*arg, "r", "restart")) {
                sendRestart = true;
            }
            else if (is_switch(*arg, "q", "quit")) {
                sendQuit = true;
            }
            else if (is_switch(*arg, "p", "replace")) {
                replace = true;
            }
            else if (is_switch(*arg, "u", "shuffle") &&
                    (arg + 1 == argv + argc || strcspn(arg[1], "01")))
            {
                shuffle = true;
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
            else if (GetArgument(value, "u", "shuffle", arg, argv + argc)) {
                shuffleArg = value;
                testFlag(&shuffle, shuffleArg);
            }
            else if (GetArgument(value, "y", "cycle", arg, argv + argc)) {
                cycle = value;
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
        if (shuffle) {
            bg.sendShuffle();
            return 0;
        }
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
    shuffleBackgroundImages |= shuffle;
    if (shuffleArg) {
        testFlag(&shuffleBackgroundImages, shuffleArg);
    }
    if (cycle) {
        cycleBackgroundsPeriod = atoi(cycle);
    }

    globalBg = nullptr;

    return bg.mainLoop();
}

// vim: set sw=4 ts=4 et:
