/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 *
 * MailBox Status
 * !!! this should be external module (replacable for POP,IMAP,...)
 */
#include "config.h"

#include "intl.h"

#ifdef CONFIG_APPLET_MAILBOX
#include "ylib.h"
#include "amailbox.h"

#include "yapp.h"
#include "sysdep.h"
#include "base.h"
#include "prefs.h"
#include <sys/socket.h>
#include <netdb.h>

extern YColor *taskBarBg;

YPixmap *mailPixmap = 0;
YPixmap *noMailPixmap = 0;
YPixmap *errMailPixmap = 0;
YPixmap *unreadMailPixmap = 0;
YPixmap *newMailPixmap = 0;

MailCheck::MailCheck(MailBoxStatus *mbx) {
    fMbx = mbx;
    sk.setListener(this);
    username = password = server = port = filename = 0;
    state = IDLE;
    protocol = FILE;
    fURL = 0;
    fLastSize = -1;
    fLastCount = -1;
    fLastUnseen = 0;
    fLastCountTime = 0;
    fLastCountSize = -1;
}

MailCheck::~MailCheck() {
    sk.close();
}

void MailCheck::setURL(const char *url) {
    if (fURL)
        free(fURL);
    fURL = newstr(url);

    if (parse_pop3(fURL) != 0) {
        puts(_("invalid mailbox"));
        return ;
    }

    if (protocol == POP3 || protocol == IMAP) {
        struct hostent *host;

        server_addr.sin_family = AF_INET;
        if (protocol == IMAP)
            server_addr.sin_port = htons(143); // IMAP
        else
            server_addr.sin_port = htons(110); // POP-3

        if (server) {
            host = gethostbyname(server); /// !!! fix, need nonblocking resolve
            if (host) {
                memcpy(&server_addr.sin_addr,
                       host->h_addr_list[0],
                       sizeof(server_addr.sin_addr));
            } else
                state = ERROR;
        } else
            server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    }
    //doConnect();
}

void MailCheck::countMessages() {
    int fd = open(filename, O_RDONLY);
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
    if (protocol == FILE) {
        struct stat st;
        //MailBoxStatus::MailBoxState fNewState = fState;

        if (filename == 0)
            return ;

        if (!countMailMessages)
            fLastCount = -1;
        if (stat(filename, &st) == -1) {
            fMbx->mailChecked(MailBoxStatus::mbxNoMail, 0);
            fLastCount = 0;
            fLastSize = 0;
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
        sk.connect((struct sockaddr *) &server_addr, sizeof(server_addr));
        state = CONNECTING;
        got = 0;
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
        if (strncmp(bf, "+OK ", 4) != 0) {
            error();
            return ;
        }
        if (state == WAIT_READY) {
            char user[128];
            sprintf(user, "USER %s\r\n", username);

            sk.write(user, strlen(user));
            state = WAIT_USER;
        } else if (state == WAIT_USER) {
            char pass[128];

            sprintf(pass, "PASS %s\r\n", password);
            sk.write(pass, strlen(pass));
            state = WAIT_PASS;
        } else if (state == WAIT_PASS) {
            static char stat[] = "STAT\r\n";
            sk.write(stat, strlen(stat));
            state = WAIT_STAT;
        } else if (state == WAIT_STAT) {
            static char quit[] = "QUIT\r\n";
            //puts(bf);
            if (sscanf(bf, "+OK %lu %lu", &fCurCount, &fCurSize) != 2) {
                fCurCount = 0;
                fCurSize = 0;
            }
            sk.write(quit, strlen(quit));
            state = WAIT_QUIT;
        } else if (state == WAIT_QUIT) {
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
        }
    } else if (protocol == IMAP) {
        if (state == WAIT_READY) {
            char login[128];

            sprintf(login, "0000 LOGIN %s %s\r\n", username, password);
            sk.write(login, strlen(login));
            state = WAIT_USER;
        } else if (state == WAIT_USER) {
            char status[] = "0001 STATUS INBOX (MESSAGES UNSEEN)\r\n";
            sk.write(status, strlen(status));
            state = WAIT_STAT;
        } else if (state == WAIT_STAT) {
            char logout[] = "0002 LOGOUT\r\n";
            if (sscanf(bf, "* STATUS INBOX (MESSAGES %lu UNSEEN %lu)", &fCurCount, &fCurUnseen) != 2) {
                fCurCount = 0;
                fCurUnseen = 0;
            }
            sk.write(logout, strlen(logout));
            state = WAIT_QUIT;
        } else if (state == WAIT_QUIT) {
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
        }
    }
    got = 0;
    sk.read(bf, sizeof(bf));
}

int MailCheck::parse_pop3(char *src) { // !!! fix this to do %XX decode
    if (strncmp(src, "pop3://", 7) == 0) {
        src += 7;
        protocol = POP3;
    } else if (strncmp(src, "imap://", 7) == 0) {
        src += 7;
        protocol = IMAP;
    } else if (strncmp(src, "file:", 5) == 0) {
        src += 5;
        protocol = FILE;

        if (src[1] == '/' && src[2] == '/') {
            return -1;
        }
        filename = src;
        return 0;
    } else if (src[0] == '/') {
        filename = src;
        return 0;

    } else if (src[0] == 0) {
        return 0;
    } else {
        return -1;
    }

    char *p = strchr(src, '/');
    if (p)
        *p = 0;

    p = strchr(src, '@');
    if (p) {
        char *r;

        *p = 0;
        r = strchr(src, ':');
        if (r) {
            *r = 0;
            username = src;
            password = r + 1;
        } else {
            username = src;
        }

        src = p + 1;
    }
    p = strchr(src, ':');
    if (p) {
        *p = 0;
        server = src;
        port = p + 1;
    } else {
        server = src;
    }

    //msg("username='%s'", username);
    //msg("password='%s'", password);
    //msg("server='%s'", server);
    //msg("port='%s'", port);
    return 0;
}

MailBoxStatus::MailBoxStatus(const char *mailBox, const char *mailCommand, YWindow *aParent): YWindow(aParent), check(this) {
    char *mail = getenv("MAIL");

    fMailBox = 0;

    if (mailBox && mailBox[0])
        fMailBox = newstr(mailBox);
    else if (mail)
        fMailBox = newstr(mail);
    else
        fMailBox = newstr("/dev/null");

    fMailCommand = mailCommand;
    setSize(16, 16);
    fMailboxCheckTimer = 0;
    fState = mbxNoMail;
    if (fMailBox) {
        MSG((_("Using MailBox: '%s'\n"), fMailBox));

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
    delete [] fMailBox; fMailBox = 0;
}

void MailBoxStatus::paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
    YPixmap *pixmap;
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
    if (!pixmap || pixmap->mask()) {
        g.setColor(taskBarBg);
        // !!! fix this to draw taskbar background pixmap too
        g.fillRect(0, 0, width(), height());
    }
    if (pixmap)
        g.drawPixmap(pixmap, 0, 0);
}

void MailBoxStatus::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 1) {
        if (count == 1) {
            checkMail();
        } else if ((count % 2) == 0) {
            if (fMailCommand && fMailCommand[0])
                app->runCommand(fMailCommand);
        }
    }
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
        char s[128];
        if (count != -1) {
            sprintf(s, count == 1 ? _("%ld mail message.")
	    			  : _("%ld mail messages."), count);
            setToolTip(s);
        } else {
            setToolTip(0);
        }
    }
}

void MailBoxStatus::newMailArrived() {
    if (beepOnNewMail)
        XBell(app->display(), 100);
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
