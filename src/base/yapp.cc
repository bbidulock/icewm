/*
 * IceWM
 *
 * Copyright (C) 1998 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "yapp.h"
#include "ysocket.h"
#include "ytimer.h"
#include "ypopup.h"
#include "yresource.h"
#include "ywindow.h"
#include "ycstring.h"

#include "sysdep.h"
#include "MwmUtil.h"
#include "WinMgr.h"
#include "prefs.h"

#ifdef SM
#include <X11/SM/SMlib.h>
#endif

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
Atom _XA_SM_CLIENT_ID;
Atom _XA_CLIPBOARD;
Atom _XA_TARGETS;

#ifdef GNOME1_HINTS
Atom _XA_WIN_PROTOCOLS;
Atom _XA_WIN_WORKSPACE;
Atom _XA_WIN_WORKSPACE_COUNT;
Atom _XA_WIN_WORKSPACE_NAMES;
Atom _XA_WIN_WORKAREA;
Atom _XA_WIN_ICONS;
Atom _XA_WIN_STATE;
Atom _XA_WIN_LAYER;
Atom _XA_WIN_HINTS;
Atom _XA_WIN_SUPPORTING_WM_CHECK;
Atom _XA_WIN_CLIENT_LIST;
Atom _XA_WIN_DESKTOP_BUTTON_PROXY;
Atom _XA_WIN_AREA;
Atom _XA_WIN_AREA_COUNT;

#endif
Atom _XA_KWM_WIN_ICON;

Atom XA_XdndAware;
Atom XA_XdndEnter;
Atom XA_XdndLeave;
Atom XA_XdndPosition;
Atom XA_XdndStatus;
Atom XA_XdndDrop;
Atom XA_XdndFinished;
Atom XA_XdndSelection;
Atom XA_XdndTypeList;

Colormap defaultColormap;

Cursor leftPointer;
Cursor rightPointer;
Cursor movePointer;

YColor *YColor::black = 0;
YColor *YColor::white = 0;

#ifdef SHAPE
int shapesSupported;
int shapeEventBase, shapeErrorBase;
#endif

#ifdef SM
static int IceSMfd = -1;
static IceConn IceSMconn = NULL;
static SmcConn SMconn = NULL;
static char *oldSessionId = NULL;
static char *newSessionId = NULL;
static char *sessionProg;

char *getsesfile() {
    static char name[1024] = "";

    if (name[0] == 0) {
        sprintf(name, "%s/.icewm", getenv("HOME"));
        mkdir(name, 0755);
        sprintf(name, "%s/.icewm/%s.ses",
                getenv("HOME"),
                newSessionId);
    }
    return name;
}

void iceWatchFD(IceConn conn,
                IcePointer /*client_data*/,
                Bool opening,
                IcePointer */*watch_data*/)
{
    if (opening) {
        if (IceSMfd != -1) { // shouldn't happen
            fprintf(stderr, "TOO MANY ICE CONNECTIONS -- not supported\n");
        } else {
            IceSMfd = IceConnectionNumber(conn);
            fcntl(IceSMfd, F_SETFD, FD_CLOEXEC);
        }
    } else {
        if (IceConnectionNumber(conn) == IceSMfd)
            IceSMfd = -1;
    }
}

void saveYourselfPhase2Proc(SmcConn /*conn*/, SmPointer /*client_data*/) {
    app->smSaveYourselfPhase2();
}

void saveYourselfProc(SmcConn /*conn*/,
                      SmPointer /*client_data*/,
                      int /*save_type*/,
                      Bool shutdown,
                      int /*interact_style*/,
                      Bool fast)
{
    app->smSaveYourself(shutdown ? true : false, fast ? true : false);
}

void shutdownCancelledProc(SmcConn /*conn*/, SmPointer /*client_data*/) {
    app->smShutdownCancelled();
}

void saveCompleteProc(SmcConn /*conn*/, SmPointer /*client_data*/) {
    app->smSaveComplete();
}

void dieProc(SmcConn /*conn*/, SmPointer /*client_data*/) {
    app->smDie();
}

static void setSMProperties() {
    SmPropValue programVal = { 0, NULL };
    SmPropValue userIDVal = { 0, NULL };
    SmPropValue restartVal[3] = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
    SmPropValue discardVal[4] = { { 0, NULL }, { 0, NULL }, { 0, NULL } };

    // broken headers in SMlib?
    SmProp programProp = { (char *)SmProgram, (char *)SmLISTofARRAY8, 1, &programVal };
    SmProp userIDProp = { (char *)SmUserID, (char *)SmARRAY8, 1, &userIDVal };
    SmProp restartProp = { (char *)SmRestartCommand, (char *)SmLISTofARRAY8, 3, (SmPropValue *)&restartVal };
    SmProp cloneProp = { (char *)SmCloneCommand, (char *)SmLISTofARRAY8, 1, (SmPropValue *)&restartVal };
    SmProp discardProp = { (char *)SmDiscardCommand, (char *)SmLISTofARRAY8, 3, (SmPropValue *)&discardVal };
    SmProp *props[] = {
        &programProp,
        &userIDProp,
        &restartProp,
        &cloneProp,
        &discardProp
    };

    char *user = getenv("USER");
    const char *clientId = "-clientId";

    programVal.length = strlen(sessionProg);
    programVal.value = sessionProg;
    userIDVal.length = strlen(user);
    userIDVal.value = (SmPointer)user;

    restartVal[0].length = strlen(sessionProg);
    restartVal[0].value = sessionProg;
    restartVal[1].length = strlen(clientId);
    restartVal[1].value = (char *)clientId;
    restartVal[2].length = strlen(newSessionId);
    restartVal[2].value = newSessionId;

    const char *rmprog = "/bin/rm";
    const char *rmarg = "-f";
    char *sidfile = getsesfile();

    discardVal[0].length = strlen(rmprog);
    discardVal[0].value = (char *)rmprog;
    discardVal[1].length = strlen(rmarg);
    discardVal[1].value = (char *)rmarg;
    discardVal[2].length = strlen(sidfile);
    discardVal[2].value = sidfile;

    SmcSetProperties(SMconn,
                     sizeof(props)/sizeof(props[0]),
                     (SmProp **)&props);
}

static void initSM() {
    if (getenv("SESSION_MANAGER") == 0)
        return;
    if (IceAddConnectionWatch(&iceWatchFD, NULL) == 0) {
        fprintf(stderr, "IceAddConnectionWatch failed.");
        return ;
    }

    char error_str[256];
    SmcCallbacks smcall;

    memset(&smcall, 0, sizeof(smcall));
    smcall.save_yourself.callback = &saveYourselfProc;
    smcall.save_yourself.client_data = NULL;
    smcall.die.callback = &dieProc;
    smcall.die.client_data = NULL;
    smcall.save_complete.callback = &saveCompleteProc;
    smcall.save_complete.client_data = NULL;
    smcall.shutdown_cancelled.callback = &shutdownCancelledProc;
    smcall.shutdown_cancelled.client_data = NULL;

    if ((SMconn = SmcOpenConnection(NULL, /* network ids */
                                    NULL, /* context */
                                    1, 0, /* protocol major, minor */
                                    SmcSaveYourselfProcMask |
                                    SmcSaveCompleteProcMask |
                                    SmcShutdownCancelledProcMask |
                                    SmcDieProcMask,
                                    &smcall,
                                    oldSessionId, &newSessionId,
                                    sizeof(error_str), error_str)) == NULL)
    {
        fprintf(stderr, "session mgr init error: %s\n", error_str);
        return ;
    }
    IceSMconn = SmcGetIceConnection(SMconn);

    setSMProperties();
}

void YApplication::smSaveYourself(bool /*shutdown*/, bool /*fast*/) {
    SmcRequestSaveYourselfPhase2(SMconn, &saveYourselfPhase2Proc, NULL);
}

void YApplication::smSaveYourselfPhase2() {
    SmcSaveYourselfDone(SMconn, True);
}

void YApplication::smSaveDone() {
    SmcSaveYourselfDone(SMconn, True);
}

void YApplication::smSaveComplete() {
}

void YApplication::smShutdownCancelled() {
    SmcSaveYourselfDone(SMconn, False);
}

void YApplication::smCancelShutdown() {
    SmcSaveYourselfDone(SMconn, False); /// !!! broken
}

void YApplication::smDie() {
    app->exit(0);
}

bool YApplication::haveSessionManager() {
    if (SMconn != NULL)
        return true;
    return false;
}

void YApplication::smRequestShutdown() {
    // !!! doesn't seem to work with xsm
    SmcRequestSaveYourself(SMconn,
                           SmSaveLocal, //!!! ???
                           True,
                           SmInteractStyleAny,
                           False,
                           True);
}

#endif

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
    sigprocmask(SIG_BLOCK, &signalMask, &oldSignalMask);

    if (pipe(signalPipe) != 0)
        die(2, "pipe create failed, errno=%d.", errno);
    fcntl(signalPipe[1], F_SETFL, O_NONBLOCK);
}

static void initAtoms() {
    Atom *atoms[] = {
#ifdef GNOME1_HINTS
        &_XA_WIN_WORKSPACE,
        &_XA_WIN_WORKSPACE_COUNT,
        &_XA_WIN_WORKSPACE_NAMES,
        &_XA_WIN_WORKAREA,
        &_XA_WIN_ICONS,
        &_XA_WIN_LAYER,
        &_XA_WIN_STATE,
        &_XA_WIN_HINTS,
        &_XA_WIN_PROTOCOLS,
        &_XA_WIN_SUPPORTING_WM_CHECK,
        &_XA_WIN_CLIENT_LIST,
        &_XA_WIN_DESKTOP_BUTTON_PROXY,
        &_XA_WIN_AREA,
        &_XA_WIN_AREA_COUNT,
#endif
        &_XA_WM_PROTOCOLS,
        &_XA_WM_TAKE_FOCUS,
        &_XA_WM_DELETE_WINDOW,
        &_XA_WM_STATE,
        &_XA_WM_CHANGE_STATE,
        &_XA_WM_COLORMAP_WINDOWS,
        &_XA_WM_CLIENT_LEADER,
        &_XA_SM_CLIENT_ID,
        &_XATOM_MWM_HINTS,
        &_XA_KWM_WIN_ICON,
        &_XA_CLIPBOARD,
        &_XA_TARGETS,
        &XA_XdndAware,
        &XA_XdndEnter,
        &XA_XdndLeave,
        &XA_XdndPosition,
        &XA_XdndStatus,
        &XA_XdndDrop,
        &XA_XdndFinished,
        &XA_XdndSelection,

        &XA_XdndTypeList
    };
    const char *names[] = {
#ifdef GNOME1_HINTS
        XA_WIN_WORKSPACE,
        XA_WIN_WORKSPACE_COUNT,
        XA_WIN_WORKSPACE_NAMES,
        XA_WIN_WORKAREA,
        XA_WIN_ICONS,
        XA_WIN_LAYER,
        XA_WIN_STATE,
        XA_WIN_HINTS,
        XA_WIN_PROTOCOLS,
        XA_WIN_SUPPORTING_WM_CHECK,
        XA_WIN_CLIENT_LIST,
        XA_WIN_DESKTOP_BUTTON_PROXY,
        XA_WIN_AREA,
        XA_WIN_AREA_COUNT,
#endif
        "WM_PROTOCOLS",
        "WM_TAKE_FOCUS",
        "WM_DELETE_WINDOW",
        "WM_STATE",
        "WM_CHANGE_STATE",
        "WM_COLORMAP_WINDOWS",
        "WM_CLIENT_LEADER",
        "SM_CLIENT_ID",
        _XA_MOTIF_WM_HINTS,
        "KWM_WIN_ICON",
        "CLIPBOARD",
        "TARGETS",
        "XdndAware",
        "XdndEnter",
        "XdndLeave",
        "XdndPosition",
        "XdndStatus",
        "XdndDrop",
        "XdndFinished",
        "XdndSelection",
        "XdndTypeList"
    };
    PRECONDITION(ACOUNT(names) == ACOUNT(atoms));

#ifdef HAVE_XINTERNATOMS
    Atom atoms2[ACOUNT(atoms)];
    XInternAtoms(app->display(), (char **)names, ACOUNT(atoms), False, atoms2);
    for (unsigned int i = 0; i < ACOUNT(atoms); i++)
        *(atoms[i]) = atoms2[i];
#else
    for (unsigned int i = 0; i < ACOUNT(atoms); i++)
        *(atoms[i]) = XInternAtom(app->display(),
                                           names[i], False);
#endif
}

static void initPointers() {
    leftPointer = XCreateFontCursor(app->display(), XC_left_ptr);
    rightPointer = XCreateFontCursor(app->display(), XC_right_ptr);
    movePointer = XCreateFontCursor(app->display(), XC_fleur);
}

static void initColors() {
    YColor::black = new YColor("rgb:00/00/00");
    YColor::white = new YColor("rgb:FF/FF/FF");
}

#if 0
void verifyPaths(pathelem *search, const char *base) {
    unsigned int j = 0, i = 0;

    for (; search[i].root; i++) {
        char *path = joinPath(search + i, base, "");
        if (access(path, R_OK | X_OK) == 0)
            search[j++] = search[i];
        delete path;

    }
    search[j] = search[i];
}
#endif

YApplication::YApplication(const char *appname, int *argc, char ***argv, const char *displayName) {
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
    fFirstSocket = fLastSocket = 0;
    fClip = 0;
    fReplayEvent = false;
    fAppName = CStr::newstr(appname);
    fPrefDomains = 0;

    bool sync = false;

    for (int i = 1; i < *argc; i++) {
        if ((*argv)[i][0] == '-') {
            if (strcmp((*argv)[i], "-display") == 0) {
                displayName = (*argv)[++i];
#ifdef SM
            } else if (strcmp((*argv)[i], "-clientId") == 0) {
                oldSessionId = (*argv)[++i];
#endif
            } else if (strcmp((*argv)[i], "-sync") == 0) {
                sync = true;
            }
        }
    }

    if (displayName == 0)
        displayName = getenv("DISPLAY");
#if 0  // will no longer be necessary
    else {
        static char disp[256] = "DISPLAY=";
        strcat(disp, displayName);
        putenv(disp);
    }
#endif

    if (!(fDisplay = XOpenDisplay(displayName)))
        die(1, "Can't open display: %s. X must be running and $DISPLAY set.", displayName ? displayName : "<none>");

    if (sync)
        XSynchronize(display(), True);

    defaultColormap = DefaultColormap(display(), DefaultScreen(display()));

    windowContext = XUniqueContext();

    initSignals();
    initAtoms();
    initPointers();
    initColors();

#ifdef SHAPE
    shapesSupported = XShapeQueryExtension(display(),
                                           &shapeEventBase, &shapeErrorBase);
#endif

    new YDesktop(0, RootWindow(display(), DefaultScreen(display())));

#ifdef SM
    sessionProg = (*argv)[0]; //"icewm";
    initSM();
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
    freePrefs();
    YIcon::freeIcons();
    delete fAppName;
#ifdef SM
    if (SMconn != 0) {
        SmcCloseConnection(SMconn, 0, NULL);
        SMconn = NULL;
        IceSMconn = NULL;
    }
#endif
    XCloseDisplay(display());
    app = 0;
}

bool YApplication::hasColormap() {
    if (DefaultVisual(display(), DefaultScreen(display()))->c_class & 1 )
        return true;
    return false;
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
    //printf("set: %d %d\n", timeout->tv_sec, timeout->tv_usec);
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

void YApplication::registerSocket(YSocket *t) {
    t->fPrev = 0;
    t->fNext = fFirstSocket;
    if (fFirstSocket)
        fFirstSocket->fPrev = t;
    else
        fLastSocket = t;
    fFirstSocket = t;
}

void YApplication::unregisterSocket(YSocket *t) {
    if (t->fPrev)
        t->fPrev->fNext = t->fNext;
    else
        fFirstSocket = t->fNext;
    if (t->fNext)
        t->fNext->fPrev = t->fPrev;
    else
        fLastSocket = t->fPrev;
    t->fPrev = t->fNext = 0;
}

int YApplication::mainLoop() {
    fLoopLevel++;
    fExitLoop = 0;

    struct timeval timeout, *tp;

    while (!fExitApp && !fExitLoop) {
        if (XPending(display()) > 0) {
            XEvent xev;

            XNextEvent(display(), &xev);
            //printf("%d\n", xev.type);

            saveEventTime(xev);

#ifdef DEBUG
            DBG logEvent(xev);
#endif

            if (xev.type == KeymapNotify) {
                XRefreshKeyboardMapping(&xev.xmapping);

                // !!! we should probably regrab everything ?
                initModifiers();
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
                            fprintf(stderr,
                                    "BUG? mapRequest for window %lX sent to destroyed frame %lX!\n",
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
#ifdef SM
            if (IceSMfd != -1)
                FD_SET(IceSMfd, &read_fds);
#endif
            {
                for (YSocket *s = fFirstSocket; s; s = s->fNext) {
                    if (s->reading) {
                        FD_SET(s->sockfd, &read_fds);
                        MSG(("wait read"));
                    }
                    if (s->connecting) {
                        FD_SET(s->sockfd, &write_fds);
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
                    fprintf(stderr, "select: errno=%d\n", errno);
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
                for (YSocket *s = fFirstSocket; s; s = s->fNext) {
                    if (s->reading && FD_ISSET(s->sockfd, &read_fds)) {
                        MSG(("got read"));
                        s->can_read();
                    }
                    if (s->connecting && FD_ISSET(s->sockfd, &write_fds)) {
                        MSG(("got connect"));
                        s->connected();
                    }
                }
            }
#ifdef SM
            if (IceSMfd != -1 && FD_ISSET(IceSMfd, &read_fds)) {
                Bool rep;
                if (IceProcessMessages(IceSMconn, NULL, &rep)
                    == IceProcessMessagesIOError)
                {
                    SmcCloseConnection(SMconn, 0, NULL);
                    IceSMconn = NULL;
                    IceSMfd = -1;
                }
            }
#endif
            }
        }
    }
    fLoopLevel--;
    return fExitCode;
}

void YApplication::dispatchEvent(YWindow *win, XEvent &xev) {
    if (xev.type == KeyPress || xev.type == KeyRelease) {
        YWindow *w = win;
        while (w && (w->handleKeyEvent(xev.xkey) == false)) {
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

void YApplication::saveEventTime(XEvent &xev) {
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
    AltMask = MetaMask = NumLockMask = ScrollLockMask = SuperMask = HyperMask = 0;
    if (xmk) {
        KeyCode numLockKeyCode = sym2code(XK_Num_Lock);
        KeyCode scrollLockKeyCode = sym2code(XK_Scroll_Lock);
        KeyCode altKeyCode = sym2code(XK_Alt_L);
        KeyCode metaKeyCode = sym2code(XK_Meta_L);
        KeyCode superKeyCode = sym2code(XK_Super_L);
        KeyCode hyperKeyCode = sym2code(XK_Hyper_L);

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
            }
        XFreeModifiermap(xmk);
    }
    if (MetaMask == AltMask)
        MetaMask = 0;
#ifdef DEBUG
    if (debug)
        fprintf(stderr, "alt:%d meta:%d num:%d scroll:%d\n",
                AltMask,
                MetaMask,
                NumLockMask,
                ScrollLockMask);
#endif

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
        HyperMask;

    ButtonMask =
        Button1Mask |
        Button2Mask |
        Button3Mask |
        Button4Mask |
        Button5Mask;

    ButtonKeyMask = KeyMask | ButtonMask;

    fInitModifiers = true;
}

void YApplication::runProgram(const char *str, const char *const *args) {
    XSync(app->display(), False);
    if (fork() == 0) {
        app->resetSignals();

        /* perhaps the right thing to to:
         create ptys .. and show console window when an application
         attempts to use it (how do we detect input with no output?) */
        setsid();
#if 0
        close(0);
        if (open("/dev/null", O_RDONLY) != 0)
            _exit(1);
#endif
        if (args)
            execvp(str, (char **)args);
        else
            execlp(str, str, 0);
        _exit(1);
    }
}

void YApplication::runCommand(const char *prog) {
    const char *cmd[4];

    cmd[0] = "/bin/sh";
    cmd[1] = "-c";
    cmd[2] = prog;
    cmd[3] = 0;

    runProgram(cmd[0], cmd);
}

void YApplication::setClipboardText(char *data, int len) {
    if (fClip == 0)
        fClip = new YClipboard();
    if (!fClip)
        return ;
    fClip->setData(data, len);
}

bool parseKey(const char *arg, KeySym *key, int *mod) { // !!!
    const char *orig_arg = arg;

    *mod = 0;
    for (;;) {
        if (strncmp("Alt+", arg, 4) == 0) {
            *mod |= kfAlt;
            arg += 4;
        } else if (strncmp("Ctrl+", arg, 5) == 0) {
            *mod |= kfCtrl;
            arg += 5;
        } else if (strncmp("Shift+", arg, 6) == 0) {
            *mod |= kfShift;
            arg += 6;
        } else if (strncmp("Meta+", arg, 5) == 0) {
            *mod |= kfMeta;
            arg += 5;
        } else if (strncmp("Super+", arg, 6) == 0) {
            *mod |= kfSuper;
            arg += 6;
        } else if (strncmp("Hyper+", arg, 6) == 0) {
            *mod |= kfHyper;
            arg += 6;
        } else
            break;
    }
    if (strcmp(arg, "") == 0) {
        *key = NoSymbol;
        return true;
    } else if (strcmp(arg, "Esc") == 0)
        *key = XK_Escape;
    else if (strcmp(arg, "Enter") == 0)
        *key = XK_Return;
    else if (strcmp(arg, "Space") == 0)
        *key = ' ';
    else if (strcmp(arg, "BackSp") == 0)
        *key = XK_BackSpace;
    else if (strcmp(arg, "Del") == 0)
        *key = XK_Delete;
    else if (strlen(arg) == 1 && arg[0] >= 'A' && arg[0] <= 'Z') {
        char s[2];
        s[0] = arg[0] - 'A' + 'a';
        s[1] = 0;
        *key = XStringToKeysym(s);
    } else {
        *key = XStringToKeysym(arg);
    }

    if (*key == NoSymbol) {
        fprintf(stderr, "unknown key %s in %s\n", arg, orig_arg);
        return false;
    }
    return true;
}

bool YApplication::fInitModifiers = false;

unsigned int YApplication::getAltMask() {
    if (!fInitModifiers)
        initModifiers();
    return AltMask;
}

unsigned int YApplication::getMetaMask() {
    if (!fInitModifiers)
        initModifiers();
    return MetaMask;
}

unsigned int YApplication::getSuperMask() {
    if (!fInitModifiers)
        initModifiers();
    return SuperMask;
}

unsigned int YApplication::getHyperMask() {
    if (!fInitModifiers)
        initModifiers();
    return HyperMask;
}

unsigned int YApplication::getNumLockMask() {
    if (!fInitModifiers)
        initModifiers();
    return NumLockMask;
}

unsigned int YApplication::getKeyMask() {
    if (!fInitModifiers)
        initModifiers();
    return KeyMask;
}

unsigned int YApplication::getButtonKeyMask() {
    if (!fInitModifiers)
        initModifiers();
    return ButtonKeyMask;
}
