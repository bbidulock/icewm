#ifndef YPOLL_H
#define YPOLL_H

class YPollBase {
public:
    YPollBase(): fFd(-1), fRegistered(false) { }
    virtual ~YPollBase();

    virtual void notifyRead() { }
    virtual void notifyWrite() { }
    virtual bool forRead() { return false; }
    virtual bool forWrite() { return false; }

    void registerPoll(int fd);
    void unregisterPoll();
    void closePoll();

    int fd() const { return fFd; }
    bool registered() const { return fRegistered; }

protected:
    void initializePoll(int fd) { fFd = fd; }

private:
    int fFd;
    bool fRegistered;
};

template<class T>
class YPoll: public YPollBase {
protected:
    explicit YPoll(T* owner):
        YPollBase(), fOwner(owner)
    { }

    T* owner() const { return fOwner; }

private:
    T *fOwner;
};

class YPidWaiter {
public:
    virtual void waitCompleted(int pid, int status) = 0;
    virtual void waitForPid(int pid);
    virtual ~YPidWaiter();
};

#endif

// vim: set sw=4 ts=4 et:
