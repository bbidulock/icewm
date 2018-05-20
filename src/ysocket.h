#ifndef YSOCKET_H_
#define YSOCKET_H_

#include "ypoll.h"

class cstring;
class mstring;

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
    void close();

    int read(char *buf, int len);
    int write(const char *buf, int len);
    int write(const cstring& str);
    void shutdown();

    void setListener(YSocketListener *l) { fListener = l; }

private:
    YSocketListener *fListener;

    bool connecting;
    bool reading;
    bool registered;

    char *rdbuf;
    int rdbuflen;

    virtual void notifyRead();
    virtual void notifyWrite();
    virtual bool forRead();
    virtual bool forWrite();
};

#endif

// vim: set sw=4 ts=4 et:
