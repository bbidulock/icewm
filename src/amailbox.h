#ifndef __MAILBOX_H
#define __MAILBOX_H

#ifdef CONFIG_APPLET_MAILBOX

#include "ywindow.h"
#include "ytimer.h"
#include "ysocket.h"
#include "yurl.h"

#include <sys/types.h>
#ifdef __FreeBSD__
#include <db.h>
#endif
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
        LOCALFILE,
        POP3,
        IMAP
    } protocol;

    MailCheck(MailBoxStatus *mbx);
    virtual ~MailCheck();

    void setURL(ustring url);
    void startCheck();

    virtual void socketConnected();
    virtual void socketError(int err);
    virtual void socketDataRead(char *buf, int len);
    void error();

private:
    YSocket sk;
    char bf[512];
    unsigned int got;
    ref<YURL> fURL;
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

class MailBoxStatus: public YWindow, public YTimerListener {
public:
    enum MailBoxState {
        mbxNoMail,
        mbxHasMail,
        mbxHasUnreadMail,
        mbxHasNewMail,
        mbxError
    };
    
    MailBoxStatus(mstring mailBox, YWindow *aParent = 0);
    virtual ~MailBoxStatus();

    virtual void paint(Graphics &g, const YRect &r);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleCrossing(const XCrossingEvent &crossing);

    void checkMail();
    void mailChecked(MailBoxState mst, long count);
    void newMailArrived();
    
    virtual bool handleTimer(YTimer *t);
private:
    mstring fMailBox;
    MailBoxState fState;
    MailCheck check;
    YTimer *fMailboxCheckTimer;
};
#endif

// !!! remove this
#ifdef CONFIG_APPLET_MAILBOX
extern ref<YPixmap> noMailPixmap;
extern ref<YPixmap> errMailPixmap;
extern ref<YPixmap> mailPixmap;
extern ref<YPixmap> unreadMailPixmap;
extern ref<YPixmap> newMailPixmap;
#endif

#endif
