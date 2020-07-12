#include "config.h"
#include "ypipereader.h"
#include "yapp.h"

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

YPipeReader::YPipeReader() :
    YPollBase(),
    fListener(nullptr),
    rdbuf(nullptr),
    rdbuflen(0),
    reading(false)
{
}


YPipeReader::~YPipeReader() {
    pipeClose();
}

void YPipeReader::pipeClose() {
    closePoll();
}

int YPipeReader::spawnvp(const char *prog, char **args) {
    int fds[2], rc;

    if (pipe(fds) == -1)
        return -1;

    if ((rc = fork()) == -1) {
        close(fds[0]);
        close(fds[1]);
        return -1;
    } else if (rc == 0) { // child
        close(fds[0]);
        int devnull = open("/dev/null", O_RDONLY);
        if (devnull > 0) {
            dup2(devnull, 0);
            close(devnull);
        }
        if (fds[1] != 1) {
            dup2(fds[1], 1);
            close(fds[1]);
        }
        execvp(prog, args);
        _exit(99);
    } else { // parent
        close(fds[1]);
        registerPoll(fds[0]);
    }
    return 0;
}

int YPipeReader::read(char *buf, int len) {
    rdbuf = buf;
    rdbuflen = len;
    reading = true;
    registerPoll(fd());
    return 0;
}

void YPipeReader::notifyRead() {
    if (reading) {
        reading = false;
        unregisterPoll();
        int rc;

        rc = ::read(fd(), rdbuf, rdbuflen);

        if (rc == 0) {
            if (fListener)
                fListener->pipeError(0);
            return ;
        } else if (rc == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                reading = true;
                registerPoll(fd());
            } else {
                if (fListener)
                    fListener->pipeError(-errno);
            }
            return ;
        }
        if (fListener)
            fListener->pipeDataRead(rdbuf, rc);
    }
}

// vim: set sw=4 ts=4 et:
