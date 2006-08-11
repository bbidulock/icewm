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

YApplication::YApplication(int * /*argc*/, char ***argv) {
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
            fExecutable = strJoin(getcwd(cwd, sizeof(cwd)), "/", cmd, NULL);
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

struct timeval idletime;

int YApplication::mainLoop() {
    fLoopLevel++;
    if (!fExitApp)
        fExitLoop = 0;

    gettimeofday(&idletime, 0);

    struct timeval timeout, *tp;

    while (!fExitApp && !fExitLoop) {
        if (handleXEvents()) {
        } else {
            int rc;
            fd_set read_fds;
            fd_set write_fds;

            handleIdle();
            gettimeofday(&idletime, 0);

            FD_ZERO(&read_fds);
            FD_ZERO(&write_fds);
            if (readFDCheckX() != -1)
                FD_SET(readFDCheckX(), &read_fds);
            if (signalPipe[0] != -1)
                FD_SET(signalPipe[0], &read_fds);

/// TODO #warning "make this more general"
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

            rc = select(sizeof(fd_set) * 8,
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

void YApplication::closeFiles() {
#ifdef linux   /* for now, some debugging code */
    int             i, max = 1024;
    struct rlimit   lim;

    if (getrlimit(RLIMIT_NOFILE, &lim) == 0)
        max = lim.rlim_max;

    for (i = 3; i < max; i++) {
        int fl = 0;
        if (fcntl(i, F_GETFD, &fl) == 0) {
            if (!(fl & FD_CLOEXEC)) {
                char path[64];
                char buf[1024];

                memset(buf, 0, sizeof(buf));
                sprintf(path, "/proc/%d/fd/%d", getpid(), i);
                readlink(path, buf, sizeof(buf) - 1);

                warn("File still open: fd=%d, target='%s' (missing FD_CLOEXEC?)", i, buf);
                warn("Closing file descriptor: %d", i);
                close (i);
            }
        }
    }
#endif
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
        closeFiles();

        if (args)
            execvp(path, (char **)args);
        else
            execlp(path, path, (void *)NULL);

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
/// TODO #warning calling /bin/sh is considered to be bloat
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
