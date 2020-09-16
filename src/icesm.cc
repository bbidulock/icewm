#include "config.h"
#include "base.h"
#include "intl.h"
#include "yapp.h"
#include "ytime.h"
#include "sysdep.h"
#include "appnames.h"
#include "ypointer.h"
#include "ascii.h"
using namespace ASCII;
#include "ywordexp.h"
#include <X11/Xlib.h>

#ifdef CONFIG_EXTERNAL_TRAY
#define NOTRAY false
#else
#define NOTRAY true
#endif

char const *ApplicationName = ICESMEXE;

class SessionManager: public YApplication {
private:
    char* trim(char *line) {
        size_t len = strlen(line);
        while (len > 0 && isWhiteSpace(line[len - 1])) {
            line[--len] = 0;
        }
        while (*line && isWhiteSpace(*line))
            ++line;
        return line;
    }

    bool expand(char *buf, size_t bufsiz) {
        wordexp_t w;
        if (wordexp(trim(buf), &w, 0) != 0 || w.we_wordc == 0)
            return false;
        size_t len = strlcpy(buf, trim(w.we_wordv[0]), bufsiz);
        for (size_t k = 1; k < size_t(w.we_wordc) && len < bufsiz; ++k) {
            strlcat(buf, " ", bufsiz);
            len = strlcat(buf, trim(w.we_wordv[k]), bufsiz);
        }
        wordfree(&w);
        if (len >= bufsiz)
            return false;
        if (buf[0] == '#' || buf[0] == '=')
            buf[0] = 0;
        return buf[0] != 0;
    }

    void setup() {
        char buf[PATH_MAX];
        char* pwd = getcwd(buf, sizeof buf);
        if (pwd && !strcmp(pwd, "/") && access(pwd, W_OK)) {
            char* home = getenv("HOME");
            if (nonempty(home) && !access(home, W_OK)) {
                (void) chdir(home);
            } else {
                csmart user(userhome(nullptr));
                if (user && chdir(user) == 0) {
                    setenv("HOME", user, true);
                }
            }
        }
    }

    const char *get_help_text() {
        return _(
        "  -c, --config=FILE   Let IceWM load preferences from FILE.\n"
        "  -t, --theme=FILE    Let IceWM load the theme from FILE.\n"
        "\n"
        "  -d, --display=NAME  Use NAME to connect to the X server.\n"
        "  -a, --alpha         Use a 32-bit visual for translucency.\n"
        "  --sync              Synchronize communication with X11 server.\n"
        "\n"
        "  -i, --icewm=FILE    Use FILE as the IceWM window manager.\n"
        "  -o, --output=FILE   Redirect all output to FILE.\n"
        "\n"
        "  -b, --nobg          Do not start icewmbg.\n"
        "  -n, --notray        Do not start icewmtray.\n"
        "  -s, --sound         Also start icesound.\n"
        );
    }

    const char *displayArg;
    const char *configArg;
    const char *themeArg;
    const char *alphaArg;
    const char *icewmExe;
    bool syncArg;
    bool nobgArg;
    bool notrayArg;
    bool soundArg;
    char* argv0;

    void options(int *argc, char ***argv) {
        const char* outputArg = nullptr;
        argv0 = **argv;
        displayArg = nullptr;
        configArg = nullptr;
        themeArg = nullptr;
        alphaArg = nullptr;
        icewmExe = ICEWMEXE;
        syncArg = false;
        nobgArg = false;
        notrayArg = NOTRAY;
        soundArg = false;

        for (char **arg = 1 + *argv; arg < *argv + *argc; ++arg) {
            if (**arg == '-') {
                char *value(nullptr);
                if (GetArgument(value, "d", "display", arg, *argv+*argc)) {
                    if (value && *value)
                        displayArg = value;
                }
                else if (GetArgument(value, "c", "config", arg, *argv+*argc)) {
                    configArg = value;
                }
                else if (GetArgument(value, "t", "theme", arg, *argv+*argc)) {
                    themeArg = value;
                }
                else if (GetArgument(value, "i", "icewm", arg, *argv+*argc)) {
                    icewmExe = value;
                }
                else if (GetArgument(value, "o", "output", arg, *argv+*argc)) {
                    outputArg = value;
                }
                else if (is_switch(*arg, "a", "alpha")) {
                    alphaArg = *arg;
                }
                else if (is_long_switch(*arg, "sync")) {
                    syncArg = true;
                }
                else if (is_switch(*arg, "b", "nobg")) {
                    nobgArg = true;
                }
                else if (is_switch(*arg, "n", "notray")) {
                    notrayArg = true;
                }
                else if (is_switch(*arg, "s", "sound")) {
                    soundArg = true;
                }
                else if (is_help_switch(*arg)) {
                    print_help_exit(get_help_text());
                }
                else if (is_version_switch(*arg)) {
                    print_version_exit(VERSION);
                }
                else if (is_copying_switch(*arg)) {
                    print_copying_exit();
                }
#ifdef PRECON
                else if (GetArgument(value, "e", "enquire", arg, *argv+*argc)) {
                    int pref = atoi(value);
                    bool term = false;
                    bool res = enquireRestart(&term, pref);
                    msg("enquire(term=%s, pref=%d) = %s",
                        boolstr(term), pref, boolstr(res));
                    ::exit(1);
                }
                else if (is_switch(*arg, "r", "rescue")) {
                    rescueFocus();
                    ::exit(1);
                }
#endif
                else {
                    warn(_("Unknown option '%s'"), *arg);
                }
            }
        }
        if (displayArg)
            setenv("DISPLAY", displayArg, 1);
        if (outputArg) {
            upath path(upath(outputArg).expand());
            int flags = O_WRONLY|O_CREAT|O_TRUNC|O_NOCTTY|O_APPEND;
            int fd(path.open(flags, 0600));
            if (fd == -1) {
                perror(path.string());
            } else {
                dup2(fd, 1);
                dup2(fd, 2);
                if (fd > 2) {
                    close(fd);
                }
            }
        }
    }

public:
    SessionManager(int *argc, char ***argv): YApplication(argc, argv) {
        setup();
        options(argc, argv);
        startup_phase = 0;
        bg_pid = -1;
        wm_pid = -1;
        tray_pid = -1;
        term_pid = -1;
        sound_pid = -1;
        rescue_pid = -1;
        crashtime = zerotime();

        catchSignal(SIGCHLD);
        catchSignal(SIGTERM);
        catchSignal(SIGINT);
        catchSignal(SIGUSR1);
    }

    void loadEnv(const char *scriptName) {
        upath scriptFile = YApplication::findConfigFile(scriptName);
        if (scriptFile.nonempty()) {
            FILE *ef = scriptFile.fopen("r");
            if (!ef)
                return;
            char scratch[1024];
            while (fgets(scratch, sizeof scratch, ef)) {
                if (expand(scratch, sizeof scratch)) {
                    char *ceq = strchr(scratch, '=');
                    if (ceq) {
                        *ceq = 0;
                        setenv(trim(scratch), trim(ceq + 1), 1);
                    }
                }
            }
            fclose(ef);
        }

        const char opts[] = "ICEWM_OPTIONS";
        const char* value = getenv(opts);
        if (value) {
            wmoptions = mstring(value).trim();
            unsetenv(opts);
        }
    }

    virtual int runProgram(const char *file, const char *const *args) {
        upath path;
        if (strchr(file, '/') == nullptr && strchr(argv0, '/') != nullptr) {
            path = upath(argv0).parent() + file;
            if (path.isExecutable()) {
                file = path.string();
                if (args && args[0])
                    *(const char**)args = file;
            }
        }
        return YApplication::runProgram(file, args);
    }

    void runScript(const char *scriptName) {
        upath scriptFile = YApplication::findConfigFile(scriptName);
        if (scriptFile.nonempty() && scriptFile.isExecutable()) {
            const char *cs = scriptFile.string();
            MSG(("Running session script: %s", cs));
            runProgram(cs, nullptr);
        }
    }

    void appendOptions(const char *args[], int start, size_t narg) {
        size_t k = (size_t) start;
        if (displayArg && k + 2 < narg) {
            args[k++] = "--display";
            args[k++] = displayArg;
        }
        if (configArg && k + 2 < narg) {
            args[k++] = "--config";
            args[k++] = configArg;
        }
        if (themeArg && k + 2 < narg) {
            args[k++] = "--theme";
            args[k++] = themeArg;
        }
        if (alphaArg && k + 1 < narg && 0 == strcmp(args[0], icewmExe)) {
            args[k++] = alphaArg;
        }
        if (syncArg && k + 1 < narg) {
            args[k++] = "--sync";
        }
        if (k < narg) {
            args[k] = nullptr;
        }
    }

    void runIcewmbg(bool quit = false) {
        const char *args[12] = { ICEWMBGEXE, nullptr, nullptr };

        if (nobgArg) {
            return;
        }
        if (quit) {
            args[1] = "-q";
        }
        else {
            appendOptions(args, 1, ACOUNT(args));
        }
        bg_pid =  runProgram(args[0], args);
    }

    void runIcewmtray(bool quit = false) {
        const char *args[12] = { ICEWMTRAYEXE, "--notify", nullptr };
        if (quit) {
            if (tray_pid != -1) {
                kill(tray_pid, SIGTERM);
                int status;
                waitpid(tray_pid, &status, 0);
            }
            tray_pid = -1;
        }
        else if (notrayArg) {
            notified();
        }
        else {
            appendOptions(args, 2, ACOUNT(args));
            tray_pid = runProgram(args[0], args);
        }
    }

    void runIcesound(bool quit = false) {
        const char *args[12] = { ICESOUNDEXE, "--verbose", nullptr };
        if (soundArg == false) {
            return;
        }
        else if (quit) {
            if (sound_pid != -1) {
                kill(sound_pid, SIGTERM);
                int status;
                waitpid(sound_pid, &status, 0);
            }
            sound_pid = -1;
        }
        else {
            appendOptions(args, 2, ACOUNT(args));
            sound_pid = runProgram(args[0], args);
        }
    }

    void runWM(bool quit = false) {
        if (rescue_pid != -1) {
            kill(rescue_pid, SIGTERM);
        }
        if (quit) {
            if (wm_pid != -1) {
                kill(wm_pid, SIGTERM);
                int status;
                waitpid(wm_pid, &status, 0);
            }
            wm_pid = -1;
        }
        else {
            const int size = 24;
            const char* args[size] = {
                icewmExe, "--notify", nullptr
            };
            appendOptions(args, 2, size);
            char* copy = nullptr;
            if (wmoptions.length()) {
                copy = strdup(wmoptions);
                if (nonempty(copy)) {
                    int count = 0;
                    while (count < size && args[count])
                        count++;
                    for (char* tok = strtok(copy, " ");
                        tok; tok = strtok(nullptr, " "))
                    {
                        if (count + 1 < size) {
                            args[count++] = tok;
                            args[count] = nullptr;
                        }
                    }
                }
            }
            wm_pid = runProgram(args[0], args);
            if (copy) {
                free(copy);
            }
        }
    }

private:
    void handleSignal(int sig) {
        if (sig == SIGTERM || sig == SIGINT) {
            signal(SIGTERM, SIG_IGN);
            signal(SIGINT, SIG_IGN);
            this->exit(0);
        }
        else if (sig == SIGCHLD) {
            int status = 0;
            int pid;

            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                MSG(("waitpid()=%d, status=%d", pid, status));
                if (pid == wm_pid) {
                    wm_pid = -1;
                    checkWMExitStatus(status);
                }
                else if (pid == tray_pid) {
                    tray_pid = -1;
                    checkTrayExitStatus(status);
                }
                else if (pid == term_pid) {
                    term_pid = -1;
                    if (wm_pid == -1)
                        runWM();
                }
                else if (pid == sound_pid)
                    sound_pid = -1;
                else if (pid == bg_pid) {
                    bg_pid = -1;
                    checkIcewmbgExitStatus(status);
                }
                else if (pid == rescue_pid) {
                    rescue_pid = -1;
                }
            }
        }
        else if (sig == SIGUSR1)
            notified();
    }

    void checkWMExitStatus(int status) {
        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) == ICESM_EXIT_RESTART) {
                tlog(_("restart %s."), icewmExe);
                runWM();
                return;
            }

            if (WEXITSTATUS(status)) {
                tlog(_("%s exited with status %d."),
                        icewmExe, WEXITSTATUS(status));
            }
            this->exit(WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status)) {
            tlog(_("%s was killed by signal %d."),
                    icewmExe, WTERMSIG(status));
            bool terminal = false;
            if (isCoreSignal(WTERMSIG(status))) {
                if (crashtime == zerotime()) {
                    crashtime = monotime();
                }
                else {
                    timeval previous = crashtime;
                    crashtime = monotime();
                    double delta = toDouble(crashtime - previous);
                    if (inrange(delta, 0.00001, 10.0)) {
                        if (enquireRestart(&terminal) == false) {
                            return;
                        }
                    }
                }
            }
            if (terminal) {
                term_pid = runProgram(TERM, nullptr);
            } else {
                runWM();
            }
        }
    }

    bool isCoreSignal(int signo) {
        switch (signo) {
            case SIGILL:
            case SIGBUS:
            case SIGFPE:
            case SIGSEGV:
            case SIGABRT: return true;
        }
        return false;
    }

    bool enquireRestart(bool* terminal, int prefer = 0) {
        const char* message = _(
            " IceWM crashed for the second time in 10 seconds. \n"
            " Do you wish to:\n\n"
            "\t1: Restart IceWM?\n"
            "\t2: Abort this session?\n"
            "\t3: Run a terminal?\n"
            );
        const char* title = _("IceWM crash response");
        csmart yadp(path_lookup("yad"));
        const char xmes[] = "/usr/bin/xmessage";
        const char kdia[] = "/usr/bin/kdialog";
        const char zeni[] = "/usr/bin/zenity";
        const size_t commandSize(1234);
        char command[commandSize] = "";
        const int timeout = 123;
        if (--prefer < 0 && upath(yadp).isExecutable()) {
            snprintf(command, commandSize,
                     "%s "
                     " --image=info "
                     " --center "
                     " --width=420 "
                     " --height=200 "
                     " --timeout=%d "
                     " --timeout-indicator=bottom "
                     " --button=Restart:0 "
                     " --button=Abort:1 "
                     " --button=Terminal:2 "
                     " --title='%s' "
                     " --text='\n%s' ",
                     (const char *) yadp, timeout, title, message);
        }
        else if (--prefer < 0 && upath(xmes).isExecutable()) {
            snprintf(command, commandSize,
                     "%s "
                     " -buttons Restart,Abort,Terminal "
                     " -default Restart "
                     " -font 10x20 "
                     " -background black "
                     " -bordercolor orange "
                     " -foreground yellow "
                     " -center "
                     " -timeout %d "
                     " -title '%s' "
                     " '\n%s\n' ",
                     xmes, timeout, title, message);
        }
        else if (--prefer < 0 && upath(kdia).isExecutable()) {
            snprintf(command, commandSize,
                     "%s "
                     " --yes-label Restart "
                     " --no-label Abort "
                     " --cancel-label Terminal "
                     " --title '%s' "
                     " --warningyesnocancel "
                     " '\n%s\n' ",
                     kdia, title, message);
        }
        else if (--prefer < 0 && upath(zeni).isExecutable()) {
            snprintf(command, commandSize,
                     "%s "
                     " --question "
                     " --ok-label Restart "
                     " --cancel-label Abort "
                     " --timeout=%d "
                     " --title '%s' "
                     " --height 200 "
                     " --width 360 "
                     " --text='%s' ",
                     zeni, timeout, title, message);
        }
        if (command[0] == '/') {
            forkRescue();
            int status = system(command);
            if (WIFEXITED(status)) {
                switch (WEXITSTATUS(status)) {
                    case 0:
                    case 5:
                    case 70:
                    case 101: return true;

                    case 1:
                    case 102: this->exit(1); return false;

                    case 2:
                    case 103: *terminal = true; return true;
                }
            }
            char *sp = strchr(command, ' ');
            if (sp) *sp = '\0';
            warn("Could not execute '%s': status %d.", command, status);
        }
        return true;
    }

    void forkRescue() {
        if (rescue_pid == -1) {
            rescue_pid = fork();
            if (rescue_pid == 0) {
                resetSignals();
                rescueFocus();
                _exit(0);
            }
        }
    }

    void rescueFocus();

    void checkIcewmbgExitStatus(int status) {
        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) == ICESM_EXIT_RESTART) {
                tlog(_("restart %s."), ICEWMBGEXE);
                runIcewmbg();
                return;
            }
        }
        else if (WIFSIGNALED(status)) {
            tlog(_("%s was killed by signal %d."),
                    ICEWMBGEXE, WTERMSIG(status));
        }
    }

    void checkTrayExitStatus(int status) {
        if (WIFEXITED(status) && WEXITSTATUS(status) == 99) {
            /*
             * Exit code 99 means that execvp of icewmtray failed.
             * Take this as a notification to continue with startup.
             */
            notified();
        }
    }

    void notified() {
        if (++startup_phase == 1)
            runIcewmtray();
        else if (startup_phase == 2) {
            runScript("startup");
        }
    }

