#ifndef __YAPP_H
#define __YAPP_H

#include <signal.h>

#include "ypaths.h"
#include "ypoll.h"

class YTimer;
class YClipboard;

class YSignalPoll: public YPoll<class YApplication> {
public:
    virtual void notifyRead();
    virtual void notifyWrite();
    virtual bool forRead();
    virtual bool forWrite();
};

class YApplication {
public:
    YApplication(int *argc, char ***argv);
    virtual ~YApplication();

    int mainLoop();
    void exitLoop(int exitCode);
    void exit(int exitCode);

    char const * executable() { return fExecutable; }

    virtual void handleSignal(int sig);
    virtual bool handleIdle();

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

    static char const *& Name;

private:
    YTimer *fFirstTimer, *fLastTimer;
    YPollBase *fFirstPoll, *fLastPoll;

    YSignalPoll sfd;
    friend class YSignalPoll;

    int fLoopLevel;
    int fExitLoop;
    int fExitCode;
    int fExitApp;

    char const * fExecutable;

    friend class YTimer;
    friend class YSocket;
    friend class YPipeReader;

    void getTimeout(struct timeval *timeout);
    void handleTimeouts();
    void decreaseTimeouts(struct timeval difftime);

    void handleSignalPipe();
    void initSignals();

public:
    void registerTimer(YTimer *t);
    void unregisterTimer(YTimer *t);
    void registerPoll(YPollBase *t);
    void unregisterPoll(YPollBase *t);

protected:
    virtual void flushXEvents() {};
    virtual bool handleXEvents() { return false; }
};

extern YApplication *app;

#endif
