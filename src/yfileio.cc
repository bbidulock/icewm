#include "config.h"
#include "yfileio.h"
#include "ytime.h"
#include "base.h"
#include "sysdep.h"

filereader::filereader(const char* filename) :
    nFd(open(filename, O_RDONLY | O_NOCTTY))
{
}

filereader::~filereader() {
    if (nFd != -1)
        close(nFd);
}

int filereader::read_all(char* buf, size_t buflen) {
    ssize_t len = -1;
    if (nFd >= 0 && (len = read(nFd, buf, buflen - 1)) >= 0) {
        buf[len] = '\0';
    }
    return int(len);
}

fcsmart filereader::read_size(size_t size) {
    fcsmart buf(fcsmart::create(size + 1));
    if (buf && read_all(buf, size + 1) > 0) {
        return buf;
    } else {
        return fcsmart();
    }
}

fcsmart filereader::read_loop() {
    size_t len = 0, size = BUFSIZ;
    fcsmart buf(fcsmart::create(size + 1));
    while (buf) {
        ssize_t got = read(nFd, buf + len, size - len);
        if (got >= 0) {
            len += got;
            buf[len] = '\0';
            if (len + 1 < size) {
                break;
            } else {
                size += size / 2;
                buf.resize(size + 1);
            }
        }
        else if (errno != EINTR) {
            buf = nullptr;
            break;
        }
    }
    return buf;
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

fcsmart filereader::read_path(const char* filename) {
    struct stat st;
    if (::stat(filename, &st) == 0) {
        if (S_ISREG(st.st_mode)) {
            if (st.st_size > 0)
                return filereader(filename).read_size(st.st_size);
            else
                return filereader(filename).read_loop();
        }
        else if (S_ISFIFO(st.st_mode) || S_ISSOCK(st.st_mode))
            return filereader(filename).read_pipe();
    }
    return fcsmart();
}

