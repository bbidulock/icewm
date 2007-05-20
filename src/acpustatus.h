#ifndef __CPUSTATUS_H
#define __CPUSTATUS_H

#if defined(linux) || defined(HAVE_KSTAT_H) || defined(HAVE_SYSCTL_CP_TIME)

#define IWM_USER   (0)
#define IWM_NICE   (1)
#define IWM_SYS    (2)
#define IWM_INTR   (3)
#define IWM_IOWAIT (4)
#define IWM_SOFTIRQ (5)
#define IWM_IDLE   (6)
#define IWM_STATES (7)

#include "ywindow.h"
#include "ytimer.h"

class CPUStatus: public YWindow, public YTimerListener {
public:
    CPUStatus(YWindow *aParent = 0,
		          bool cpustatusShowRamUsage = 0,
							bool cpustatusShowSwapUsage = 0,
							bool cpustatusShowAcpiTemp = 0,
							bool cpustatusShowCpuFreq = 0);
    virtual ~CPUStatus();
    
    virtual void paint(Graphics &g, const YRect &r);

    virtual bool handleTimer(YTimer *t);

    virtual void handleClick(const XButtonEvent &up, int count);

    void updateStatus();
    void getStatus();
    int getAcpiTemp(char* tempbuf, int buflen);
    float getCpuFreq(unsigned int cpu);
    void updateToolTip();

private:
    int **cpu;
    unsigned long long last_cpu[IWM_STATES];
    YColor *color[IWM_STATES];
    YTimer *fUpdateTimer;
		bool ShowRamUsage, ShowSwapUsage, ShowAcpiTemp, ShowCpuFreq;
};
#else
#undef CONFIG_APPLET_CPU_STATUS
#endif

#endif
