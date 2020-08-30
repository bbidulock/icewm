#include "yfileio.h"
#include "ytime.h"
#include "sysdep.h"
#include "base.h"
#include "debug.h"

void set_nb(int fd) {
#ifndef F_GETFL
#error fcntl flags not available on this system
#else
    int flags = fcntl(fd, F_GETFL);
    if(flags == -1)
        return;
    flags |= O_NONBLOCK;
    flags = fcntl(fd, F_SETFL, flags);
#endif
}

/* read from file descriptor and zero terminate buffer. */
static int read_fd(int fd, char *buf, size_t buflen,
        const timeval *pExpTime,
        bool *bTimedOut) {
    if (fd == -1 || !buf || !buflen)
        return -1;
    auto ptr = buf;
    auto len = ssize_t(buflen) - 1;
    while (len > 0) {
        if (pExpTime) {
            auto timeout = *pExpTime - monotime();
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(fd, &rfds);
            errno = 0;
            auto nRes = select(fd + 1, &rfds, nullptr, nullptr, &timeout);
            switch (nRes) {
            case 0:
                if (bTimedOut)
                    *bTimedOut = true;
                return -1;
            case 1:
                if (!FD_ISSET(fd, &rfds))
                    return -1;
                break;
            default:
                MSG(("select failed, errno: %d.", errno));
                return -1;
            }
        }
        auto got = read(fd, ptr, (size_t) len);
        if (got > 0) {
            auto shortRead = got < len;
            ptr += got;
            len -= got;
            // got not all but is not pending read either?
            if (shortRead && errno != EINTR && errno != EAGAIN)
                break;

        } else if (got != -1 || errno != EINTR)
            break;
    }
    *ptr = 0;
    return int(ptr - buf);
}

/* read from filename and zero terminate the buffer. */
filereader::filereader(const char *filename) :
        bFinClose(true) {
    nFd = open(filename, O_RDONLY | O_TEXT);
}
filereader::~filereader() {
    if (nFd != -1 && bFinClose)
        close (nFd);
}

int filereader::read_all(char *buf, size_t buflen) {
    return nFd == -1 ? -1 : read_fd(nFd, buf, buflen, nullptr, nullptr);
}

/* read all of filedescriptor and return a zero-terminated new[] string. */
fcsmart filereader::read_all(bool assumeRegular, int timeoutMS,
        bool *bTimedOut) {

    timeval expTime;
    timeval *pExpTime = nullptr;
    if (timeoutMS >= 0) {
        set_nb (nFd);
        expTime = monotime() + millitime(timeoutMS);
        pExpTime = &expTime;
    }
    struct stat st;
    if (assumeRegular && (fstat(nFd, &st) == 0) && S_ISREG(st.st_mode)
            && (st.st_size > 0)) {
        auto ret = fcsmart::create(st.st_size + 1);
        if (!ret)
            return ret;
        int len = read_fd(nFd, ret, st.st_size + 1, pExpTime, bTimedOut);
        if (len != st.st_size)
            ret.release();
        return ret;
    }
    size_t offset = 0;
    size_t bufsiz = 1024;
    auto ret = fcsmart::create(bufsiz + 1);
    while (ret) {
        int len = read_fd(nFd, ret.data() + offset, bufsiz + 1 - offset,
                pExpTime, bTimedOut);
        if (len <= 0 || ((offset + len) < bufsiz)) {
            if (len < 0 && offset == 0) {
                ret.release();
            }
            break;
        } else {
            size_t tmpsiz = 2 * bufsiz;
            auto old = ret.release();
            auto tmp = (char*) realloc(old, tmpsiz + 1);
            if (!tmp) {
                free(old);
                break;
            }
            offset = bufsiz;
            bufsiz = tmpsiz;
            ret = tmp;
        }
    }
    return ret;
}

file_raii::~file_raii() {
    if (m_fd != -1) {
        ::close (m_fd);
    }
}

bool filereader::make_pipe(int fds[2]) {
    return 0 == pipe(fds);
}
