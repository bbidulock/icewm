#include "config.h"
#include "yapp.h"
#include "ysocket.h"
#include "base.h"

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>

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
        puts("Connected");

        const char *s = "GET / HTTP/1.0\r\n\r\n";

        sk.write(s, strlen(s));
        puts("Written");
        sk.read(bf, sizeof(bf));
    }

    virtual void socketError(int err) {
        if (err) printf("error: %d\n", err);
        else puts("EOF");
        app->exit(err ? 1 : 0);
    }

    virtual void socketDataRead(char * /*buf*/, int len) {
        printf("read %d\n", len);
        if (len > 0) {
            //write(1, buf, len);
            sk.read(bf, sizeof(bf));
        }
    }
private:
    YSocket sk;
    char bf[4096];
};

int main(int argc, char **argv) {
    YApplication app("iceskt", &argc, &argv);

    SockTest sk;
    return app.mainLoop();
}