private:
    int startup_phase;
    int bg_pid;
    int wm_pid;
    int tray_pid;
    int term_pid;
    int sound_pid;
    int rescue_pid;
    timeval crashtime;
    mstring wmoptions;
};

void SessionManager::rescueFocus() {
    Display* display = XOpenDisplay(nullptr);
    if (display == nullptr) {
        return;
    }
    XSynchronize(display, True);
    Window root = DefaultRootWindow(display);
    XSelectInput(display, root, PropertyChangeMask | SubstructureNotifyMask);
    const int atomCount = 5;
    Atom atoms[atomCount] = {
        XInternAtom(display, "_NET_CLIENT_LIST", True),
        XInternAtom(display, "_NET_CLIENT_LIST_STACKING", True),
        XInternAtom(display, "_NET_ACTIVE_WINDOW", True),
        XInternAtom(display, "_NET_SUPPORTING_WM_CHECK", True),
        XInternAtom(display, "_NET_SUPPORTED", True),
    };
    Window dummy, *tree = None;
    unsigned num = 0;
    long inputMask = EnterWindowMask | LeaveWindowMask;
    if (XQueryTree(display, root, &dummy, &dummy, &tree, &num)) {
        for (unsigned i = 0; i < num; ++i) {
            XWindowAttributes attr;
            if (XGetWindowAttributes(display, tree[i], &attr) &&
                attr.override_redirect == False &&
                attr.map_state != IsUnmapped)
            {
                XSelectInput(display, tree[i], inputMask);
            }
        }
        XFree(tree);
    }
    Window enterWindow = None;
    double enterTime = 0.0;
    double delay = 0.2;
    bool loop = true;
    while (loop) {
        fsleep(delay);
        while (loop && XEventsQueued(display, QueuedAfterReading) > 0) {
            XEvent e;
            XNextEvent(display, &e);
            if (e.type == PropertyNotify) {
                if (e.xproperty.state == PropertyNewValue) {
                    for (int i = 0; i < atomCount; ++i) {
                        if (e.xproperty.atom == atoms[i]) {
                            loop = false;
                        }
                    }
                }
            }
            else if (e.type == MapNotify) {
                XWindowAttributes attr;
                if (XGetWindowAttributes(display, e.xmap.window, &attr) &&
                    attr.override_redirect == False &&
                    attr.map_state != IsUnmapped)
                {
                    XSelectInput(display, e.xmap.window, inputMask);
                }
            }
            else if (e.type == UnmapNotify) {
                if (enterWindow == e.xunmap.window) {
                    enterWindow = None;
                }
            }
            else if (e.type == DestroyNotify) {
                if (enterWindow == e.xdestroywindow.window) {
                    enterWindow = None;
                }
            }
            else if (e.type == EnterNotify) {
                enterWindow = e.xcrossing.window;
                enterTime = toDouble(monotime());
            }
            else if (e.type == LeaveNotify) {
                if (e.xcrossing.window == enterWindow &&
                    e.xcrossing.mode == NotifyNormal &&
                    e.xcrossing.detail != NotifyInferior)
                {
                    enterWindow = None;
                }
            }
        }
        if (loop) {
            int revert = RevertToNone;
            Window win = None;
            XGetInputFocus(display, &win, &revert);
            if (win == None) {
                XSetInputFocus(display, PointerRoot,
                               RevertToPointerRoot,
                               CurrentTime);
            }
            if (enterWindow) {
                double now = toDouble(monotime());
                if (now - enterTime >= delay * 0.9) {
                    XSetInputFocus(display, enterWindow,
                                   RevertToPointerRoot,
                                   CurrentTime);
                    enterWindow = None;
                }
            }
        }
    }
    XCloseDisplay(display);
}

int main(int argc, char **argv) {
    SessionManager xapp(&argc, &argv);

    xapp.loadEnv("env");

    xapp.runIcewmbg();
    xapp.runIcesound();
    xapp.runWM();

    int status = xapp.mainLoop();

    xapp.runScript("shutdown");
    xapp.runIcewmtray(true);
    xapp.runWM(true);
    xapp.runIcewmbg(true);
    xapp.runIcesound(true);

    return status;
}

// vim: set sw=4 ts=4 et:
