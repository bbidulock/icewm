/*
 * IceWM
 *
 * Copyright (C) 1998-2015 Marko Macek, Matthew Ogilvie
 *
 * Memory Status
 */
#include "config.h"

#ifdef CONFIG_APPLET_MEM_STATUS
#include "ylib.h"
#include "wmapp.h"

#include "amemstatus.h"
#include "sysdep.h"
#include "default.h"

#include "ytimer.h"

#include "intl.h"

#if defined(__linux__)

#define USE_PROC_MEMINFO

extern ref<YPixmap> taskbackPixmap;

MEMStatus::MEMStatus(YWindow *aParent): YWindow(aParent) {
    samples = new unsigned long long *[taskBarMEMSamples];

    for (int a(0); a < taskBarMEMSamples; a++)
        samples[a] = new unsigned long long[MEM_STATES];

    fUpdateTimer = new YTimer(taskBarMEMDelay);
    if (fUpdateTimer) {
        fUpdateTimer->setTimerListener(this);
        fUpdateTimer->startTimer();
    }

    for (int j(0); j < MEM_STATES; j++)
    {
        color[j] = NULL;
    }
    color[MEM_USER] = new YColor(clrMemUser);
    color[MEM_BUFFERS] = new YColor(clrMemBuffers);
    color[MEM_CACHED] = new YColor(clrMemCached);
    if (*clrMemFree) {
        color[MEM_FREE] = new YColor(clrMemFree);
    }
    for (int i = 0; i < taskBarMEMSamples; i++) {
        for (int j=0; j < MEM_STATES; j++)
            samples[i][j]=0;
        samples[i][MEM_FREE] = 1;
    }
    setSize(taskBarMEMSamples, 20);
    getStatus();
    updateStatus();
    updateToolTip();
}

MEMStatus::~MEMStatus() {
    delete fUpdateTimer;
    for (int a(0); a < taskBarMEMSamples; a++) {
        delete samples[a]; samples[a] = 0;
    }
    delete samples; samples = 0;
    for (int j(0); j < MEM_STATES; j++) {
        delete color[j]; color[j] = 0;
    }
}

void MEMStatus::paint(Graphics &g, const YRect &/*r*/) {
    int h = height();

    for (int i(0); i < taskBarMEMSamples; i++) {
        unsigned long long total = 0;
        int j;
        for (j = 0; j < MEM_STATES; j++) {
            total += samples[i][j];
        }

        int y = h;
        for (j = 0; j < MEM_STATES; j++) {
            int bar;
            if (j == MEM_STATES-1) {
                bar = y;
            } else {
                bar = (int)((h * samples[i][j]) / total);
            }

            if (bar <= 0) {
                continue;
            }

            if(color[j]) {
                g.setColor(color[j]);
                g.drawLine(i, y-1, i, y-bar);
            } else {
#ifdef CONFIG_GRADIENTS
                ref<YImage> gradient = parent()->getGradient();

                if (gradient != null)
                    g.drawImage(gradient,
                                this->x() + i, this->y() + y - bar,
                                width(), bar,
                                i, y - bar);
                else
#endif
                    if (taskbackPixmap != null)
                        g.fillPixmap(taskbackPixmap,
                                     i, y - bar,
                                     width(), bar,
                                     this->x() + i, this->y() + y - bar);
            }
            y -= bar;
        }
    }
}

bool MEMStatus::handleTimer(YTimer *t) {
    if (t != fUpdateTimer)
        return false;
    updateStatus();
    if (toolTipVisible())
        updateToolTip();
    return true;
}

void MEMStatus::printAmount(char *out, size_t outSize,
                            unsigned long long amount) {
    if(amount >= (200ull*1024*1024*1024)) {
        snprintf( out, outSize, "%llu %.20s",
                  amount/(1024*1024*1024),
                  _("GB") );
    } else if(amount >= (200*1024*1024)) {
        snprintf( out, outSize, "%llu %.20s",
                  amount/(1024*1024),
                  _("MB") );
    } else if(amount >= (200*1024)) {
        snprintf( out, outSize, "%llu %.20s",
                  amount/1024,
                  _("kB") );
    } else {
        snprintf( out, outSize, "%llu %.20s",
                  amount,
                  _("bytes") );
    }
    out[outSize-1]='\0';
}

void MEMStatus::updateToolTip() {
    unsigned long long *cur=samples[taskBarMEMSamples-1];

    unsigned long long total = 0;
    for (int j(0); j < MEM_STATES; j++) {
        total += cur[j];
    }

    char totalStr[64];
    printAmount(totalStr, sizeof(totalStr), total);
    char freeStr[64];
    printAmount(freeStr, sizeof(freeStr), cur[MEM_FREE]);
    char userStr[64];
    printAmount(userStr, sizeof(userStr), cur[MEM_USER]);
    char buffersStr[64];
    printAmount(buffersStr, sizeof(buffersStr), cur[MEM_BUFFERS]);
    char cachedStr[64];
    printAmount(cachedStr, sizeof(cachedStr), cur[MEM_CACHED]);

    char *memmsg = cstrJoin(_("Memory Total: "), totalStr, "\n   ",
                            _("Free: "), freeStr, "\n   ",
                            _("Cached: "), cachedStr, "\n   ",
                            _("Buffers: "), buffersStr, "\n   ",
                            _("User: "), userStr,
                            NULL );
    setToolTip(memmsg);
    delete [] memmsg;
}

void MEMStatus::updateStatus() {
    for (int i(1); i < taskBarMEMSamples; i++) {
        for (int j(0); j < MEM_STATES ; j++) {
            samples[i-1][j] = samples[i][j];
        }
    }
    getStatus(),
    repaint();
}

unsigned long long MEMStatus::parseField(const char *buf, size_t bufLen,
                                         const char *needle) {
#ifdef USE_PROC_MEMINFO
    ptrdiff_t needleLen = strlen(needle);
    const char *end = buf + bufLen;
    while(buf < end) {
        const char *nl = (const char *)memchr(buf, '\n', end-buf);
        if(nl == 0)
            break;

        if(nl-buf > needleLen && memcmp(buf, needle, needleLen) == 0) {
            char *endptr = NULL;
            unsigned long long result = strtoull(buf+needleLen, &endptr, 10);

            while(endptr!=0 && *endptr==' ')
                endptr++;

            if(*endptr=='k') {   // normal case
                result *= 1024;
            } else if(*endptr=='M') {
                result *= 1024*1024;
            } else if(*endptr=='G') {
                result *= 1024*1024*1024;
            }
            return result;
        }

        buf = nl+1;
    }
#endif /*USE_PROC_MEMINFO*/
    return 0;
}

void MEMStatus::getStatus() {
    unsigned long long *cur=samples[taskBarMEMSamples-1];
    int j;
    for (j = 0; j < MEM_STATES; j++) {
        cur[j] = 0;
    }
    cur[MEM_FREE] = 1;

#ifdef USE_PROC_MEMINFO
    int fd = open("/proc/meminfo", O_RDONLY);
    if (fd == -1)
        return;

    char buf[4096];
    ssize_t len = read(fd, buf, sizeof(buf)-1);
    close(fd);
    if (len < 0) {
        return;
    }
    buf[len] = '\0';

    cur[MEM_BUFFERS] = parseField(buf, len, "Buffers:");
    cur[MEM_CACHED] = parseField(buf, len, "Cached:");
    cur[MEM_FREE] = parseField(buf, len, "MemFree:");

    unsigned long long total = parseField(buf, len, "MemTotal:");
    if (total < 1)
        total = 1;

    unsigned long long user = total;
    for (j = 0; j < MEM_STATES; j++) {
        user -= cur[j];
    }
    cur[MEM_USER] = user;
#endif // USE_PROC_MEMINFO
}
#endif
#endif
