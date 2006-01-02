#include "config.h"

#include "yworker.h"
#include "yapp.h"

#include "sysdep.h"

YWorkerProcess::YWorkerProcess() {
    body_buffer = 0;
    body_read = 0;
    body_len = 0;
}

int YWorkerProcess::start(const char *command, char **args) {
    int worker_fd;
    if (worker.socketpair(&worker_fd) != 0)
        return -1;
    worker_pid = app->startWorker(worker_fd, command, args);
    ::close(worker_fd);
    if (worker_pid == -1)
        return -1;
    int rc = worker.read(head_buffer, sizeof(head_buffer));
    if (rc != 0)
        return -1;
    readState = rsWAIT;
    worker.setListener(this);
    msg("started read %d", rc);
    return 0;
}

void YWorkerProcess::socketConnected() {
    msg("connected");
}

void YWorkerProcess::socketError(int err) {
    msg("socket error: %d", err);
    content_read(0, 0);
}

void YWorkerProcess::socketDataRead(char *buf, int len) {
    msg("socket data read: %d", len);
    if (readState == rsWAIT) {
        readState = rsHEAD;

        const char *content_length = "Content-Length: ";
        int cl_len = strlen(content_length);

        if (len < cl_len ||
            strncmp(buf, content_length, cl_len) != 0)
        {
            readState = rsERROR;
            return;
        }

        buf += cl_len;
        len -= cl_len;


        char *eol = (char *)memchr(buf, '\n', len);
        if (eol == NULL) {
            readState = rsERROR;
            return;
        }

        *eol = 0;
        int length = atoi(buf);

        msg("content length: %d", length);
        len -= eol - buf + 1;
        buf = eol + 1;

        if (len < 1 || buf[0] != '\n' || length < 0) {
            readState = rsERROR;
            return;
        }

        buf++;
        len--;

        if (len > length) {
            readState = rsERROR;
            return;
        }

        if (length == 0) {
            content_read(0, 0);
            readState = rsWAIT;
            return;
        }

        if (body_buffer != NULL) {
            free(body_buffer);
            body_buffer = NULL;
        }

        body_len = length;
        body_buffer = (char *)malloc(body_len);
        if (body_buffer == NULL) {
            readState = rsERROR;
            return;
        }
        body_read = len;
        memcpy(body_buffer, buf, len);

        if (body_len == body_read) {
            content_read(body_buffer, body_len);
            readState = rsWAIT;
        } else {
            worker.read(body_buffer + body_read, body_len - body_read);
            readState = rsBODY;
        }
    } else if (readState == rsBODY){
        body_read += len;

        if (body_len == body_read) {
            content_read(body_buffer, body_len);
            readState = rsWAIT;
            free(body_buffer);
        } else {
            worker.read(body_buffer + body_read, body_len - body_read);
        }
    } else {
        if (body_buffer != NULL) {
            free(body_buffer);
            body_buffer = NULL;
        }
        readState = rsERROR;
    }
}

void YWorkerProcess::content_read(char *buffer, int length) {
    if (fListener)
        fListener->handleMessage(this, buffer, length);
}
