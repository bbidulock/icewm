/*
 * IceWM
 *
 * Copyright (C) 1998-2001 Marko Macek
 *
 * CPU Status
 *
 * KNOWN BUGS:
 * 1. On startup, the second bar shows always zero, and the first bar shows
 *    total cpu info from bootup time (maybe this can be also considered as
 *    a feature) -stibor-
 *
 * 2. There are no checks for overflows: when sys, user, nice or idle time
 *    in /proc/stat will exceed usigned long (256^4), CPU monitor will
 *    probably show nonsenses. (but it is at least after 500 days of uptime)
 *    -stibor-
 */
#include "config.h"
#include "ylib.h"
#include "wmapp.h"
#include "applet.h"
#include "ypointer.h"
#include "acpustatus.h"

#if IWM_STATES

#include "sysdep.h"
#include "default.h"
#include "ascii.h"
#include "ymenuitem.h"

#if __linux__
#include <sys/sysinfo.h>
#else
#if __FreeBSD__
#include <sys/resource.h>
#endif
#if __sun__
#include <sys/loadavg.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#if HAVE_SYS_SYSCTL_H || HAVE_SYSCTLBYNAME || __FreeBSD__
#include <sys/sysctl.h>
#endif
#ifdef HAVE_UVM_UVM_PARAM_H
#include <uvm/uvm_param.h>
#endif
#ifdef HAVE_SCHED_H
#include <sched.h>
#endif
#ifdef HAVE_SYS_SCHED_H
#include <sys/sched.h>
#endif
#if defined(HW_PHYSMEM) && !defined(HW_PHYSMEM64)
#define HW_PHYSMEM64 HW_PHYSMEM
#endif
#if defined(HW_USERMEM) && !defined(HW_USERMEM64)
#define HW_USERMEM64 HW_USERMEM
#endif

#endif /*__linux__*/

#include "udir.h"
#include "intl.h"

extern ref<YPixmap> taskbackPixmap;

ref<YFont> CPUStatus::tempFont;

CPUStatus::CPUStatus(YWindow *aParent, CPUStatusHandler *aHandler, int cpuid) :
    IApplet(this, aParent),
    fCpuID(cpuid),
    statusUpdateCount(0),
    unchanged(taskBarCPUSamples),
    cpu(taskBarCPUSamples, IWM_STATES),
    fHandler(aHandler),
    fTempColor(&clrCpuTemp)
{
    cpu.clear();
    for (int a(0); a < taskBarCPUSamples; a++) {
        cpu[a][IWM_IDLE] = 1;
    }
    memset(last_cpu, 0, sizeof(last_cpu));
    color[IWM_USER] = &clrCpuUser;
    color[IWM_NICE] = &clrCpuNice;
    color[IWM_SYS]  = &clrCpuSys;
    color[IWM_INTR] = &clrCpuIntr;
    color[IWM_IOWAIT] = &clrCpuIoWait;
    color[IWM_SOFTIRQ] = &clrCpuSoftIrq;
    color[IWM_IDLE] = &clrCpuIdle;
    color[IWM_STEAL] = &clrCpuSteal;

    fUpdateTimer->setTimer(taskBarCPUDelay, this, true);

    setSize(taskBarCPUSamples, taskBarGraphHeight);
    getStatus();
    updateStatus();
    updateToolTip();
    char buf[99];
    snprintf(buf, 99, "CPU%d", cpuid);
    setTitle(buf);
}

CPUStatus::~CPUStatus() {
}

bool CPUStatus::picture() {
    bool change = (hasPixmap() == false);

    Graphics G(getPixmap(), width(), height(), depth());

    if (cpustatusShowAcpiTempInGraph) {
        change = true;
        statusUpdateCount = taskBarCPUSamples;
    }
    if (change) {
        unchanged = 0;
        fill(G);
    }

    if (statusUpdateCount && unchanged < taskBarCPUSamples) {
        draw(G);
        change = true;
    }
    if (cpustatusShowAcpiTempInGraph) {
        temperature(G);
    }
    return change;
}

