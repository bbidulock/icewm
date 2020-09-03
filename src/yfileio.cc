#include "yfileio.h"
#include "ytime.h"
#include "sysdep.h"
#include "base.h"
#include "debug.h"

/* read from filename and zero terminate the buffer. */
filereader::filereader(const char* filename) :
        bCloses(true)
{
    nFd = open(filename, O_RDONLY | O_TEXT);
}

filereader::~filereader() {
    if (nFd != -1 && bCloses)
        close(nFd);
}

int filereader::read_all(char* buf, size_t buflen) {
    char* ptr = buf;
    ssize_t got = 0, len = ssize_t(buflen) - 1;
    while (len > 0) {
        if ((got = read(nFd, ptr, size_t(len))) > 0) {
            ptr += got;
            len -= got;
        } else if (got != -1 || errno != EINTR)
            break;
    }
    *ptr = 0;
    return (ptr > buf) ? int(ptr - buf) : int(got);
}

fcsmart filereader::read_all() {
    struct stat st;
    if (fstat(nFd, &st) == 0) {
        if (S_ISREG(st.st_mode)) {
            if (st.st_size > 0) {
                fcsmart buf(fcsmart::create(st.st_size + 1));
                if (buf && read_all(buf, st.st_size + 1) == st.st_size) {
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
    fcsmart nothing;
    if (fcntl(nFd, F_SETFL, 0 <= timeout ? O_NONBLOCK : 0) == -1) {
        return nothing;
    }
    if (expired) {
        *expired = false;
    }
    timeval start(monotime());
    size_t offset = 0;
    size_t bufsiz = 2047;
    fcsmart ret(fcsmart::create(bufsiz + 1));
    while (ret) {
        ssize_t len = read(nFd, &ret[offset], bufsiz - offset);
        if (len == -1) {
            if ((errno == EAGAIN || errno == EINTR) && timeout >= 0) {
                timeval elapsed(monotime() - start);
                timeval remains(millitime(timeout) - elapsed);
                if (remains <= zerotime()) {
                    if (expired) {
                        *expired = true;
                    }
                    if (offset > 0) {
                        ret[offset] = '\0';
                        return ret;
                    } else {
                        return nothing;
                    }
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
                    if (offset > 0) {
                        ret[offset] = '\0';
                        return ret;
                    } else {
                        return nothing;
                    }
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
        else if (len == 0) {
            if (offset > 0) {
                ret[offset] = '\0';
                return ret;
            }
            else {
                return nothing;
            }
        }
        else if (0 < len) {
            offset += len;
            ret[offset] = '\0';
            if (bufsiz - offset < 512) {
                bufsiz += (bufsiz + 1) / 2;
                ret.resize(bufsiz + 1);
                if (ret == nullptr) {
                    return nothing;
                }
            }
        }
    }
    return ret;
}

