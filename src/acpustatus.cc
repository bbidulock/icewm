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

#ifdef CONFIG_APPLET_CPU_STATUS
#include "ylib.h"
#include "wmapp.h"

#include "acpustatus.h"
#include "sysdep.h"
#include "default.h"

#if defined(linux)
//#include <linux/kernel.h>
#include <sys/sysinfo.h>
#endif
#if defined(sun) && defined(SVR4)
#include <sys/loadavg.h>
#endif
#ifdef HAVE_KSTAT_H
#include <sys/resource.h>
#include <kstat.h>
#include <sys/sysinfo.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#ifdef HAVE_UVM_UVM_PARAM_H
#include <uvm/uvm_param.h>
#endif
#ifdef HAVE_SYS_DKSTAT_H
#include <sys/dkstat.h>
#endif
#ifdef HAVE_SCHED_H
#include <sched.h>
#endif
#include <dirent.h>
#include "intl.h"

static int read_file(const char *filename, char *buf, size_t buflen)
{
    int fd, len = -1;
    fd = open(filename, O_RDONLY);
    if (fd >= 0) {
        len = read(fd, buf, buflen - 1);
        if (len >= 0) {
            buf[len] = '\0';
        }
        close(fd);
    }
    return len;
}

#if (defined(linux) || defined(HAVE_KSTAT_H)) || defined(HAVE_SYSCTL_CP_TIME)

extern ref<YPixmap> taskbackPixmap;

ref<YFont> CPUStatus::tempFont;

CPUStatus::CPUStatus(
    YSMListener *smActionListener,
    YWindow *aParent,
    bool cpustatusShowRamUsage,
    bool cpustatusShowSwapUsage,
    bool cpustatusShowAcpiTemp,
    bool cpustatusShowCpuFreq,
    bool cpustatusShowAcpiTempInGraph): YWindow(aParent),
    		m_nCachedFd(-1)
{
    this->smActionListener = smActionListener;
    cpu = new int *[taskBarCPUSamples];

    for (int a(0); a < taskBarCPUSamples; a++)
        cpu[a] = new int[IWM_STATES];

    fUpdateTimer = new YTimer(taskBarCPUDelay);
    if (fUpdateTimer) {
        fUpdateTimer->setTimerListener(this);
        fUpdateTimer->startTimer();
    }

    if (tempFont == null)
        tempFont = YFont::getFont(XFA(tempFontName));

    tempColor = new YColor(clrCpuTemp);

    color[IWM_USER] = new YColor(clrCpuUser);
    color[IWM_NICE] = new YColor(clrCpuNice);
    color[IWM_SYS]  = new YColor(clrCpuSys);
    color[IWM_INTR] = new YColor(clrCpuIntr);
    color[IWM_IOWAIT] = new YColor(clrCpuIoWait);
    color[IWM_SOFTIRQ] = new YColor(clrCpuSoftIrq);
    color[IWM_IDLE] = *clrCpuIdle
        ? new YColor(clrCpuIdle) : NULL;
    for (int i = 0; i < taskBarCPUSamples; i++) {
        cpu[i][IWM_USER] = cpu[i][IWM_NICE] =
        cpu[i][IWM_SYS] = cpu[i][IWM_INTR] =
            cpu[i][IWM_IOWAIT] = cpu[i][IWM_SOFTIRQ] = 0;
        cpu[i][IWM_IDLE] = 1;
    }
    setSize(taskBarCPUSamples, 20);
    last_cpu[IWM_USER] = last_cpu[IWM_NICE] = last_cpu[IWM_SYS] =
    last_cpu[IWM_IDLE] = last_cpu[IWM_INTR] =
    last_cpu[IWM_IOWAIT] = last_cpu[IWM_SOFTIRQ] = 0;
    ShowRamUsage = cpustatusShowRamUsage;
    ShowSwapUsage = cpustatusShowSwapUsage;
    ShowAcpiTemp = cpustatusShowAcpiTemp;
    ShowCpuFreq = cpustatusShowCpuFreq;
    ShowAcpiTempInGraph = cpustatusShowAcpiTempInGraph;
    getStatus();
    updateStatus();
    updateToolTip();
}

CPUStatus::~CPUStatus() {
    for (int a(0); a < taskBarCPUSamples; a++) {
        delete cpu[a]; cpu[a] = 0;
    }
    delete cpu; cpu = 0;
    delete color[IWM_USER]; color[IWM_USER] = 0;
    delete color[IWM_NICE]; color[IWM_NICE] = 0;
    delete color[IWM_SYS];  color[IWM_SYS]  = 0;
    delete color[IWM_IDLE]; color[IWM_IDLE] = 0;
    delete color[IWM_INTR]; color[IWM_INTR] = 0;
    delete color[IWM_IOWAIT]; color[IWM_IOWAIT] = 0;
    delete color[IWM_SOFTIRQ]; color[IWM_SOFTIRQ] = 0;
    delete tempColor;
    if(m_nCachedFd>=0)
    	close(m_nCachedFd);
}

void CPUStatus::paint(Graphics &g, const YRect &/*r*/) {
    int h = height();

    for (int i(0); i < taskBarCPUSamples; i++) {
        int user = cpu[i][IWM_USER];
        int nice = cpu[i][IWM_NICE];
        int sys = cpu[i][IWM_SYS];
        int idle = cpu[i][IWM_IDLE];
        int intr = cpu[i][IWM_INTR];
        int iowait = cpu[i][IWM_IOWAIT];
        int softirq = cpu[i][IWM_SOFTIRQ];
        int total = user + sys + intr + nice + idle + iowait + softirq;

        int y = height() - 1;

        if (total > 1) { /* better show 0 % CPU than nonsense on startup */
            int intrbar, sysbar, nicebar, userbar, iowaitbar, softirqbar;
            int round = total / h / 2;  /* compute also with rounding errs */

            if ((intrbar = (h * (intr + round)) / total)) {
                g.setColor(color[IWM_INTR]);
                g.drawLine(i, y, i, y - (intrbar - 1));
                y -= intrbar;
            }
            if ((softirqbar = (h * (softirq + round)) / total)) {
                g.setColor(color[IWM_SOFTIRQ]);
                g.drawLine(i, y, i, y - (softirqbar - 1));
                y -= softirqbar;
            }
            iowaitbar = (h * (iowait + round)) / total;
            if ((sysbar = (h * (sys + round)) / total)) {
                g.setColor(color[IWM_SYS]);
                g.drawLine(i, y, i, y - (sysbar - 1));
                y -= sysbar;
            }

            nicebar = (h * (nice + round)) / total;

            /* minor rounding errors are counted into user bar: */
            if ((userbar = (h * ((sys + nice + user + intr + iowait + softirq) +
                                 round)) / total -
                           (sysbar + nicebar + intrbar + iowaitbar + softirqbar)
                ))
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
#if 0
            msg("stat:\tuser = %i, nice = %i, sys = %i, idle = %i", cpu[i][IWM_USER], cpu[i][IWM_NICE], cpu[i][IWM_SYS], cpu[i][IWM_IDLE]);
            msg("bars:\tuser = %i, nice = %i, sys = %i (h = %i)\n", userbar, nicebar, sysbar, h);
#endif
        }
        if (y > 0) {
            if (color[IWM_IDLE]) {
                g.setColor(color[IWM_IDLE]);
                g.drawLine(i, 0, i, y);
            } else {
#ifdef CONFIG_GRADIENTS
                ref<YImage> gradient = parent()->getGradient();

                if (gradient != null)
                    g.drawImage(gradient,
                                 this->x() + i, this->y(), width(), y + 1, i, 0);
                else
#endif
                    if (taskbackPixmap != null)
                        g.fillPixmap(taskbackPixmap,
                                     i, 0, width(), y + 1, this->x() + i, this->y());
            }
        }
    }

    if (ShowAcpiTempInGraph) {
        char test[10];
        getAcpiTemp(test, sizeof(test));
        g.setColor(tempColor);
        g.setFont(tempFont);
        int y =  (h - 1 - tempFont->height()) / 2 + tempFont->ascent();
        // If we draw three characters we can get temperatures above 100
        // without including the "C".
        g.drawChars(test, 0, 3, 2, y);
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
#ifdef linux
    char fmt[255] = "";
#define ___checkspace if(more<0 || rest-more<=0) return; pos+=more; rest-=more;
    struct sysinfo sys;

    if(0==sysinfo(&sys))
    {
        char *pos=fmt;
        int rest=sizeof(fmt);
        float l1 = float(sys.loads[0]) / 65536.0,
              l5 = float(sys.loads[1]) / 65536.0,
              l15 = float(sys.loads[2]) / 65536.0;
        int more=snprintf(pos, rest, _("CPU Load: %3.2f %3.2f %3.2f, %u"),
                l1, l5, l15, (unsigned) sys.procs);
        ___checkspace;
        if (ShowRamUsage) {
#define MBnorm(x) ((float)x * (float)sys.mem_unit / 1048576.0f)
            more=snprintf(pos, rest, _("\nRam (free): %5.2f (%.2f) M"),
                    MBnorm(sys.totalram), MBnorm(sys.freeram));
            ___checkspace;
        }
        if (ShowSwapUsage) {
            more=snprintf(pos, rest, _("\nSwap (free): %.2f (%.2f) M"),
                    MBnorm(sys.totalswap), MBnorm(sys.freeswap));
            ___checkspace;
        }
        if (ShowAcpiTemp) {
            char *posEx=pos;
            more=snprintf(pos, rest, _("\nACPI Temp: "));
            ___checkspace;
            more=getAcpiTemp(pos, rest);
            if(more)
            {
              ___checkspace;
            }
            else
               pos=posEx;
        }
        if (ShowCpuFreq) {
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
            if (cpus < 2 || perc == NULL) {
                more=snprintf(pos, rest, form, maxf / 1e6);
            } else {
                more=snprintf(pos, rest, "%.*s%.3f %.3f %s", perc - form,
                        form, maxf / 1e6, minf / 1e6, perc + 4);
            }
        }
        setToolTip(ustring(fmt));
    }
#elif defined HAVE_GETLOADAVG2
    char load[sizeof("999.99 999.99 999.99")];
    double loadavg[3];
    if (getloadavg(loadavg, 3) < 0)
        return;
    snprintf(load, sizeof(load), "%3.2g %3.2g %3.2g",
            loadavg[0], loadavg[1], loadavg[2]);
    {
        char *loadmsg = cstrJoin(_("CPU Load: "), load, NULL);
        setToolTip(ustring(loadmsg));
        delete [] loadmsg;
    }
#endif
}

void CPUStatus::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 1) {
        if (cpuCommand && cpuCommand[0] &&
            (taskBarLaunchOnSingleClick ? count == 1 : !(count % 2)))
            smActionListener->runCommandOnce(cpuClassHint, cpuCommand);
    }
}

void CPUStatus::updateStatus() {
    for (int i(1); i < taskBarCPUSamples; i++) {
        cpu[i - 1][IWM_USER] = cpu[i][IWM_USER];
        cpu[i - 1][IWM_NICE] = cpu[i][IWM_NICE];
        cpu[i - 1][IWM_SYS]  = cpu[i][IWM_SYS];
        cpu[i - 1][IWM_IDLE] = cpu[i][IWM_IDLE];
        cpu[i - 1][IWM_INTR] = cpu[i][IWM_INTR];
        cpu[i - 1][IWM_IOWAIT] = cpu[i][IWM_IOWAIT];
        cpu[i - 1][IWM_SOFTIRQ] = cpu[i][IWM_SOFTIRQ];
    }
    getStatus(),
    repaint();
}

int CPUStatus::getAcpiTemp(char *tempbuf, int buflen) {
    int retbuflen = 0;
    char namebuf[64];
    char buf[64];

    memset(tempbuf, 0, buflen);
    DIR *dir;
    if ((dir = opendir("/proc/acpi/thermal_zone")) != NULL) {
        struct dirent *de;

        while ((de = readdir(dir)) != NULL) {
            int len, seglen = 7;

            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
                continue;
 
            sprintf(namebuf, "/proc/acpi/thermal_zone/%s/temperature", de->d_name);
            len = read_file(namebuf, buf, sizeof(buf));
            if (len > seglen) {
                if (retbuflen + seglen >= buflen) {
                    break;
                }
                retbuflen += seglen;
                strncat(tempbuf, buf + len - seglen, seglen);
            }
        }
        closedir(dir);
    } 
    else if ((dir = opendir("/sys/class/thermal")) != NULL) {
        struct dirent *de;

        while ((de = readdir(dir)) != NULL) {
            int len;

            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
                continue;

            sprintf(namebuf, "/sys/class/thermal/%s/temp", de->d_name);
            len = read_file(namebuf, buf, sizeof(buf));
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
        closedir(dir);
        if (1 < retbuflen && retbuflen + 1 < buflen) {
            tempbuf[retbuflen++] = '\xB0';
            tempbuf[retbuflen++] = 'C';
            tempbuf[retbuflen] = '\0';
        }
    }
    return retbuflen;
}

float CPUStatus::getCpuFreq(unsigned int cpu) {
    char buf[16], namebuf[64];
    const char * categories[] = { "cpuinfo", "scaling" };
    for(unsigned i = 0; i < ACOUNT(categories); ++i)
    {
        float cpufreq = 0;
        sprintf(namebuf, "/sys/devices/system/cpu/cpu%d/cpufreq/%s_cur_freq",
                cpu, categories[i]);
        if (read_file(namebuf, buf, sizeof(buf)) > 0) {
            sscanf(buf, "%f", &cpufreq);
            return cpufreq;
        }
    }
    return 0;
}


void CPUStatus::getStatus() {
#ifdef linux
    char *p, buf[4096];
    unsigned long long cur[IWM_STATES];
    int len, s;
    int &fd = m_nCachedFd;
    if (fd < 0) {
    	fd = open("/proc/stat", O_RDONLY);
        if (fd < 0)
            return;
        fcntl(fd, F_SETFD, FD_CLOEXEC);
    }
    else
    	lseek(fd, 0L, SEEK_SET);

    cpu[taskBarCPUSamples - 1][IWM_USER] = 0;
    cpu[taskBarCPUSamples - 1][IWM_NICE] = 0;
    cpu[taskBarCPUSamples - 1][IWM_INTR] = 0;
    cpu[taskBarCPUSamples - 1][IWM_IOWAIT] = 0;
    cpu[taskBarCPUSamples - 1][IWM_SOFTIRQ] = 0;
    cpu[taskBarCPUSamples - 1][IWM_SYS] = 0;
    cpu[taskBarCPUSamples - 1][IWM_IDLE] = 0;

    len = read(fd, buf, sizeof(buf) - 1);
    if (len < 0) {
        close(fd);
        fd = -1;
        return;
    }
    buf[len] = 0;

    p = buf;
    while (*p && (*p < '0' || *p > '9'))
        p++;
    /* Linux 2.4:  cpu  3557 8 1052 7049
     * Linux 2.6:  cpu  3537 44 1064 676229 7792 142 5
     */
    if ((s = strcspn(p, "\n")) <= 60)
        strcpy(p + s, " 0 0 0\n");   /* Linux 2.4 */

    int i = 0;
    for (i = 0; i < 7; i++) {
        int d = -1;

        /*  linux/Documentation/filesystems/proc.txt: 1.8  */
        switch (i) {
        case 0: d = IWM_USER; break;
        case 1: d = IWM_NICE; break;
        case 2: d = IWM_SYS; break;
        case 3: d = IWM_IDLE; break;
        case 4: d = IWM_IOWAIT; break;
        case 5: d = IWM_INTR; break;
        case 6: d = IWM_SOFTIRQ; break;
        }
        cur[d] = strtoll(p, &p, 10);
        cpu[taskBarCPUSamples - 1][d] = (int)(cur[d] - last_cpu[d]);
        last_cpu[d] = cur[d];
    }
#if 0
    msg("cpu: %d %d %d %d %d %d %d",
        cpu[taskBarCPUSamples-1][IWM_USER],
        cpu[taskBarCPUSamples-1][IWM_NICE],
        cpu[taskBarCPUSamples-1][IWM_SYS],
        cpu[taskBarCPUSamples-1][IWM_IDLE],
        cpu[taskBarCPUSamples-1][IWM_IOWAIT],
        cpu[taskBarCPUSamples-1][IWM_INTR],
        cpu[taskBarCPUSamples-1][IWM_SOFTIRQ]);
#endif
#endif /* linux */
#ifdef HAVE_KSTAT_H
#ifdef HAVE_OLD_KSTAT
#define ui32 ul
#endif
    static kstat_ctl_t  *kc = NULL;
    static kid_t        kcid;
    kid_t               new_kcid;
    kstat_t             *ks = NULL;
    kstat_named_t       *kn = NULL;
    int                 changed,change,total_change;
    unsigned int        thiscpu;
    register int        i,j;
    static unsigned int ncpus;
    static kstat_t      **cpu_ks=NULL;
    static cpu_stat_t   *cpu_stat=NULL;
    static long         cp_old[CPU_STATES];
    long                cp_time[CPU_STATES], cp_pct[CPU_STATES];

    /* Initialize the kstat */
    if (!kc) {
        kc = kstat_open();
        if (!kc) {
            perror("kstat_open ");
            return;/* FIXME : need err handler? */
        }
        changed = 1;
        kcid = kc->kc_chain_id;
        fcntl(kc->kc_kd, F_SETFD, FD_CLOEXEC);
    } else {
        changed = 0;
    }
    /* Fetch the kstat data. Whenever we detect that the kstat has been
     changed by the kernel, we 'continue' and restart this loop.
     Otherwise, we break out at the end. */
    while (1) {
        new_kcid = kstat_chain_update(kc);
        if (new_kcid) {
            changed = 1;
            kcid = new_kcid;
        }
        if (new_kcid < 0) {
            perror("kstat_chain_update ");
            return;/* FIXME : need err handler? */
        }
        if (new_kcid != 0)
            continue; /* kstat changed - start over */
        ks = kstat_lookup(kc, "unix", 0, "system_misc");
        if (kstat_read(kc, ks, 0) == -1) {
            perror("kstat_read ");
            return;/* FIXME : need err handler? */
        }
        if (changed) {
            /* the kstat has changed - reread the data */
            thiscpu = 0; ncpus = 0;
            kn = (kstat_named_t *)kstat_data_lookup(ks, "ncpus");
            if ((kn) && (kn->value.ui32 > ncpus)) {
                /* I guess I should be using 'new' here... FIXME */
                ncpus = kn->value.ui32;
                if ((cpu_ks = (kstat_t **)
                     realloc(cpu_ks, ncpus * sizeof(kstat_t *))) == NULL)
                {
                    perror("realloc: cpu_ks ");
                    return;/* FIXME : need err handler? */
                }
                if ((cpu_stat = (cpu_stat_t *)
                     realloc(cpu_stat, ncpus * sizeof(cpu_stat_t))) == NULL)
                {
                    perror("realloc: cpu_stat ");
                    return;/* FIXME : need err handler? */
                }
            }
            for (ks = kc->kc_chain; ks; ks = ks->ks_next) {
                if (strncmp(ks->ks_name, "cpu_stat", 8) == 0) {
                    new_kcid = kstat_read(kc, ks, NULL);
                    if (new_kcid < 0) {
                        perror("kstat_read ");
                        return;/* FIXME : need err handler? */
                    }
                    if (new_kcid != kcid)
                        break;
                    cpu_ks[thiscpu] = ks;
                    thiscpu++;
                    if (thiscpu > ncpus) {
                        warn("kstat finds too many cpus: should be %d",
                             ncpus);
                        return;/* FIXME : need err handler? */
                    }
                }
            }
            if (new_kcid != kcid)
                continue;
            ncpus = thiscpu;
            changed = 0;
        }
        for (i = 0; i<(int)ncpus; i++) {
            new_kcid = kstat_read(kc, cpu_ks[i], &cpu_stat[i]);
            if (new_kcid < 0) {
                perror("kstat_read ");
                return;/* FIXME : need err handler? */
            }
        }
        if (new_kcid != kcid)
            continue; /* kstat changed - start over */
        else
            break;
    } /* while (1) */

    /* Initialize the cp_time array */
    for (i = 0; i < CPU_STATES; i++)
        cp_time[i] = 0L;
    for (i = 0; i < (int)ncpus; i++) {
        for (j = 0; j < CPU_STATES; j++)
            cp_time[j] += (long) cpu_stat[i].cpu_sysinfo.cpu[j];
    }
    /* calculate the percent utilization for each category */
    /* cpu_state calculations */
    total_change = 0;
    for (i = 0; i < CPU_STATES; i++) {
        change = cp_time[i] - cp_old[i];
        if (change < 0) /* The counter rolled over */
            change = (int) ((unsigned long)cp_time[i] - (unsigned long)cp_old[i]);
        cp_pct[i] = change;
        total_change += change;
        cp_old[i] = cp_time[i]; /* copy the data for the next run */
    }
    /* this percent calculation isn't really needed, since the repaint
     routine takes care of this... */
    for (i = 0; i < CPU_STATES; i++)
        cp_pct[i] =
            ((total_change > 0) ?
             ((int)(((1000.0 * cp_pct[i]) / total_change) + 0.5)) :
             ((i == CPU_IDLE) ? (1000) : (0)));

    /* OK, we've got the data. Now copy it to cpu[][] */
    cpu[taskBarCPUSamples-1][IWM_USER] = cp_pct[CPU_USER];
    cpu[taskBarCPUSamples-1][IWM_NICE] = cp_pct[CPU_WAIT];
    cpu[taskBarCPUSamples-1][IWM_SYS]  = cp_pct[CPU_KERNEL];
    cpu[taskBarCPUSamples-1][IWM_IDLE] = cp_pct[CPU_IDLE];

#endif /* have_kstat_h */
#if defined HAVE_SYSCTL_CP_TIME && defined CP_INTR
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

    cpu[taskBarCPUSamples - 1][IWM_USER] = 0;
    cpu[taskBarCPUSamples - 1][IWM_NICE] = 0;
    cpu[taskBarCPUSamples - 1][IWM_SYS] = 0;
    cpu[taskBarCPUSamples - 1][IWM_INTR] = 0;
    cpu[taskBarCPUSamples - 1][IWM_IOWAIT] = 0;
    cpu[taskBarCPUSamples - 1][IWM_SOFTIRQ] = 0;
    cpu[taskBarCPUSamples - 1][IWM_IDLE] = 0;


    cp_time_t cp_time[CPUSTATES];
    size_t len = sizeof( cp_time );
#if defined HAVE_SYSCTLBYNAME
    if (sysctlbyname("kern.cp_time", cp_time, &len, NULL, 0) < 0)
        return;
#else
    if (sysctl(mib, 2, cp_time, &len, NULL, 0) < 0)
        return;
#endif

    unsigned long long cur[IWM_STATES];
    cur[IWM_USER] = cp_time[CP_USER];
    cur[IWM_NICE] = cp_time[CP_NICE];
    cur[IWM_SYS] = cp_time[CP_SYS];
    cur[IWM_INTR] = cp_time[CP_INTR];
    cur[IWM_IOWAIT] = 0;
    cur[IWM_SOFTIRQ] = 0;
    cur[IWM_IDLE] = cp_time[CP_IDLE];

    for (int i = 0; i < IWM_STATES; i++) {
        cpu[taskBarCPUSamples - 1][i] = cur[i] - last_cpu[i];
        last_cpu[i] = cur[i];
    }
#endif
}
#endif
#endif

