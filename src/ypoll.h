#ifndef __YPOLL_H__
#define __YPOLL_H__

class YPoll {
protected:
    friend class YApplication;

    YPoll(): fd(-1) { }

    int fd;
    YPoll *fPrev;
    YPoll *fNext;

    virtual void notifyRead() = 0;
    virtual void notifyWrite() = 0;
    virtual bool forRead() = 0;
    virtual bool forWrite() = 0;
protected:
    virtual ~YPoll() {};
};

#endif
