/*
 * IceWM
 *
 * Copyright (C) 2002 Marko Macek
 */
#include "config.h"
#include "base.h"
#include "ysocket.h"
#include "yapp.h"
#include "debug.h"

#include <sys/types.h>
#ifdef __FreeBSD__
#include <db.h>
#endif
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>

YSocket::YSocket() {
    fPrev = fNext = 0;
    fListener = 0;
    rdbuf = 0;
    rdbuflen = 0;
    connecting = false;
    reading = false;
    registered = false;
}

YSocket::~YSocket() {
    if (registered) {
        registered = false;
        app->unregisterPoll(this);
    }
    if (fd != -1) {
        ::close(fd);
        fd = -1;
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

    if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
        ::close(fd);
        return -errno;
    }

    MSG(("connecting."));
    if (::connect(fd, server_addr, addrlen) == -1) {
        if (errno == EINPROGRESS) {
            MSG(("in progress"));
            connecting = true;
            this->fd = fd;
            if (!registered) {
                registered = true;
                app->registerPoll(this);
            }
            return 0;
        }
        MSG(("error"));
        ::close(fd);
        return -errno;
    }
    this->fd = fd;

    if (fListener)
        fListener->socketConnected();

    return 0;
}

int YSocket::close() {
    if (registered) {
        registered = false;
        app->unregisterPoll(this);
    }
    if (fd == -1)
        return -EINVAL;
    if (::close(fd) == -1)
        return -errno;
    fd = -1;
    return 0;
}

int YSocket::read(char *buf, int len) {
    rdbuf = buf;
    rdbuflen = len;
    reading = true;
    if (!registered) {
        registered = true;
        app->registerPoll(this);
    }
    return 0;
}

int YSocket::write(const char *buf, int len) {
    int rc;

    // must loop !!!

    rc = ::write(fd, (const void *)buf, len);
    if (rc == -1)
        return -errno;
    else
        return rc;
}

void YSocket::notifyRead() {
    if (reading) {
        reading = false;
        if (registered) {
            registered = false;
            app->unregisterPoll(this);
        }
        int rc;

        rc = ::read(fd, rdbuf, rdbuflen);

        if (rc == 0) {
            if (fListener)
                fListener->socketError(0);
            return ;
        } else if (rc == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                reading = true;
                if (!registered) {
                    registered = true;
                    app->registerPoll(this);
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
}

void YSocket::notifyWrite() {
    if (connecting) {
        int err = 0;
        char x[1];
        connecting = false;
        MSG(("connected"));
        if (registered) {
            registered = false;
            app->unregisterPoll(this);
        }
        if (::recv(fd, x, 0, 0) == -1) { ///!!!
            MSG(("after connect check"));
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
            } else if (errno == ENOTCONN) {
                connecting = true;
                if (!registered) {
                    registered = true;
                    app->registerPoll(this);
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
}

bool YSocket::forRead() {
    if (reading)
        return true;
    else
        return false;
}

bool YSocket::forWrite() {
    if (connecting)
        return true;
    else
        return false;
}
