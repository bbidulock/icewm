/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 *
 * MailBox Status
 * !!! this should be external module (replacable for POP,IMAP,...)
 */
#include "config.h"
#include "amailbox.h"
#include "sysdep.h"
#include "prefs.h"
#include "wmapp.h"
#include "wpixmaps.h"
#include "udir.h"
#include <sys/socket.h>
#include <netdb.h>
#include "intl.h"

extern YColorName taskBarBg;

MailCheck::MailCheck(MailBoxStatus *mbx):
    state(IDLE),
    protocol(NOPROTOCOL),
    fMbx(mbx),
    fLastSize(-1),
    fLastCount(-1),
    fLastUnseen(-1),
    fCurSize(0),
    fCurCount(0),
    fCurUnseen(0),
    fLastCountSize(-1),
    fLastCountTime(0),
    fPort(0),
    fPid(0),
    fTrace(getenv("ICEWM_MAILCHECK_TRACE") != 0)
{
    sk.setListener(this);
}

MailCheck::~MailCheck() {
    release();
}

bool MailCheck::ssl() const {
    return fPort == IMAP_SSL || fPort == POP3_SSL;
}

bool MailCheck::net() const {
    return protocol == IMAP || protocol == POP3;
}

int MailCheck::portNumber() {
    if (fURL.port != null) {
        int port = atoi(fURL.port);
        if (port > 0)
            return port;
        servent* ser = getservbyname(fURL.port, "tcp");
        if (ser && ser->s_port > 0)
            return ser->s_port;
    }
    if (fURL.scheme == "pop3") return POP3_PORT;
    if (fURL.scheme == "pop3s") return POP3_SSL;
    if (fURL.scheme == "imap") return IMAP_PORT;
    if (fURL.scheme == "imaps") return IMAP_SSL;
    return 0;
}

void MailCheck::setState(ProtocolState newState) {
    if (state != ERROR || newState == IDLE || newState == CONNECTING) {
        if (fTrace) tlog(" state %s -> %s ", s(state), s(newState));
        state = newState;
    }
}

void MailCheck::setURL(ustring url) {
    fURL = url;

    protocol = NOPROTOCOL;
    if (fURL.scheme == "file")
        protocol = LOCALFILE;
    else if (fURL.scheme == "pop3" || fURL.scheme == "pop3s")
        protocol = POP3;
    else if (fURL.scheme == "imap" || fURL.scheme == "imaps")
        protocol = IMAP;
    else if (fURL.scheme != null)
        warn(_("Invalid mailbox protocol: \"%s\""), fURL.scheme.c_str());
    else
        warn(_("Invalid mailbox path: \"%s\""), cstring(url).c_str());

    if (net()) {
        setState(IDLE);
        memset(&fAddr, 0, sizeof(fAddr));

        fPort = portNumber();
        if (inrange(fPort, 1, USHRT_MAX)) {
            fAddr.sin_family = AF_INET;
            fAddr.sin_port = htons((unsigned short)fPort);
            if (fURL.host != null) { /// !!! fix, need nonblocking resolve
                hostent* host = gethostbyname(fURL.host);
                if (host)
                    memcpy(&fAddr.sin_addr, host->h_addr_list[0],
                           sizeof(fAddr.sin_addr));
                else {
                    warn(_("DNS name lookup failed for %s"), fURL.host.c_str());
                    setState(ERROR);
                }
            } else
                fAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        } else {
            warn(_("Invalid mailbox port: \"%s\""), fURL.port.c_str());
            setState(ERROR);
        }
    }
}

void MailCheck::countMessages() {
    int fd = open(fURL.path, O_RDONLY);
    long mails = 0;
    long mread = 0;

    if (fd != -1) {
        const int size = 4096;
        char buf[size + 16];
        int len;
        bool newl = true;
        bool head = true;
        bool seen = false;

        while ((len = read(fd, buf, size)) > 0) {
            for (char *ptr = buf, *end = buf + len; ptr < end; ++ptr) {
                if (newl) {
                    if (*ptr == 'F') {
                        if (*++ptr == 'r' &&
                            *++ptr == 'o' &&
                            *++ptr == 'm' &&
                            *++ptr == ' ')
                        {
                            head = true;
                            seen = false;
                            ++mails;
                        }
                    }
                    else if (head) {
                        if (*ptr == 'S' && seen == false) {
                            if (*++ptr == 't' &&
                                *++ptr == 'a' &&
                                *++ptr == 't' &&
                                *++ptr == 'u' &&
                                *++ptr == 's' &&
                                *++ptr == ':' &&
                                *++ptr == ' ' &&
                                *++ptr == 'R')
                            {
                                seen = true;
                                ++mread;
                            }
                        }
                        else if (*ptr == '\n') {
                            head = false;
                        }
                    }
                }
                newl = *ptr == '\n';
            }
        }
        close(fd);
    }
    fLastCount = mails;
    fLastUnseen = max(0L, mails - mread);
}

void MailCheck::startCheck() {
    if (state == ERROR)
        setState(IDLE);
    if (state != IDLE && state != SUCCESS)
        return ;

    if (protocol == LOCALFILE) {
        struct stat st;

        if (fURL.path == null)
            return;

        if (!countMailMessages) {
            fLastCount = -1;
            fLastUnseen = -1;
        }

        if (0 == ::stat(fURL.path, &st) && S_ISREG(st.st_mode)) {
            if (countMailMessages &&
                (st.st_size != fLastCountSize || st.st_mtime != fLastCountTime))
            {
                countMessages();
                fLastCountTime = st.st_mtime;
                fLastCountSize = fLastSize;
            }
            if (st.st_size == 0)
                fMbx->mailChecked(MailBoxStatus::mbxNoMail,
                                  fLastCount, fLastUnseen);
            else if (st.st_size > fLastSize && fLastSize != -1)
                fMbx->mailChecked(MailBoxStatus::mbxHasNewMail,
                                  fLastCount, fLastUnseen);
            else if (st.st_mtime > st.st_atime)
                fMbx->mailChecked(MailBoxStatus::mbxHasUnreadMail,
                                  fLastCount, fLastUnseen);
            else
                fMbx->mailChecked(MailBoxStatus::mbxHasMail,
                                  fLastCount, fLastUnseen);
            fLastSize = st.st_size;
        }
        else if (S_ISDIR(st.st_mode)) {
            fLastUnseen = 0;
            cdir dir(cstring(upath(fURL.path).child("new")));
            while (dir.next())
                fLastUnseen++;
            fLastCount = fLastUnseen;
            dir.open(cstring(upath(fURL.path).child("cur")));
            while (dir.next())
                fLastCount++;
            if (fLastCount < 1)
                fMbx->mailChecked(MailBoxStatus::mbxNoMail,
                                  fLastCount, fLastUnseen);
            else if (fLastUnseen > 0)
                fMbx->mailChecked(MailBoxStatus::mbxHasNewMail,
                                  fLastCount, fLastUnseen);
            else
                fMbx->mailChecked(MailBoxStatus::mbxHasMail,
                                  fLastCount, fLastUnseen);
        }
        else {
            fLastSize = 0;
            fLastCount = 0;
            fLastUnseen = 0;
            fMbx->mailChecked(MailBoxStatus::mbxNoMail,
                              fLastCount, fLastUnseen);
        }
    }
    else if (net()) {
        if (ssl()) {
            if (fTrace) tlog("starting SSL");
            startSSL();
        }
        else if (sk.connect((sockaddr *) &fAddr, sizeof(fAddr)) == 0) {
            if (fTrace) tlog("connected non-SSL");
            setState(CONNECTING);
            got = 0;
        } else {
            int e = errno;
            if (fTrace || testOnce(fURL.host, fPort))
                warn(_("Could not connect to %s: %s"),
                        fURL.host.c_str(), strerror(e));
            error();
        }
    }
    else if (state != ERROR) {
        error();
    }
}

void MailCheck::startSSL() {
    const char file[] = "openssl";
    upath path = findPath(getenv("PATH"), X_OK, file);
    if (path == null) {
        if (ONCE)
            warn(_("Failed to find %s command"), file);
        return;
    }

    int other;
    if (sk.socketpair(&other) == 0 && other > 0) {
        fflush(stderr);
        fflush(stdout);
        fPid = fork();
        if (fPid == -1) {
            close(other);
            sk.close();
        }
        else if (fPid == 0) {
            sk.close();
            dup2(other, 0);
            dup2(other, 1);
            if (other > 2)
                close(other);
            dup2(open("/dev/null", O_WRONLY), 2);

            char num[32];
            snprintf(num, sizeof num, "%d", fPort);
            cstring hostnamePort(mstring(fURL.host, ":", num));
            const char* args[] = {
                file, "s_client", "-quiet", "-no_ign_eof",
                "-connect", hostnamePort, 0
            };
            execv(cstring(path), (char* const*) args);
            fail(_("Failed to execute %s"), cstring(path).c_str());
            _exit(1);
        }
        else {
            close(other);
            setState(CONNECTING);
            socketConnected();
        }
    }
}

void MailCheck::socketConnected() {
    got = 0;
    sk.read(bf, sizeof(bf));
    setState(WAIT_READY);
}

void MailCheck::release() {
    if (ssl()) {
        sk.shutdown();
        sk.close();
        if (fPid > 1) {
            kill(fPid, SIGKILL);
            fPid = 0;
        }
    }
    else sk.close();
}

void MailCheck::socketError(int err) {
    if (fTrace) tlog("socketError %d in state %s", err, s(state));
    if (err == 0 && (state == SUCCESS || state == WAIT_QUIT)) {
        release();
        setState(IDLE);
    }
    else {
        error();
    }
}

void MailCheck::error() {
    release();
    setState(ERROR);
    fMbx->mailChecked(MailBoxStatus::mbxError, -1, -1);
}

cstring MailCheck::inbox() {
   return fURL.path == null || fURL.path == "/" ? "INBOX" : fURL.path + 1;
}

void MailCheck::escape(const char* buf, int len, char* tmp, int siz) {
    for (int i = 0, j = 0; i < len && buf[i] && j + 5 < siz; ++i) {
        if (buf[i] >= ' ' && buf[i] <= '~')
            tmp[j++] = buf[i];
        else if (buf[i] == '\r')
            tmp[j++] = '\\',
            tmp[j++] = 'r';
        else if (buf[i] == '\n')
            tmp[j++] = '\\',
            tmp[j++] = 'n';
        else
            tmp[j++] = '\\',
            tmp[j++] = ((buf[i] & 0300) >> 6) + '0',
            tmp[j++] = ((buf[i] & 0070) >> 3) + '0',
            tmp[j++] = ((buf[i] & 0007) >> 0) + '0';
        tmp[j] = 0;
    }
}

int MailCheck::write(const char *buf, int len) {
    if (len == 0)
        len = int(strlen(buf));
    if (fTrace) {
        char tmp[99] = "";
        escape(buf, len, tmp, 99);
        tlog(" write '%s'.", tmp);
    }
    int n = sk.write(buf, len);
    if (n != len) {
        fail(_("Write to socket failed"));
        error();
    }
    return n;
}

int MailCheck::write(const cstring& str) {
    return write(str, str.length());
}

void MailCheck::socketDataRead(char *buf, int len) {
    if (fTrace) {
        char tmp[567] = "";
        escape(buf, len, tmp, 567);
        tlog("got %d state=%s: '%s'.", len, s(state), tmp);
    }

    got += len;

    do {
        bool found = false;
        for (int i = 0; i < len; i++) {
            if (buf[i] == '\r')
                buf[i] = 0;
            if (buf[i] == '\n') {
                found = true;
                buf[i] = 0;
                break;
            }
        }

        if (!found) {
            if (got < sizeof(bf)) {
                sk.read(bf + got, sizeof(bf) - got);
            }
            else if (state == WAIT_READY &&
                     ((protocol == IMAP && bf[0] != '*') ||
                      (protocol == POP3 && bf[0] != '+' && bf[0] != '-')))
            {
                // ignore remainder of this line
                bf[0] = ' ';
                got = 1;
                sk.read(bf + got, sizeof(bf) - got);
            }
            else {
                error();
            }
            return ;
        }

        if (protocol == POP3)
            parsePop3();
        else if (protocol == IMAP)
            parseImap();
        if (state < WAIT_READY || state > WAIT_QUIT)
            return;

        len = strnlen(bf, got);
        while (unsigned(len) < got && bf[len] == 0)
            ++len;
        memmove(bf, bf + len, got - len);
        got -= len;
        buf = bf;
        len = got;
    } while (got > 0);
    sk.read(bf, sizeof(bf));
}

void MailCheck::parsePop3() {
    if (strncmp(bf, "+OK", 3) != 0) {
        if (fTrace) tlog("pop3: not +OK: '%s'.", bf);
        return error();
    }
    else if (state == WAIT_READY) {
        if (fTrace) tlog("pop3: ready");
        write("USER " + fURL.user + "\r\n");
        setState(WAIT_USER);
    }
    else if (state == WAIT_USER) {
        if (fTrace) tlog("pop3: login");
        write("PASS " + fURL.pass + "\r\n");
        setState(WAIT_PASS);
    }
    else if (state == WAIT_PASS) {
        if (fTrace) tlog("pop3: stat");
        write("STAT\r\n");
        setState(WAIT_STAT);
    }
    else if (state == WAIT_STAT) {
        if (fTrace) tlog("pop3: quit");
        if (sscanf(bf, "+OK %ld %ld", &fCurCount, &fCurSize) != 2
                || fCurCount < 0 || fCurSize < 0) {
            fCurCount = 0;
            fCurSize = 0;
        }
        write("QUIT\r\n");
        setState(WAIT_QUIT);
        if (fCurSize == 0)
            fMbx->mailChecked(MailBoxStatus::mbxNoMail, fCurCount, -1);
        else if (fCurSize > fLastSize && fLastSize != -1)
            fMbx->mailChecked(MailBoxStatus::mbxHasNewMail, fCurCount, -1);
        else
            fMbx->mailChecked(MailBoxStatus::mbxHasMail, fCurCount, -1);
        fLastSize = fCurSize;
        fLastCount = fCurCount;
    }
    else if (state == WAIT_QUIT) {
        setState(SUCCESS);
        release();
    }
    else {
        if (fTrace) tlog("pop3: invalid state %s: '%s'.", s(state), bf);
    }
}

void MailCheck::parseImap() {
    int seqnr = 0;
    char reply[32] = "";
    bool okay = false;

    if (state > WAIT_READY && bf[0] == '0' && bf[1] == '0') {
        if (sscanf(bf, "%d %30s", &seqnr, reply) < 2) {
            seqnr = 0;
            reply[0] = '\0';
        }
        else okay = (0 == strcmp(reply, "OK"));
    }
    if (fTrace) tlog("state %s, seqnr %d, reply '%s', for buf '%s'.",
                    s(state), seqnr, reply, bf);

    if (state == WAIT_READY) {
        if (0 == strncmp(bf, "* OK", 4)) {
            if (fTrace) tlog("imap: login");
            write("0001 LOGIN " + fURL.user + " " + fURL.pass + "\r\n");
            setState(WAIT_USER);
        }
        else if (bf[0] == '*') {
            if (fTrace) tlog("imap: invalid greeting: '%s'.", bf);
            error();
        }
        return ;
    }
    else if (state == WAIT_USER) {
        if (seqnr == 1 && okay) {
            if (fTrace) tlog("imap: status");
            write("0002 STATUS " + inbox() + " (MESSAGES)\r\n");
            setState(WAIT_STAT);
        }
        else if (seqnr) return error();
    }
    else if (state == WAIT_STAT) {
        if (bf[0] == '*') {
            char folder[128] = "";
            if (sscanf(bf, "* STATUS %127s (MESSAGES %ld)",
                       folder, &fCurCount) != 2 || fCurCount < 0) {
                fCurCount = 0;
            }
            fCurUnseen = -1;
        }
        else if (seqnr == 2 && okay) {
            if (fTrace) tlog("imap: unseen");
            write("0003 STATUS " + inbox() + " (UNSEEN)\r\n");
            setState(WAIT_UNSEEN);
        }
        else if (seqnr) return error();
    }
    else if (state == WAIT_UNSEEN) {
        if (bf[0] == '*') {
            char folder[128] = "";
            if (sscanf(bf, "* STATUS %127s (UNSEEN %ld)",
                       folder, &fCurUnseen) != 2 || fCurUnseen < 0) {
                fCurUnseen = -1;
            }
        }
        else if (seqnr == 3 && okay) {
            if (fTrace) tlog("imap: logout");
            const char logout[] = "0004 LOGOUT\r\n";
            write(logout, sizeof(logout) - 1);
            setState(WAIT_QUIT);
        }
        else if (seqnr) return error();
    }
    else if (state == WAIT_QUIT) {
        if (seqnr == 4 && okay) {
            if (fTrace) tlog("imap: done");
            release();
            setState(SUCCESS);
            if (fCurCount == 0)
                fMbx->mailChecked(MailBoxStatus::mbxNoMail,
                                  fCurCount, fCurUnseen);
            // A.Galanin: 'has unseen' flag has priority higher than 'has new' flag
            else if (fCurUnseen != 0)
                fMbx->mailChecked(MailBoxStatus::mbxHasUnreadMail,
                                  fCurCount, fCurUnseen);
            else if (fCurCount > fLastCount && fLastCount != -1)
                fMbx->mailChecked(MailBoxStatus::mbxHasNewMail,
                                  fCurCount, fCurUnseen);
            else
                fMbx->mailChecked(MailBoxStatus::mbxHasMail,
                                  fCurCount, fCurUnseen);
            fLastUnseen = fCurUnseen;
            fLastCount = fCurCount;
        }
        else if (seqnr) return error();
    }
    else {
        if (fTrace) tlog("imap: invalid state %s: '%s'.", s(state), bf);
        return error();
    }
}

const char* MailCheck::s(ProtocolState p) {
    switch (p) {
        case IDLE:        return "IDLE";
        case CONNECTING:  return "CONNECTING";
        case WAIT_READY:  return "WAIT_READY";
        case WAIT_USER:   return "WAIT_USER";
        case WAIT_PASS:   return "WAIT_PASS";
        case WAIT_STAT:   return "WAIT_STAT";
        case WAIT_UNSEEN: return "WAIT_UNSEEN";
        case WAIT_QUIT:   return "WAIT_QUIT";
        case ERROR:       return "ERROR";
        case SUCCESS:     return "SUCCESS";
    }
    return 0;
}

MailBoxStatus::MailBoxStatus(IApp *app, YSMListener *smActionListener,
                             mstring mailbox, YWindow *aParent):
    YWindow(aParent),
    fMailBox(mailbox),
    check(this),
    smActionListener(smActionListener),
    app(app)
{
    setSize(16, 16);
    fState = mbxNoMail;
    if (fMailBox != null) {
        MSG((_("Using MailBox \"%s\"\n"), cstring(fMailBox).c_str()));
        check.setURL(fMailBox);
        checkMail();
        if (mailCheckDelay > 0) {
            // caution creating too many openssl processes hogging the cpu
            long delay = check.ssl() ? max(30, mailCheckDelay)
                       : check.net() ? max(10, mailCheckDelay)
                       : mailCheckDelay;
            fMailboxCheckTimer->setTimer(delay * 1000L, this, true);
        }
    }
    setTitle("MailBox");
}

MailBoxStatus::~MailBoxStatus() {
}

void MailBoxStatus::paint(Graphics &g, const YRect &/*r*/) {
    ref<YPixmap> pixmap;
    switch (fState) {
    case mbxHasMail:
        pixmap = mailPixmap;
        break;
    case mbxHasNewMail:
        pixmap = newMailPixmap;
        break;
    case mbxHasUnreadMail:
        pixmap = unreadMailPixmap;
        break;
    case mbxNoMail:
        pixmap = noMailPixmap;
        break;
    default:
        pixmap = errMailPixmap;
        break;
    }

    if (pixmap == null || pixmap->mask()) {
        ref<YImage> gradient = parent()->getGradient();

        if (gradient != null)
            g.drawImage(gradient, x(), y(), width(), height(), 0, 0);
        else
            if (taskbackPixmap != null)
                g.fillPixmap(taskbackPixmap,
                             0, 0, width(), height(), x(), y());
        else {
            g.setColor(taskBarBg);
            g.fillRect(0, 0, width(), height());
        }
    }
    if (pixmap != null)
        g.drawPixmap(pixmap, 0, 0);
}

void MailBoxStatus::handleClick(const XButtonEvent &up, int count) {
    if ((taskBarLaunchOnSingleClick ? up.button == 2
         : up.button == 1) && count == 1)
        checkMail();
    else if (mailCommand && mailCommand[0] && up.button == 1 &&
             (taskBarLaunchOnSingleClick ? count == 1 : !(count % 2)))
        smActionListener->runCommandOnce(mailClassHint, mailCommand);
}

void MailBoxStatus::handleCrossing(const XCrossingEvent &crossing) {
    if (crossing.type == EnterNotify) {
#if 0
        if (countMailMessages) {
            struct stat st;
            unsigned long countSize;
            time_t countTime;

            if (stat(fMailBox, &st) != -1) {
                countSize = st.st_size;
                countTime = st.st_mtime;
            } else {
                countSize = 0;
                countTime = 0;
            }
            if (fLastCountSize != countSize || fLastCountTime != countTime)
            fLastCountSize = countSize;
        } else {
            setToolTip(0);
        }
#endif
    }
    YWindow::handleCrossing(crossing);
}

void MailBoxStatus::checkMail() {
    check.startCheck();
}

void MailBoxStatus::mailChecked(MailBoxState mst, long count, long unread) {
    if (mst != mbxError && fMailboxCheckTimer && mailCheckDelay > 0)
        fMailboxCheckTimer->startTimer();
    if (mst != fState) {
        fState = mst;
        repaint();
        if (fState == mbxHasNewMail)
            newMailArrived();
    }
    if (fState == mbxError)
        setToolTip(_("Error checking mailbox."));
    else {
        char s[128] = "";
        if (count >= 1 && unread >= 0) {
            snprintf(s, sizeof s,
                    count == 1 ?
                    _("%ld mail message, %ld unread.") :
                    _("%ld mail messages, %ld unread."),
                    count, unread);
        }
        else {
            snprintf(s, sizeof s,
                    count == 1 ?
                    _("%ld mail message.") :
                    _("%ld mail messages."), // too hard to do properly
                    count);
        }
        setToolTip(s);
    }
}

void MailBoxStatus::newMailArrived() {
    if (beepOnNewMail)
        xapp->alert();
    if (newMailCommand && newMailCommand[0])
        app->runCommand(newMailCommand);
}

bool MailBoxStatus::handleTimer(YTimer *t) {
    if (t == fMailboxCheckTimer) {
        checkMail();
        return true;
    }
    return false;
}

// vim: set sw=4 ts=4 et:
