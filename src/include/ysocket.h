#ifndef YSOCKET_H_
#define YSOCKET_H_

#pragma interface

//#warning "replace char with byte below"
typedef unsigned char byte;

class YSocketListener {
public:
    virtual void socketConnected() = 0;
    virtual void socketError(int err) = 0;
    virtual void socketDataRead(byte *buf, int len) = 0;
};

struct sockaddr;

class YSocket {
public:
    YSocket();
    ~YSocket();

    int connect(struct sockaddr *server_addr, int addrlen);
    int close();

    int read(byte *buf, int len);
    int write(const byte *buf, int len);

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

    byte *rdbuf;
    int rdbuflen;

    void can_read();
    void connected();
private: // not-used
    YSocket(const YSocket &);
    YSocket &operator=(const YSocket &);
};

#endif
