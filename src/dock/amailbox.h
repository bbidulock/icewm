#ifndef __MAILBOX_H
#define __MAILBOX_H

#include "ywindow.h"
#include "ytimer.h"
#include "ysocket.h"
#include "yconfig.h"

#include <netinet/in.h>

class MailBoxStatus;

class MailCheck: public YSocketListener {
public:
    enum {
        IDLE,
        CONNECTING,
        WAIT_READY,
        WAIT_USER,
        WAIT_PASS,
        WAIT_STAT,
        WAIT_QUIT,
        ERROR,
        SUCCESS
    } state;

    enum {
        FILE,
        POP3,
        IMAP
    } protocol;

    MailCheck(MailBoxStatus *mbx);
    virtual ~MailCheck();

    void setURL(const char *url);
    void startCheck();

    virtual void socketConnected();
    virtual void socketError(int err);
    virtual void socketDataRead(char *buf, int len);
    void error();

private:
    YSocket sk;
    char bf[512];
    unsigned int got;
    char *username;
    char *password;
    char *server;
    char *port;
    char *filename;
    char *fURL;
    MailBoxStatus *fMbx;
    long fLastSize;
    long fLastCount;
    long fLastUnseen;
    long fCurSize;
    long fCurCount;
    long fCurUnseen;
    long fLastCountSize;
    time_t fLastCountTime;
    sockaddr_in server_addr;

    int parse_pop3(char *src);
    void countMessages();
};

class MailBoxStatus: public YWindow, public YTimerListener {
public:
    enum MailBoxState {
        mbxNoMail,
        mbxHasMail,
        mbxHasUnreadMail,
        mbxHasNewMail,
        mbxError
    };
    
    MailBoxStatus(const char *mailBox, const char *mailCommand, YWindow *aParent = 0);
    virtual ~MailBoxStatus();

    virtual void paint(Graphics &g, const YRect &er);
    virtual bool eventClick(const YClickEvent &up);
    virtual bool eventCrossing(const YCrossingEvent &crossing);

    void checkMail();
    void mailChecked(MailBoxState mst, long count);
    void newMailArrived();
    
    virtual bool handleTimer(YTimer *t);
private:
    char *fMailBox;
    const char *fMailCommand;
    MailBoxState fState;
    MailCheck check;
    YTimer fMailboxCheckTimer;
    long fMailCheckDelay;
    char *fNewMailCommand;

    YPixmap *noMailPixmap;
    YPixmap *errMailPixmap;
    YPixmap *mailPixmap;
    YPixmap *unreadMailPixmap;
    YPixmap *newMailPixmap;

    static YColorPrefProperty gTaskBarBg;
    static YPixmapPrefProperty gPixmapMail;
    static YPixmapPrefProperty gPixmapNoMail;
    static YPixmapPrefProperty gPixmapErrMail;
    static YPixmapPrefProperty gPixmapUnreadMail;
    static YPixmapPrefProperty gPixmapNewMail;
};


#endif
