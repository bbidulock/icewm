#include "config.h"
#include "base.h"
#include "ysocket.h"
#include "yapp.h"
#include "debug.h"

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>


YSingleList<YSocket> YSocket::sockets;


YSocket::YSocket():
    fSocket(-1), fConnecting(false), fReading(false), fRegistered(false),
    rdbuf(NULL), rdbuflen(0),
    fListener(NULL) {
}

YSocket::~YSocket() {
    if (fSocket != -1) ::close(fSocket);

    stopMonitoring();
}

int YSocket::connect(struct sockaddr *server_addr, int addrlen) {
    int fd;

    fd = ::socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1) return -errno;

    if (::fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        ::close(fd); return -errno;
    }

    if (::fcntl(fd, F_SETFD, 1) == -1) {
        ::close(fd); return -errno;
    }

    MSG(("connecting socket"));

    if (::connect(fd, server_addr, addrlen) == -1) {
        if (errno == EINPROGRESS) {
            MSG(("socket in progress"));

            fConnecting = true;
            fSocket = fd;

            startMonitoring();

            return 0;
        }

        MSG(("error during connect"));
        ::close(fd);

        return -errno;
    }

    fSocket = fd;

    if (fListener) fListener->socketConnected();

    return 0;
}

int YSocket::close() {
    stopMonitoring();

    if (fSocket == -1) return -EINVAL;
    if (::close(fSocket) == -1) return -errno;

    fSocket = -1;
    return 0;
}

int YSocket::read(char *buf, int len) {
#if 0
    int rc(::read(fSocket, (void *)buf, len));

    if (rc == 0) {
        if (fListener) fListener->socketError(0);
        return 0;
    } else if (rc == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
#endif
            rdbuf = buf;
            rdbuflen = len;
            fReading = true;
            
            startMonitoring();
#if 0
        } else {
            return -errno;
        }
    }
    if (fListener) fListener->socketDataRead(buf, rc);
    return rc;
#else
    return 0;
#endif    
}

int YSocket::write(const char *buf, int len) {
    int rc(::write(fSocket, (const void *)buf, len));

    // must loop !!!

    return (rc != -1 ? rc : -errno);
}

void YSocket::readable() {
    fReading = false;
    stopMonitoring();

    int rc(::read(fSocket, rdbuf, rdbuflen));

    if (rc == 0) {
        if (fListener) fListener->socketError(0);
        return;
    } else if (rc == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            fReading = true;
            startMonitoring();
        } else
            if (fListener) fListener->socketError(-errno);

        return;
    }

    if (fListener) fListener->socketDataRead(rdbuf, rc);
}

void YSocket::connected() {
    int err = 0;
    char x[1];

    fConnecting = false;
    MSG(("connected"));

    stopMonitoring();

    if (::recv(fSocket, x, 0, 0) == -1) { // ???!!!
        MSG(("after connect check"));
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
        } else if (errno == ENOTCONN) {
            fConnecting = true;
            startMonitoring();
            return;
        } else {
            err = errno;
            MSG(("here"));
            if (fListener)
                fListener->socketError(err);
            return ;
        }
    }
    if (fListener)
        fListener->socketConnected();
}

void YSocket::registerSockets(fd_set & readers, fd_set & writers) {
    for (Iterator socket(sockets); socket; ++socket) {
        if (socket->fReading) {
            FD_SET(socket->fSocket, &readers);
            MSG(("waiting for data for socket %d", socket->fSocket));
        }
        if (socket->fConnecting) {
            FD_SET(socket->fSocket, &writers);
            MSG(("waiting for connect for socket %d", socket->fSocket));
        }
    }
}

void YSocket::handleSockets(fd_set & readers, fd_set & writers) {
    for (Iterator socket(sockets); socket; ++socket) {
        if (socket->fReading && FD_ISSET(socket->fSocket, &readers)) {
            MSG(("got data for socket %d", socket->fSocket));
            socket->readable();
        }
        if (socket->fConnecting && FD_ISSET(socket->fSocket, &writers)) {
            MSG(("got connect for socket %d", socket->fSocket));
            socket->connected();
        }
    }
}

void YSocket::startMonitoring() {
    if (!fRegistered) {
        fRegistered = true;
        sockets.prepend(this);
    }
}

void YSocket::stopMonitoring() {
    if (fRegistered) {
        fRegistered = false;
        sockets.remove(this);
    }
}
