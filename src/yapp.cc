/*
 * IceWM
 *
 * Copyright (C) 1998-2002 Marko Macek
 */
#include "config.h"
#include "yapp.h"

#include "ypoll.h"
#include "ytimer.h"
#include "yprefs.h"

#include "sysdep.h"
#include "sys/resource.h"

#include "intl.h"

extern char const *ApplicationName;
char const *&YApplication::Name = ApplicationName;

YApplication *app = 0;
static int signalPipe[2] = { 0, 0 };
static sigset_t oldSignalMask;
static sigset_t signalMask;

void YApplication::initSignals() {
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
    sfd.registerPoll(this, signalPipe[0]);
}

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

    if (name[0] == '/')
        return newstr(name);

    p = cstrJoin(getPrivConfDir(), "/", name, NULL);
    if (access(p, mode) == 0) return p;
    delete[] p;

    p = cstrJoin(configDir, "/", name, NULL);
    if (access(p, mode) == 0) return p;
    delete[] p;

    p = cstrJoin(REDIR_ROOT(libDir), "/", name, NULL);
    if (access(p, mode) == 0) return p;
    delete[] p;

    return 0;
}

YApplication::YApplication(int *argc, char ***argv) {
    app = this;
    fLoopLevel = 0;
    fExitApp = 0;
    fFirstTimer = fLastTimer = 0;
    fFirstPoll = fLastPoll = 0;

    {
        char const * cmd(**argv);
        char cwd[PATH_MAX + 1];

        if ('/' == *cmd)
            fExecutable = newstr(cmd);
        else if (strchr (cmd, '/'))
            fExecutable = cstrJoin(getcwd(cwd, sizeof(cwd)), "/", cmd, NULL);
        else
            fExecutable = findPath(getenv("PATH"), X_OK, cmd);
    }

    initSignals();
#if 0
    struct sigaction sig;
    sig.sa_handler = SIG_IGN;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = 0;
    sigaction(SIGCHLD, &sig, &oldSignalCHLD);
#endif
}

YApplication::~YApplication() {
    delete[] fExecutable;
    unregisterPoll(&sfd);
    app = NULL;
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

    if (fFirstTimer == 0)
        return;

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

void YApplication::decreaseTimeouts(struct timeval difftime) {
    YTimer *t = fFirstTimer;

    while (t) {
        t->timeout.tv_sec += difftime.tv_sec;
        t->timeout.tv_usec += difftime.tv_usec;
        if (t->timeout.tv_usec < 0) {
            t->timeout.tv_sec--;
            t->timeout.tv_usec += difftime.tv_usec;
        }
        t = t->fNext;
    }
}

void YApplication::registerPoll(YPollBase *t) {
    PRECONDITION(t->fFd != -1);
    t->fPrev = 0;
    t->fNext = fFirstPoll;
    if (fFirstPoll)
        fFirstPoll->fPrev = t;
    else
        fLastPoll = t;
    fFirstPoll = t;
}

void YApplication::unregisterPoll(YPollBase *t) {
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

YPollBase::~YPollBase() {
    unregisterPoll();
}

void YPollBase::unregisterPoll() {
    if (fFd != -1) {
        app->unregisterPoll(this);
        fFd = -1;
    }
}


void YPollBase::registerPoll(int fd) {
    unregisterPoll();
    fFd = fd;
    if (fFd != -1)
        app->registerPoll(this);
}

int YApplication::mainLoop() {
    fLoopLevel++;
    if (!fExitApp)
        fExitLoop = 0;

    struct timeval timeout, *tp;
    struct timeval prevtime;
    struct timeval idletime;

    gettimeofday(&idletime, 0);
    gettimeofday(&prevtime, 0);
    while (!fExitApp && !fExitLoop) {
        int rc;
        fd_set read_fds;
        fd_set write_fds;

        bool didIdle = handleIdle();
#if 0
        gettimeofday(&idletime, 0);
        {
            struct timeval difftime;
            struct timeval curtime;
            gettimeofday(&curtime, 0);
            difftime.tv_sec = curtime.tv_sec - idletime.tv_sec;
            difftime.tv_usec = curtime.tv_usec - idletime.tv_usec;
            if (difftime.tv_usec < 0) {
                difftime.tv_sec--;
                difftime.tv_usec += 1000000;
            }
            if (difftime.tv_sec != 0 || difftime.tv_usec > 50 * 1000) {
                didIdle = handleIdle();
                gettimeofday(&idletime, 0);
            }
        }
#endif

        {
            struct timeval difftime;
            struct timeval curtime;
            gettimeofday(&curtime, 0);
            difftime.tv_sec = curtime.tv_sec - prevtime.tv_sec;
            difftime.tv_usec = curtime.tv_usec - prevtime.tv_usec;
            if (difftime.tv_usec < 0) {
                difftime.tv_sec--;
                difftime.tv_usec += 1000000;
            }
            if (difftime.tv_sec > 0 || difftime.tv_usec >= 50 * 1000) {
                warn("latency: %d.%06d",
                     difftime.tv_sec, difftime.tv_usec);
            }
        }

        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        {
            for (YPollBase *s = fFirstPoll; s; s = s->fNext) {
                PRECONDITION(s->fFd != -1);
                if (s->forRead()) {
                    FD_SET(s->fd(), &read_fds);
                    MSG(("wait read"));
                }
                if (s->forWrite()) {
                    FD_SET(s->fd(), &write_fds);
                    MSG(("wait connect"));
                }
            }
        }

        gettimeofday(&prevtime, 0);

        tp = &timeout;
        if (didIdle) {
            timeout.tv_sec = 0;
            timeout.tv_usec = 0;
        } else {
            timeout.tv_sec = 0;
            timeout.tv_usec = 0;
            getTimeout(&timeout);
            if (timeout.tv_sec == 0 && timeout.tv_usec == 0)
                tp = 0;
        }

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

        {
            struct timeval difftime;
            struct timeval curtime;

            gettimeofday(&curtime, 0);
            difftime.tv_sec = curtime.tv_sec - prevtime.tv_sec;
            difftime.tv_usec = curtime.tv_usec - prevtime.tv_usec;
            if (difftime.tv_usec < 0) {
                difftime.tv_sec--;
                difftime.tv_usec += 1000000;
            }
            // if time travel to past, decrease the timeouts
            if (difftime.tv_sec < 0 ||
                (difftime.tv_sec == 0 && difftime.tv_usec < 0))
            {
                warn("time warp of %d.%06d",
                     difftime.tv_sec, difftime.tv_usec);
                decreaseTimeouts(difftime);
            } else {
                // no detection for time travel to the future
            }
        }
        gettimeofday(&prevtime, 0);

        if (rc == 0) {
            handleTimeouts();
        } else if (rc == -1) {
            if (errno != EINTR)
                warn(_("Message Loop: select failed (errno=%d)"), errno);
        } else {
            for (YPollBase *s = fFirstPoll; s; s = s->fNext) {
                PRECONDITION(s->fFd != -1);
                int fd = s->fd();
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
    }
    fLoopLevel--;
    return fExitCode;
}

void YApplication::exitLoop(int exitCode) {
    fExitLoop = 1;
    fExitCode = exitCode;
}

void YApplication::exit(int exitCode) {
    fExitApp = 1;
    exitLoop(exitCode);
}

void YApplication::handleSignal(int sig) {
    if (sig == SIGCHLD) {
        int s, rc;
        do {
            rc = waitpid(-1, &s, WNOHANG);
        } while (rc > 0);
    }
}

bool YApplication::handleIdle() {
    return false;
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

int YApplication::runProgram(const char *path, const char *const *args) {
    flushXEvents();

    int cpid = -1;
    if (path && (cpid = fork()) == 0) {
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
#ifdef linux   /* for now, some debugging code */
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
            execlp(path, path, NULL);

        _exit(99);
    }
    return cpid;
}

int YApplication::waitProgram(int p) {
    int status;

    if (p == -1)
        return -1;

    if (waitpid(p, &status, 0) != p)
        return -1;
    return status;
}

void YApplication::runCommand(const char *cmdline) {
#warning calling /bin/sh is considered to be bloat
    char const * argv[] = { "/bin/sh", "-c", cmdline, NULL };
    runProgram(argv[0], argv);
}

#ifndef NO_CONFIGURE
bool YApplication::loadConfig(struct cfoption *options, const char *name) {
    char *configFile = YApplication::findConfigFile(name);
    bool rc = false;
    if (configFile) {
        ::loadConfig(options, configFile);
        delete[] configFile;
        rc = true;
    }
    return rc;
}
#endif

void YApplication::handleSignalPipe() {
    unsigned char sig;
    if (read(signalPipe[0], &sig, 1) == 1) {
        handleSignal(sig);
    }
}

void YSignalPoll::notifyRead() {
    owner()->handleSignalPipe();
}

void YSignalPoll::notifyWrite() {
}

bool YSignalPoll::forRead() {
    return true;
}

bool YSignalPoll::forWrite() {
    return false;
}
