/*
 * IceWM
 *
 * Copyright (C) 1998 Marko Macek
 *
 * CPU Status
 */
#include "config.h"

#include "ylib.h"
#include "yapp.h"

#include "acpustatus.h"
#include "sysdep.h"
#include "default.h"

#if defined(linux)
//#include <linux/kernel.h>
#include <sys/sysinfo.h>
#endif

#if (defined(linux) || defined(HAVE_KSTAT_H))

#define UPDATE_INTERVAL 500

CPUStatus::CPUStatus(const char *CpuCommand, YWindow *aParent): YWindow(aParent) {
    cpu = new int *[taskBarCPUSamples];
    for (unsigned int a = 0; a < taskBarCPUSamples; a++) {
        cpu[a] = new int[IWM_STATES];
    }
    fUpdateTimer = new YTimer(UPDATE_INTERVAL);
    if (fUpdateTimer) {
        fUpdateTimer->setTimerListener(this);
        fUpdateTimer->startTimer();
    }
    fCPUCommand = CpuCommand;
    color[IWM_USER] = new YColor(clrCpuUser);
    color[IWM_NICE] = new YColor(clrCpuNice);
    color[IWM_SYS]  = new YColor(clrCpuSys);
    color[IWM_IDLE] = new YColor(clrCpuIdle);
    for (unsigned int i = 0; i < taskBarCPUSamples; i++) {
        cpu[i][IWM_USER] = cpu[i][IWM_NICE] = cpu[i][IWM_SYS] = 0;
        cpu[i][IWM_IDLE] = 1;
    }
    setSize(taskBarCPUSamples, 20);
    last_cpu[IWM_USER] = last_cpu[IWM_NICE] = last_cpu[IWM_SYS] = last_cpu[IWM_IDLE] = 0;
    getStatus();
    updateStatus();
    updateToolTip();
}

CPUStatus::~CPUStatus() {
    for (unsigned int a = 0; a < taskBarCPUSamples; a++) {
        delete cpu[a]; cpu[a] = 0;
    }
    delete cpu; cpu = 0;
    delete color[IWM_USER]; color[IWM_USER] = 0;
    delete color[IWM_NICE]; color[IWM_NICE] = 0;
    delete color[IWM_SYS];  color[IWM_SYS]  = 0;
    delete color[IWM_IDLE]; color[IWM_IDLE] = 0;
}

void CPUStatus::paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
    int n, h = height();

    for (unsigned int i = 0; i < taskBarCPUSamples; i++) {
        int user = cpu[i][IWM_USER];
        int nice = cpu[i][IWM_NICE];
        int sys = cpu[i][IWM_SYS];
        int idle = cpu[i][IWM_IDLE];
        int total = user + sys + nice + idle;

        int y = height() - 1;

        if (total > 0) {
            if (sys) {
                n = (h * (total - sys)) / total; // check rounding
                if (n >= y) n = y;
                g.setColor(color[IWM_SYS]);
                g.drawLine(i, y, i, n);
                y = n - 1;
            }

            if (nice) {
                n = (h * (total - sys - nice))/ total;
                if (n >= y) n = y;
                g.setColor(color[IWM_NICE]);
                g.drawLine(i, y, i, n);
                y = n - 1;
            }

            if (user) {
                n = (h * (total - sys - nice - user))/ total;
                if (n >= y) n = y;
                g.setColor(color[IWM_USER]);
                g.drawLine(i, y, i, n);
                y = n - 1;
            }
        }
        if (idle) {
            g.setColor(color[IWM_IDLE]);
            g.drawLine(i, 0, i, y);
        }
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
    char load[64];
    struct sysinfo sys;
    float l1, l5, l15;

    sysinfo(&sys);
    l1 = (float)sys.loads[0] / 65536.0;
    l5 = (float)sys.loads[1] / 65536.0;
    l15 = (float)sys.loads[2] / 65536.0;
    sprintf(load, "CPU Load: %3.2f %3.2f %3.2f, %d processes.",
            l1, l5, l15, sys.procs);
    setToolTip(load);
#endif
}

void CPUStatus::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 1) {
	 if ((count % 2) == 0) {
            if (fCPUCommand && fCPUCommand[0])
                app->runCommand(fCPUCommand);
        }
    }
}

void CPUStatus::updateStatus() {
    for (unsigned int i = 1; i < taskBarCPUSamples; i++) {
        cpu[i - 1][IWM_USER] = cpu[i][IWM_USER];
        cpu[i - 1][IWM_NICE] = cpu[i][IWM_NICE];
        cpu[i - 1][IWM_SYS]  = cpu[i][IWM_SYS];
        cpu[i - 1][IWM_IDLE] = cpu[i][IWM_IDLE];
    }
    getStatus(),
    repaint();
}

void CPUStatus::getStatus() {
#ifdef linux
    char *p, buf[128];
    long cur[IWM_STATES];
    int len, fd = open("/proc/stat", O_RDONLY);

    cpu[taskBarCPUSamples-1][IWM_USER] = 0;
    cpu[taskBarCPUSamples-1][IWM_NICE] = 0;
    cpu[taskBarCPUSamples-1][IWM_SYS] = 0;
    cpu[taskBarCPUSamples-1][IWM_IDLE] = 0;

    if (fd == -1)
        return;
    len = read(fd, buf, sizeof(buf) - 1);
    if (len != sizeof(buf) - 1) {
        close(fd);
        return;
    }
    buf[len] = 0;

    p = buf;
    while (*p && (*p < '0' || *p > '9'))
        p++;

    for (int i = 0; i < 4; i++) {
        cur[i] = strtoul(p, &p, 10);
        cpu[taskBarCPUSamples-1][i] = cur[i] - last_cpu[i];
        last_cpu[i] = cur[i];
    }
    close(fd);
#if 0
    fprintf(stderr, "cpu: %d %d %d %d\n",
            cpu[taskBarCPUSamples-1][IWM_USER], cpu[taskBarCPUSamples-1][IWM_NICE],
            cpu[taskBarCPUSamples-1][IWM_SYS],  cpu[taskBarCPUSamples-1][IDLE]);
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
                        fprintf(stderr, "kstat finds too many cpus: should be %d\n",ncpus);
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
             ((int)(((1000.0 * (float)cp_pct[i]) / total_change) + 0.5)) :
             ((i == CPU_IDLE) ? (1000) : (0)));

    /* OK, we've got the data. Now copy it to cpu[][] */
    cpu[taskBarCPUSamples-1][IWM_USER] = cp_pct[CPU_USER];
    cpu[taskBarCPUSamples-1][IWM_NICE] = cp_pct[CPU_WAIT];
    cpu[taskBarCPUSamples-1][IWM_SYS]  = cp_pct[CPU_KERNEL];
    cpu[taskBarCPUSamples-1][IWM_IDLE] = cp_pct[CPU_IDLE];

#endif /* have_kstat_h */

}
#endif

