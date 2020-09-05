#ifndef SRC_YFILEIO_H_
#define SRC_YFILEIO_H_

#include "ypointer.h"

class filereader {
public:
    filereader(int fd, bool closes = true) :
        nFd(fd),
        bCloses(closes)
    {
    }

    filereader(const char* filename);

    filereader(filereader&& rv) :
        nFd(rv.nFd),
        bCloses(rv.bCloses)
    {
        rv.nFd = -1;
    }

    ~filereader();

    int read_all(char* buf, size_t buflen);
    fcsmart read_all();
    fcsmart read_pipe(long timeout, bool* timedOut);

private:
    int nFd;
    bool bCloses;

    filereader(const filereader&) = delete;
};

#endif /* SRC_YFILEIO_H_ */
