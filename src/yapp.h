#ifndef __YAPP_H
#define __YAPP_H

#include <signal.h>

#include "ypaths.h"

class YTimer;
class YPoll;
class YClipboard;

class YApplication {
public:
    YApplication(int *argc, char ***argv);
    virtual ~YApplication();

    int mainLoop();
    void exitLoop(int exitCode);
    void exit(int exitCode);

    char const * executable() { return fExecutable; }

    virtual void handleSignal(int sig);
    virtual void handleIdle();

    void catchSignal(int sig);
    void resetSignals();
    //void unblockSignal(int sig);

    int runProgram(const char *path, const char *const *args);
    int waitProgram(int p);
    void runCommand(const char *prog);

    static const char *getPrivConfDir();

    static char *findConfigFile(const char *name);
    static char *findConfigFile(const char *name, int mode);
    static bool loadConfig(struct cfoption *options, const char *name);

    virtual int readFdCheckSM() { return -1; }
    virtual void readFdActionSM() {}

    static char const *& Name;

private:
    YTimer *fFirstTimer, *fLastTimer;
    YPoll *fFirstPoll, *fLastPoll;

    int fLoopLevel;
    int fExitLoop;
    int fExitCode;
    int fExitApp;

    char const * fExecutable;

    friend class YTimer;
    friend class YSocket;
    friend class YPipeReader;

    void registerTimer(YTimer *t);
    void unregisterTimer(YTimer *t);
    void getTimeout(struct timeval *timeout);
    void handleTimeouts();
    void registerPoll(YPoll *t);
    void unregisterPoll(YPoll *t);

protected:
    virtual bool handleXEvents() { return false; }
    virtual int readFDCheckX() { return -1; }
    virtual void flushXEvents() {}

    void closeFiles();
};

extern YApplication *app;

#endif
