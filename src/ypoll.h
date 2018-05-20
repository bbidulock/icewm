#ifndef __YPOLL_H__
#define __YPOLL_H__

class YPollBase {
public:
    YPollBase(): fFd(-1), fPrev(0), fNext(0) { }
    virtual ~YPollBase();

    virtual void notifyRead() = 0;
    virtual void notifyWrite() = 0;
    virtual bool forRead() = 0;
    virtual bool forWrite() = 0;

    void registerPoll(int fd);
    void unregisterPoll();

    int fd() const { return fFd; }

protected:
    int fFd;
    YPollBase *fPrev;
    YPollBase *fNext;
};

template<class T> class YPoll: public YPollBase {
public:
    YPoll(): YPollBase() {}

    void registerPoll(T *owner, int fd) {
        fOwner = owner;
        YPollBase::registerPoll(fd);
    }

    T *owner() { return fOwner; }
private:
    T *fOwner;
protected:
    virtual ~YPoll() {}
};

#endif

// vim: set sw=4 ts=4 et:
