/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 *
 * MailBox Status
 * !!! this should be external module (replacable for POP,IMAP,...)
 */
#include "config.h"

#include "intl.h"

#ifdef CONFIG_APPLET_MAILBOX
#include "ylib.h"
#include "amailbox.h"

#include "sysdep.h"
#include "base.h"
#include "prefs.h"
#include "wmapp.h"
#include <sys/socket.h>
#include <netdb.h>

static YColor *taskBarBg = 0;
extern ref<YPixmap> taskbackPixmap;

ref<YPixmap> mailPixmap;
ref<YPixmap> noMailPixmap;
ref<YPixmap> errMailPixmap;
ref<YPixmap> unreadMailPixmap;
ref<YPixmap> newMailPixmap;

MailCheck::MailCheck(MailBoxStatus *mbx):
    state(IDLE), fMbx(mbx), fLastSize(-1), fLastCount(-1),
    fLastUnseen(0), fLastCountSize(-1), fLastCountTime(0)
{
    sk.setListener(this);
}

MailCheck::~MailCheck() {
    sk.close();
}

void MailCheck::setURL(ustring url) {
    if (url.startsWith(mstring("/")))
        url = url.insert(0, mstring("file://"));

    fURL.init(new YURL(url));
    if (fURL != null && fURL->scheme() != null) {
        if (fURL->scheme().equals(mstring("pop3")) ||
            fURL->scheme().equals(mstring("imap")))
        {
            if (fURL->scheme().charAt(0) == 'i')
                protocol = IMAP;
            else
                protocol = POP3;

            server_addr.sin_family = AF_INET;
            server_addr.sin_port =
                htons(fURL->port() != null? atoi(cstring(fURL->port()).c_str())
                      : (protocol == IMAP ? 143 : 110));

            if (fURL->host() != null) { /// !!! fix, need nonblocking resolve
                struct hostent const * host(gethostbyname(cstring(fURL->host()).c_str()));

                if (host)
                    memcpy(&server_addr.sin_addr,
                           host->h_addr_list[0],
                           sizeof(server_addr.sin_addr));
                else
                    state = ERROR;
            } else
                server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        } else if (!strcmp(cstring(fURL->scheme()).c_str(), "file"))
            protocol = LOCALFILE;
        else
            warn(_("Invalid mailbox protocol: \"%s\""), cstring(fURL->scheme()).c_str());
    } else
        warn(_("Invalid mailbox path: \"%s\""), cstring(url).c_str());
}

void MailCheck::countMessages() {
    int fd = open(cstring(fURL->path()).c_str(), O_RDONLY);
    int mails = 0;

    if (fd != -1) {
        char buf[4096];
        const char *pat = "\nFrom ";
        int plen = strlen(pat);
        int pos = 1;
        int i, len;

        while ((len = read(fd, buf, sizeof(buf))) > 0) {
            for (i = 0; i < len;) {
                if (buf[i] != pat[pos])
                    if (pos)
                        pos = 0;
                    else
                        i++;
                else {
                    i++;
                    if (++pos == plen) {
                        pos = 0;
                        mails++;
                    }
                }
            }
        }
        close(fd);
    }
    fLastCount = mails;
}

void MailCheck::startCheck() {
    if (state == ERROR)
        state = IDLE;
    if (state != IDLE && state != SUCCESS)
        return ;
    //puts(protocol == FILE ? "file" : protocol == POP3 ? "POP3" : "IMAP");
    if (protocol == LOCALFILE) {
        struct stat st;
        //MailBoxStatus::MailBoxState fNewState = fState;
        if (fURL->path() == null)
            return;

        if (!countMailMessages)
            fLastCount = -1;
        if (stat(cstring(fURL->path()).c_str(), &st) == -1) {
            fMbx->mailChecked(MailBoxStatus::mbxNoMail, 0);
            fLastCount = 0;
            fLastSize = 0;
            fLastUnseen = 0;
        } else {
            if (countMailMessages &&
                (st.st_size != fLastCountSize || st.st_mtime != fLastCountTime))
            {
                countMessages();
                fLastCountTime = st.st_mtime;
                fLastCountSize = fLastSize;
            }
            if (st.st_size == 0)
                fMbx->mailChecked(MailBoxStatus::mbxNoMail, fLastCount);
            else if (st.st_size > fLastSize && fLastSize != -1)
                fMbx->mailChecked(MailBoxStatus::mbxHasNewMail, fLastCount);
            else if (st.st_mtime > st.st_atime)
                fMbx->mailChecked(MailBoxStatus::mbxHasUnreadMail, fLastCount);
            else
                fMbx->mailChecked(MailBoxStatus::mbxHasMail, fLastCount);
            fLastSize = st.st_size;
        }
    } else {
        if (sk.connect((struct sockaddr *) &server_addr, sizeof(server_addr))
            == 0) 
	{
            state = CONNECTING;
            got = 0;
	} else {
	    error();
        }
    }
}

void MailCheck::socketConnected() {
    //puts("CONNECTED");
    got = 0;
    sk.read(bf, sizeof(bf));
    state = WAIT_READY;
}

void MailCheck::socketError(int err) {
    //if (err) printf("error: %d\n", err);
    //else puts("EOF");
    //app->exit(err ? 1 : 0);
    if (err != 0 || state != SUCCESS)
        error();
    else {
        sk.close();
    }
}

void MailCheck::error() {
    sk.close();
    state = ERROR;
    fMbx->mailChecked(MailBoxStatus::mbxError, -1);
}

void MailCheck::socketDataRead(char *buf, int len) {
    //msg("got %d state=%d", len, state);

    bool found = false;
    for (int i = 0; i < len; i++) {
        //putchar(buf[i]);
        if (buf[i] == '\r')
            buf[i] = 0;
        if (buf[i] == '\n') {
            found = true;
            buf[i] = 0;
            break;
        }
    }
    got += len;
    if (!found) {
        if (got < sizeof(bf)) {
            sk.read(bf + got , sizeof(bf) - got);
            return ;
        } else {
            error();
            return ;
        }
    }

    if (protocol == POP3) {
        if (strncmp(bf, "+OK", 3) != 0) {
            MSG(("pop3: not +OK: %s", bf));
            error();
            return ;
        }
        if (state == WAIT_READY) {
            MSG(("pop3: ready"));
            char * user(cstrJoin("USER ", cstring(fURL->user()).c_str(), "\r\n", NULL));
            sk.write(user, strlen(user));
            state = WAIT_USER;
            delete[] user;
        } else if (state == WAIT_USER) {
            MSG(("pop3: login"));
            char * pass(cstrJoin("PASS ", cstring(fURL->password()).c_str(), "\r\n", NULL));
            sk.write(pass, strlen(pass));
            state = WAIT_PASS;
            delete[] pass;
        } else if (state == WAIT_PASS) {
            MSG(("pop3: stat"));
            static char stat[] = "STAT\r\n";
            sk.write(stat, strlen(stat));
            state = WAIT_STAT;
        } else if (state == WAIT_STAT) {
            static char quit[] = "QUIT\r\n";
            MSG(("pop3: quit"));
            //puts(bf);
            if (sscanf(bf, "+OK %lu %lu", &fCurCount, &fCurSize) != 2) {
                fCurCount = 0;
                fCurSize = 0;
            }
            sk.write(quit, strlen(quit));
            state = WAIT_QUIT;
        } else if (state == WAIT_QUIT) {
            MSG(("pop3: done"));
            //puts("GOT QUIT");
            //app->exit(0);
            sk.close();
            state = SUCCESS;
            if (fCurSize == 0)
                fMbx->mailChecked(MailBoxStatus::mbxNoMail, fCurCount);
            else if (fCurSize > fLastSize && fLastSize != -1)
                fMbx->mailChecked(MailBoxStatus::mbxHasNewMail, fCurCount);
            else
                fMbx->mailChecked(MailBoxStatus::mbxHasMail, fCurCount);
            fLastSize = fCurSize;
            fLastCount = fCurCount;
            return ;
        } else {
            MSG(("pop3: what?: %s", bf));
        }
    } else if (protocol == IMAP) {
        if (state == WAIT_READY) {
            MSG(("imap: login"));
            char * login(cstrJoin("0000 LOGIN ",
                                  cstring(fURL->user()).c_str(), " ",
                                  cstring(fURL->password()).c_str(), "\r\n", NULL));
            sk.write(login, strlen(login));
            state = WAIT_USER;
            delete[] login;
        } else if (state == WAIT_USER) {
            MSG(("imap: status"));
            char * status(cstrJoin("0001 STATUS ",
                                   (fURL->path() == null || fURL->path().equals("/")) ? "INBOX" : cstring(fURL->path()).c_str() + 1,
                                   " (MESSAGES)\r\n", NULL));
            sk.write(status, strlen(status));
            state = WAIT_STAT;
            delete[] status;
        } else if (state == WAIT_STAT) {
            MSG(("imap: logout"));
            char logout[] = "0002 LOGOUT\r\n", folder[128];
            if (sscanf(bf, "* STATUS %127s (MESSAGES %lu)",
                       folder, &fCurCount) != 2) {
                fCurCount = 0;
            }
            fCurUnseen = 0;
            sk.write(logout, strlen(logout));
            state = WAIT_QUIT;
        } else if (state == WAIT_QUIT) {
            MSG(("imap: done"));
            //app->exit(0);
            sk.close();
            state = SUCCESS;
            if (fCurCount == 0)
                fMbx->mailChecked(MailBoxStatus::mbxNoMail, fCurCount);
            else if (fCurCount > fLastCount && fLastCount != -1)
                fMbx->mailChecked(MailBoxStatus::mbxHasNewMail, fCurCount);
            else if (fCurUnseen != 0)
                fMbx->mailChecked(MailBoxStatus::mbxHasUnreadMail, fCurCount);
            else
                fMbx->mailChecked(MailBoxStatus::mbxHasMail, fCurCount);
            fLastUnseen = fCurUnseen;
            fLastCount = fCurCount;
            return ;
        } else {
            MSG(("imap: what?: %s", bf));
        }
    }
    got = 0;
    sk.read(bf, sizeof(bf));
}

MailBoxStatus::MailBoxStatus(mstring mailbox, YWindow *aParent):
YWindow(aParent), fMailBox(mailbox), check(this)
{
    if (taskBarBg == 0) {
        taskBarBg = new YColor(clrDefaultTaskBar);
    }

    setSize(16, 16);
    fMailboxCheckTimer = 0;
    fState = mbxNoMail;
    if (fMailBox != null) {
        cstring cs(fMailBox);
        MSG((_("Using MailBox \"%s\"\n"), cs.c_str()));
        check.setURL(fMailBox);

        fMailboxCheckTimer = new YTimer(mailCheckDelay * 1000);
        if (fMailboxCheckTimer) {
            fMailboxCheckTimer->setTimerListener(this);
            fMailboxCheckTimer->startTimer();
        }
        checkMail();
    }
}

MailBoxStatus::~MailBoxStatus() {
    if (fMailboxCheckTimer) {
        fMailboxCheckTimer->stopTimer();
        fMailboxCheckTimer->setTimerListener(0);
    }
    delete fMailboxCheckTimer; fMailboxCheckTimer = 0;
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
#ifdef CONFIG_GRADIENTS
        ref<YImage> gradient = parent()->getGradient();

        if (gradient != null)
            g.drawImage(gradient, x(), y(), width(), height(), 0, 0);
        else
#endif
            if (taskbackPixmap != null)
                g.fillPixmap(taskbackPixmap, 0, 0,
                             width(), height(), this->x(), this->y());
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
        wmapp->runCommandOnce(mailClassHint, mailCommand);
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

void MailBoxStatus::mailChecked(MailBoxState mst, long count) {
    if (mst != mbxError)
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
        if (count != -1) {
            sprintf(s,
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
    if (t != fMailboxCheckTimer)
        return false;
    checkMail();
    return true;
}

#endif
