#include "config.h"

#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <X11/Xatom.h>
#include "yxapp.h"
#include "yprefs.h"
#include "ytimer.h"
#include "udir.h"
#include "intl.h"
#include "appnames.h"

#define CFGDEF
#include "icewmbg_prefs.h"

char const *ApplicationName = nullptr;

typedef ref<YImage> Image;

class Cache {
public:
    Cache() { }
    ~Cache() { clear(); }
    void clear() { iname = null; image = null; }
    Image get(const mstring& name) {
        if (iname != name) {
            iname = name;
            image = YImage::load(iname);
            if (image == null && testOnce(iname, __LINE__)) {
                tlog(_("Failed to load image '%s'."), iname.c_str());
            }
        }
        return image;
    }
private:
    mstring iname;
    Image image;
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
    bool haveBackgrounds() { return imageInited; }

private:
    virtual bool filterEvent(const XEvent& xev);
    virtual void handleSignal(int sig);
    virtual bool handleTimer(YTimer* timer);
    virtual int handleError(XErrorEvent* xev);

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
    ref<YPixmap> renderBackground(Image back, YColor color);
    Image getBackgroundImage();
    YColor getBackgroundColor();
    Image getTransparencyImage();
    YColor getTransparencyColor();
    Atom atom(const char* name) const;
    static Window window() { return desktop->handle(); }
    upath getThemeDir();
    bool checkWM();
    void syncWM();

private:
    char** mainArgv;
    bool verbose;
    bool randInited;
    bool themeInited;
    bool imageInited;
    Strings backgroundImages;
    YColors backgroundColors;
    Strings transparencyImages;
    YColors transparencyColors;
    Cache cache;
    upath themeDir;
    const int mypid;
    int activeWorkspace;
    int cycleOffset;
    int desktopCount;
    unsigned desktopWidth, desktopHeight;
    ref<YPixmap> currentTransparencyPixmap;
    YColor currentBackgroundColor;
    mstring backgroundName;
    YArray<int> sequence;
    lazy<YTimer> cycleTimer;
    lazy<YTimer> checkTimer;

    Atom _XA_XROOTPMAP_ID;
    Atom _XA_XROOTCOLOR_PIXEL;
    Atom _XA_NET_CURRENT_DESKTOP;
    Atom _XA_NET_DESKTOP_GEOMETRY;
    Atom _XA_NET_NUMBER_OF_DESKTOPS;
    Atom _XA_NET_SUPPORTING_WM_CHECK;
    Atom _XA_NET_WORKAREA;
    Atom _XA_ICEWMBG_QUIT;
    Atom _XA_ICEWMBG_IMAGE;
    Atom _XA_ICEWMBG_SHUFFLE;
    Atom _XA_ICEWMBG_RESTART;
    Atom _XA_ICEWMBG_PID;
    Atom _XA_ICEWM_WINOPT;
};

Background::Background(int *argc, char ***argv, bool verb):
    YXApplication(argc, argv),
    mainArgv(*argv),
    verbose(verb),
    randInited(false),
    themeInited(false),
    imageInited(false),
    mypid(getpid()),
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
    _XA_NET_SUPPORTING_WM_CHECK(atom("_NET_SUPPORTING_WM_CHECK")),
    _XA_NET_WORKAREA(atom("_NET_WORKAREA")),
    _XA_ICEWMBG_QUIT(atom("_ICEWMBG_QUIT")),
    _XA_ICEWMBG_IMAGE(atom("_ICEWMBG_IMAGE")),
    _XA_ICEWMBG_SHUFFLE(atom("_ICEWMBG_SHUFFLE")),
    _XA_ICEWMBG_RESTART(atom("_ICEWMBG_RESTART")),
    _XA_ICEWM_WINOPT(atom("_ICEWM_WINOPTHINT"))
{
    char abuf[42];
    snprintf(abuf, sizeof abuf, "_ICEWMBG_PID_S%d", screen());
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
    if (themeInited == false && nonempty(themeName)) {
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
        themeInited = true;
    }
    return themeDir;
}

Background::~Background() {
    int pid = getBgPid();
    if (supportSemitransparency) {
        if (pid <= 0 || pid == mypid) {
            if (_XA_XROOTPMAP_ID)
                deleteProperty(_XA_XROOTPMAP_ID);
            if (_XA_XROOTCOLOR_PIXEL)
                deleteProperty(_XA_XROOTCOLOR_PIXEL);
        }
    }
    if (pid == mypid) {
        deleteProperty(_XA_ICEWMBG_PID);
        deleteProperty(_XA_ICEWMBG_IMAGE);
    }
    clearBackgroundImages();
    clearBackgroundColors();
    clearTransparencyImages();
    clearTransparencyColors();
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
/* #ifdef CONFIG_LIBRSVG
        if (0 == strcasecmp(ext, "svg"))
            return true;
* #endif */
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
        if (path.name() == "*") {
            upath up(path.parent());
            if (up.dirExists() && up.hasglob() == false) {
                path = up;
            }
        }
        if (path.hasglob()) {
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
                    }
                }
            }
        }
        else {
            bool isAbs = path.isAbsolute();
            path = path.expand();
            bool xist = (isAbs && path.isReadable());
            if (xist == false && isAbs == false) {
                upath p(getThemeDir() + path);
                if (p.isReadable()) {
                    path = p;
                    xist = true;
                }
            }
            if (xist && path.dirExists()) {
                int start = images.getCount();
                for (udir dir(path); dir.nextFile(); ) {
                    if (hasImageExt(dir.entry())) {
                        upath file(path + dir.entry());
                        images.append(file);
                        if (images.getCount() >= ICEBG_MAX_ARGS)
                            break;
                    }
                }
                if (permute) {
                    int k = images.getCount();
                    randinit(k);
                    while (start < --k) {
                        images.swap(k, start + randmax(k - start));
                    }
                }
            }
            else if (xist && path.fileExists()) {
                images.append(path);
            }
        }
    }
}

