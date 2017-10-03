#ifndef SMAPP_H
#define SMAPP_H

#include "yxapp.h"

#ifdef CONFIG_SESSION

class YSMPoll: public YPoll<class YSMApplication> {
public:
    virtual void notifyRead();
    virtual void notifyWrite();
    virtual bool forRead();
    virtual bool forWrite();
};

class YSMApplication: public YXApplication { /// !!! should be possible without X
public:
    YSMApplication(int *argc, char ***argv, const char *displayName = 0);
    virtual ~YSMApplication();

    bool haveSessionManager();
    virtual void smSaveYourself(bool shutdown, bool fast);
    virtual void smSaveYourselfPhase2();
    virtual void smSaveComplete();
    virtual void smShutdownCancelled();
    virtual void smDie();
    void smSaveDone();
    void smRequestShutdown();
    void smCancelShutdown();
private:
    YSMPoll psm;
};

extern YSMApplication *smapp;

#else

typedef YXApplication YSMApplication;

#endif

#endif

// vim: set sw=4 ts=4 et:
