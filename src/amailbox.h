#ifndef __MAILBOX_H
#define __MAILBOX_H

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
class YSMListener;
class IApp;

class MailCheck: public YSocketListener {
public:
    enum ProtocolPort {
        POP3_PORT = 110,
        IMAP_PORT = 143,
        IMAP_SSL  = 993,
        POP3_SSL  = 995,
    };

    enum ProtocolState {
        IDLE,
        CONNECTING,
        WAIT_READY,
        WAIT_USER,
        WAIT_PASS,
        WAIT_STAT,
        WAIT_UNSEEN,
        WAIT_QUIT,
        ERROR,
        SUCCESS
    } state;

    enum ProtocolKind {
        NOPROTOCOL,
        LOCALFILE,
        POP3,
        IMAP
    } protocol;

    MailCheck(MailBoxStatus *mbx);
    virtual ~MailCheck();

    void setURL(ustring url);
    void startCheck();
    void startSSL();
    cstring inbox();
    int portNumber();
    void setState(ProtocolState newState);

    virtual void socketConnected();
    virtual void socketError(int err);
    virtual void socketDataRead(char *buf, int len);

    void parsePop3();
    void parseImap();

    int write(const char* buf, int len = 0);
    int write(const cstring& str);
    void error();
    void release();
    bool ssl() const;
    bool net() const;

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
    sockaddr_in fAddr;
    int fPort;
    int fPid;
    bool fTrace;

    void countMessages();
    const char* s(ProtocolState t);
    void escape(const char* buf, int len, char* tmp, int siz);
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

    MailBoxStatus(IApp *app, YSMListener *smActionListener, mstring mailBox, YWindow *aParent = 0);
    virtual ~MailBoxStatus();

    virtual void paint(Graphics &g, const YRect &r);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleCrossing(const XCrossingEvent &crossing);

    void checkMail();
    void mailChecked(MailBoxState mst, long count, long unread);
    void newMailArrived();

    virtual bool handleTimer(YTimer *t);
private:
    mstring fMailBox;
    MailBoxState fState;
    MailCheck check;
    lazy<YTimer> fMailboxCheckTimer;
    YSMListener *smActionListener;
    IApp *app;
};
#endif

extern ref<YPixmap> noMailPixmap;
extern ref<YPixmap> errMailPixmap;
extern ref<YPixmap> mailPixmap;
extern ref<YPixmap> unreadMailPixmap;
extern ref<YPixmap> newMailPixmap;

// vim: set sw=4 ts=4 et:
