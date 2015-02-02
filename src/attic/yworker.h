#ifndef _WORKERPROCESS_H_
#define _WORKERPROCESS_H_

#include "ysocket.h"

class YWorkerProcess;

class IWorkerListener {
public:
    virtual void handleMessage(YWorkerProcess *worker, char *buffer, int length) = 0;
protected:
    virtual ~IWorkerListener() {};
};

class YWorkerProcess: public YSocketListener {
public:
    YWorkerProcess();

    int start(const char *command, char **args);
    void setListener(IWorkerListener *listener) { fListener = listener; }

    virtual void socketConnected();
    virtual void socketError(int err);
    virtual void socketDataRead(char *buf, int len);
private:
    YSocket worker;
    int worker_pid;
    char head_buffer[128];
    enum { rsWAIT, rsHEAD, rsBODY, rsERROR } readState;
    char *body_buffer;
    int body_len;
    int body_read;
    IWorkerListener *fListener;

    void content_read(char *buffer, int length);
};

#endif
