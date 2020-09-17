#include "config.h"
#include "yfileio.h"
#include "ytime.h"
#include "base.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

/* read from filename and zero terminate the buffer. */
filereader::filereader(const char* filename) :
    bCloses(true)
{
    nFd = open(filename, O_RDONLY);
}

filereader::~filereader() {
    if (nFd != -1 && bCloses)
        close(nFd);
}

int filereader::read_all(char* buf, size_t buflen) {
    char* ptr = buf;
    ssize_t got = 0, len = ssize_t(buflen) - 1;
    while (nFd >= 0 && len > 0) {
        if ((got = read(nFd, ptr, size_t(len))) > 0) {
            ptr += got;
            len -= got;
        } else if (got != -1 || errno != EINTR)
            break;
    }
    *ptr = 0;
    return (ptr > buf) ? int(ptr - buf) : (nFd < 0) ? -1 : int(got);
}

fcsmart filereader::read_all() {
    struct stat st;
    if (nFd == -1) {
    }
    else if (fstat(nFd, &st) == 0) {
        if (S_ISREG(st.st_mode)) {
            if (st.st_size > 0) {
                fcsmart buf(fcsmart::create(st.st_size + 1));
                if (buf && read_all(buf, st.st_size + 1) == st.st_size) {
                    return buf;
                }
            }
            else {
                int got = 0;
                fcsmart buf;
                size_t size = BUFSIZ + 1;
                size_t len = 0;
                buf.resize(size + 1);
                while (buf && (got = read_all(buf + len, size - len)) > 0) {
                    len += got;
                    if (len + 1 < size) {
                        break;
                    } else {
                        size += size / 2;
                        buf.resize(size + 1);
                    }
                }
                if (len > 0 && buf) {
                    buf.resize(len + 1);
                    return buf;
                }
            }
        }
        else if (S_ISFIFO(st.st_mode) || S_ISSOCK(st.st_mode)) {
            int timeout = -1;
            bool expired = false;
            return read_pipe(timeout, &expired);
        }
    }
    else {
        fail("fstat");
    }
    return fcsmart();
}

fcsmart filereader::read_pipe(long timeout, bool* expired) {
    if (expired) {
        *expired = false;
    }
    fcsmart nothing;
    if (nFd < 0 || fcntl(nFd, F_SETFL, 0 <= timeout ? O_NONBLOCK : 0) == -1) {
        return nothing;
    }
    long pipe_size = clamp<long>(fpathconf(nFd, _PC_PIPE_BUF),
                                 _POSIX_PIPE_BUF, BUFSIZ);
    size_t offset = 0;
    size_t bufsiz = size_t(pipe_size);
    fcsmart ret(fcsmart::create(bufsiz + 1));
    timeval start(monotime());
    while (ret) {
        ssize_t len = read(nFd, &ret[offset], bufsiz - offset);
        if (len == 0) {
            break;
        }
        else if (0 < len) {
            offset += len;
            ret[offset] = '\0';
            if (bufsiz < offset + 512) {
                bufsiz += pipe_size;
                ret.resize(bufsiz + 1);
            }
        }
        else if (len == -1) {
            if ((errno == EAGAIN || errno == EINTR) && timeout >= 0) {
                timeval elapsed(monotime() - start);
                timeval remains(millitime(timeout) - elapsed);
                if (remains <= zerotime()) {
                    if (expired) {
                        *expired = true;
                    }
                    break;
                }
                fd_set rfds;
                FD_ZERO(&rfds);
                FD_SET(nFd, &rfds);
                errno = 0;
                int nRes = select(nFd + 1, &rfds, nullptr, nullptr, &remains);
                if (nRes == 0) {
                    if (expired) {
                        *expired = true;
                    }
                    break;
                }
                else if (nRes == 1) {
                    continue;
                }
                else if (nRes == -1 && errno == EINTR) {
                    continue;
                }
                else {
                    return nothing;
                }
            }
            else if (errno == EINTR) {
                continue;
            }
            else {
                return nothing;
            }
        }
    }
    if (ret && offset) {
        ret[offset] = '\0';
        return ret;
    } else {
        return nothing;
    }
}

