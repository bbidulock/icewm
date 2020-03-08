#ifndef __YPOPEN_H__
#define __YPOPEN_H__

#include "ypoll.h"

class YPipeListener {
public:
    virtual void pipeError(int error) = 0;
    virtual void pipeDataRead(char *buf, int len) = 0;
protected:
    virtual ~YPipeListener() {}
};

class YPipeReader: public YPollBase {
public:
    YPipeReader();
    virtual ~YPipeReader();

    int spawnvp(const char *prog, char **args);
    int read(char *buf, int len);
    void pipeClose();

    void setListener(YPipeListener *l) { fListener = l; }

private:
    YPipeListener *fListener;
    char *rdbuf;
    int rdbuflen;
    bool reading;

    virtual void notifyRead();
    virtual bool forRead() { return reading; }
};

#endif

// vim: set sw=4 ts=4 et:
