/*
 * IceWM
 *
 * Copyright (C) 1998-2002 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "yapp.h"
#include "ypoll.h"

#include "sysdep.h"
#include "wmprog.h"
#include "wmmgr.h"
#include "MwmUtil.h"
#include "prefs.h"
#include "ypixbuf.h"

#include <sys/resource.h>

#include "intl.h"

extern char const *ApplicationName;
char const *&YApplication::Name = ApplicationName;

YApplication *app = 0;
YDesktop *desktop = 0;
XContext windowContext;
static int signalPipe[2] = { 0, 0 };
static sigset_t oldSignalMask;
static sigset_t signalMask;

Atom _XA_WM_PROTOCOLS;
Atom _XA_WM_TAKE_FOCUS;
Atom _XA_WM_DELETE_WINDOW;
Atom _XA_WM_STATE;
Atom _XA_WM_CHANGE_STATE;
Atom _XATOM_MWM_HINTS;
//Atom _XA_MOTIF_WM_INFO;!!!
Atom _XA_WM_COLORMAP_WINDOWS;
Atom _XA_WM_CLIENT_LEADER;
Atom _XA_WM_WINDOW_ROLE;
Atom _XA_WINDOW_ROLE;
Atom _XA_SM_CLIENT_ID;
Atom _XA_ICEWM_ACTION;
Atom _XA_CLIPBOARD;
Atom _XA_TARGETS;

Atom _XA_WIN_PROTOCOLS;
Atom _XA_WIN_WORKSPACE;
Atom _XA_WIN_WORKSPACE_COUNT;
Atom _XA_WIN_WORKSPACE_NAMES;
Atom _XA_WIN_WORKAREA;
#ifdef CONFIG_TRAY
Atom _XA_WIN_TRAY;
#endif
Atom _XA_WIN_ICONS;
Atom _XA_WIN_STATE;
Atom _XA_WIN_LAYER;
Atom _XA_WIN_HINTS;
Atom _XA_WIN_SUPPORTING_WM_CHECK;
Atom _XA_WIN_CLIENT_LIST;
Atom _XA_WIN_DESKTOP_BUTTON_PROXY;
Atom _XA_WIN_AREA;
Atom _XA_WIN_AREA_COUNT;

Atom _XA_NET_SUPPORTED;
Atom _XA_NET_SUPPORTING_WM_CHECK;
Atom _XA_NET_CLIENT_LIST;
Atom _XA_NET_CLIENT_LIST_STACKING;
Atom _XA_NET_NUMBER_OF_DESKTOPS;
Atom _XA_NET_CURRENT_DESKTOP;
Atom _XA_NET_WORKAREA;
Atom _XA_NET_WM_MOVERESIZE;

Atom _XA_NET_WM_STRUT;
Atom _XA_NET_WM_DESKTOP;
Atom _XA_NET_CLOSE_WINDOW;
Atom _XA_NET_ACTIVE_WINDOW;
Atom _XA_NET_WM_STATE;

Atom _XA_NET_WM_STATE_SHADED;
Atom _XA_NET_WM_STATE_MAXIMIZED_VERT;
Atom _XA_NET_WM_STATE_MAXIMIZED_HORZ;
Atom _XA_NET_WM_STATE_SKIP_TASKBAR;
Atom _XA_NET_WM_STATE_HIDDEN;
Atom _XA_NET_WM_STATE_FULLSCREEN;
Atom _XA_NET_WM_WINDOW_TYPE;
Atom _XA_NET_WM_WINDOW_TYPE_DOCK;
Atom _XA_NET_WM_WINDOW_TYPE_DESKTOP;
Atom _XA_NET_WM_WINDOW_TYPE_SPLASH;

Atom _XA_NET_WM_NAME;
Atom _XA_NET_WM_PID;

Atom _XA_KWM_WIN_ICON;

Atom XA_XdndAware;
Atom XA_XdndEnter;
Atom XA_XdndLeave;
Atom XA_XdndPosition;
Atom XA_XdndStatus;
Atom XA_XdndDrop;
Atom XA_XdndFinished;

YCursor YApplication::leftPointer;
YCursor YApplication::rightPointer;
YCursor YApplication::movePointer;

YColor *YColor::black(NULL);
YColor *YColor::white(NULL);

YPixmap *buttonIPixmap(NULL);
YPixmap *buttonAPixmap(NULL);

YPixmap *logoutPixmap(NULL);
YPixmap *switchbackPixmap(NULL);
YPixmap *listbackPixmap(NULL);
YPixmap *dialogbackPixmap(NULL);

YPixmap *menubackPixmap(NULL);
YPixmap *menusepPixmap(NULL);
YPixmap *menuselPixmap(NULL);

#ifdef CONFIG_GRADIENTS
YPixbuf *buttonIPixbuf(NULL);
YPixbuf *buttonAPixbuf(NULL);

YPixbuf *logoutPixbuf(NULL);
YPixbuf *switchbackPixbuf(NULL);
YPixbuf *listbackPixbuf(NULL);
YPixbuf *dialogbackPixbuf(NULL);

YPixbuf *menubackPixbuf(NULL);
YPixbuf *menuselPixbuf(NULL);
YPixbuf *menusepPixbuf(NULL);
#endif

YPixmap *closePixmap[2] = { 0, 0 };
YPixmap *minimizePixmap[2] = { 0, 0 };
YPixmap *maximizePixmap[2] = { 0, 0 };
YPixmap *restorePixmap[2] = { 0, 0 };
YPixmap *hidePixmap[2] = { 0, 0 };
YPixmap *rollupPixmap[2] = { 0, 0 };
YPixmap *rolldownPixmap[2] = { 0, 0 };
YPixmap *depthPixmap[2] = { 0, 0 };

YIcon *defaultAppIcon = 0;

#ifdef CONFIG_SHAPE
int shapesSupported;
int shapeEventBase, shapeErrorBase;
#endif

int xeventcount = 0;

class YClipboard: public YWindow {
public:
    YClipboard(): YWindow() {
        fLen = 0;
        fData = 0;
    }
    ~YClipboard() {
        if (fData)
            delete [] fData;
        fData = 0;
        fLen = 0;
    }

    void setData(char *data, int len) {
        if (fData)
            delete [] fData;
        fLen = len;
        fData = new char[len];
        if (fData)
            memcpy(fData, data, len);
        if (fLen == 0)
            clearSelection(false);
        else
            acquireSelection(false);
    }
    void handleSelectionClear(const XSelectionClearEvent &clear) {
        if (clear.selection == _XA_CLIPBOARD) {
            if (fData)
                delete [] fData;
            fLen = 0;
            fData = 0;
        }
    }
    void handleSelectionRequest(const XSelectionRequestEvent &request) {
        if (request.selection == _XA_CLIPBOARD) {
            XSelectionEvent notify;

            notify.type = SelectionNotify;
            notify.requestor = request.requestor;
            notify.selection = request.selection;
            notify.target = request.target;
            notify.time = request.time;
            notify.property = request.property;

            if (request.selection == _XA_CLIPBOARD &&
                request.target == XA_STRING &&
                fLen > 0)
            {
                XChangeProperty(app->display(),
                                request.requestor,
                                request.property,
                                request.target,
                                8, PropModeReplace,
                                (unsigned char *)(fData ? fData : ""),
                                fLen);
            } else if (request.selection == _XA_CLIPBOARD &&
                       request.target == _XA_TARGETS &&
                       fLen > 0)
            {
                Atom type = XA_STRING;

                XChangeProperty(app->display(),
                                request.requestor,
                                request.property,
                                request.target,
                                32, PropModeReplace,
                                (unsigned char *)&type, 1);
            } else {
                notify.property = None;
            }

            XSendEvent(app->display(), notify.requestor, False, 0L, (XEvent *)&notify);
        }
    }

private:
    int fLen;
    char *fData;
};

void initSignals() {
    sigemptyset(&signalMask);
    sigaddset(&signalMask, SIGHUP);
    sigprocmask(SIG_BLOCK, &signalMask, &oldSignalMask);
    sigemptyset(&signalMask);
    sigprocmask(SIG_BLOCK, &signalMask, &oldSignalMask);

    if (pipe(signalPipe) != 0)
        die(2, _("Failed to create anonymous pipe (errno=%d)."), errno);
    fcntl(signalPipe[1], F_SETFL, O_NONBLOCK);
    fcntl(signalPipe[0], F_SETFD, FD_CLOEXEC);
    fcntl(signalPipe[1], F_SETFD, FD_CLOEXEC);
}

static void initAtoms() {
    struct {
        Atom *atom;
        const char *name;
    } atom_info[] = {
        { &_XA_WM_PROTOCOLS, "WM_PROTOCOLS" },
        { &_XA_WM_TAKE_FOCUS, "WM_TAKE_FOCUS" },
        { &_XA_WM_DELETE_WINDOW, "WM_DELETE_WINDOW" },
        { &_XA_WM_STATE, "WM_STATE" },
        { &_XA_WM_CHANGE_STATE, "WM_CHANGE_STATE" },
        { &_XA_WM_COLORMAP_WINDOWS, "WM_COLORMAP_WINDOWS" },
        { &_XA_WM_CLIENT_LEADER, "WM_CLIENT_LEADER" },
        { &_XA_WINDOW_ROLE, "WINDOW_ROLE" },
        { &_XA_WM_WINDOW_ROLE, "WM_WINDOW_ROLE" },
        { &_XA_SM_CLIENT_ID, "SM_CLIENT_ID" },
        { &_XA_ICEWM_ACTION, "_ICEWM_ACTION" },
        { &_XATOM_MWM_HINTS, _XA_MOTIF_WM_HINTS },
        { &_XA_KWM_WIN_ICON, "KWM_WIN_ICON" },
        { &_XA_WIN_WORKSPACE, XA_WIN_WORKSPACE },
        { &_XA_WIN_WORKSPACE_COUNT, XA_WIN_WORKSPACE_COUNT },
        { &_XA_WIN_WORKSPACE_NAMES, XA_WIN_WORKSPACE_NAMES },
        { &_XA_WIN_WORKAREA, XA_WIN_WORKAREA },
        { &_XA_WIN_ICONS, XA_WIN_ICONS },
        { &_XA_WIN_LAYER, XA_WIN_LAYER },
#ifdef CONFIG_TRAY
        { &_XA_WIN_TRAY, XA_WIN_TRAY },
#endif
        { &_XA_WIN_STATE, XA_WIN_STATE },
        { &_XA_WIN_HINTS, XA_WIN_HINTS },
        { &_XA_WIN_PROTOCOLS, XA_WIN_PROTOCOLS },
        { &_XA_WIN_SUPPORTING_WM_CHECK, XA_WIN_SUPPORTING_WM_CHECK },
        { &_XA_WIN_CLIENT_LIST, XA_WIN_CLIENT_LIST },
        { &_XA_WIN_DESKTOP_BUTTON_PROXY, XA_WIN_DESKTOP_BUTTON_PROXY },
        { &_XA_WIN_AREA, XA_WIN_AREA },
        { &_XA_WIN_AREA_COUNT, XA_WIN_AREA_COUNT },

        { &_XA_NET_SUPPORTED, "_NET_SUPPORTED" },
        { &_XA_NET_SUPPORTING_WM_CHECK, "_NET_SUPPORTING_WM_CHECK" },
        { &_XA_NET_CLIENT_LIST, "_NET_CLIENT_LIST" },
        { &_XA_NET_CLIENT_LIST_STACKING, "_NET_CLIENT_LIST_STACKING" },
        { &_XA_NET_NUMBER_OF_DESKTOPS, "_NET_NUMBER_OF_DESKTOPS" },
        { &_XA_NET_CURRENT_DESKTOP, "_NET_CURRENT_DESKTOP" },
        { &_XA_NET_WORKAREA, "_NET_WOKRAREA" },
        { &_XA_NET_WM_MOVERESIZE, "_NET_WM_MOVERESIZE" },

        { &_XA_NET_WM_STRUT, "_NET_WM_STRUT" },
        { &_XA_NET_WM_DESKTOP, "_NET_WM_DESKTOP" },
        { &_XA_NET_CLOSE_WINDOW, "_NET_CLOSE_WINDOW" },
        { &_XA_NET_ACTIVE_WINDOW, "_NET_ACTIVE_WINDOW" },
        { &_XA_NET_WM_STATE, "_NET_WM_STATE" },

        { &_XA_NET_WM_STATE_SHADED, "_NET_WM_STATE_SHADED" },
        { &_XA_NET_WM_STATE_MAXIMIZED_VERT, "_NET_WM_STATE_MAXIMIZED_VERT" },
        { &_XA_NET_WM_STATE_MAXIMIZED_HORZ, "_NET_WM_STATE_MAXIMIZED_HORZ" },
        { &_XA_NET_WM_STATE_SKIP_TASKBAR, "_NET_WM_STATE_SKIP_TASKBAR" },
        { &_XA_NET_WM_STATE_HIDDEN, "_NET_WM_STATE_HIDDEN" },
        { &_XA_NET_WM_STATE_FULLSCREEN, "_NET_WM_STATE_FULLSCREEN" },
        { &_XA_NET_WM_WINDOW_TYPE, "_NET_WM_WINDOW_TYPE" },
        { &_XA_NET_WM_WINDOW_TYPE_DOCK, "_NET_WM_WINDOW_TYPE_DOCK" },
        { &_XA_NET_WM_WINDOW_TYPE_DESKTOP, "_NET_WM_WINDOW_TYPE_DESKTOP" },
        { &_XA_NET_WM_WINDOW_TYPE_SPLASH, "_NET_WM_WINDOW_TYPE_SPLASH" },

        { &_XA_NET_WM_NAME, "_NET_WM_NAME" },
        { &_XA_NET_WM_PID, "_NET_WM_PID" },

        { &_XA_CLIPBOARD, "CLIPBOARD" },
        { &_XA_TARGETS, "TARGETS" },
        { &XA_XdndAware, "XdndAware" },
        { &XA_XdndEnter, "XdndEnter" },
        { &XA_XdndLeave, "XdndLeave" },
        { &XA_XdndPosition, "XdndPosition" },
        { &XA_XdndStatus, "XdndStatus" },
        { &XA_XdndDrop, "XdndDrop" },
        { &XA_XdndFinished, "XdndFinished" }
    };
    unsigned int i;

#ifdef HAVE_XINTERNATOMS
    const char *names[ACOUNT(atom_info)];
    Atom atoms[ACOUNT(atom_info)];

    for (i = 0; i < ACOUNT(atom_info); i++)
        names[i] = atom_info[i].name;

    XInternAtoms(app->display(), (char **)names, ACOUNT(atom_info), False, atoms);

    for (i = 0; i < ACOUNT(atom_info); i++)
        *(atom_info[i].atom) = atoms[i];
#else
    for (i = 0; i < ACOUNT(atom_info); i++)
        *(atom_info[i].atom) = XInternAtom(app->display(),
                                           atom_info[i].name, False);
#endif
}

static void initPointers() {
    YApplication::leftPointer.load("left.xpm",  XC_left_ptr);
    YApplication::rightPointer.load("right.xpm", XC_right_ptr);
    YApplication::movePointer.load("move.xpm",  XC_fleur);
}

static void initColors() {
    YColor::black = new YColor("rgb:00/00/00");
    YColor::white = new YColor("rgb:FF/FF/FF");
}

#ifndef LITE
YResourcePaths YApplication::iconPaths;

void initIcons() {
    YApplication::iconPaths.init("icons/");
    defaultAppIcon = YIcon::getIcon("app");
}
#endif

const char *YApplication::getPrivConfDir() {
    static char cfgdir[PATH_MAX] = "";
    
    if (*cfgdir == '\0') {
    	const char *env = getenv("ICEWM_PRIVCFG");

	if (NULL == env) {
	    env = getenv("HOME");
	    strcpy(cfgdir, env ? env : "");
	    strcat(cfgdir, "/.icewm");
	} else {
	    strcpy(cfgdir, env);
	}
	
	msg("using %s for private configuration files", cfgdir);
    }
    
    return cfgdir;
}

char *YApplication::findConfigFile(const char *name) {
    return findConfigFile(name, R_OK);
}

char *YApplication::findConfigFile(const char *name, int mode) {
    char *p;

    p = strJoin(getPrivConfDir(), "/", name, NULL);
    if (access(p, mode) == 0) return p;
    delete[] p;

    p = strJoin(configDir, "/", name, NULL);
    if (access(p, mode) == 0) return p;
    delete[] p;

    p = strJoin(REDIR_ROOT(libDir), "/", name, NULL);
    if (access(p, mode) == 0) return p;
    delete[] p;

    return 0;
}

YApplication::YApplication(int *argc, char ***argv, const char *displayName) {
    app = this;
    fLoopLevel = 0;
    fExitApp = 0;
    lastEventTime = CurrentTime;
    fGrabWindow = 0;
    fGrabTree = 0;
    fXGrabWindow = 0;
    fGrabMouse = 0;
    fPopup = 0;
    fFirstTimer = fLastTimer = 0;
    fFirstPoll = fLastPoll = 0;
    fClip = 0;
    fReplayEvent = false;

    {
	char const * cmd(**argv);
	char cwd[PATH_MAX + 1];

	if ('/' == *cmd)
	    fExecutable = newstr(cmd);
	else if (strchr (cmd, '/'))
	    fExecutable = strJoin(getcwd(cwd, sizeof(cwd)), "/", cmd, NULL);
	else
	    fExecutable = findPath(getenv("PATH"), X_OK, cmd);
    }

    bool runSynchronized(false);

    for (char ** arg = *argv + 1; arg < *argv + *argc; ++arg) {
        if (**arg == '-') {
            char *value;

            if ((value = GET_LONG_ARGUMENT("display")) != NULL)
                displayName = value;
            else if (IS_LONG_SWITCH("sync"))
                runSynchronized = true;
        }
    }
    
    if (displayName == 0)
        displayName = getenv("DISPLAY");
    else {
        static char disp[256] = "DISPLAY=";
        strcat(disp, displayName);
        putenv(disp);
    }

    if (!(fDisplay = XOpenDisplay(displayName)))
        die(1, _("Can't open display: %s. X must be running and $DISPLAY set."),
	         displayName ? displayName : _("<none>"));

    if (runSynchronized)
        XSynchronize(display(), True);

#if CONFIG_XFREETYPE
    int renderEvents, renderErrors;

    haveXft&= (XRenderQueryExtension(display(), &renderEvents, &renderErrors) &&
	       XftDefaultHasRender(display()));

    MSG(("RENDER extension: %d", haveXft));
#endif

    windowContext = XUniqueContext();

    new YDesktop(0, RootWindow(display(), DefaultScreen(display())));
    YPixbuf::init();

    initSignals();
    initAtoms();
    initModifiers();
    initPointers();
    initColors();

#ifdef CONFIG_SHAPE
    shapesSupported = XShapeQueryExtension(display(),
                                           &shapeEventBase, &shapeErrorBase);
#endif

#ifndef LITE
    initIcons();
#endif

#if 0
    struct sigaction sig;
    sig.sa_handler = SIG_IGN;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = 0;
    sigaction(SIGCHLD, &sig, &oldSignalCHLD);
#endif
}

YApplication::~YApplication() {
    XCloseDisplay(display());
    delete[] fExecutable;

    app = NULL;
}

bool YApplication::hasColormap() {
    XVisualInfo pattern;
    pattern.screen = DefaultScreen(display());

    int nVisuals;
    bool rc = false;

    XVisualInfo *first_visual(XGetVisualInfo(display(), VisualScreenMask,
                                              &pattern, &nVisuals));
    XVisualInfo *visual = first_visual;

    while(visual && nVisuals--) {
	if (visual->c_class & 1)
            rc = true;
        visual++;
    }

    if (first_visual)
        XFree(first_visual);

    return rc;
}

void YApplication::registerTimer(YTimer *t) {
    t->fPrev = 0;
    t->fNext = fFirstTimer;
    if (fFirstTimer)
        fFirstTimer->fPrev = t;
    else
        fLastTimer = t;
    fFirstTimer = t;
}

void YApplication::unregisterTimer(YTimer *t) {
    if (t->fPrev)
        t->fPrev->fNext = t->fNext;
    else
        fFirstTimer = t->fNext;
    if (t->fNext)
        t->fNext->fPrev = t->fPrev;
    else
        fLastTimer = t->fPrev;
    t->fPrev = t->fNext = 0;
}

void YApplication::getTimeout(struct timeval *timeout) {
    YTimer *t;
    struct timeval curtime;
    bool fFirst = true;

    gettimeofday(&curtime, 0);
    timeout->tv_sec += curtime.tv_sec;
    timeout->tv_usec += curtime.tv_usec;
    while (timeout->tv_usec >= 1000000) {
        timeout->tv_usec -= 1000000;
        timeout->tv_sec++;
    }

    t = fFirstTimer;
    while (t) {
        if (t->isRunning() && (fFirst || timercmp(timeout, &t->timeout, >))) {
            *timeout = t->timeout;
            fFirst = false;
        }
        t = t->fNext;
    }
    if ((curtime.tv_sec == timeout->tv_sec &&
         curtime.tv_usec == timeout->tv_usec)
        || timercmp(&curtime, timeout, >))
    {
        timeout->tv_sec = 0;
        timeout->tv_usec = 1;
    } else {
        timeout->tv_sec -= curtime.tv_sec;
        timeout->tv_usec -= curtime.tv_usec;
        while (timeout->tv_usec < 0) {
            timeout->tv_usec += 1000000;
            timeout->tv_sec--;
        }
    }
    //msg("set: %d %d", timeout->tv_sec, timeout->tv_usec);
    PRECONDITION(timeout->tv_sec >= 0);
    PRECONDITION(timeout->tv_usec >= 0);
}

void YApplication::handleTimeouts() {
    YTimer *t, *n;
    struct timeval curtime;
    gettimeofday(&curtime, 0);

    t = fFirstTimer;
    while (t) {
        n = t->fNext;
        if (t->isRunning() && timercmp(&curtime, &t->timeout, >)) {
            YTimerListener *l = t->getTimerListener();
            t->stopTimer();
            if (l && l->handleTimer(t))
                t->startTimer();
        }
        t = n;
    }
}

void YApplication::afterWindowEvent(XEvent & /*xev*/) {
}

extern void logEvent(XEvent xev);

void YApplication::registerPoll(YPoll *t) {
    PRECONDITION(t->fd != -1);
    t->fPrev = 0;
    t->fNext = fFirstPoll;
    if (fFirstPoll)
        fFirstPoll->fPrev = t;
    else
        fLastPoll = t;
    fFirstPoll = t;
}

void YApplication::unregisterPoll(YPoll *t) {
    if (t->fPrev)
        t->fPrev->fNext = t->fNext;
    else
        fFirstPoll = t->fNext;
    if (t->fNext)
        t->fNext->fPrev = t->fPrev;
    else
        fLastPoll = t->fPrev;
    t->fPrev = t->fNext = 0;
}

bool YApplication::filterEvent(const XEvent &xev) {
    if (xev.type == KeymapNotify) {
        XMappingEvent xmapping = xev.xmapping; /// X headers const missing?
        XRefreshKeyboardMapping(&xmapping);

        // !!! we should probably regrab everything ?
        initModifiers();
        return true;
    }
    return false;
}

int YApplication::mainLoop() {
    fLoopLevel++;
    fExitLoop = 0;

    struct timeval timeout, *tp;

    struct timeval prevtime, curtime, difftime, maxtime = { 0, 0 };

    while (!fExitApp && !fExitLoop) {
        if (XPending(display()) > 0) {
            XEvent xev;

            XNextEvent(display(), &xev);
            xeventcount++;
            //msg("%d", xev.type);

            gettimeofday(&prevtime, 0);
            saveEventTime(xev);

#ifdef DEBUG
            DBG logEvent(xev);
#endif

            if (filterEvent(xev)) {
                ;
            } else {
                YWindow *window = 0;
                int rc = 0;
                int ge = (xev.type == ButtonPress ||
                          xev.type == ButtonRelease ||
                          xev.type == MotionNotify ||
                          xev.type == KeyPress ||
                          xev.type == KeyRelease /*||
                          xev.type == EnterNotify ||
                          xev.type == LeaveNotify*/) ? 1 : 0;

                fReplayEvent = false;

                if (fPopup && ge) {
                    handleGrabEvent(fPopup, xev);
                } else if (fGrabWindow && ge) {
                    handleGrabEvent(fGrabWindow, xev);
                } else {
                    if ((rc = XFindContext(display(),
                                           xev.xany.window,
                                           windowContext,
                                           (XPointer *)&window)) == 0)
                    {
                        window->handleEvent(xev);
                    } else {
                        if (xev.type == MapRequest) {
                            // !!! java seems to do this ugliness
                            //YFrameWindow *f = getFrame(xev.xany.window);
                            msg("APP BUG? mapRequest for window %lX sent to destroyed frame %lX!",
                                xev.xmaprequest.parent,
                                xev.xmaprequest.window);
                            desktop->handleEvent(xev);
                        } else if (xev.type == ConfigureRequest) {
                            msg("APP BUG? configureRequest for window %lX sent to destroyed frame %lX!",
                                xev.xmaprequest.parent,
                                xev.xmaprequest.window);
                            desktop->handleEvent(xev);
                        } else if (xev.type != DestroyNotify) {
                            MSG(("unknown window 0x%lX event=%d", xev.xany.window, xev.type));
                        }
                    }
                    if (xev.type == KeyPress || xev.type == KeyRelease) ///!!!
                        afterWindowEvent(xev);
                }
                if (fGrabWindow) {
                    if (xev.type == ButtonPress || xev.type == ButtonRelease || xev.type == MotionNotify)
                    {
                        if (!fReplayEvent) {
                            XAllowEvents(app->display(), SyncPointer, CurrentTime);
                        }
                    }
                }
            }
            gettimeofday(&curtime, 0);
            difftime.tv_sec = curtime.tv_sec - prevtime.tv_sec;
            difftime.tv_usec = curtime.tv_usec - prevtime.tv_usec;
            if (difftime.tv_usec < 0) {
                difftime.tv_sec--;
                difftime.tv_usec += 1000000;
            }
            if (difftime.tv_sec > maxtime.tv_sec ||
                (difftime.tv_sec == maxtime.tv_sec && difftime.tv_usec > maxtime.tv_usec))
            {
                MSG(("max_latency: %d.%06d", difftime.tv_sec, difftime.tv_usec));
                maxtime = difftime;
            }
        } else {
            int rc;
            fd_set read_fds;
            fd_set write_fds;

            handleIdle();

            FD_ZERO(&read_fds);
            FD_ZERO(&write_fds);
            FD_SET(ConnectionNumber(app->display()), &read_fds);
            if (signalPipe[0] != -1)
                FD_SET(signalPipe[0], &read_fds);

#warning "make this more general"
            int IceSMfd = readFdCheckSM();
            if (IceSMfd != -1)
                FD_SET(IceSMfd, &read_fds);

            {
                for (YPoll *s = fFirstPoll; s; s = s->fNext) {
                    PRECONDITION(s->fd != -1);
                    if (s->forRead()) {
                        FD_SET(s->fd, &read_fds);
                        MSG(("wait read"));
                    }
                    if (s->forWrite()) {
                        FD_SET(s->fd, &write_fds);
                        MSG(("wait connect"));
                    }
                }
            }

            timeout.tv_sec = 0;
            timeout.tv_usec = 0;
            getTimeout(&timeout);
            tp = 0;
            if (timeout.tv_sec != 0 || timeout.tv_usec != 0)
                tp = &timeout;
            else
                tp = 0;

            sigprocmask(SIG_UNBLOCK, &signalMask, NULL);

            rc = select(sizeof(fd_set),
                        SELECT_TYPE_ARG234 &read_fds,
                        SELECT_TYPE_ARG234 &write_fds,
                        0,
                        tp);

            sigprocmask(SIG_BLOCK, &signalMask, NULL);
#if 0
            sigset_t mask;
            sigpending(&mask);
            if (sigismember(&mask, SIGINT))
                handleSignal(SIGINT);
            if (sigismember(&mask, SIGTERM))
                handleSignal(SIGTERM);
            if (sigismember(&mask, SIGHUP))
                handleSignal(SIGHUP);
#endif

            if (rc == 0) {
                handleTimeouts();
            } else if (rc == -1) {
                if (errno != EINTR)
                    warn(_("Message Loop: select failed (errno=%d)"), errno);
            } else {
                if (signalPipe[0] != -1) {
                    if (FD_ISSET(signalPipe[0], &read_fds)) {
                        unsigned char sig;
                        if (read(signalPipe[0], &sig, 1) == 1) {
                            handleSignal(sig);
                        }
                    }
                }
                {
                    for (YPoll *s = fFirstPoll; s; s = s->fNext) {
                        PRECONDITION(s->fd != -1);
                        int fd = s->fd;
                        if (FD_ISSET(fd, &read_fds)) {
                            MSG(("got read"));
                            s->notifyRead();
                        }
                        if (FD_ISSET(fd, &write_fds)) {
                            MSG(("got connect"));
                            s->notifyWrite();
                        }
                    }
                }
                if (IceSMfd != -1 && FD_ISSET(IceSMfd, &read_fds)) {
                    readFdActionSM();
                }
            }
        }
    }
    fLoopLevel--;
    return fExitCode;
}

void YApplication::dispatchEvent(YWindow *win, XEvent &xev) {
    if (xev.type == KeyPress || xev.type == KeyRelease) {
        YWindow *w = win;
        while (w && (w->handleKey(xev.xkey) == false)) {
            if (fGrabTree && w == fXGrabWindow)
                break;
            w = w->parent();
        }
    } else {
        Window child;

        if (xev.type == MotionNotify) {
            if (xev.xmotion.window != win->handle())
                if (XTranslateCoordinates(app->display(),
                                          xev.xany.window, win->handle(),
                                          xev.xmotion.x, xev.xmotion.y,
                                          &xev.xmotion.x, &xev.xmotion.y, &child) == True)
                    xev.xmotion.window = win->handle();
                else
                    return ;
        } else if (xev.type == ButtonPress || xev.type == ButtonRelease ||
                   xev.type == EnterNotify || xev.type == LeaveNotify)
        {
            if (xev.xbutton.window != win->handle())
                if (XTranslateCoordinates(app->display(),
                                          xev.xany.window, win->handle(),
                                          xev.xbutton.x, xev.xbutton.y,
                                          &xev.xbutton.x, &xev.xbutton.y, &child) == True)
                    xev.xbutton.window = win->handle();
                else
                    return ;
        } else if (xev.type == KeyPress || xev.type == KeyRelease) {
            if (xev.xkey.window != win->handle())
                if (XTranslateCoordinates(app->display(),
                                          xev.xany.window, win->handle(),
                                          xev.xkey.x, xev.xkey.y,
                                          &xev.xkey.x, &xev.xkey.y, &child) == True)
                    xev.xkey.window = win->handle();
                else
                    return ;
        }
        win->handleEvent(xev);
    }
}

void YApplication::handleGrabEvent(YWindow *winx, XEvent &xev) {
    YWindow *win = winx;

    PRECONDITION(win != 0);
    if (fGrabTree) {
        if (xev.xbutton.subwindow != None) {
            if (XFindContext(display(),
                         xev.xbutton.subwindow,
                         windowContext,
                         (XPointer *)&win) != 0);
                if (xev.type == EnterNotify || xev.type == LeaveNotify)
                    win = 0;
                else
                    win = fGrabWindow;
        } else {
            if (XFindContext(display(),
                         xev.xbutton.window,
                         windowContext,
                         (XPointer *)&win) != 0)
                if (xev.type == EnterNotify || xev.type == LeaveNotify)
                    win = 0;
                else
                    win = fGrabWindow;
        }
        if (win == 0)
            return ;
        {
            YWindow *p = win;
            while (p) {
                if (p == fXGrabWindow)
                    break;
                p = p->parent();
            }
            if (p == 0) {
                if (xev.type == EnterNotify || xev.type == LeaveNotify)
                    return ;
                else
                    win = fGrabWindow;
            }
        }
        if (xev.type == EnterNotify || xev.type == LeaveNotify)
            if (win != fGrabWindow)
                return ;
        if (fGrabWindow != fXGrabWindow)
            win = fGrabWindow;
    }
    dispatchEvent(win, xev);
}

