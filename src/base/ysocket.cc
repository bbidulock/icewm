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

YSocket::YSocket() {
    fPrev = fNext = 0;
    fListener = 0;
    sockfd = -1;
    rdbuf = 0;
    rdbuflen = 0;
    connecting = false;
    reading = false;
    registered = false;
}

YSocket::~YSocket() {
    if (sockfd != -1)
        ::close(sockfd);
    if (registered) {
        registered = false;
        app->unregisterSocket(this);
    }
}

int YSocket::connect(struct sockaddr *server_addr, int addrlen) {
    int fd;

    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1)
        return -errno;

    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        ::close(fd);
        return -errno;
    }

    if (fcntl(fd, F_SETFD, 1) == -1) {
        ::close(fd);
        return -errno;
    }

    MSG(("connecting."));
    if (::connect(fd, server_addr, addrlen) == -1) {
        if (errno == EINPROGRESS) {
            MSG(("in progress"));
            connecting = true;
            sockfd = fd;
            if (!registered) {
                registered = true;
                app->registerSocket(this);
            }
            return 0;
        }
        MSG(("error"));
        ::close(fd);
        return -errno;
    }
    sockfd = fd;

    if (fListener)
        fListener->socketConnected();

    return 0;
}

int YSocket::close() {
    if (registered) {
        registered = false;
        app->unregisterSocket(this);
    }
    if (sockfd == -1)
        return -EINVAL;
    if (::close(sockfd) == -1)
        return -errno;
    sockfd = -1;
    return 0;
}

int YSocket::read(char *buf, int len) {
#if 0
    int rc;

    rc = ::read(sockfd, (void *)buf, len);

    if (rc == 0) {
        if (fListener)
            fListener->socketError(0);
        return 0;
    } else if (rc == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
#endif
            rdbuf = buf;
            rdbuflen = len;
            reading = true;
            if (!registered) {
                registered = true;
                app->registerSocket(this);
            }
#if 0
        } else {
            return -errno;
        }
    }
    if (fListener)
        fListener->socketDataRead(buf, rc);
    return rc;
#endif
#if 1
    return 0;
#endif
}

int YSocket::write(const char *buf, int len) {
    int rc;

    // must loop !!!

    rc = ::write(sockfd, (const void *)buf, len);
    if (rc == -1)
        return -errno;
    else
        return rc;
}

void YSocket::can_read() {
    reading = false;
    if (registered) {
        registered = false;
        app->unregisterSocket(this);
    }
    int rc;

    rc = ::read(sockfd, rdbuf, rdbuflen);

    if (rc == 0) {
        if (fListener)
            fListener->socketError(0);
        return ;
    } else if (rc == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            reading = true;
            if (!registered) {
                registered = true;
                app->registerSocket(this);
            }
        } else {
            if (fListener)
                fListener->socketError(-errno);
        }
        return ;
    }
    if (fListener)
        fListener->socketDataRead(rdbuf, rc);
}

void YSocket::connected() {
    int err = 0;
    char x[1];
    connecting = false;
    MSG(("connected"));
    if (registered) {
        registered = false;
        app->unregisterSocket(this);
    }
    if (::recv(sockfd, x, 0, 0) == -1) {
        MSG(("after connect check"));
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
        } else if (errno == ENOTCONN) {
            connecting = true;
            if (!registered) {
                registered = true;
                app->registerSocket(this);
            }
            return ;
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
