#include "config.h"
#include "base.h"
#include "yapp.h"
#include "sysdep.h"

char const *ApplicationName = "icewm-session";
bool logout = false;

void runScript(const char *scriptName) {
    char *scriptFile = app->findConfigFile(scriptName, X_OK);
    const char *args[] = { scriptFile, 0, 0 };

    MSG(("Running session script: %s", scriptFile));
    app->runProgram(scriptFile, args);
    delete[] scriptFile;
}

int runIcewmbg(bool quit = false) {
    const char *args[] = { "icewmbg", 0, 0 };
    if (quit) {
        args[1] = "-q";
    }
    return app->runProgram(args[0], args);
}

int runIcewmtray() {
    const char *args[] = { "icewmtray", 0 };
    return app->runProgram(args[0], args);
}

int runWM() {
    const char *args[] = { "icewm", 0 };
    return app->runProgram(args[0], args);
}

int main(int argc, char **argv) {

    YApplication xapp(&argc, &argv);

    runIcewmbg();
    runIcewmtray();
    runScript("startup");

    int wm_pid = runWM();
    while (!logout) {
        int status = -1;
        int pid = -1;
	msg("wm_pid=%d", wm_pid);
        pid = waitpid(-1, &status, 0);
        if (pid == wm_pid) {
            msg("status=%X", status);
            if (WIFEXITED(status))
                break;
            if (WEXITSTATUS(status) != 0)
                wm_pid = runWM();
        }
    }

    runScript("shutdown");
    runIcewmbg(true);
    return 0;
}
