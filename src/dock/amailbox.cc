/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 *
 * MailBox Status
 * !!! this should be external module (replacable for POP,IMAP,...)
 */
#include "config.h"

#include "amailbox.h"
#include "yresource.h"
#include "ybuttonevent.h"
#include "ycrossingevent.h"

#include "yapp.h"
#include "yconfig.h"
#include "sysdep.h"
#include "base.h"
#include "prefs.h"
#include "ypaint.h"
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

YColorPrefProperty MailBoxStatus::gTaskBarBg("taskbar", "ColorBackground", "rgb:C0/C0/C0");
YPixmapPrefProperty MailBoxStatus::gPixmapMail("taskbar", "PixmapMail", "mail.xpm", DATADIR);
YPixmapPrefProperty MailBoxStatus::gPixmapNoMail("taskbar", "PixmapNoMail", "nomail.xpm", DATADIR);
YPixmapPrefProperty MailBoxStatus::gPixmapErrMail("taskbar", "PixmapErrMail", "errmail.xpm", DATADIR);
YPixmapPrefProperty MailBoxStatus::gPixmapUnreadMail("taskbar", "PixmapUnreadMail", "unreadmail.xpm", DATADIR);
YPixmapPrefProperty MailBoxStatus::gPixmapNewMail("taskbar", "PixmapNewMail", "newmail.xpm", DATADIR);

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

