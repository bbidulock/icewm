#ifndef __MAILBOX_H
#define __MAILBOX_H

#ifdef CONFIG_APPLET_MAILBOX

#include "ywindow.h"
#include "ytimer.h"
#include "ysocket.h"
#include "yurl.h"

class MailBoxStatus;

class MailCheck:
public YSocket::Listener {
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
    YURL fURL;
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

    void countMessages();
};

class MailBoxStatus:
public YWindow,
public YTimer::Listener {
public:
    enum MailBoxState {
        mbxNoMail,
        mbxHasMail,
        mbxHasUnreadMail,
        mbxHasNewMail,
        mbxError
    };
    
    MailBoxStatus(const char *mailBox, YWindow *aParent = 0);
    virtual ~MailBoxStatus();

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleCrossing(const XCrossingEvent &crossing);

    void checkMail();
    void mailChecked(MailBoxState mst, long count);
    void newMailArrived();
    
    virtual bool handleTimer(YTimer *t);
private:
    char *fMailBox;
    MailBoxState fState;
    MailCheck check;
    YTimer *fMailboxCheckTimer;
};
#endif

// !!! remove this
#ifdef CONFIG_APPLET_MAILBOX
extern YPixmap *noMailPixmap;
extern YPixmap *errMailPixmap;
extern YPixmap *mailPixmap;
extern YPixmap *unreadMailPixmap;
extern YPixmap *newMailPixmap;
#endif

#endif