void CPUStatus::fill(Graphics& g) {
    if (color[IWM_IDLE]) {
        g.setColor(color[IWM_IDLE]);
        g.fillRect(0, 0, width(), height());
    } else {
        ref<YImage> gradient(getGradient());

        if (gradient != null)
            g.drawImage(gradient,
                        x(), y(), width(), height(), 0, 0);
        else
            if (taskbackPixmap != null)
                g.fillPixmap(taskbackPixmap,
                             0, 0, width(), height(), x(), y());
    }
}

void CPUStatus::draw(Graphics& g) {
    int h = height();
    int first = max(0, taskBarCPUSamples - statusUpdateCount);
    if (0 < first && first < taskBarCPUSamples)
        g.copyArea(taskBarCPUSamples - first, 0, first, h, 0, 0);
    const int limit = (statusUpdateCount <= (1 + unchanged) / 2)
                    ? taskBarCPUSamples - statusUpdateCount : taskBarCPUSamples;
    statusUpdateCount = 0;

    for (int i = first; i < limit; i++) {
        unsigned long long
            user    = cpu[i][IWM_USER],
            nice    = cpu[i][IWM_NICE],
            sys     = cpu[i][IWM_SYS],
            idle    = cpu[i][IWM_IDLE],
            iowait  = cpu[i][IWM_IOWAIT],
            intr    = cpu[i][IWM_INTR],
            softirq = cpu[i][IWM_SOFTIRQ],
            steal   = cpu[i][IWM_STEAL],
            total   = user + sys + nice + idle + iowait + intr + softirq + steal;

        int y = height() - 1;

        if (total > 1) { /* better show 0 % CPU than nonsense on startup */
            unsigned long long
                intrbar, sysbar, nicebar, userbar,
                iowaitbar, softirqbar, stealbar, totalbar = 0,
                round = total / h / 2;  /* compute also with rounding errs */
            if ((stealbar = (h * (steal + round)) / total)) {
                g.setColor(color[IWM_STEAL]);
                g.drawLine(i, y, i, y - (stealbar - 1));
                y -= stealbar;
            }
            totalbar += stealbar;

            if ((intrbar = (h * (intr + round)) / total)) {
                g.setColor(color[IWM_INTR]);
                g.drawLine(i, y, i, y - (intrbar - 1));
                y -= intrbar;
            }
            totalbar += intrbar;
            if ((softirqbar = (h * (softirq + round)) / total)) {
                g.setColor(color[IWM_SOFTIRQ]);
                g.drawLine(i, y, i, y - (softirqbar - 1));
                y -= softirqbar;
            }
            totalbar += softirqbar;
            iowaitbar = (h * (iowait + round)) / total;
            totalbar += iowaitbar;
            if ((sysbar = (h * (sys + round)) / total)) {
                g.setColor(color[IWM_SYS]);
                g.drawLine(i, y, i, y - (sysbar - 1));
                y -= sysbar;
            }
            totalbar += sysbar;

            nicebar = (h * (nice + round)) / total;
            totalbar += nicebar;

            /* minor rounding errors are counted into user bar: */
            if ((userbar = (h * ((total - idle) + round)) / total - totalbar))
            {
                g.setColor(color[IWM_USER]);
                g.drawLine(i, y, i, y - (userbar - 1));
                y -= userbar;
            }

            if (nicebar) {
                g.setColor(color[IWM_NICE]);
                g.drawLine(i, y, i, y - (nicebar - 1));
                y -= nicebar;
            }
            if (iowaitbar) {
                g.setColor(color[IWM_IOWAIT]);
                g.drawLine(i, y, i, y - (iowaitbar - 1));
                y -= iowaitbar;
            }
         /* MSG((_("stat:\tuser = %llu, nice = %llu, sys = %llu, idle = %llu, "
                "iowait = %llu, intr = %llu, softirq = %llu, steal = %llu\n"),
                cpu[i][IWM_USER], cpu[i][IWM_NICE], cpu[i][IWM_SYS],
                cpu[i][IWM_IDLE], cpu[i][IWM_IOWAIT], cpu[i][IWM_INTR],
                cpu[i][IWM_SOFTIRQ], cpu[i][IWM_STEAL]));
            MSG((_("bars:\tuser = %llu, nice = %llu, sys = %llu, iowait = %llu, "
                "intr = %llu, softirq = %llu, steal = %llu (h = %i)\n"),
                userbar, nicebar, sysbar, iowaitbar, intrbar,
                softirqbar, stealbar, h)); */
        }
        if (y > 0) {
            if (color[IWM_IDLE]) {
                g.setColor(color[IWM_IDLE]);
                g.drawLine(i, 0, i, y);
            } else {
                ref<YImage> gradient(getGradient());

                if (gradient != null)
                    g.drawImage(gradient,
                                 this->x() + i, this->y(), width(), y + 1, i, 0);
                else
                    if (taskbackPixmap != null)
                        g.fillPixmap(taskbackPixmap,
                                     i, 0, width(), y + 1, this->x() + i, this->y());
            }
        }
    }
}

void CPUStatus::temperature(Graphics& g) {
    if (cpustatusShowAcpiTempInGraph) {
        char temp[10];
        getAcpiTemp(temp, sizeof(temp));
        g.setColor(fTempColor);
        if (tempFont == null)
            tempFont = YFont::getFont(XFA(tempFontName));
        g.setFont(tempFont);
        int h = height();
        int y = (h - 1 - tempFont->height()) / 2 + tempFont->ascent();
        int w = tempFont->textWidth(temp);
        int x = max(0, (int(g.rwidth()) - w) / 2);
        g.drawChars(temp, 0, 3, x, y);
    }
}

bool CPUStatus::handleTimer(YTimer *t) {
    if (t != fUpdateTimer)
        return false;
    if (toolTipVisible())
        updateToolTip();
    updateStatus();
    return true;
}

void CPUStatus::updateToolTip() {
    char cpuid[16] = "";
    if (fCpuID >= 0)
        snprintf(cpuid, sizeof(cpuid), "%d", fCpuID);
#ifdef __linux__
    char fmt[255] = "";
#define ___checkspace if (more<0 || rest-more<=0) return; pos+=more; rest-=more;
    struct sysinfo sys;

    if (0==sysinfo(&sys))
    {
        char *pos=fmt;
        int rest=sizeof(fmt);
        float l1 = float(sys.loads[0]) / 65536.0,
              l5 = float(sys.loads[1]) / 65536.0,
              l15 = float(sys.loads[2]) / 65536.0;
        int more=snprintf(pos, rest, _("CPU %s Load: %3.2f %3.2f %3.2f, %u"),
                cpuid, l1, l5, l15, (unsigned) sys.procs);
        ___checkspace;
        if (cpustatusShowRamUsage) {
#define MBnorm(x) ((float)x * (float)sys.mem_unit / 1048576.0f)
#define GBnorm(x) ((double)x * (double)sys.mem_unit / 1073741824.0)
            more=snprintf(pos, rest, _("\nRam (free): %5.3f (%.3f) G"),
                    GBnorm(sys.totalram), GBnorm(sys.freeram));
            ___checkspace;
        }
        if (cpustatusShowSwapUsage) {
            more=snprintf(pos, rest, _("\nSwap (free): %.3f (%.3f) G"),
                    GBnorm(sys.totalswap), GBnorm(sys.freeswap));
            ___checkspace;
        }
        if (cpustatusShowAcpiTemp) {
            char *posEx=pos;
            more=snprintf(pos, rest, _("\nACPI Temp: "));
            ___checkspace;
            more=getAcpiTemp(pos, rest);
            if (more)
            {
              ___checkspace;
            }
            else
               pos=posEx;
        }
        if (cpustatusShowCpuFreq) {
            float maxf = getCpuFreq(0), minf = maxf;
            int cpus;
            for (cpus = 1; cpus < 8; ++cpus) {
                float freq = getCpuFreq(cpus);
                if (freq < 1000) break;
                if (freq < minf) minf = freq;
                if (freq > maxf) maxf = freq;
            }
            const char *const form = _("\nCPU Freq: %.3fGHz");
            const char *const perc = strstr(form, "%.3f");
            if (cpus < 2 || perc == nullptr) {
                snprintf(pos, rest, form, maxf / 1e6);
            } else {
                snprintf(pos, rest, "%.*s%.3f %.3f %s", (int)(perc - form),
                        form, maxf / 1e6, minf / 1e6, perc + 4);
            }
        }
        setToolTip(mstring(fmt));
    }
#else
    char buf[99];
    double loadavg[3];
    if (getloadavg(loadavg, 3) < 0)
        return;
    snprintf(buf, sizeof buf, "%s %s %s %3.2g %3.2g %3.2g",
             _("CPU"), cpuid, _("Load: "),
            loadavg[0], loadavg[1], loadavg[2]);

#if HAVE_SYSCTL && defined(HW_PHYSMEM64) && defined(HW_USERMEM64)
    const int M = 2;
    int mib[M][2] = { { CTL_HW, HW_PHYSMEM64 }, { CTL_HW, HW_USERMEM64 }, };
    int64_t data[M] = { 0, 0, };
    size_t len[M] = { sizeof(data[0]), sizeof(data[1]), };

    for (int i = 0; i < M; ++i)
        if (sysctl(mib[i], 2, data + i, len + i, NULL, 0) == -1)
            if (ONCE) tlog("sysctl %d, %d", mib[i][0], mib[i][1]);

    if (data[0] && data[1]) {
        snprintf(buf + strlen(buf), sizeof buf - strlen(buf),
                _("\nRam (user): %5.3f (%.3f) G"),
                (data[0] >> 20) * 1e-3, (data[1] >> 20) * 1e-3);
    }
#endif

    float freq = getCpuFreq(0);
    if (freq > 1e6 && strlen(buf) + 20 < sizeof buf) {
        snprintf(buf + strlen(buf), sizeof buf - strlen(buf),
                 _("\nCPU Freq: %.3fGHz"), freq * 1e-9);
    }

    setToolTip(buf);
#endif
}

void CPUStatus::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 1) {
        if (cpuCommand && cpuCommand[0] &&
            (taskBarLaunchOnSingleClick ? count == 1 : !(count % 2)))
            fHandler->runCommandOnce(cpuClassHint, cpuCommand);
    }
    else if (up.button == Button3) {
        fHandler->handleClick(up, fCpuID);
    }
}

void CPUStatus::updateStatus() {
    for (int i(1); i < taskBarCPUSamples; i++)
        cpu.copyTo(i, i - 1);
    getStatus();
    repaint();
}

int CPUStatus::getAcpiTemp(char *tempbuf, int buflen) {
    int retbuflen = 0;
#if __linux__
    char namebuf[300];
    char buf[64];

    memset(tempbuf, 0, buflen);

    cdir dir;
    if (dir.open("/sys/class/thermal")) {
        while (dir.next()) {
            if (strncmp(dir.entry(), "thermal", 7))
                continue;

            snprintf(namebuf, sizeof namebuf,
                    "/sys/class/thermal/%s/temp", dir.entry());
            auto len = filereader(namebuf).read_all(BUFNSIZE(buf));
            if (len > 4) {
                int seglen = len - 4;
                if (retbuflen + seglen + 4 >= buflen) {
                    break;
                }
                strncat(tempbuf + retbuflen, buf, seglen);
                retbuflen += seglen;
                tempbuf[retbuflen++] = '.';
                tempbuf[retbuflen++] = buf[seglen];
                tempbuf[retbuflen++] = ' ';
                tempbuf[retbuflen] = '\0';
            }
        }
        if (1 < retbuflen && retbuflen + 1 < buflen) {
            // TRANSLATORS: Please translate the string "C" into "Celsius Temperature" in your language.
            // TRANSLATORS: Please make sure the translated string could be shown in your non-utf8 locale.
            static const char *T = _("°C");
            int i = -1;
            while (T[++i]) tempbuf[retbuflen++] = T[i];
            tempbuf[retbuflen] = '\0';
        }
    }
    else if (dir.open("/proc/acpi/thermal_zone")) {
        while (dir.next()) {
            const int seglen = 7;
            snprintf(namebuf, sizeof namebuf,
                    "/proc/acpi/thermal_zone/%s/temperature", dir.entry());
            auto len = filereader(namebuf).read_all(BUFNSIZE(buf));
            if (len > seglen) {
                if (retbuflen + seglen >= buflen) {
                    break;
                }
                retbuflen += seglen;
                strncat(tempbuf, buf + len - seglen, seglen);
            }
        }
    }
#endif

    return retbuflen;
}

float CPUStatus::getCpuFreq(unsigned int cpu) {

#if __linux__
    char buf[16], namebuf[100];
    const char * categories[] = { "cpuinfo", "scaling" };
    for (unsigned i = 0; i < ACOUNT(categories); ++i)
    {
        float cpufreq = 0;
        snprintf(BUFNSIZE(namebuf),
                "/sys/devices/system/cpu/cpu%d/cpufreq/%s_cur_freq",
                cpu, categories[i]);
        if (filereader(namebuf).read_all(BUFNSIZE(buf)) > 0) {
            sscanf(buf, "%f", &cpufreq);
            return cpufreq;
        }
    }

#elif HAVE_SYSCTL && defined(CTL_HW) && defined(HW_CPUSPEED) /*OpenBSD*/
    int mib[2] = { CTL_HW, HW_CPUSPEED };
    int speed = 0;
    size_t len = sizeof(speed);

    if (sysctl(mib, 2, &speed, &len, NULL, 0) == -1) {
        if (ONCE) tlog("sysctl hw cpuspeed");
    }
    else {
        return (float) speed * 1e6;
    }
#endif

    return 0;
}


void CPUStatus::getStatusPlatform() {
#ifdef __linux__
    char *p = nullptr, buf[4096], *end = nullptr;
    unsigned long long cur[IWM_STATES];
    int s;

    fileptr fd(fopen("/proc/stat", "r"));
    if (fd == nullptr)
        return;

    while (fgets(buf, sizeof buf, fd) && 0 == strncmp(buf, "cpu", 3)) {
        if (fCpuID == -1) {
            if (ASCII::isSpaceOrTab(buf[3]))
                p = buf + 4;
            break;
        }
        if (ASCII::isDigit(buf[3]) && strtol(buf + 3, &end, 10) == fCpuID) {
            if (end > buf && ASCII::isSpaceOrTab(*end)) {
                p = 1 + end;
                break;
            }
        }
    }
    fd.close();
    if (p == nullptr)
        return;

    s = sscanf(p, "%llu %llu %llu %llu %llu %llu %llu %llu",
               &cur[IWM_USER],    &cur[IWM_NICE],
               &cur[IWM_SYS],     &cur[IWM_IDLE],
               &cur[IWM_IOWAIT],  &cur[IWM_INTR],
               &cur[IWM_SOFTIRQ], &cur[IWM_STEAL]);
    switch (s) {
    case 4:
        /* Linux 2.4 */
        cur[IWM_INTR]    =
        cur[IWM_IOWAIT]  =
        cur[IWM_SOFTIRQ] = 0;
        /* FALL THROUGH */
    case 7:
        /* Linux < 2.6.11 */
        cur[IWM_STEAL] = 0;
        /* FALL THROUGH */
    case 8:
        break;
    default:
        return;
    }
    int i = 0;
    for (i = 0; i < IWM_STATES; i++) {
        cpu[taskBarCPUSamples - 1][i] = cur[i] - last_cpu[i];
        last_cpu[i] = cur[i];
    }

    return;

#elif __OpenBSD__ || __NetBSD__ || __FreeBSD__

#if defined __NetBSD__
    typedef u_int64_t cp_time_t;
#else
    typedef long cp_time_t;
#endif
#if defined KERN_CPTIME
    static int mib[] = { CTL_KERN, KERN_CPTIME };
#elif defined KERN_CP_TIME
    static int mib[] = { CTL_KERN, KERN_CP_TIME };
#else
    static int mib[] = { 0, 0 };
#endif

    cp_time_t cp_time[CPUSTATES];
    size_t len = sizeof( cp_time );
#if defined HAVE_SYSCTLBYNAME
    if (sysctlbyname("kern.cp_time", cp_time, &len, NULL, 0) < 0) {
        if (ONCE)
            fail("sysctlbyname kern.cp_time");
        return;
    }
#else
    if (sysctl(mib, 2, cp_time, &len, NULL, 0) < 0) {
        if (ONCE)
            fail("sysctl kern cp_time");
        return;
    }
#endif

    unsigned long long cur[IWM_STATES];
    cur[IWM_USER]    = cp_time[CP_USER];
    cur[IWM_NICE]    = cp_time[CP_NICE];
    cur[IWM_SYS]     = cp_time[CP_SYS];
    cur[IWM_INTR]    = cp_time[CP_INTR];
    cur[IWM_IOWAIT]  = 0;
    cur[IWM_SOFTIRQ] = 0;
    cur[IWM_IDLE]    = cp_time[CP_IDLE];
    cur[IWM_STEAL]   = 0;

    for (int i = 0; i < IWM_STATES; i++) {
        cpu[taskBarCPUSamples - 1][i] = cur[i] - last_cpu[i];
        last_cpu[i] = cur[i];
    }
#endif
}

void CPUStatus::getStatus() {
    cpu.clear(taskBarCPUSamples - 1);

    getStatusPlatform();

 /* MSG((_("CPU: %llu %llu %llu %llu %llu %llu %llu %llu"),
        cpu[taskBarCPUSamples - 1][IWM_USER],
        cpu[taskBarCPUSamples - 1][IWM_NICE],
        cpu[taskBarCPUSamples - 1][IWM_SYS],
        cpu[taskBarCPUSamples - 1][IWM_IDLE],
        cpu[taskBarCPUSamples - 1][IWM_IOWAIT],
        cpu[taskBarCPUSamples - 1][IWM_INTR],
        cpu[taskBarCPUSamples - 1][IWM_SOFTIRQ],
        cpu[taskBarCPUSamples - 1][IWM_STEAL])); */

    ++statusUpdateCount;

    int last = taskBarCPUSamples - 1;
    bool same = 0 < last && 0 == cpu.compare(last, last - 1);
    unchanged = same ? 1 + unchanged : 0;
}

CPUStatusControl::CPUStatusControl(YSMListener *smActionListener,
                                   IAppletContainer *iapp,
                                   YWindow *aParent):
    smActionListener(smActionListener),
    iapp(iapp),
    aParent(aParent),
    fMenuCPU(-1),
    fPid(0)
{
    GetCPUStatus(cpuCombine);
}

void CPUStatusControl::GetCPUStatus(bool combine) {
    int count = 0;
#if __linux__
    if (combine == false) {
        fileptr fd("/proc/stat", "r");
        if (fd) {
            char buf[128];
            while (fgets(buf, sizeof buf, fd) && 0 == memcmp(buf, "cpu", 3))
                count += ASCII::isDigit(buf[3]);
        }
    }
#endif
    do {
        fCPUStatus += new CPUStatus(aParent, this, --count);
    } while (0 < count);
}

void CPUStatusControl::runCommandOnce(const char *resource, const char *cmdline)
{
    smActionListener->runCommandOnce(resource, cmdline, &fPid);
}

void CPUStatusControl::handleClick(const XButtonEvent &up, int cpuid) {
    if (up.button == Button3) {
        fMenu = new YMenu();
        fMenu->setActionListener(this);

        char title[24];
        if (cpuid < 0) strlcpy(title, _("CPU"), sizeof title);
        else snprintf(title, sizeof title, _("CPU%d"), cpuid);
        fMenu->addItem(title, -2, null, actionNull)->setEnabled(false);
        fMenu->addSeparator();

        fMenu->addItem(_("_Disable"), -2, null, actionClose);
        if (cpuid >= 0) {
            fMenu->addItem(_("_Combine"), -2, null, actionArrange);
        } else {
            fMenu->addItem(_("_Separate"), -2, null, actionCascade);
        }
        fMenuCPU = cpuid;
        fMenu->popup(nullptr, nullptr, nullptr, up.x_root, up.y_root,
                     YPopupWindow::pfCanFlipVertical |
                     YPopupWindow::pfCanFlipHorizontal |
                     YPopupWindow::pfPopupMenu);
    }
}

void CPUStatusControl::actionPerformed(YAction action, unsigned int modifiers) {
    if (action == actionClose) {
        for (IterType iter = getIterator(); ++iter; ) {
            if (iter->getCpuID() == fMenuCPU) {
                iter.remove();
                iapp->relayout();
                break;
            }
        }
    }
    else if (action == actionArrange) {
        fCPUStatus.clear();
        GetCPUStatus(true);
        iapp->relayout();
    }
    else if (action == actionCascade) {
        fCPUStatus.clear();
        GetCPUStatus(false);
        iapp->relayout();
    }
}

#endif

// vim: set sw=4 ts=4 et:
