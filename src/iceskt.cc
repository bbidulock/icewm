#include "config.h"
#include "yapp.h"
#include "ysocket.h"
#include "debug.h"

#include <stdio.h>
#include <string.h>

class SockTest: public YSocketListener {
public:
    SockTest() {
        sk.setListener(this);

        sockaddr_in in;

        in.sin_family = AF_INET;
        in.sin_port = htons(8080);
        in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        sk.connect((struct sockaddr *) &in, sizeof(in));
    }
    virtual ~SockTest() {
        sk.close();
    }

    virtual void socketConnected() {
        MSG("Connected\n");

        const char *s = "GET / HTTP/1.0\r\n\r\n";

        sk.write((unsigned char *)s, strlen(s));
        MSG("Written");
        sk.read(bf, sizeof(bf));
    }

    virtual void socketError(int err) {
        if (err) warn(_("Socket error: %d"), err);
        else MSG("EOF\n");
        app->exit(err ? 1 : 0);
    }

    virtual void socketDataRead(unsigned char *buf, int len) {
        msg("read %d\n", len);
        if (len > 0) {
            //write(1, buf, len);
            sk.read(bf, sizeof(bf));
        }
    }
private:
    YSocket sk;
    unsigned char bf[4096];
};

int main(int argc, char **argv) {

#ifdef ENABLE_NLS
    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);
#endif

    YApplication app(&argc, &argv);

    SockTest sk;
    return app.mainLoop();
}
