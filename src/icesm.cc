#include "config.h"
#include "base.h"
#include "yapp.h"
#include "sysdep.h"

char const *ApplicationName = "icewm-session";

class SessionManager: public YApplication {
public:
    SessionManager(int *argc, char ***argv, const char *displayName = 0): YApplication(argc, argv, displayName) {
        logout = false;
        wm_pid = -1;
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
        const char *args[] = { "icewmbg", 0, 0 };
        if (quit) {
            args[1] = "-q";
        }
        bg_pid =  app->runProgram(args[0], args);
    }

    void runIcewmtray() {
        const char *args[] = { "icewmtray", 0 };
        tray_pid = app->runProgram(args[0], args);
    }

    void runWM() {
        const char *args[] = { "icewm", 0 };
        wm_pid =  app->runProgram(args[0], args);
    }

    void handleSignal(int sig) {
	if (sig == SIGTERM || sig == SIGINT) 
            exit(0);
        if (sig == SIGCHLD) {
            int status = -1;
            int pid = -1;

            msg("wm_pid=%d", wm_pid);
            pid = waitpid(wm_pid, &status, 0);
            if (pid == wm_pid) {
                msg("status=%X", status);
                if (WIFEXITED(status)) {
                    exit(0);
                } else {
                    if (WEXITSTATUS(status) != 0)
                        runWM();
                    else if (WIFSIGNALED(status) != 0)
                        runWM();
                }
            }
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
    xapp.runIcewmtray();
    xapp.runScript("startup");

    xapp.runWM();

    xapp.mainLoop();

    xapp.runScript("shutdown");
    xapp.runIcewmbg(true);
    return 0;
}
