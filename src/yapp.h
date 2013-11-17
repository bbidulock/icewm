#ifndef __YAPP_H
#define __YAPP_H

#include <signal.h>

#include "ypaths.h"
#include "upath.h"
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

class IApp {
public:
    virtual upath findConfigFile(upath relativePath) = 0;
    virtual void runCommand(const char *prog) = 0;
    virtual int runProgram(const char *path, const char *const *args) = 0;
    virtual void exit(int exitCode) = 0;
    virtual int waitProgram(int p) = 0;
};

class IMainLoop {
public:
    virtual void registerTimer(YTimer *t) = 0;
    virtual void unregisterTimer(YTimer *t) = 0;
    virtual void registerPoll(YPollBase *t) = 0;
    virtual void unregisterPoll(YPollBase *t) = 0;
    virtual int startWorker(int socket, const char *path, const char *const *args) = 0; 
};

class YApplication: public IApp, public IMainLoop {
public:
    YApplication(int *argc, char ***argv);
    virtual ~YApplication();

    int mainLoop();
    void exitLoop(int exitCode);
    virtual void exit(int exitCode);

#if 0
    upath executable() { return fExecutable; }
#endif

    virtual void handleSignal(int sig);
    virtual bool handleIdle();

    void catchSignal(int sig);
    void resetSignals();
    //void unblockSignal(int sig);

    virtual int runProgram(const char *path, const char *const *args);
    virtual int startWorker(int socket, const char *path, const char *const *args);
    virtual void runCommand(const char *prog);
    virtual int waitProgram(int p);

    virtual upath findConfigFile(upath relativePath);
    static const char *getLibDir();
    static const char *getConfigDir();
    static const char *getPrivConfDir();

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

#if 0
    upath fExecutable;
#endif

    friend class YSocket;
    friend class YPipeReader;

    void getTimeout(struct timeval *timeout);
    void handleTimeouts();
    void decreaseTimeouts(struct timeval difftime);

    void handleSignalPipe();
    void initSignals();

protected:
    friend class YTimer;
    friend class YPollBase;

    virtual void registerTimer(YTimer *t);
    virtual void unregisterTimer(YTimer *t);
    void nextTimeout(struct timeval *timeout);
    void nextTimeoutWithFuzziness(struct timeval *timeout);
    virtual void registerPoll(YPollBase *t);
    virtual void unregisterPoll(YPollBase *t);

protected:
    virtual void flushXEvents() {};
    virtual bool handleXEvents() { return false; }

    void closeFiles();
};

//extern YApplication *app;
extern IMainLoop *mainLoop;

#endif
