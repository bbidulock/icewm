#ifndef YSOCKET_H_
#define YSOCKET_H_

#include <sys/types.h>
#include <netinet/in.h>

class YSocketListener {
public:
    virtual void socketConnected() = 0;
    virtual void socketError(int err) = 0;
    virtual void socketDataRead(char *buf, int len) = 0;
};

class YSocket {
public:
    YSocket();
    ~YSocket();

    int connect(struct sockaddr *server_addr, int addrlen);
    int close();

    int read(char *buf, int len);
    int write(const char *buf, int len);

    int handle() { return sockfd; }

    void setListener(YSocketListener *l) { fListener = l; }
private:
    friend class YApplication;

    YSocket *fPrev;
    YSocket *fNext;

    YSocketListener *fListener;

    int sockfd;
    bool connecting;
    bool reading;
    bool registered;

    char *rdbuf;
    int rdbuflen;

    void can_read();
    void connected();
};

#endif
