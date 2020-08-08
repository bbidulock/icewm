#include "yfileio.h"
#include "sysdep.h"
#include <stdio.h>

/* read from file descriptor and zero terminate buffer. */
int filereader::read_fd(int fd, char *buf, size_t buflen) {
    if (fd == -1 || !buf || !buflen)
        return -1;
    auto ptr = buf;
    ssize_t got = 0, len = (ssize_t) (buflen - 1);
    while (len > 0) {
        if ((got = read(fd, ptr, (size_t) len)) > 0) {
            ptr += got;
            len -= got;
        } else if (got != -1 || errno != EINTR)
            break;
    }
    *ptr = 0;
    return (ptr > buf) ? (int) (ptr - buf) : (int) got;
}

/* read from filename and zero terminate the buffer. */
filereader::filereader(const char *filename) : m_closeAfter(true) {
    m_fd = open(filename, O_RDONLY | O_TEXT);
}
filereader::~filereader() {
    if (m_fd != -1 && m_closeAfter)
        close(m_fd);
}

/* read from filename and zero terminate the buffer. */
int filereader::read_all(char *buf, size_t buflen) {
    return m_fd == -1 ? -1 : read_fd(m_fd, buf, buflen);
}


/* read all of filedescriptor and return a zero-terminated new[] string. */
fcsmart filereader::read_all() {
    struct stat st;
    fcsmart ret(nullptr);
    if (fstat(m_fd, &st) == -1)
        return ret;
    if (S_ISREG(st.st_mode) && st.st_size > 0) {
        ret = (char*) malloc(st.st_size + 1);
        if (!ret)
            return ret;
        int len = read_fd(m_fd, ret, st.st_size + 1);
        if (len != st.st_size)
            ret.release();
    } else {
        size_t offset = 0;
        size_t bufsiz = 1024;
        ret = fcsmart::create(bufsiz + 1);
        if (!ret)
            return ret;
        while (true) {
            int len = read_fd(m_fd, ret.data() + offset, bufsiz + 1 - offset);
            if (len <= 0 || offset + len < bufsiz) {
                if (len < 0 && offset == 0)
                    ret.release();
                break;
            } else {
                size_t tmpsiz = 2 * bufsiz;
                auto old = ret.release();
                auto tmp = (char*) realloc(old, 2 * bufsiz);
                if (tmp) {
                    offset = bufsiz;
                    bufsiz = tmpsiz;
                    ret = tmp;
                } else {
                    free(old);
                    break;
                }
            }
        }
    }
    return ret;
}