void Background::add(const char* name, const char* value, bool append) {
    if (isEmpty(value)) {
    }
    else if (0 == strcmp(name, "DesktopBackgroundImage")) {
        addImage(backgroundImages, value, append);
        imageInited = true;
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
        update(true);
        return true;
    }
    else if (timer == checkTimer) {
        if (checkWM())
            syncWM();
        update(true);
    }
    return false;
}

int Background::handleError(XErrorEvent* xev) {
    return BadRequest;
}

Image Background::getBackgroundImage() {
    Image image;
    int count = backgroundImages.getCount();
    if (count > 0 && activeWorkspace >= 0) {
        for (int i = 0; i < count; ++i) {
            int k = shuffle((i + activeWorkspace + cycleOffset) % count);
            image = cache.get(backgroundImages[k]);
            if (image != null) {
                backgroundName = backgroundImages[k];
                break;
            }
        }
    }
    return image;
}

YColor Background::getBackgroundColor() {
    int count = backgroundColors.getCount();
    return count > 0
        ? backgroundColors[activeWorkspace % count]
        : YColor::black;
}

Image Background::getTransparencyImage() {
    Image image;
    int count = transparencyImages.getCount();
    int numbg = backgroundImages.getCount();
    if (count > 0 && numbg > 0 && activeWorkspace >= 0) {
        for (int i = 0; i < count; ++i) {
            int k = shuffle((i + activeWorkspace + cycleOffset) % numbg) % count;
            image = cache.get(transparencyImages[k]);
            if (image != null)
                break;
        }
    }
    return image;
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
        srand(unsigned((seed + mypid) * now.tv_sec + now.tv_usec));
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
    YProperty prop(window(), property, F32, long(n), XA_CARDINAL);
    if (prop && unsigned(n) <= prop.size()) {
        return prop.retrieve<long>();
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

ref<YPixmap> Background::renderBackground(Image back, YColor color) {
    if (verbose) tlog("rendering...");
    unsigned width = desktopWidth;
    unsigned height = desktopHeight;
    if (back == null || width == 0 || height == 0) {
        if (verbose) tlog("%s null return", __func__);
        return null;
    }

    int numScreens = multiheadBackground ? 1 : desktop->getScreenCount();
    if (numScreens == 1) {
        if (width == back->width() && height == back->height()) {
            if (verbose) tlog("%s numScreens == 1 return", __func__);
            return back->renderToPixmap(desktop->depth());
        }
    }

    ref<YPixmap> cBack = YPixmap::create(width, height, desktop->depth());
    Graphics g(cBack);
    g.setColor(color);
    g.fillRect(0, 0, width, height);

    if (centerBackground == false && scaleBackground == false) {
        ref<YPixmap> pixm = back->renderToPixmap(desktop->depth());
        g.fillPixmap(pixm, 0, 0, width, height, 0, 0);
        if (verbose) tlog("%s fillPixmap false return", __func__);
        return cBack;
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

    Image scaled;
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

        Image todraw;
        if (bw != back->width() || bh != back->height()) {
            if (scaled == null ||
                scaled->width() != bw ||
                scaled->height() != bh) {
                scaled = back->scale(bw, bh);
            }
            todraw = scaled;
        }
        if (todraw == null) {
            todraw = back;
        }
        ref<YPixmap> pixm = todraw->renderToPixmap(g.rdepth());
        if (pixm != null) {
            g.drawPixmap(pixm,
                         max(0, int(bw - width) / 2),
                         max(0, int(bh - height) / 2),
                         min(width, bw),
                         min(height, bh),
                         max(x, x + int(width - bw) / 2),
                         max(y, y + int(height - bh) / 2));
        }
    }

    return cBack;
}

void Background::changeBackground(bool force) {
    Image backgroundImage = getBackgroundImage();
    YColor backgroundColor(getBackgroundColor());
    if (force == false) {
        if (backgroundImage == null &&
            backgroundColor == currentBackgroundColor) {
            cache.clear();
            return;
        }
    }
    unsigned long const bPixel(backgroundColor.pixel());
    bool handleBackground = false;
    Pixmap bPixmap(None);
    ref<YPixmap> back = renderBackground(backgroundImage, backgroundColor);

    if (back != null) {
        back->forgetImage();
        bPixmap = back->pixmap();
        XSetWindowBackgroundPixmap(display(), window(), bPixmap);
        changeStringProperty(_XA_ICEWMBG_IMAGE, (char *) backgroundName.c_str());
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
            Image trans = getTransparencyImage();
            bool samePixmap = trans == backgroundImage;
            bool sameColor = tColor == backgroundColor;
            currentTransparencyPixmap =
                trans == null || (samePixmap && sameColor) ? back :
                renderBackground(trans, tColor);
            if (currentTransparencyPixmap != null)
                currentTransparencyPixmap->forgetImage();

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
    cache.clear();

    if (false == supportSemitransparency &&
        backgroundImages.getCount() <= 1 &&
        backgroundColors.getCount() <= 1)
    {
        this->exit(0);
    }
    // currentBackgroundPixmap = backgroundPixmap;
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
    else if (xev.type == PropertyNotify &&
             xev.xproperty.state == PropertyNewValue)
    {
        if (xev.xproperty.atom == _XA_NET_CURRENT_DESKTOP) {
            update();
        }
        else if (xev.xproperty.atom == _XA_NET_NUMBER_OF_DESKTOPS) {
            getNumberOfDesktops();
        }
        else if (xev.xproperty.atom == _XA_NET_DESKTOP_GEOMETRY) {
            unsigned w = desktopWidth, h = desktopHeight;
            getDesktopGeometry();
            update(w != desktopWidth || h != desktopHeight);
        }
        else if (xev.xproperty.atom == _XA_NET_WORKAREA) {
            unsigned w = desktopWidth, h = desktopHeight;
            desktop->updateXineramaInfo(desktopWidth, desktopHeight);
            update(w != desktopWidth || h != desktopHeight);
        }
        else if (xev.xproperty.atom == _XA_ICEWMBG_PID) {
            int pid = getBgPid();
            if (pid > 0 && pid != mypid) {
                this->exit(0);
            }
            return true;
        }
        else if (xev.xproperty.atom == _XA_NET_SUPPORTING_WM_CHECK) {
            checkTimer->setTimer(100L, this, true);
        }
    }
    else if (xev.type == ClientMessage) {
        if (xev.xclient.message_type == _XA_ICEWMBG_QUIT) {
            if (xev.xclient.data.l[0] != mypid) {
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

bool Background::checkWM() {
    YProperty wchk(window(), _XA_NET_SUPPORTING_WM_CHECK, F32, 1L, XA_WINDOW);
    if (wchk) {
        YProperty name(*wchk, _XA_NET_WM_NAME, F8, 5L, _XA_UTF8_STRING);
        return name && strncmp(name.string(), "IceWM", 5) == 0;
    }
    return false;
}

void Background::syncWM() {
    YProperty wopt(window(), _XA_ICEWM_WINOPT, F8, 99L, _XA_ICEWM_WINOPT);
    if (wopt == false) {
        wopt.append("\0\0\0", 3);
        sync();
    }
    for (int i = 0; ++i < 10 && wopt; ++i) {
        usleep(i * 1000);
        wopt.update();
    }
}

void Background::sendClientMessage(Atom message) const {
    XClientMessageEvent xev = {};
    xev.type = ClientMessage;
    xev.window = window();
    xev.message_type = message;
    xev.format = 32;
    xev.data.l[0] = mypid;
    send(xev, window(), StructureNotifyMask);
    sync();
}

bool Background::become(bool replace) {
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
        long lpid = mypid;
        changeLongProperty(_XA_ICEWMBG_PID, (unsigned char *) &lpid);
        sync();
        pid = mypid;
    }

    return pid == mypid;
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
    if (isEmpty(configFile))
        configFile = "preferences";
    if (nonempty(overrideTheme))
        themeName = overrideTheme;
    else
    {
        cfoption theme_prefs[] = {
            OSV("Theme", &themeName, "Theme name"),
            OK0()
        };
        YConfig(theme_prefs).load(configFile).load("theme");
    }
    YConfig(icewmbg_prefs).load(configFile).loadTheme().loadOverride();
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
            else if (GetArgument(value, "d", "display", arg, argv + argc)) {
                /*ignore*/;
            }
            else if (GetArgument(value, "C", "copying", arg, argv + argc)) {
                /*ignore*/;
            }
            else
                warn(_("Unrecognized option '%s'."), *arg);
        }
    }

    if (getpriority(PRIO_PROCESS, 0) < 5) {
        if (nice(5)) {
            /*ignore*/;
        }
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

    if (bg.haveBackgrounds() == false) {
#ifdef CONFIG_DEFAULT_BACKGROUND
        const char def[] = CONFIG_DEFAULT_BACKGROUND "";
        if (def[0]) {
            bgParse("DesktopBackgroundImage", def);
        }
#endif
    }

    globalBg = nullptr;

    return bg.mainLoop();
}

// vim: set sw=4 ts=4 et:
