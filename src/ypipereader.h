#ifndef __YPOPEN_H__
#define __YPOPEN_H__

#include "ypoll.h"

class YPipeListener {
public:
    virtual void pipeError(int error) = 0;
    virtual void pipeDataRead(char *buf, int len) = 0;
protected:
    virtual ~YPipeListener() {};
};

class YPipeReader: public YPoll {
public:
    YPipeReader();
    virtual ~YPipeReader();

    int spawnvp(const char *prog, char **args);
    int read(char *buf, int len);

    void setListener(YPipeListener *l) { fListener = l; }
private:
    friend class YApplication;

    YPipeListener *fListener;
    char *rdbuf;
    int rdbuflen;
    bool reading;
    bool registered;

    virtual void notifyRead();
    virtual void notifyWrite();
    virtual bool forRead();
    virtual bool forWrite();
};

#endif
