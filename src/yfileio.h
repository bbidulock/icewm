#ifndef SRC_YFILEIO_H_
#define SRC_YFILEIO_H_

#include "ypointer.h"

class filereader {
public:
    filereader(int fd) : nFd(fd) { }
    filereader(const char* filename);
    filereader(filereader&& rv) : nFd(rv.nFd) { rv.nFd = -1; }
    ~filereader();

    int read_all(char* buf, size_t buflen);
    fcsmart read_pipe(long timeout = -1L, bool* timedOut = nullptr);
    fcsmart read_size(size_t size);
    static fcsmart read_path(const char* filename);

private:
    int nFd;

    fcsmart read_loop();
    filereader(const filereader&) = delete;
};

#endif /* SRC_YFILEIO_H_ */