void MailCheck::__setURL(const char *url) {
    if (fURL)
        free(fURL);
    fURL = __newstr(url, strlen(url));

    if (parse_pop3(fURL) != 0) {
        warn("invalid mailbox");
        return;
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
            for (i = 0; i < len; ) {
                if (buf[i] != pat[pos]) {
                    if (pos)
                        pos = 0;
                    else
                        i++;
                } else {
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
        return;
    //puts(protocol == FILE ? "file" : protocol == POP3 ? "POP3" : "IMAP");
    if (protocol == FILE) {
        struct stat st;
        //MailBoxStatus::MailBoxState fNewState = fState;

        if (filename == 0)
            return;

        YPref prefCountMessages("mailboxstatus_applet", "MailCountMessages");
        bool countMailMessages = prefCountMessages.getBool(false);
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

void MailCheck::socketDataRead(byte *buf, int len) {
    //printf("got %d state=%d\n", len, state);

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
            sk.read(bf + got, sizeof(bf) - got);
            return;
        } else {
            error();
            return;
        }
    }
    if (protocol == POP3) {
        if (strncmp((char *)bf, "+OK ", 4) != 0) {
            error();
            return;
        }
        if (state == WAIT_READY) {
            char user[128];
            sprintf(user, "USER %s\r\n", username);

            sk.write((byte *)user, strlen(user));
            state = WAIT_USER;
        } else if (state == WAIT_USER) {
            char pass[128];

            sprintf(pass, "PASS %s\r\n", password);
            sk.write((byte *)pass, strlen(pass));
            state = WAIT_PASS;
        } else if (state == WAIT_PASS) {
            static char stat[] = "STAT\r\n";
            sk.write((byte *)stat, strlen(stat));
            state = WAIT_STAT;
        } else if (state == WAIT_STAT) {
            static char quit[] = "QUIT\r\n";
            //puts(bf);
            if (sscanf((char *)bf, "+OK %lu %lu", &fCurCount, &fCurSize) != 2) {
                fCurCount = 0;
                fCurSize = 0;
            }
            sk.write((byte *)quit, strlen(quit));
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
            return;
        }
    } else if (protocol == IMAP) {
        if (state == WAIT_READY) {
            char login[128];

            sprintf(login, "0000 LOGIN %s %s\r\n", username, password);
            sk.write((byte *)login, strlen(login));
            state = WAIT_USER;
        } else if (state == WAIT_USER) {
            char status[] = "0001 STATUS INBOX (MESSAGES UNSEEN)\r\n";
            sk.write((byte *)status, strlen(status));
            state = WAIT_STAT;
        } else if (state == WAIT_STAT) {
            char logout[] = "0002 LOGOUT\r\n";
            if (sscanf((char *)bf, "* STATUS INBOX (MESSAGES %lu UNSEEN %lu)", &fCurCount, &fCurUnseen) != 2) {
                fCurCount = 0;
                fCurUnseen = 0;
            }
            sk.write((byte *)logout, strlen(logout));
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
            return;
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

    //printf("username='%s'\n", username);
    //printf("password='%s'\n", password);
    //printf("server='%s'\n", server);
    //printf("port='%s'\n", port);
    return 0;
}

MailBoxStatus::MailBoxStatus(const char *mailBox, const char *mailCommand, YWindow *aParent):
    YWindow(aParent), check(this), fMailboxCheckTimer(this, fMailCheckDelay * 1000)
{
    char *mail = getenv("MAIL");

    fMailBox = 0;

    if (mailBox && mailBox[0])
        fMailBox = __newstr(mailBox);
    else if (mail)
        fMailBox = __newstr(mail);
    else
        fMailBox = __newstr("/dev/null");

    fMailCommand = mailCommand;

    YPref prefMailCheckDelay("mailboxstatus_applet", "MailCheckDelay");
    fMailCheckDelay = prefMailCheckDelay.getNum(30);

    YPref prefNewMailCommand("mailboxstatus_applet", "MailCheckDelay");
    fNewMailCommand = __newstr(prefNewMailCommand.getStr(0));

    setSize(16, 16);
    fState = mbxNoMail;
    if (fMailBox) {
        MSG(("Using MailBox: '%s'\n", fMailBox));

        check.__setURL(fMailBox);
        fMailboxCheckTimer.startTimer();
        checkMail();
    }
}

MailBoxStatus::~MailBoxStatus() {
    delete [] fMailBox; fMailBox = 0;
}

void MailBoxStatus::paint(Graphics &g, const YRect &/*er*/) {
    YPixmap *pixmap;
    switch (fState) {
    case mbxHasMail:
        pixmap = gPixmapMail.getPixmap();
        break;
    case mbxHasNewMail:
        pixmap = gPixmapNewMail.getPixmap();
        break;
    case mbxHasUnreadMail:
        pixmap = gPixmapUnreadMail.getPixmap();
        break;
    case mbxNoMail:
        pixmap = gPixmapNoMail.getPixmap();
        break;
    default:
        pixmap = gPixmapErrMail.getPixmap();
        break;
    }
    if (!pixmap || pixmap->mask()) {
        g.setColor(gTaskBarBg);
        // !!! fix this to draw taskbar background pixmap too
        g.fillRect(0, 0, width(), height());
    }
    if (pixmap)
        g.drawPixmap(pixmap, 0, 0);
}

bool MailBoxStatus::eventClick(const YClickEvent &up) {
    if (up.leftButton()) {
        if (up.isSingleClick()) {
            checkMail();
        } else if (up.isDoubleClick()) {
            if (fMailCommand && fMailCommand[0])
                app->runCommand(fMailCommand);
        }
        return true;
    }
    return YWindow::eventClick(up);
}

bool MailBoxStatus::eventCrossing(const YCrossingEvent &crossing) {
    if (crossing.type() == YEvent::etPointerIn) {
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
    return YWindow::eventCrossing(crossing);
}

void MailBoxStatus::checkMail() {
    check.startCheck();
}

void MailBoxStatus::mailChecked(MailBoxState mst, long count) {
    if (mst != mbxError)
        fMailboxCheckTimer.startTimer();
    if (mst != fState) {
        fState = mst;
        repaint();
        if (fState == mbxHasNewMail)
            newMailArrived();
    }
    if (fState == mbxError)
        __setToolTip("Error checking mailbox.");
    else {
        char s[128];
        if (count != -1) {
            sprintf(s, "%ld mail message%s.", count, count == 1 ? ""  :"s");
            __setToolTip(s);
        } else {
            __setToolTip(0);
        }
    }
}

void MailBoxStatus::newMailArrived() {
    YPref prefBeep("mailboxstatus_applet", "BeepOnNewMail");
    bool beepOnNewMail = prefBeep.getBool(false);
    if (beepOnNewMail)
        app->beep();
    if (fNewMailCommand && fNewMailCommand[0])
        app->runCommand(fNewMailCommand);
}



bool MailBoxStatus::handleTimer(YTimer *t) {
    if (t != &fMailboxCheckTimer)
        return false;
    checkMail();
    return true;
}
