#include "config.h"
#include "ypipereader.h"
#include "yapp.h"

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

YPipeReader::YPipeReader() {
    fListener = 0;
    rdbuf = 0;
    rdbuflen = 0;
    reading = false;
    registered = false;
}


YPipeReader::~YPipeReader() {
    if (registered) {
        app->unregisterPoll(this);
        registered = false;
    }
    if (fd != -1)
        close(fd);
    fd = -1;
}

int YPipeReader::spawnvp(const char *prog, char **args) {
    int fds[2], rc;

    if (pipe(fds) == -1)
        return -1;

    if ((rc = fork()) == -1) {
        return -1;
    } else if (rc == 0) { // child
        close(fds[0]);
        dup2(fds[1], 1);
        int devnull = open("/dev/null", O_RDONLY);
        if (devnull != -1) {
            dup2(devnull, 0);
            close(devnull);
        }
        execvp(prog, args);
        _exit(99);
    } else { // parent
        close(fds[1]);
        fd = fds[0];
    }
    return 0;
}

int YPipeReader::read(char *buf, int len) {
    rdbuf = buf;
    rdbuflen = len;
    reading = true;
    if (!registered) {
        registered = true;
        app->registerPoll(this);
    }
    return 0;
}

void YPipeReader::notifyRead() {
    if (reading) {
        reading = false;
        if (registered) {
            registered = false;
            app->unregisterPoll(this);
        }
        int rc;

        rc = ::read(fd, rdbuf, rdbuflen);

        if (rc == 0) {
            if (fListener)
                fListener->pipeError(0);
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
                    fListener->pipeError(-errno);
            }
            return ;
        }
        if (fListener)
            fListener->pipeDataRead(rdbuf, rc);
    }
}
bool YPipeReader::forRead() {
    if (reading)
        return true;
    else
        return false;
}

void YPipeReader::notifyWrite() {
}

bool YPipeReader::forWrite() {
    return false;
}
