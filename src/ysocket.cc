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
    if (fFd != -1) {
        ::close(fFd);
        fFd = -1;
    }
}

int YSocket::connect(struct sockaddr *server_addr, int addrlen) {
    int fd;

    if (this->fFd != -1) {
         ::close(this->fFd);
         this->fFd = -1;
    }
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
            this->fFd = fd;
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
    this->fFd = fd;

    if (fListener)
        fListener->socketConnected();

    return 0;
}

int YSocket::socketpair(int *otherfd) {
    int fds[2] = { 0, 0 };
    *otherfd = -1;
    if (registered) {
        registered = false;
        app->unregisterPoll(this);
    }
    if (this->fFd != -1) {
        ::close(this->fFd);
        this->fFd = -1;
    }
    int rc = ::socketpair(AF_UNIX, SOCK_STREAM, PF_UNIX, fds);
    if (rc != -1) {
        this->fFd = fds[0];
        *otherfd = fds[1];

        if (fcntl(this->fFd, F_SETFL, O_NONBLOCK) == -1) {
            ::close(this->fFd);
            ::close(*otherfd);
            return -errno;
        }

        if (fcntl(this->fFd, F_SETFD, FD_CLOEXEC) == -1) {
            ::close(this->fFd);
            ::close(*otherfd);
            return -errno;
        }

        registered = true;
        app->registerPoll(this);
    }
    return rc;
}

int YSocket::close() {
    if (registered) {
        registered = false;
        app->unregisterPoll(this);
    }
    if (fFd == -1)
        return -EINVAL;
    if (::close(fFd) == -1)
        return -errno;
    fFd = -1;
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

    rc = ::write(fFd, (const void *)buf, len);
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

        rc = ::read(fFd, rdbuf, rdbuflen);

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
        if (::recv(fFd, x, 0, 0) == -1) { ///!!!
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