void YApplication::replayEvent() {
    if (!fReplayEvent) {
        fReplayEvent = true;
        XAllowEvents(app->display(), ReplayPointer, CurrentTime);
    }
}

void YApplication::captureGrabEvents(YWindow *win) {
    if (fGrabWindow == fXGrabWindow && fGrabTree) {
        fGrabWindow = win;
    }
}

void YApplication::releaseGrabEvents(YWindow *win) {
    if (win == fGrabWindow && fGrabTree) {
        fGrabWindow = fXGrabWindow;
    }
}

int YApplication::grabEvents(YWindow *win, Cursor ptr, unsigned int eventMask, int grabMouse, int grabKeyboard, int grabTree) {
    int rc;

    if (fGrabWindow != 0)
        return 0;
    if (win == 0)
        return 0;

    XSync(display(), 0);
    fGrabTree = grabTree;
    if (grabMouse) {
        fGrabMouse = 1;
        rc = XGrabPointer(display(), win->handle(),
                          grabTree ? True : False,
                          eventMask,
                          GrabModeSync, GrabModeAsync,
                          None, ptr, CurrentTime);

        if (rc != Success) {
            MSG(("grab status = %d\x7", rc));
            return 0;
        }
    } else {
        fGrabMouse = 0;

        XChangeActivePointerGrab(display(),
                                 eventMask,
                                 ptr, CurrentTime);
    }

    if (grabKeyboard) {
        rc = XGrabKeyboard(display(), win->handle(),
                           ///False,
                           grabTree ? True : False,
                           GrabModeSync, GrabModeAsync, CurrentTime);
        if (rc != Success && grabMouse) {
            MSG(("grab status = %d\x7", rc));
            XUngrabPointer(display(), CurrentTime);
            return 0;
        }
    }
    XAllowEvents(app->display(), SyncPointer, CurrentTime);

    desktop->resetColormapFocus(false);

    fXGrabWindow = win;
    fGrabWindow = win;
    return 1;
}

int YApplication::releaseEvents() {
    if (fGrabWindow == 0)
        return 0;
    fGrabWindow = 0;
    fXGrabWindow = 0;
    fGrabTree = 0;
    if (fGrabMouse) {
        XUngrabPointer(display(), CurrentTime);
        fGrabMouse = 0;
    }
    XUngrabKeyboard(display(), CurrentTime);
    desktop->resetColormapFocus(true);
    return 1;
}

void YApplication::exitLoop(int exitCode) {
    fExitLoop = 1;
    fExitCode = exitCode;
}

void YApplication::exit(int exitCode) {
    fExitApp = 1;
    exitLoop(exitCode);
}

void YApplication::saveEventTime(const XEvent &xev) {
    //lastEventTime = CurrentTime;
    //return ;
    switch (xev.type) {
    case ButtonPress:
    case ButtonRelease:
        lastEventTime = xev.xbutton.time;
        break;

    case MotionNotify:
        lastEventTime = xev.xmotion.time;
        break;

    case KeyPress:
    case KeyRelease:
        lastEventTime = xev.xkey.time;
        break;

    case EnterNotify:
    case LeaveNotify:
        lastEventTime = xev.xcrossing.time;
        break;

    case PropertyNotify:
        lastEventTime = xev.xproperty.time;
        break;

    case SelectionClear:
        lastEventTime = xev.xselectionclear.time;
        break;

    case SelectionRequest:
        lastEventTime = xev.xselectionrequest.time;
        break;

    case SelectionNotify:
        lastEventTime = xev.xselection.time;
        break;
    default:
        lastEventTime = CurrentTime;
        break;
    }
}

void YApplication::handleSignal(int sig) {
    if (sig == SIGCHLD) {
        int s, rc;
        do {
            rc = waitpid(-1, &s, WNOHANG);
        } while (rc > 0);
    }
}

void YApplication::handleIdle() {
}

void sig_handler(int sig) {
    unsigned char uc = sig;
    static const char *s = "icewm: signal error\n";
    if (write(signalPipe[1], &uc, 1) != 1)
        write(2, s, strlen(s));
}

void YApplication::catchSignal(int sig) {
    sigaddset(&signalMask, sig);
    sigprocmask(SIG_BLOCK, &signalMask, NULL);

    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(sig, &sa, NULL);
}

void YApplication::resetSignals() {
    sigset_t mask;
    //struct sigaction old_sig;

    //sigaction(SIGCHLD, &oldSignalCHLD, &old_sig);
    sigprocmask(SIG_SETMASK, &oldSignalMask, &mask);
}

static KeyCode sym2code(KeySym k) {
    return XKeysymToKeycode(app->display(), k);
}

void YApplication::initModifiers() {
    XModifierKeymap *xmk = XGetModifierMapping(app->display());
    AltMask = MetaMask = WinMask = SuperMask = HyperMask = 
	NumLockMask = ScrollLockMask = ModeSwitchMask = 0;
    
    if (xmk) {
        KeyCode numLockKeyCode = sym2code(XK_Num_Lock);
        KeyCode scrollLockKeyCode = sym2code(XK_Scroll_Lock);
        KeyCode altKeyCode = sym2code(XK_Alt_L);
        KeyCode metaKeyCode = sym2code(XK_Meta_L);
        KeyCode superKeyCode = sym2code(XK_Super_L);
        KeyCode hyperKeyCode = sym2code(XK_Hyper_L);
        KeyCode modeSwitchCode = sym2code(XK_Mode_switch);

        // A keyboard might lack Alt_L but have Alt_R.
        if (!altKeyCode)
            altKeyCode = sym2code(XK_Alt_R);
        if (!metaKeyCode)
            metaKeyCode = sym2code(XK_Meta_R);
        if (!superKeyCode)
            superKeyCode = sym2code(XK_Super_R);
        if (!hyperKeyCode)
            hyperKeyCode = sym2code(XK_Hyper_R);

        KeyCode *c = xmk->modifiermap;

        for (int m = 0; m < 8; m++)
            for (int k = 0; k < xmk->max_keypermod; k++, c++) {
                if (*c == NoSymbol)
                    continue;
                // If numLockKeyCode == 0, it can never match.
                if (*c == numLockKeyCode)
                    NumLockMask = (1 << m);
                if (*c == scrollLockKeyCode)
                    ScrollLockMask = (1 << m);
                if (*c == altKeyCode)
                    AltMask = (1 << m);
                if (*c == metaKeyCode)
                    MetaMask = (1 << m);
                if (*c == superKeyCode)
                    SuperMask = (1 << m);
                if (*c == hyperKeyCode)
                    HyperMask = (1 << m);
                if (*c == modeSwitchCode)
                    ModeSwitchMask = (1 << m);
            }

	XFreeModifiermap(xmk);
    }
    if (MetaMask == AltMask)
        MetaMask = 0;

    MSG(("alt:%d meta:%d super:%d hyper:%d mode:%d num:%d scroll:%d",
         AltMask, MetaMask, SuperMask, HyperMask, ModeSwitchMask,
	 NumLockMask, ScrollLockMask));

    // some hacks for "broken" modifier configurations

    // this basically does what <0.9.13 versions did
    if (AltMask != 0 && MetaMask == Mod1Mask) {
        MetaMask = AltMask;
        AltMask = Mod1Mask;
    }

    if (AltMask == 0 && MetaMask != 0) {
        if (MetaMask != Mod1Mask) {
            AltMask = Mod1Mask;
        }
        else {
            AltMask = MetaMask;
            MetaMask = 0;
        }
    }

    if (AltMask == 0)
        AltMask = Mod1Mask;

    PRECONDITION(app->AltMask != 0);
    PRECONDITION(app->AltMask != ShiftMask);
    PRECONDITION(app->AltMask != ControlMask);
    PRECONDITION(app->AltMask != app->MetaMask);

    KeyMask =
        ControlMask |
        ShiftMask |
        AltMask |
        MetaMask |
        SuperMask |
        HyperMask |
	ModeSwitchMask;

    ButtonMask =
        Button1Mask |
        Button2Mask |
        Button3Mask |
        Button4Mask |
        Button5Mask;

    ButtonKeyMask = KeyMask | ButtonMask;

#if 0
    KeySym wl = XKeycodeToKeysym(app->display(), 115, 0);
    KeySym wr = XKeycodeToKeysym(app->display(), 116, 0);

    if (wl == XK_Super_L) {
    } else if (wl == XK_Meta_L) {
    }
#endif
    // this will do for now, but we should actualy check the keycodes
    Win_L = Win_R = 0;

    if (SuperMask != 0) {
        WinMask = SuperMask;
	
	if (win95keys) {
	    Win_L = XK_Super_L;
	    Win_R = XK_Super_R;
	}
    } else if (MetaMask != 0) {
        WinMask = MetaMask;
	
	if (win95keys) {
            Win_L = XK_Super_L;
            Win_R = XK_Super_R;
	}
    }

    MSG(("alt:%d meta:%d super:%d hyper:%d win:%d mode:%d num:%d scroll:%d",
         AltMask, MetaMask, SuperMask, HyperMask, WinMask, ModeSwitchMask,
	 NumLockMask, ScrollLockMask));

}

void YApplication::alert() {
    XBell(display(), 100);
}

void YApplication::runProgram(const char *path, const char *const *args) {
    XSync(app->display(), False);

    if (path && fork() == 0) {
        app->resetSignals();
        sigemptyset(&signalMask);
        sigaddset(&signalMask, SIGHUP);
        sigprocmask(SIG_UNBLOCK, &signalMask, NULL);

        /* perhaps the right thing to to:
         create ptys .. and show console window when an application
         attempts to use it (how do we detect input with no output?) */
        setsid();
#if 0
        close(0);
        if (open("/dev/null", O_RDONLY) != 0)
            _exit(1);
#endif
#if 1   /* for now, some debugging code */
        {
            /* close all files */

            int             i, max = 1024;
            struct rlimit   lim;

            if (getrlimit(RLIMIT_NOFILE, &lim) == 0)
                max = lim.rlim_max;

            for (i = 3; i < max; i++) {
                int fl;
                if (fcntl(i, F_GETFD, &fl) == 0) {
                    if (!(fl & FD_CLOEXEC)) {
                        warn("file descriptor still open: %d. "
                             " Check /proc/$icewmpid/fd/%d when running next time. "
                             "Please report a bug (perhaps not an icewm problem)!", i, i);
                    }
                }
                close (i);
            }
        }
#endif

        if (args)
            execvp(path, (char **)args);
        else
            execlp(path, path, 0);

        _exit(99);
    }
}

void YApplication::runCommand(const char *cmdline) {
#warning calling /bin/sh is considered to be bloat
    char const * argv[] = { "/bin/sh", "-c", cmdline, NULL };
    runProgram(argv[0], argv);
}

void YApplication::setClipboardText(char *data, int len) {
    if (fClip == 0)
        fClip = new YClipboard();
    if (!fClip)
        return ;
    fClip->setData(data, len);
}

bool YApplication::hasGNOME() {
#ifdef CONFIG_GNOME_ROOT_PROPERTY
    Atom GNOME_ROOT_PROPERTY(XInternAtom(display(),
    			     CONFIG_GNOME_ROOT_PROPERTY, true));

    Atom r_type; int r_format;
    unsigned long count, bytes_remain;
    Window gnomeName;

    return (GNOME_ROOT_PROPERTY != None &&
	    XGetWindowProperty(display(), desktop->handle(),
			       GNOME_ROOT_PROPERTY, 0, 1, false, XA_WINDOW,
			       &r_type, &r_format, &count, &bytes_remain,
			       (unsigned char **) &gnomeName) == Success);
#else // this also detects xsm as GNOME, how to detect KDE?
    return getenv("SESSION_MANAGER");
#endif
}
