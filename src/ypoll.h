#ifndef __YPOLL_H__
#define __YPOLL_H__

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

#endif

// vim: set sw=4 ts=4 et:
