/*
 * IceWM
 *
 * Copyright (C) 1998-2001 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "yapp.h"
#include "ysocket.h"

#include "sysdep.h"
#include "wmprog.h"
#include "wmmgr.h"
#include "MwmUtil.h"
#include "prefs.h"
#include "ypixbuf.h"

#include "intl.h"

#ifdef CONFIG_SESSION
#include <X11/SM/SMlib.h>
#endif

YApplication *app = 0;
YDesktop *desktop = 0;
XContext windowContext;
static int signalPipe[2] = { 0, 0 };
static sigset_t oldSignalMask;
static sigset_t signalMask;

YAtoms atoms;

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

#ifdef CONFIG_SESSION
int IceSMfd = -1;
IceConn IceSMconn = NULL;
SmcConn SMconn = NULL;
char *oldSessionId = NULL;
char *newSessionId = NULL;
char *sessionProg;

char *getsesfile() {
    static char *filename(NULL);

    if (NULL == filename)
        filename = YResourcePaths::getPrivateFilename(newSessionId, ".session");

    return filename;
}

static void iceWatchFD(IceConn conn,
                       IcePointer /*client_data*/,
                       Bool opening,
                       IcePointer */*watch_data*/)
{
    if (opening) {
        if (IceSMfd != -1) { // shouldn't happen
            warn(_("TOO MANY ICE CONNECTIONS -- not supported"));
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
    app->smSaveYourself(shutdown, fast);
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
    if (!user) // not a user?
        user = getenv("LOGNAME");
    if (!user) {
        msg(_("$USER or $LOGNAME not set?"));
        return ;
    }
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
        warn(_("Session Manager: IceAddConnectionWatch failed."));
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
        warn(_("Session Manager: Init error: %s"), error_str);
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

#endif /* CONFIG_SESSION */

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
        if (clear.selection == atoms.clipboard) {
            if (fData)
                delete [] fData;
            fLen = 0;
            fData = 0;
        }
    }
    void handleSelectionRequest(const XSelectionRequestEvent &request) {
        if (request.selection == atoms.clipboard) {
            XSelectionEvent notify;

            notify.type = SelectionNotify;
            notify.requestor = request.requestor;
            notify.selection = request.selection;
            notify.target = request.target;
            notify.time = request.time;
            notify.property = request.property;

            if (request.selection == atoms.clipboard &&
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
            } else if (request.selection == atoms.clipboard &&
                       request.target == atoms.targets &&
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
        die(2, _("Pipe creation failed (errno=%d)."), errno);

    fcntl(signalPipe[1], F_SETFL, O_NONBLOCK);
    fcntl(signalPipe[0], F_SETFD, FD_CLOEXEC);
    fcntl(signalPipe[1], F_SETFD, FD_CLOEXEC);
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
    defaultAppIcon = getIcon("app");
}
#endif

char *YApplication::findConfigFile(const char *name) {
    char *p, *h;

    h = getenv("HOME");
    if (h) {
        p = strJoin(h, "/.icewm/", name, NULL);
        if (access(p, R_OK) == 0)
            return p;
        delete p;
    }

    p = strJoin(configDir, "/", name, NULL);
    if (access(p, R_OK) == 0)
        return p;
    delete p;

    p = strJoin(REDIR_ROOT(libDir), "/", name, NULL);
    if (access(p, R_OK) == 0)
        return p;
    delete p;
    return 0;
}

YApplication::YApplication(int *argc, char ***argv, const char *displayName):
    lastEventTime(CurrentTime),
    fReplayEvent(false),

    fGrabTree(false),
    fGrabMouse(false),
    fXGrabWindow(NULL),
    fGrabWindow(NULL),
    fPopup(NULL),
    
    fClip(NULL),

    fLoopLevel(0),
    fAppContinue(true) {

    app = this;

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

    bool sync(false);

    for (int i = 1; i < *argc; i++) {
        if ((*argv)[i][0] == '-') {
            if (strcmp((*argv)[i], "-display") == 0) {
                displayName = (*argv)[++i];
#ifdef CONFIG_SESSION
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
    else {
        static char disp[256] = "DISPLAY=";
        strcat(disp, displayName);
        putenv(disp);
    }

    if (!(fDisplay = XOpenDisplay(displayName)))
        die(1, _("Can't open display: %s. X must be running and $DISPLAY set."),
	         displayName ? displayName : _("<none>"));

    if (sync)
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
    atoms.init();
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

#ifdef CONFIG_SESSION
    sessionProg = (*argv)[0]; //ICEWMEXE;
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
#ifdef CONFIG_SESSION
    if (SMconn != 0) {
        SmcCloseConnection(SMconn, 0, NULL);
        SMconn = NULL;
        IceSMconn = NULL;
    }
#endif
    XCloseDisplay(display());
    delete[] fExecutable;

    app = NULL;
}

bool YApplication::hasColormap() {
    XVisualInfo pattern;
    pattern.screen = DefaultScreen(display());

    int nVisuals;
    XVisualInfo * visual(XGetVisualInfo(display(), VisualScreenMask,
    					&pattern, &nVisuals));

    while(visual && nVisuals--)
	if ((visual++)->c_class & 1) return true;

    return false;
}

void YApplication::afterWindowEvent(XEvent & /*xev*/) {
}

extern void logEvent(XEvent xev);

int YApplication::mainLoop() {
    ++fLoopLevel;
    fLoopContinue = true;

    while (fAppContinue && fLoopContinue) {
        if (XPending(display()) > 0) {
            XEvent xev;

            XNextEvent(display(), &xev);
            xeventcount++;
            //msg("%d", xev.type);

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
                            msg("BUG? mapRequest for window %lX sent to destroyed frame %lX!",
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

#ifdef CONFIG_SESSION
            if (IceSMfd != -1)
                FD_SET(IceSMfd, &read_fds);
#endif

            YSocket::registerSockets(read_fds, write_fds);

            YTimeout timeout;
            YTimer::nextTimeout(timeout);

            sigprocmask(SIG_UNBLOCK, &signalMask, NULL);

            rc = select(sizeof(fd_set),
                        SELECT_TYPE_ARG234 &read_fds,
                        SELECT_TYPE_ARG234 &write_fds,
                        SELECT_TYPE_ARG234 NULL,
                        timeout.tv_sec || timeout.tv_usec ? &timeout : NULL);

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

            if (0 == rc) {
                YTimer::handleTimeouts();
            } else if (-1 == rc) {
                if (errno != EINTR)
                    warn(_("Message Loop: select failed (errno=%d)"), errno);
            } else {
                if (signalPipe[0] != -1) {
                    if (FD_ISSET(signalPipe[0], &read_fds)) {
                        unsigned char sig;
                        if (read(signalPipe[0], &sig, 1) == 1)
                            handleSignal(sig);
                    }
                }
            
                YSocket::handleSockets(read_fds, write_fds);
            
#ifdef CONFIG_SESSION
                if (IceSMfd != -1 && FD_ISSET(IceSMfd, &read_fds)) {
                    Bool rep;
                    if (IceProcessMessagesIOError ==
                        IceProcessMessages(IceSMconn, NULL, &rep)) {
                        SmcCloseConnection(SMconn, 0, NULL);
                        IceSMconn = NULL;
                        IceSMfd = -1;
                    }
                }
#endif
            }
        }
    }

    --fLoopLevel;
    return fExitCode;
}

void YApplication::dispatchEvent(YWindow *win, XEvent &xev) {
    if (xev.type == KeyPress || xev.type == KeyRelease) {
        YWindow *w = win;
        while (w && (w->handleKey(xev.xkey) == false)) {
            if (fGrabTree && w == fXGrabWindow) break;
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
            if (win != fGrabWindow) return;
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

int YApplication::grabEvents(YWindow *win, Cursor ptr, unsigned int eventMask,
                             bool grabMouse, bool grabKeyboard, bool grabTree) {
    int rc;

    if (NULL != fGrabWindow) return 0;
    if (NULL == win) return 0;

    XSync(display(), 0);
    fGrabTree = grabTree;

    if (grabMouse) {
        fGrabMouse = true;
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
        fGrabMouse = false;
        XChangeActivePointerGrab(display(), eventMask, ptr, CurrentTime);
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
    if (NULL == fGrabWindow) return 0;

    fGrabWindow = NULL;
    fXGrabWindow = NULL;
    fGrabTree = false;

    if (fGrabMouse) {
        XUngrabPointer(display(), CurrentTime);
        fGrabMouse = false;
    }

    XUngrabKeyboard(display(), CurrentTime);

    desktop->resetColormapFocus(true);
    return 1;
}

void YApplication::exitLoop(int exitCode) {
    fLoopContinue = false;
    fExitCode = exitCode;
}

void YApplication::exit(int exitCode) {
    fAppContinue = false;
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
            Win_L = XK_Meta_L;
            Win_R = XK_Meta_R;
	}
    }

    MSG(("alt:%d meta:%d super:%d hyper:%d win:%d mode:%d num:%d scroll:%d",
         AltMask, MetaMask, SuperMask, HyperMask, WinMask, ModeSwitchMask,
	 NumLockMask, ScrollLockMask));

}

void YApplication::alert() {
    XBell(display(), 100);
}

void YApplication::runProgram(const char *str, const char *const *args) {
    XSync(app->display(), False);

    if (fork() == 0) {
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
        if (args)
            execvp(str, (char **)args);
        else
            execlp(str, str, 0);
        _exit(1);
    }
}

void YApplication::runCommand(const char *cmdline) {
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

/******************************************************************************/

Atom YApplication::internAtom(char const *name, bool queryOnly) {
    return XInternAtom(display(), name, queryOnly);
}


void YApplication::internAtoms(YAtomInfo * info, unsigned const count,
                               bool queryOnly) {
#ifdef HAVE_XINTERNATOMS
    const char *names[count];
    for (unsigned n(0); n < count; ++n) names[n] = info[n].name;

    Atom atoms[count];
    XInternAtoms(display(), (char **)names, count, queryOnly, atoms);

    for (unsigned n(0); n < count; ++n) *info[n].atom = atoms[n];
#else
    for (unsigned n(0); n < count; ++n) 
        atom_info[i].atom = internAtom(info[i].name, queryOnly);
#endif
}

/******************************************************************************/

void YAtoms::init() {
    YAtomInfo info[] = {
        { &clipboard,               "CLIPBOARD"                             },
        { &targets,                 "TARGETS"                               },

        { &wmProtocols,             "WM_PROTOCOLS"                          },
        { &wmDeleteWindow,          "WM_DELETE_WINDOW"                      },
        { &wmTakeFocus,             "WM_TAKE_FOCUS"                         },
        { &wmColormapWindows,       "WM_COLORMAP_WINDOWS"                   },
        { &wmName,                  "WM_NAME"                               },
        { &wmState,                 "WM_STATE"                              },
        { &wmChangeState,           "WM_CHANGE_STATE"                       },
        { &smClientId,              "WM_CLIENT_LEADER"                      },
        { &wmClientLeader,          "SM_CLIENT_ID"                          },

#ifdef CONFIG_XDND_HINTS
        { &xdndAware,               "XdndAware"                             },
        { &xdndEnter,               "XdndEnter"                             },
        { &xdndLeave,               "XdndLeave"                             },
        { &xdndPosition,            "XdndPosition"                          },
        { &xdndStatus,              "XdndStatus"                            },
        { &xdndDrop,                "XdndDrop"                              },
        { &xdndFinished,            "XdndFinished"                          },
        { &xdndSelection,           "XdndSelection"                         },
        { &xdndTypelist,            "XdndTypelist"                          },
#endif

#ifdef CONFIG_MOTIF_HINTS
        { &mwmHints,                _XA_MOTIF_WM_HINTS                      },
#endif

#ifdef CONFIG_GNOME_HINTS
        { &winProtocols,            XA_WIN_PROTOCOLS                        },
        { &winSupportingWmCheck,    XA_WIN_SUPPORTING_WM_CHECK              },
        { &winIcons,                XA_WIN_ICONS                            },
        { &winWorkspace,            XA_WIN_WORKSPACE                        },
        { &winWorkspaceCount,       XA_WIN_WORKSPACE_COUNT                  },
        { &winWorkspaceNames,       XA_WIN_WORKSPACE_NAMES                  },
        { &winWorkspaces,           XA_WIN_WORKSPACES                       },
        { &winWorkspacesAdd,        XA_WIN_WORKSPACES_ADD                   },
        { &winWorkspacesRemove,     XA_WIN_WORKSPACES_REMOVE                },
        { &winLayer,                XA_WIN_LAYER                            },
        { &winHints,                XA_WIN_HINTS                            },
        { &winState,                XA_WIN_STATE                            },
        { &winWorkarea,             XA_WIN_WORKAREA                         },
        { &winClientList,           XA_WIN_CLIENT_LIST                      },
        { &winDesktopButtonProxy,   XA_WIN_DESKTOP_BUTTON_PROXY             },
        { &winArea,                 XA_WIN_AREA                             },
        { &winAreaCount,            XA_WIN_AREA_COUNT                       },
#endif

#ifdef CONFIG_KDE_HINTS
        { &kwmWinIcon,              "KWM_WIN_ICON"                          },
        { &kwmDockwindow,           "KWM_DOCKWINDOW"                        },
#ifdef CONFIG_WMSPEC_HINTS
        { &kdeNetSystemTrayWindows, "_KDE_NET_SYSTEM_TRAY_WINDOWS"          },
        { &kdeNetwmSystemTrayWindowFor,
                                    "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR"    },
#endif
#endif

#ifdef CONFIG_WMSPEC_HINTS
        { &netSupported,            "_NET_SUPPORTED"                        },
        { &netClientList,           "_NET_CLIENT_LIST"                      },
        { &netClientListStacking,   "_NET_CLIENT_LIST_STACKING"             },
        { &netNumberOfDesktops,     "_NET_NUMBER_OF_DESKTOPS"               },
        { &netDesktopGeometry,      "_NET_DESKTOP_GEOMETRY"                 },
        { &netDesktopViewport,      "_NET_DESKTOP_VIEWPORT"                 },
        { &netCurrentDesktop,       "_NET_CURRENT_DESKTOP"                  },
        { &netCurrentDesktopNames,  "_NET_CURRENT_DESKTOP_NAMES"            },
        { &netActiveWindow,         "_NET_ACTIVE_WINDOW"                    },
        { &netWorkarea,             "_NET_WORKAREA"                         },
        { &netSupportingWmCheck,    "_NET_SUPPORTING_WM_CHECK"              },

        { &netCloseWindow,          "_NET_CLOSE_WINDOW"                     },
        { &netwmMoveResize,         "_NET_WM_MOVERESIZE"                    },

        { &netwmName,               "_NET_WM_NAME"                          },
        { &netwmIconName,           "_NET_WM_ICON_NAME"                     },
        { &netwmDesktop,            "_NET_WM_DESKTOP"                       },
        { &netwmWindowType,         "_NET_WM_WINDOW_TYPE"                   },
        { &netwmWindowTypeDesktop,  "_NET_WM_WINDOW_TYPE_DESKTOP"           },
        { &netwmWindowTypeDock,     "_NET_WM_WINDOW_TYPE_DOCK"              },
        { &netwmWindowTypeToolbar,  "_NET_WM_WINDOW_TYPE_TOOLBAR"           },
        { &netwmWindowTypeMenu,     "_NET_WM_WINDOW_TYPE_MENU"              },
        { &netwmWindowTypeDialog,   "_NET_WM_WINDOW_TYPE_DIALOG"            },
        { &netwmWindowTypeNormal,   "_NET_WM_WINDOW_TYPE_NORMAL"            },
        { &netwmState,              "_NET_WM_STATE"                         },
        { &netwmStateModal,         "_NET_WM_STATE_MODAL"                   },
        { &netwmStateSticky,        "_NET_WM_STATE_STICKY"                  },
        { &netwmStateMaximizedVert, "_NET_WM_STATE_MAXIMIZED_VERT"          },
        { &netwmStateMaximizedHorz, "_NET_WM_STATE_MAXIMIZED_HORZ"          },
        { &netwmStateShaded,        "_NET_WM_STATE_SHADED"                  },
        { &netwmStateSkipTaskbar,   "_NET_WM_STATE_SKIP_TASKBAR"            },
        { &netwmStateSkipPager,     "_NET_WM_STATE_SKIP_PAGER"              },
        { &netwmStateFullscreen,    "_ICEWM_NET_WM_STATE_FULLSCREEN"        },
        
        { &netwmStrut,              "_NET_WM_STRUT"                         },
        { &netwmIconGeometry,       "_NET_WM_ICON_GEOMETRY"                 },
        { &netwmIcon,               "_NET_WM_ICON"                          },
        { &netwmPid,                "_NET_WM_PID"                           },
        { &netwmHandledIcons,       "_NET_WM_HANDLED_ICON"                  },

        { &netwmPing,               "_NET_WM_PING"                          },
#endif

        { &xrootPixmapId,           XA_XROOTPMAP_ID                         },
        { &xrootColorPixel,         XA_XROOTCOLOR_PIXEL                     },

#ifndef NO_WINDOW_OPTIONS
        { &icewmWinOpt,             XA_ICEWM_WINOPT                         },
#endif
#ifdef CONFIG_TRAY
        { &icewmTrayOpt,            XA_ICEWM_TRAYOPT                        },
#endif
#ifdef CONFIG_GUIEVENTS
        { &icewmGuiEvent,           XA_ICEWM_GUI_EVENT                      },          
#endif
        { &icewmFontPath,           XA_ICEWM_FONTPATH                       }
    };

    app->internAtoms(info, ACOUNT(info));
};
