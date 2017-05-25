#include "config.h"
#include "base.h"
#include "yxapp.h"
#include "sysdep.h"
#include "yconfig.h"
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

public:
    SessionManager(int *argc, char ***argv): YApplication(argc, argv) {
        startup_phase = 0;
        logout = false;
        wm_pid = -1;
        tray_pid = -1;
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

    void runScript(const char *scriptName) {
        upath scriptFile = YApplication::findConfigFile(scriptName);
        if (scriptFile.nonempty() && scriptFile.isExecutable()) {
            const char *cs = scriptFile.string();
            MSG(("Running session script: %s", cs));
            runProgram(cs, 0);
        }
    }

    void runIcewmbg(bool quit = false) {
        const char *args[] = { ICEWMBGEXE, 0, 0 };

        if (quit) {
            args[1] = "-q";
        }
        bg_pid =  runProgram(args[0], args);
    }

    void runIcewmtray(bool quit = false) {
        const char *args[] = { ICEWMTRAYEXE, "--notify", 0 };
        if (quit) {
            if (tray_pid != -1) {
                kill(tray_pid, SIGTERM);
                int status;
                waitpid(tray_pid, &status, 0);
            }
            tray_pid = -1;
        } else
            tray_pid = runProgram(args[0], args);
    }

    void runWM(bool quit = false) {
        const char *args[] = { ICEWMEXE, "--notify", 0 };
        if (quit) {
            if (wm_pid != -1) {
                kill(wm_pid, SIGTERM);
                int status;
                waitpid(wm_pid, &status, 0);
            }
            wm_pid = -1;
        }
        else
            wm_pid = runProgram(args[0], args);
    }

    void handleSignal(int sig) {
        if (sig == SIGTERM || sig == SIGINT) {
            signal(SIGTERM, SIG_IGN);
            signal(SIGINT, SIG_IGN);
            exit(0);
        }
        if (sig == SIGCHLD) {
            int status = -1;
            int pid = -1;

            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                MSG(("waitpid()=%d, status=%d", pid, status));
                if (pid == wm_pid) {
                    wm_pid = -1;
                    if (WIFEXITED(status)) {
                        exit(0);
                    } else {
                        if (WEXITSTATUS(status) != 0)
                            runWM();
                        else if (WIFSIGNALED(status) != 0)
                            runWM();
                    }
                }
                if (pid == tray_pid)
                    tray_pid = -1;
                if (pid == bg_pid)
                    bg_pid = -1;
            }
        }

        if (sig == SIGUSR1)
        {
           if (++startup_phase == 1)
              runScript("startup");
           else if (startup_phase == 2)
              runIcewmtray();
        }
    }

private:
    int startup_phase;
    int wm_pid;
    int tray_pid;
    int bg_pid;
    bool logout;
};

int main(int argc, char **argv) {
    check_argv(argc, argv, "", VERSION);

    SessionManager xapp(&argc, &argv);

    xapp.loadEnv("env");

    xapp.runIcewmbg();
    xapp.runWM();

    xapp.mainLoop();

    xapp.runScript("shutdown");
    xapp.runIcewmtray(true);
    xapp.runWM(true);
    xapp.runIcewmbg(true);
    return 0;
}
