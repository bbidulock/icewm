#ifndef YSOCKET_H_
#define YSOCKET_H_

#include "ypoll.h"

class YSocketListener {
public:
    virtual void socketConnected() = 0;
    virtual void socketError(int err) = 0;
    virtual void socketDataRead(char *buf, int len) = 0;
protected:
    virtual ~YSocketListener() {};
};

class YSocket: private YPollBase {
public:
    YSocket();
    virtual ~YSocket();

    int connect(struct sockaddr *server_addr, int addrlen);
    int socketpair(int *otherfd);
    int close();

    int read(char *buf, int len);
    int write(const char *buf, int len);

    void setListener(YSocketListener *l) { fListener = l; }
private:
    YSocketListener *fListener;

    bool connecting;
    bool reading;
    bool registered;

    char *rdbuf;
    int rdbuflen;

    friend class YApplication;

    virtual void notifyRead();
    virtual void notifyWrite();
    virtual bool forRead();
    virtual bool forWrite();
};

#endif
