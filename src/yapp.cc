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
#include "intl.h"

//#define USE_SIGNALFD

#ifdef USE_SIGNALFD
#include <sys/signalfd.h>
#endif
#include <pwd.h>

IMainLoop *mainLoop;
static int signalPipe[2];
static sigset_t oldSignalMask;
static sigset_t signalMask;

void YApplication::initSignals() {
    sigemptyset(&signalMask);
    sigaddset(&signalMask, SIGHUP);
    sigprocmask(SIG_BLOCK, &signalMask, &oldSignalMask);

#ifdef USE_SIGNALFD
    signalPipe[0] = signalfd(-1, &signalMask, 0);
    if (signalPipe[0]<0)
        perror("signalfd");
    // XXX: with kernel 2.6.27 the flags above should become effective
    // but let's not require that yet
    fcntl(signalPipe[0], F_SETFL, O_NONBLOCK);
    fcntl(signalPipe[0], F_SETFD, FD_CLOEXEC);
#else
    sigemptyset(&signalMask);
    sigaddset(&signalMask, SIGPIPE);
    sigprocmask(SIG_BLOCK, &signalMask, &oldSignalMask);

    if (pipe(signalPipe) != 0)
        die(2, "Failed to create anonymous pipe (errno=%d).", errno);
    fcntl(signalPipe[1], F_SETFL, O_NONBLOCK);
    fcntl(signalPipe[0], F_SETFD, FD_CLOEXEC);
    fcntl(signalPipe[1], F_SETFD, FD_CLOEXEC);
#endif
    sfd.registerPoll(this, signalPipe[0]);
}

#ifdef __linux__
void alrm_handler(int /*sig*/) {
    show_backtrace();
}
#endif

YApplication::YApplication(int * /*argc*/, char *** /*argv*/) {
    ::mainLoop = this;

    fLoopLevel = 0;
    fExitApp = 0;

    setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
    setvbuf(stderr, NULL, _IOLBF, BUFSIZ);

    initSignals();
}

YApplication::~YApplication() {
    sfd.unregisterPoll();
    ::mainLoop = 0;
}

void YApplication::registerTimer(YTimer *t) {
    if (find(timers, t) < 0)
        timers.append(t);
}

void YApplication::unregisterTimer(YTimer *t) {
    int k = find(timers, t);
    if (k >= 0)
        timers.remove(k);
}

bool YApplication::nextTimeout(timeval *timeout) {
    bool fFirst = true;
    YArrayIterator<YTimer*> t = timers.iterator();
    while (++t) {
        if (t->isRunning() && (fFirst || t->timeout < *timeout)) {
            *timeout = t->timeout;
            fFirst = false;
        }
    }
    return fFirst == false;
}

bool YApplication::nextTimeoutWithFuzziness(timeval *timeout) {
    bool fixedExists = false;
    timeval timeout_fixed = {0, 0L};

    bool fuzzyExists = false;
    timeval timeout_fuzzy = {0, 0L};
    timeval timeout_max = {0, 0L};

    for (YArrayIterator<YTimer*> iter = timers.iterator(); ++iter; ) {
        if (iter->isRunning()) {
            if (iter->isFixed()) {
                // Update if no fixed timer yet or current
                // is earlier than previously registered one.
                if (false == fixedExists || iter->timeout < timeout_fixed) {
                    timeout_fixed = iter->timeout;
                    fixedExists = true;
                }
            }
            else if (false == fuzzyExists) {
                // encountered first fuzzy timer, register everything
                timeout_fuzzy = iter->timeout;
                timeout_max = iter->timeout_max;
                fuzzyExists = true;
            }
            else if (iter->timeout < timeout_fuzzy) {
                // this fuzzy timer is earlier than previous one
                timeout_fuzzy = iter->timeout;
                if (iter->timeout_max < timeout_max)
                    timeout_max = iter->timeout_max;
            }
        }
    }

    if (fixedExists && fuzzyExists)
        *timeout = min(timeout_max, timeout_fixed);
    else if (fixedExists)
        *timeout = timeout_fixed;
    else if (fuzzyExists)
        *timeout = timeout_max;
    return fixedExists | fuzzyExists;
}

bool YApplication::getTimeout(timeval *timeout) {
    bool found = false;
    if (0 < timers.getCount()) {
        timeval tval = {0, 0L};
        if (inrange(DelayFuzziness, 1, 100))
            found = nextTimeoutWithFuzziness(&tval);
        else
            found = nextTimeout(&tval);
        if (found)
            *timeout = max(tval - monotime(), (timeval) { 0L, 1L });
    }
    return found;
}

void YApplication::handleTimeouts() {
    timeval now = monotime();
    YTimer *timeout = 0;
    YArrayIterator<YTimer*> iter = timers.reverseIterator();
    // we must be careful since the callback may modify the array.
    while (iter.hasNext()) {
        if (++iter &&
            *iter != timeout &&
            iter->isRunning() &&
            iter->timeout_min < now)
        {
            timeout = *iter;
            YTimerListener *listener = timeout->getTimerListener();
            timeout->stopTimer();
            if (listener && listener->handleTimer(timeout))
                timeout->startTimer();
        }
    }
}

void YApplication::decreaseTimeouts(timeval diff) {
    YArrayIterator<YTimer*> iter = timers.reverseIterator();
    while (++iter)
        iter->timeout += diff;
}

void YApplication::registerPoll(YPollBase *t) {
    PRECONDITION(t->fd() >= 0);
    if (find(polls, t) < 0)
        polls.append(t);
}

void YApplication::unregisterPoll(YPollBase *t) {
    int k = find(polls, t);
    if (k >= 0)
        polls.remove(k);
}

YPollBase::~YPollBase() {
    unregisterPoll();
}

void YPollBase::unregisterPoll() {
    if (fd() >= 0) {
        mainLoop->unregisterPoll(this);
        fFd = -1;
    }
}


void YPollBase::registerPoll(int fd) {
    unregisterPoll();
    fFd = fd;
    if (fFd >= 0) {
        mainLoop->registerPoll(this);
    }
}

int YApplication::mainLoop() {
    if (fLoopLevel == 0)
        handleSignal(SIGCHLD);

    fLoopLevel++;

    timeval prevtime = monotime();

    for (fExitLoop = fExitApp; (fExitApp | fExitLoop) == false; ) {
        bool didIdle = handleIdle();

        fd_set read_fds;
        FD_ZERO(&read_fds);
        fd_set write_fds;
        FD_ZERO(&write_fds);

        YArrayIterator<YPollBase*> iPoll = polls.iterator();
        while (++iPoll) {
            PRECONDITION(iPoll->fd() >= 0);
            if (iPoll->forRead()) {
                FD_SET(iPoll->fd(), &read_fds);
            }
            if (iPoll->forWrite()) {
                FD_SET(iPoll->fd(), &write_fds);
            }
        }

        timeval timeout = {0, 0L};
        timeval *tp = &timeout;
        if (!didIdle && getTimeout(tp) == false)
            tp = 0;

#ifndef USE_SIGNALFD
        sigprocmask(SIG_UNBLOCK, &signalMask, NULL);
#endif

        int rc;
        rc = select(sizeof(fd_set) * 8,
                    SELECT_TYPE_ARG234 &read_fds,
                    SELECT_TYPE_ARG234 &write_fds,
                    0,
                    tp);

#ifndef USE_SIGNALFD
        sigprocmask(SIG_BLOCK, &signalMask, NULL);
#endif

        {
            timeval diff = monotime() - prevtime;
            // This is irrelevant when using monotonic clocks:
            // if time travel to past, decrease the timeouts
            if (diff < zerotime()) {
                warn("time warp of %ld.%06ld", diff.tv_sec, diff.tv_usec);
                decreaseTimeouts(diff);
            } else {
                // no detection for time travel to the future
            }
            prevtime += diff;
        }

        if (rc == 0) {
            handleTimeouts();
        } else if (rc == -1) {
            if (errno != EINTR)
                fail(_("%s: select failed"), __func__);
        } else {
            for (iPoll = polls.reverseIterator(); ++iPoll; ) {
                if (iPoll->fd() >= 0 && FD_ISSET(iPoll->fd(), &read_fds)) {
                    iPoll->notifyRead();
                    if (iPoll.isValid() == false)
                        continue;
                }
                if (iPoll->fd() >= 0 && FD_ISSET(iPoll->fd(), &write_fds)) {
                    iPoll->notifyWrite();
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

#ifndef USE_SIGNALFD
void sig_handler(int sig) {
    unsigned char uc = (unsigned char)sig;
    if (write(signalPipe[1], &uc, 1) != 1)
        fprintf(stderr, "icewm: signal error\n");
}
#endif

void YApplication::catchSignal(int sig) {
    sigaddset(&signalMask, sig);
    sigprocmask(SIG_BLOCK, &signalMask, NULL);

#ifdef USE_SIGNALFD
    signalPipe[0]=signalfd(signalPipe[0], &signalMask, 0);
#else
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(sig, &sa, NULL);
#endif
}

void YApplication::resetSignals() {
    sigprocmask(SIG_SETMASK, &oldSignalMask, 0);
}

void YApplication::closeFiles() {
#ifdef DEBUG
#ifdef __linux__   /* for now, some debugging code */
    int             i, max = dup(0);

    for (i = 3; i < max; i++) {
        int fl = 0;
        if (fcntl(i, F_GETFD, &fl) == 0) {
            if (!(fl & FD_CLOEXEC)) {
                char path[64];
                char buf[64];

                memset(buf, 0, sizeof(buf));
                snprintf(path, sizeof path, "/proc/%d/fd/%d", (int) getpid(), i);
                if (readlink(path, buf, sizeof(buf) - 1) == -1)
                    buf[0] = '\0';

                warn("File still open: fd=%d, target='%s' (missing FD_CLOEXEC?)", i, buf);
                warn("Closing file descriptor: %d", i);
                close (i);
            }
        }
    }
    close(max);
#endif
#endif
}

int YApplication::runProgram(const char *path, const char *const *args) {
    flushXEvents();

    int cpid = -1;
    if (path && (cpid = fork()) == 0) {
        resetSignals();
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

        fail("%s", path);
        _exit(99);
    }
    return cpid;
}

int YApplication::waitProgram(int p) {
    int rc = -1, status = 0;
    if (p > 0) {
        do {
            errno = status = 0;
            rc = waitpid(p, &status, 0);
        } while (rc == -1 && errno == EINTR);
    }
    return rc == -1 ? -1 : status;
}

void YApplication::runCommand(const char *cmdline) {
/// TODO #warning calling /bin/sh is considered to be bloat
    char const * argv[] = { "/bin/sh", "-c", cmdline, NULL };
    runProgram(argv[0], argv);
}

#ifndef USE_SIGNALFD
void YApplication::handleSignalPipe() {
    unsigned char sig;
    if (read(signalPipe[0], &sig, 1) == 1) {
        handleSignal(sig);
    }
}


void YSignalPoll::notifyRead() {
    owner()->handleSignalPipe();
}
#else
void YSignalPoll::notifyRead()
{
        struct signalfd_siginfo fdsi;
        int s = read(signalPipe[0], &fdsi, sizeof(struct signalfd_siginfo));
        if (s == sizeof(struct signalfd_siginfo))
                owner()->handleSignal(fdsi.ssi_signo);
}
#endif

void YSignalPoll::notifyWrite() {
}

bool YSignalPoll::forRead() {
    return true;
}

bool YSignalPoll::forWrite() {
    return false;
}

const upath& YApplication::getLibDir() {
    static upath dir( REDIR_ROOT(LIBDIR) );
    return dir;
}

const upath& YApplication::getConfigDir() {
    static upath dir( CFGDIR );
    return dir;
}

const upath& YApplication::getPrivConfDir() {
    static upath dir;
    if (dir.isEmpty()) {
        const char *env = getenv("ICEWM_PRIVCFG");
        if (env) {
            dir = env;
            if ( ! dir.dirExists())
                dir.mkdir();
        }
        else {
            env = getenv("XDG_CONFIG_HOME");
            if (env)
                dir = env;
            else {
                dir = getHomeDir() + "/.config";
            }
            dir += "/icewm";
            if (!dir.dirExists()) {
                dir = getHomeDir() + "/.icewm";
                if ( ! dir.dirExists())
                    dir.mkdir();
            }
        }
        MSG(("using %s for private configuration files", cstring(dir).c_str()));
    }
    return dir;
}

upath YApplication::getHomeDir() {
    char *env = getenv("HOME");
    if (env) {
        return upath(env);
    } else {
        passwd *pw = getpwuid(getuid());
        if (pw) {
            return upath(pw->pw_dir);
        }
    }
    return upath(null);
}

upath YApplication::findConfigFile(upath name) {
    upath p;

    if (name.isAbsolute())
        return name;

    p = getPrivConfDir() + name;
    if (p.fileExists())
        return p;

    p = getConfigDir() + name;
    if (p.fileExists())
        return p;

    p = getLibDir() + name;
    if (p.fileExists())
        return p;

    return null;
}

// vim: set sw=4 ts=4 et:
