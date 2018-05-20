#ifndef __YAPP_H
#define __YAPP_H

#include "upath.h"
#include "yarray.h"
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
};

class YApplication: public IApp, public IMainLoop {
public:
    YApplication(int *argc, char ***argv);
    virtual ~YApplication();

    int mainLoop();
    void exitLoop(int exitCode);
    virtual void exit(int exitCode);

    virtual void handleSignal(int sig);
    virtual bool handleIdle();

    void catchSignal(int sig);
    void resetSignals();

    virtual int runProgram(const char *path, const char *const *args);
    virtual void runCommand(const char *prog);
    virtual int waitProgram(int p);

    virtual upath findConfigFile(upath relativePath);
    static const upath& getLibDir();
    static const upath& getConfigDir();
    static const upath& getPrivConfDir();
    static upath getHomeDir();

private:
    YArray<YTimer*> timers;
    YArray<YPollBase*> polls;

    YSignalPoll sfd;
    friend class YSignalPoll;

    int fLoopLevel;
    int fExitLoop;
    int fExitCode;
    int fExitApp;

    bool getTimeout(struct timeval *timeout);
    void handleTimeouts();
    void decreaseTimeouts(struct timeval difftime);

    void handleSignalPipe();
    void initSignals();

protected:
    friend class YTimer;
    friend class YPollBase;

    virtual void registerTimer(YTimer *t);
    virtual void unregisterTimer(YTimer *t);
    bool nextTimeout(struct timeval *timeout);
    bool nextTimeoutWithFuzziness(struct timeval *timeout);
    virtual void registerPoll(YPollBase *t);
    virtual void unregisterPoll(YPollBase *t);

protected:
    virtual void flushXEvents() {}
    virtual bool handleXEvents() { return false; }

    void closeFiles();
};

extern IMainLoop *mainLoop;


#endif

// vim: set sw=4 ts=4 et:
