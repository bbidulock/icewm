#include "config.h"
#include "base.h"
#include "yxapp.h"
#include "sysdep.h"

char const *ApplicationName = ICESMEXE;

class SessionManager: public YApplication {
public:
    SessionManager(int *argc, char ***argv): YApplication(argc, argv) {
        logout = false;
        wm_pid = -1;
        tray_pid = -1;
        bg_pid = -1;
        catchSignal(SIGCHLD);
        catchSignal(SIGTERM);
        catchSignal(SIGINT);
    }

    void runScript(const char *scriptName) {
        char *scriptFile = app->findConfigFile(scriptName, X_OK);
        const char *args[] = { scriptFile, 0, 0 };

        MSG(("Running session script: %s", scriptFile));
        app->runProgram(scriptFile, args);
        delete[] scriptFile;
    }

    void runIcewmbg(bool quit = false) {
        const char *args[] = { ICEWMBGEXE, 0, 0 };

        if (quit) {
            args[1] = "-q";
        }
        bg_pid =  app->runProgram(args[0], args);
    }

    void runIcewmtray(bool quit = false) {
        const char *args[] = { ICEWMTRAYEXE, 0 };
        if (quit) {
            if (tray_pid != -1) {
                kill(tray_pid, SIGTERM);
                int status;
                waitpid(tray_pid, &status, 0);
            }
            tray_pid = -1;
        } else
            tray_pid = app->runProgram(args[0], args);
    }

    void runWM(bool quit = false) {
        const char *args[] = { ICEWMEXE, 0 };
        if (quit) {
            if (wm_pid != -1) {
                kill(wm_pid, SIGTERM);
                int status;
                waitpid(wm_pid, &status, 0);
            }
            wm_pid = -1;
        }
        else
            wm_pid =  app->runProgram(args[0], args);
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

            pid = waitpid(-1, &status, 0);
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
        YApplication::handleSignal(sig);
    }

private:
    int wm_pid;
    int tray_pid;
    int bg_pid;
    bool logout;
};

int main(int argc, char **argv) {
    SessionManager xapp(&argc, &argv);

    xapp.runIcewmbg();
    xapp.runWM();
    xapp.runIcewmtray();
    xapp.runScript("startup");

    xapp.mainLoop();

    xapp.runScript("shutdown");
    xapp.runIcewmtray(true);
    xapp.runWM(true);
    xapp.runIcewmbg(true);
    return 0;
}
