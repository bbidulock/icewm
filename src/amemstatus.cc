/*
 * IceWM
 *
 * Copyright (C) 1998-2015 Marko Macek, Matthew Ogilvie
 *
 * Memory Status
 */
#include "config.h"
#include "ylib.h"
#include "wmapp.h"
#include "applet.h"
#include "amemstatus.h"
#include "ymenuitem.h"
#include "sysdep.h"
#include "default.h"
#include "ytimer.h"
#include "intl.h"

#if MEM_STATES

#define USE_PROC_MEMINFO

extern ref<YPixmap> taskbackPixmap;

MEMStatus::MEMStatus(IAppletContainer* taskBar, YWindow *aParent):
    IApplet(this, aParent),
    samples(taskBarMEMSamples, MEM_STATES),
    statusUpdateCount(0),
    unchanged(taskBarMEMSamples),
    taskBar(taskBar)
{
    fUpdateTimer->setTimer(taskBarMEMDelay, this, true);

    color[MEM_USER] = &clrMemUser;
    color[MEM_BUFFERS] = &clrMemBuffers;
    color[MEM_CACHED] = &clrMemCached;
    color[MEM_FREE] = &clrMemFree;

    samples.clear();
    for (int i = 0; i < taskBarMEMSamples; i++) {
        samples[i][MEM_FREE] = 1;
    }
    setSize(taskBarMEMSamples, taskBarGraphHeight);
    getStatus();
    updateStatus();
    updateToolTip();
    setTitle("MEM");
    show();
}

MEMStatus::~MEMStatus() {
}

bool MEMStatus::picture() {
    bool create = (hasPixmap() == false);

    Graphics G(getPixmap(), width(), height(), depth());

    if (create)
        fill(G);

    return (statusUpdateCount && unchanged < taskBarMEMSamples)
        ? draw(G), true : create;
}

void MEMStatus::fill(Graphics& g) {
    if (color[MEM_FREE]) {
        g.setColor(color[MEM_FREE]);
        g.fillRect(0, 0, width(), height());
    } else {
        ref<YImage> gradient(parent()->getGradient());

        if (gradient != null)
            g.drawImage(gradient,
                        x(), y(), width(), height(), 0, 0);
        else
            if (taskbackPixmap != null)
                g.fillPixmap(taskbackPixmap,
                             0, 0, width(), height(), x(), y());
    }
}

void MEMStatus::draw(Graphics& g) {
    int h = height();
    int first = max(0, taskBarMEMSamples - statusUpdateCount);
    if (0 < first && first < taskBarMEMSamples)
        g.copyArea(taskBarMEMSamples - first, 0, first, h, 0, 0);
    const int limit = (statusUpdateCount <= (1 + unchanged) / 2)
                    ? taskBarMEMSamples - statusUpdateCount : taskBarMEMSamples;
    statusUpdateCount = 0;

    for (int i = first; i < limit; i++) {
        membytes total = samples.sum(i);

        int y = h;
        for (int j = 0; j < MEM_STATES; j++) {
            int bar;
            if (j == MEM_STATES-1) {
                bar = y;
            } else {
                bar = (int)((h * samples[i][j]) / total);
            }

            if (bar <= 0) {
                continue;
            }

            if (color[j]) {
                g.setColor(color[j]);
                g.drawLine(i, y-1, i, y-bar);
            } else {
                ref<YImage> gradient = parent()->getGradient();

                if (gradient != null)
                    g.drawImage(gradient,
                                this->x() + i, this->y() + y - bar,
                                width(), bar,
                                i, y - bar);
                else
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
    if (amount >= (200ull*1024*1024*1024)) {
        snprintf( out, outSize, "%llu %.20s",
                  amount/(1024*1024*1024),
                  _("GB") );
    } else if (amount >= (200*1024*1024)) {
        snprintf( out, outSize, "%llu %.20s",
                  amount/(1024*1024),
                  _("MB") );
    } else if (amount >= (200*1024)) {
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
    membytes* cur = samples[taskBarMEMSamples-1];
    membytes total = samples.sum(taskBarMEMSamples-1);

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
        samples.copyTo(i, i - 1);
    }
    getStatus();
    repaint();
}

unsigned long long MEMStatus::parseField(const char *buf, size_t bufLen,
                                         const char *needle) {
#ifdef USE_PROC_MEMINFO
    ptrdiff_t needleLen = strlen(needle);
    for (const char* str(buf); str && (str = strstr(str, needle)) != 0; ) {
        if (str == buf || str[-1] == '\n') {
            char *endptr = NULL;
            membytes result = strtoull(str+needleLen, &endptr, 10);

            while (*endptr != 0 && *endptr == ' ')
                endptr++;

            if (*endptr=='k') {   // normal case
                result *= 1024;
            } else if (*endptr=='M') {
                result *= 1024*1024;
            } else if (*endptr=='G') {
                result *= 1024*1024*1024;
            }
            return result;
        }
        str = strchr(str + needleLen, '\n');
    }
#endif /*USE_PROC_MEMINFO*/
    return 0;
}

void MEMStatus::getStatus() {
    membytes *cur = samples[taskBarMEMSamples-1];
    samples.clear(taskBarMEMSamples-1);
    cur[MEM_FREE] = 1;

#ifdef USE_PROC_MEMINFO
    char buf[4096];
    int len = read_file("/proc/meminfo", buf, sizeof buf);
    if (len > 0) {
        cur[MEM_BUFFERS] = parseField(buf, len, "Buffers:");
        cur[MEM_CACHED] = parseField(buf, len, "Cached:");
        cur[MEM_FREE] = parseField(buf, len, "MemFree:");

        membytes total = max(1ULL, parseField(buf, len, "MemTotal:"));

        membytes user = total;
        for (int j = 0; j < MEM_STATES; j++) {
            user -= cur[j];
        }
        cur[MEM_USER] = user;
    }
#endif // USE_PROC_MEMINFO

    ++statusUpdateCount;
    int last = taskBarMEMSamples - 1;
    bool same = 0 < last && 0 == samples.compare(last, last - 1);
    unchanged = same ? 1 + unchanged : 0;
}

void MEMStatus::actionPerformed(YAction action, unsigned int modifiers) {
    if (action == actionClose) {
        hide();
        taskBar->relayout();
    }
    fMenu = 0;
}

void MEMStatus::handleClick(const XButtonEvent &up, int count) {
    if (up.button == Button3) {
        fMenu = new YMenu();
        fMenu->setActionListener(this);
        fMenu->addItem(_("MEM"), -2, null, actionNull)->setEnabled(false);
        fMenu->addItem(_("_Disable"), -2, null, actionClose);
        fMenu->popup(0, 0, 0, up.x_root, up.y_root,
                     YPopupWindow::pfCanFlipVertical |
                     YPopupWindow::pfCanFlipHorizontal |
                     YPopupWindow::pfPopupMenu);
    }
}

#endif

// vim: set sw=4 ts=4 et:
