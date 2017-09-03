#include "config.h"
#include "base.h"
#include "intl.h"
#include "yapp.h"
#include "sysdep.h"
#include "appnames.h"
#ifdef HAVE_WORDEXP
#include <wordexp.h>
#endif

char const *ApplicationName = ICESMEXE;

class SessionManager: public YApplication {
private:
    char* trim(char *line) {
        size_t len = strlen(line);
        while (len > 0 && isspace((unsigned char) line[len - 1])) {
            line[--len] = 0;
        }
        while (*line && isspace((unsigned char) *line))
            ++line;
        return line;
    }

    bool expand(char *buf, size_t bufsiz) {
#ifdef HAVE_WORDEXP
        wordexp_t w;
        if (wordexp(trim(buf), &w, 0) != 0 || w.we_wordc == 0)
            return false;
        size_t len = strlcpy(buf, trim(w.we_wordv[0]), bufsiz);
        for (size_t k = 1; k < w.we_wordc && len < bufsiz; ++k) {
            strlcat(buf, " ", bufsiz);
            len = strlcat(buf, trim(w.we_wordv[k]), bufsiz);
        }
        wordfree(&w);
        if (len >= bufsiz)
            return false;
#else
        char *str = trim(buf);
        if (str > buf)
            strlcpy(buf, str, bufsiz);
#endif
        if (buf[0] == '#' || buf[0] == '=')
            buf[0] = 0;
        return buf[0] != 0;
    }

    const char *get_help_text() {
        return _(
        "  -c, --config=FILE   Let IceWM load preferences from FILE.\n"
        "  -t, --theme=FILE    Let IceWM load the theme from FILE.\n"
        "\n"
        "  --display=NAME      Use NAME to connect to the X server.\n"
        "  --sync              Synchronize communication with X11 server.\n"
        "\n"
        "  -n, --notray        Do not start icewmtray.\n"
        "  -s, --sound         Also start icesound.\n"
        );
    }

    const char *displayArg;
    const char *configArg;
    const char *themeArg;
    bool syncArg;
    bool notrayArg;
    bool soundArg;
    char* argv0;

    void options(int *argc, char ***argv) {
        argv0 = **argv;
        displayArg = 0;
        configArg = 0;
        themeArg = 0;
        syncArg = false;
        notrayArg = false;
        soundArg = false;

        for (char **arg = 1 + *argv; arg < *argv + *argc; ++arg) {
            if (**arg == '-') {
                char *value(0);
                if (GetLongArgument(value, "display", arg, *argv+*argc)) {
                    if (value && *value)
                        displayArg = value;
                }
                else if (GetArgument(value, "c", "config", arg, *argv+*argc)) {
                    configArg = value;
                }
                else if (GetArgument(value, "t", "theme", arg, *argv+*argc)) {
                    themeArg = value;
                }
                else if (is_long_switch(*arg, "sync")) {
                    syncArg = true;
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
                else {
                    warn(_("Unknown option '%s'"), *arg);
                }
            }
        }
        if (displayArg)
            setenv("DISPLAY", displayArg, 1);
    }

public:
    SessionManager(int *argc, char ***argv): YApplication(argc, argv) {
        options(argc, argv);
        startup_phase = 0;
        logout = false;
        wm_pid = -1;
        tray_pid = -1;
        sound_pid = -1;
        bg_pid = -1;
        catchSignal(SIGCHLD);
        catchSignal(SIGTERM);
        catchSignal(SIGINT);
        catchSignal(SIGUSR1);
    }

    void loadEnv(const char *scriptName) {
        upath scriptFile = YApplication::findConfigFile(scriptName);
        if (scriptFile.nonempty()) {
            FILE *ef = scriptFile.fopen("r");
            if(!ef)
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
    }

    virtual int runProgram(const char *file, const char *const *args) {
        upath path;
        if (strchr(file, '/') == NULL && strchr(argv0, '/') != NULL) {
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
            runProgram(cs, 0);
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
        if (syncArg && k + 1 < narg) {
            args[k++] = "--sync";
        }
        if (k < narg) {
            args[k] = 0;
        }
    }

    void runIcewmbg(bool quit = false) {
        const char *args[12] = { ICEWMBGEXE, 0, 0 };

        if (quit) {
            args[1] = "-q";
        }
        else {
            appendOptions(args, 1, ACOUNT(args));
        }
        bg_pid =  runProgram(args[0], args);
    }

    void runIcewmtray(bool quit = false) {
        const char *args[12] = { ICEWMTRAYEXE, "--notify", 0 };
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
        const char *args[12] = { ICESOUNDEXE, "--verbose", 0 };
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
        const char *args[12] = { ICEWMEXE, "--notify", 0 };
        if (quit) {
            if (wm_pid != -1) {
                kill(wm_pid, SIGTERM);
                int status;
                waitpid(wm_pid, &status, 0);
            }
            wm_pid = -1;
        }
        else {
            appendOptions(args, 2, ACOUNT(args));
            wm_pid = runProgram(args[0], args);
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
                else if (pid == bg_pid)
                    bg_pid = -1;
            }
        }
        else if (sig == SIGUSR1)
            notified();
    }

    void checkWMExitStatus(int status) {
        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status)) {
                tlog(_("%s exited with status %d."),
                        ICEWMEXE, WEXITSTATUS(status));
            }
            this->exit(0);
        }
        else if (WIFSIGNALED(status)) {
            tlog(_("%s was killed by signal %d."),
                    ICEWMEXE, WTERMSIG(status));
            runWM();
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
    int wm_pid;
    int tray_pid;
    int sound_pid;
    int bg_pid;
    bool logout;
};

int main(int argc, char **argv) {
    SessionManager xapp(&argc, &argv);

    xapp.loadEnv("env");

    xapp.runIcewmbg();
    xapp.runIcesound();
    xapp.runWM();

    xapp.mainLoop();

    xapp.runScript("shutdown");
    xapp.runIcewmtray(true);
    xapp.runWM(true);
    xapp.runIcewmbg(true);
    xapp.runIcesound(true);
    return 0;
}
