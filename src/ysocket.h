#ifndef YSOCKET_H_
#define YSOCKET_H_

#include "ylists.h"
#include <sys/types.h>
#include <netinet/in.h>

class YSocket:
public YSingleList<YSocket>::Item {
public:
    class Listener {
    public:
        virtual void socketConnected() = 0;
        virtual void socketError(int err) = 0;
        virtual void socketDataRead(char *buf, int len) = 0;
    };

    YSocket();
    ~YSocket();

    int connect(struct sockaddr *server_addr, int addrlen);
    int close();

    int read(char *buf, int len);
    int write(const char *buf, int len);

    int handle() { return fSocket; }

    void socketListener(Listener *l) { fListener = l; }
    
    static void registerSockets(fd_set & readers, fd_set & writers);
    static void handleSockets(fd_set & readers, fd_set & writers);

private:
    int fSocket;
    bool fConnecting;
    bool fReading;
    bool fRegistered;

    char *rdbuf;
    int rdbuflen;

    Listener *fListener;

    void readable();
    void connected();

    void startMonitoring();
    void stopMonitoring();

    static YSingleList<YSocket> sockets;
};

#endif
