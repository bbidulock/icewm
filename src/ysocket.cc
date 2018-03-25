/*
 * IceWM
 *
 * Copyright (C) 2002 Marko Macek
 */
#include "config.h"
#include "ysocket.h"
#include "yapp.h"

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

static const int sockStreamFlags = SOCK_STREAM
#if defined(__linux__) && defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
                                 | SOCK_NONBLOCK | SOCK_CLOEXEC
#endif
                                 ;

YSocket::YSocket() {
    fListener = 0;
    rdbuf = 0;
    rdbuflen = 0;
    connecting = false;
    reading = false;
    registered = false;
}

YSocket::~YSocket() {
    close();
}

int YSocket::socket() const {
    return fd();
}

int YSocket::connect(struct sockaddr *server_addr, int addrlen) {
    close();

    int domain = server_addr->sa_family;
    if (domain != AF_INET && domain != AF_INET6) {
        errno = EAFNOSUPPORT;
        return -1;
    }

    int fd = ::socket(domain, sockStreamFlags, 0);
    if (fd == -1)
        return -1;

    if (sockStreamFlags == SOCK_STREAM) {
        if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1 ||
            fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
            ::close(fd);
            return -1;
        }
    }

    MSG(("connecting."));
    if (::connect(fd, server_addr, addrlen) == -1) {
        if (errno == EINPROGRESS) {
            MSG(("in progress"));
            connecting = true;
            fFd = fd;
            if (!registered) {
                registered = true;
                mainLoop->registerPoll(this);
            }
            return 0;
        }
        MSG(("error"));
        ::close(fd);
        return -1;
    }
    fFd = fd;

    if (fListener)
        fListener->socketConnected();

    return 0;
}

int YSocket::socketpair(int *otherfd) {
    close();
    *otherfd = -1;

    int fds[2] = { 0, 0 };
    int rc = ::socketpair(AF_UNIX, sockStreamFlags, PF_UNIX, fds);
    if (rc >= 0) {
        if (sockStreamFlags == SOCK_STREAM) {
            if (fcntl(fds[0], F_SETFL, O_NONBLOCK) == -1 ||
                fcntl(fds[0], F_SETFD, FD_CLOEXEC) == -1) {
                ::close(fds[0]);
                ::close(fds[1]);
                return -1;
            }
        }

        fFd = fds[0];
        *otherfd = fds[1];

        registered = true;
        mainLoop->registerPoll(this);
    }
    return rc;
}

void YSocket::close() {
    if (registered) {
        registered = false;
        mainLoop->unregisterPoll(this);
    }
    if (fFd != -1) {
        ::close(fFd);
        fFd = -1;
    }
}

int YSocket::read(char *buf, int len) {
    rdbuf = buf;
    rdbuflen = len;
    reading = true;
    if (!registered) {
        registered = true;
        mainLoop->registerPoll(this);
    }
    return 0;
}

int YSocket::write(const char *buf, int len) {
    do {
        int rc = ::write(fFd, (const void *)buf, len);
        if (rc >= 0)
            return rc;
    } while (errno == EINTR);
    return -1;
}

int YSocket::write(const cstring& str) {
    return write(str, str.length());
}

void YSocket::shutdown() {
    if (fd() > 0)
        ::shutdown(fd(), SHUT_RDWR);
}

void YSocket::notifyRead() {
    if (reading) {
        reading = false;
        if (registered) {
            registered = false;
            mainLoop->unregisterPoll(this);
        }
        int rc = ::read(fFd, rdbuf, rdbuflen);
        if (rc == 0) {
            if (fListener)
                fListener->socketError(0);
        }
        else if (rc == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                reading = true;
                if (!registered) {
                    registered = true;
                    mainLoop->registerPoll(this);
                }
            } else {
                if (fListener)
                    fListener->socketError(errno);
            }
        }
        else {
            if (fListener)
                fListener->socketDataRead(rdbuf, rc);
        }
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
            mainLoop->unregisterPoll(this);
        }
        if (::recv(fFd, x, 0, 0) == -1) { ///!!!
            MSG(("after connect check"));
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
            } else if (errno == ENOTCONN) {
                connecting = true;
                if (!registered) {
                    registered = true;
                    mainLoop->registerPoll(this);
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
    return reading;
}

bool YSocket::forWrite() {
    return connecting;
}

// vim: set sw=4 ts=4 et:
