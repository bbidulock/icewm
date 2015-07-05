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

// the notification of startup step
unsigned int startup_phase = 1; // 1: run icewm, 2: run icewmtray, 3: run icewmbg, 4: run startup script

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
// startup steps notifications
        catchSignal(SIGUSR1);
    }

    void runScript(const char *scriptName, bool asenv=false) {
        upath scriptFile = YApplication::findConfigFile(scriptName);
        cstring cs(scriptFile.path());
        const char *args[] = { cs.c_str(), 0, 0 };
        if (strlen(cs.c_str()) != 0) {

        if(asenv) {
            FILE *ef = fopen(cs.c_str(), "r");
            if(!ef)
                return;
            tTempBuf scratch(500);
            if(!scratch)
                return;
            scratch.p[499] = 0;
            while(!feof(ef) && !ferror(ef))
            {
                char *line(scratch);
                if (!fgets(line, 497, ef))
                    break;
                for(int tlen = strlen(line)-1; isspace((unsigned)line[tlen]) && tlen; --tlen)
                    line[tlen] = 0;
#ifdef HAVE_WORDEXP
                wordexp_t w;
                wordexp(line, &w, 0);
                if(w.we_wordc > 0)
                    line = w.we_wordv[0];
#endif
                while(isspace( (unsigned) *line)) line++;
                char *ceq = strchr(line, (unsigned) '=');
                if(ceq) {
                    *ceq = 0;
                    setenv(line, ceq+1, 1);
                }
#ifdef HAVE_WORDEXP
                wordfree(&w);
#endif
            }
            fclose(ef);
            return;
        }

        MSG(("Running session script: %s", cs.c_str()));
        runProgram(cs.c_str(), args);
        }
    }

    void runIcewmbg(bool quit = false) {
        const char *args[] = { ICEWMBGEXE, "-n", 0 };

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
           switch ( startup_phase++ )
              {
              case 1:
                  runWM();
                  break;
              case 2:
                  runIcewmtray();
                  break;
              case 3:
                  runIcewmbg();
                  break;
              case 4:
                  runScript("startup");
                  break;
              default:
                  break;
              }
        }
    }

private:
    int wm_pid;
    int tray_pid;
    int bg_pid;
    bool logout;
};

int main(int argc, char **argv) {
    SessionManager xapp(&argc, &argv);

    xapp.runScript("env", true);

    xapp.handleSignal(SIGUSR1);
    
    xapp.mainLoop();

    xapp.runScript("shutdown");
    xapp.runIcewmbg(true);
    xapp.runIcewmtray(true);
    xapp.runWM(true);
    return 0;
}
