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

CPUStatus::CPUStatus(YSMListener *smActionListener, YWindow *aParent, int cpuid) : YWindow(aParent)
{
    fCpuID = cpuid;
    this->smActionListener = smActionListener;
    cpu = new unsigned long long *[taskBarCPUSamples];
    for (int a(0); a < taskBarCPUSamples; a++) {
        cpu[a] = new unsigned long long[IWM_STATES];
        memset(cpu[a], 0, IWM_STATES * sizeof(cpu[0][0]));
        cpu[a][IWM_IDLE] = 1;
    }
    memset(last_cpu, 0, sizeof(last_cpu));

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
    color[IWM_STEAL] = new YColor(clrCpuSteal);
    setSize(taskBarCPUSamples, 20);
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
    delete color[IWM_STEAL]; color[IWM_STEAL] = 0;
    delete tempColor;
}

void CPUStatus::paint(Graphics &g, const YRect &/*r*/) {
    int h = height();

    for (int i(0); i < taskBarCPUSamples; i++) {
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
            MSG((_("stat:\tuser = %llu, nice = %llu, sys = %llu, idle = %llu, "
                "iowait = %llu, intr = %llu, softirq = %llu, steal = %llu\n"),
                cpu[i][IWM_USER], cpu[i][IWM_NICE], cpu[i][IWM_SYS],
                cpu[i][IWM_IDLE], cpu[i][IWM_IOWAIT], cpu[i][IWM_INTR],
                cpu[i][IWM_SOFTIRQ], cpu[i][IWM_STEAL]));
            MSG((_("bars:\tuser = %llu, nice = %llu, sys = %llu, iowait = %llu, "
                "intr = %llu, softirq = %llu, steal = %llu (h = %i)\n"),
                userbar, nicebar, sysbar, iowaitbar, intrbar,
                softirqbar, stealbar, h));
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
    char cpuid[16] = "";
    if (fCpuID >= 0)
        snprintf(cpuid, sizeof(cpuid), "%d", fCpuID);
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
        int more=snprintf(pos, rest, _("CPU %s Load: %3.2f %3.2f %3.2f, %u"),
                cpuid, l1, l5, l15, (unsigned) sys.procs);
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
                more=snprintf(pos, rest, "%.*s%.3f %.3f %s", (int)(perc - form),
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
        char id[10];
        snprintf(id, sizeof[id], " %d ", cpuid);
        char *loadmsg = cstrJoin(_("CPU"), id ,_("Load: "), load, NULL);
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
    for (int i(1); i < taskBarCPUSamples; i++)
        memcpy(cpu[i - 1], cpu[i], IWM_STATES * sizeof(cpu[0][0]));
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
            // TRANSLATORS: Please translate the string "C" into "Celsius Temperature" in your language, like "°C"
            // TRANSLATORS: Please make sure the translated string could be shown in your non-utf8 locale.
            static const char *T = _("C");
            int i = -1;
            while (T[++i]) tempbuf[retbuflen++] = T[i];
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
    memset(cpu[taskBarCPUSamples - 1], 0, IWM_STATES * sizeof(cpu[0][0]));
#ifdef linux
    char *p, buf[4096], *tok;
    unsigned long long cur[IWM_STATES];
    int s;

    char cpuname[32] = "cpu";
    if (fCpuID >= 0)
        snprintf(cpuname, sizeof(cpuname), "cpu%d", fCpuID);

    FILE *fd = fopen("/proc/stat", "r");
    if (fd == NULL)
    {
        fclose(fd);
        return;
    }

    /* find the line that starts with `cpuname` */
    do {
        if (!fgets(buf, sizeof(buf) - 1, fd)) {
            fclose(fd);
            return;
        }
        tok = strtok_r(buf, " \t", &p);
        if (!tok) {
            fclose(fd);
            return;
        }
    } while (strcmp(tok, cpuname));
    fclose(fd);
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
    unsigned long long  changed,change,total_change;
    unsigned int        thiscpu;
    register int        i,j;
    static unsigned int ncpus;
    static kstat_t      **cpu_ks=NULL;
    kstat_t             **cpu_ks_new;
    static cpu_stat_t   *cpu_stat=NULL;
    cpu_stat_t          *cpu_stat_new;
    static unsigned long long cp_old[CPU_STATES];
    unsigned long long  cp_time[CPU_STATES], cp_pct[CPU_STATES];

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
                if ((cpu_ks_new = (kstat_t **)
                     realloc(cpu_ks, ncpus * sizeof(kstat_t *))) == NULL)
                {
                    if (cpu_ks)
                        free(cpu_ks);
                    perror("realloc: cpu_ks ");
                    return;/* FIXME : need err handler? */
                }
                cpu_ks = cpu_ks_new;
                if ((cpu_stat_new = (cpu_stat_t *)
                     realloc(cpu_stat, ncpus * sizeof(cpu_stat_t))) == NULL)
                {
                    if (cpu_stat)
                        free(cpu_stat);
                    perror("realloc: cpu_stat ");
                    return;/* FIXME : need err handler? */
                }
                cpu_stat = cpu_stat_new;
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
        if (fCpuID >= 0 && fCpuID < ncpus) {
            new_kcid = kstat_read(kc, cpu_ks[fCpuID], &cpu_stat[fCpuID]);
            if (new_kcid < 0) {
                perror("kstat_read ");
                return;/* FIXME : need err handler? */
            }
        } else {
            for (i = 0; i<(int)ncpus; i++) {
                new_kcid = kstat_read(kc, cpu_ks[i], &cpu_stat[i]);
                if (new_kcid < 0) {
                    perror("kstat_read ");
                    return;/* FIXME : need err handler? */
                }
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
    if (fCpuID >= 0 && fCpuID < ncpus) {
        for (j = 0; j < CPU_STATES; j++)
            cp_time[j] += (long) cpu_stat[fCpuID].cpu_sysinfo.cpu[j];
    } else {
        for (i = 0; i < (int)ncpus; i++) {
            for (j = 0; j < CPU_STATES; j++)
                cp_time[j] += (long) cpu_stat[i].cpu_sysinfo.cpu[j];
        }
    }
    /* calculate the percent utilization for each category */
    /* cpu_state calculations */
    total_change = 0;
    for (i = 0; i < CPU_STATES; i++) {
        change = cp_time[i] - cp_old[i];
        if (change < 0) /* The counter rolled over */
            change = (unsigned long long) ((unsigned long long)cp_time[i] - (unsigned long long)cp_old[i]);
        cp_pct[i] = change;
        total_change += change;
        cp_old[i] = cp_time[i]; /* copy the data for the next run */
    }
    /* this percent calculation isn't really needed, since the repaint
     routine takes care of this... */
    for (i = 0; i < CPU_STATES; i++)
        cp_pct[i] =
            ((total_change > 0) ?
             ((unsigned long long)(((1000.0 * (float)cp_pct[i]) / total_change) + 0.5)) :
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
    cur[IWM_USER]    = cp_time[CP_USER];
    cur[IWM_NICE]    = cp_time[CP_NICE];
    cur[IWM_SYS]     = cp_time[CP_SYS];
    cur[IWM_IDLE]    = cp_time[CP_IDLE];
    cur[IWM_IOWAIT]  = 0;
    cur[IWM_INTR]    = cp_time[CP_INTR];
    cur[IWM_SOFTIRQ] = 0;
    cur[IWM_IDLE]    = cp_time[CP_IDLE];
    cur[IWM_STEAL]   = 0;

    for (int i = 0; i < IWM_STATES; i++) {
        cpu[taskBarCPUSamples - 1][i] = cur[i] - last_cpu[i];
        last_cpu[i] = cur[i];
    }
#endif
    MSG((_("%s: %llu %llu %llu %llu %llu %llu %llu %llu"),
        cpuname,
        cpu[taskBarCPUSamples - 1][IWM_USER],
        cpu[taskBarCPUSamples - 1][IWM_NICE],
        cpu[taskBarCPUSamples - 1][IWM_SYS],
        cpu[taskBarCPUSamples - 1][IWM_IDLE],
        cpu[taskBarCPUSamples - 1][IWM_IOWAIT],
        cpu[taskBarCPUSamples - 1][IWM_INTR],
        cpu[taskBarCPUSamples - 1][IWM_SOFTIRQ],
        cpu[taskBarCPUSamples - 1][IWM_STEAL]));
}
void CPUStatus::GetCPUStatus(YSMListener *smActionListener, YWindow *aParent, CPUStatus **&fCPUStatus, bool combine) {
    if (combine) {
        CPUStatus::getCPUStatusCombined(smActionListener, aParent, fCPUStatus);
        return;
    }
#if defined(linux)
    char buf[128];
    unsigned cnt = 0;
    FILE *fd = fopen("/proc/stat", "r");
    if (!fd) {
        CPUStatus::getCPUStatusCombined(smActionListener, aParent, fCPUStatus);
        return;
    }
    /* skip first line for combined cpu */
    fgets(buf, sizeof(buf), fd);
    /* count lines that begins with "cpu" */
    while (1) {
        if (!fgets(buf, sizeof(buf), fd))
            break;
        if (strncmp(buf, "cpu", 3))
            break;
        cnt++;
    };
    fclose(fd);
    CPUStatus::getCPUStatus(smActionListener, aParent, fCPUStatus, cnt);
#elif defined(HAVE_KSTAT_H)
    kstat_named_t       *kn = NULL;
    kn = (kstat_named_t *)kstat_data_lookup(ks, "ncpus");
    if (kn) {
        CPUStatus::getCPUStatus(aParent, fCPUStatus, kn->value.ui32);
    } else {
        CPUStatus::getCPUStatusCombined(aParent, fCPUStatus);
    }
#elif defined(HAVE_SYSCTL_CP_TIME)
    CPUStatus::getCPUStatusCombined(aParent, fCPUStatus);
#endif
}
void CPUStatus::getCPUStatusCombined(YSMListener *smActionListener, YWindow *aParent, CPUStatus **&fCPUStatus) {
    fCPUStatus = new CPUStatus*[2];
    fCPUStatus[0] = new CPUStatus(smActionListener, aParent);
    fCPUStatus[1] = NULL;
}
void CPUStatus::getCPUStatus(YSMListener *smActionListener, YWindow *aParent, CPUStatus **&fCPUStatus, unsigned ncpus) {
    fCPUStatus = new CPUStatus*[ncpus + 1];
    /* we must reverse the order, so that left is cpu(0) and right is cpu(ncpus-1) */
    for (unsigned i(0); i < ncpus; i++)
        fCPUStatus[i] = new CPUStatus(smActionListener, aParent, ncpus - 1 - i);
    fCPUStatus[ncpus] = NULL;
}
#endif
#endif

