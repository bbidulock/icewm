#ifndef YSOCKET_H_
#define YSOCKET_H_

#include "ypoll.h"

class YSocketListener {
public:
    virtual void socketConnected() = 0;
    virtual void socketError(int err) = 0;
    virtual void socketDataRead(char *buf, int len) = 0;
protected:
    virtual ~YSocketListener() {}
};

class YSocket: private YPollBase {
public:
    YSocket();
    virtual ~YSocket();

    int connect(struct sockaddr *server_addr, int addrlen);
    int socketpair(int *otherfd);
    int socket() const;
    void terminate();

    int read(char *buf, int len);
    int write(const char *buf, int len);
    void shutdown();

    void setListener(YSocketListener *l) { fListener = l; }

private:
    YSocketListener *fListener;

    char *rdbuf;
    int rdbuflen;

    bool connecting;
    bool reading;

    virtual void notifyRead();
    virtual void notifyWrite();
    virtual bool forRead() { return reading; }
    virtual bool forWrite() { return connecting; }
};

#endif

// vim: set sw=4 ts=4 et:
