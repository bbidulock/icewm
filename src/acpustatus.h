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
#define IWM_STEAL  (7)
#define IWM_STATES (8)

#include "ywindow.h"
#include "ytimer.h"

class YSMListener;

class CPUStatus: public YWindow, public YTimerListener {
public:
    CPUStatus(
        YWindow *aParent = 0,
	int cpuid = -1);
    virtual ~CPUStatus();
    
    virtual void paint(Graphics &g, const YRect &r);

    virtual bool handleTimer(YTimer *t);

    virtual void handleClick(const XButtonEvent &up, int count);

    void updateStatus();
    void getStatus();
    int getAcpiTemp(char* tempbuf, int buflen);
    float getCpuFreq(unsigned int cpu);
    void updateToolTip();

    static void GetCPUStatus(YWindow *aParent, CPUStatus **&fCPUStatus, bool combine);

private:
    int fCpuID;
    unsigned long long **cpu;
    unsigned long long last_cpu[IWM_STATES];
    YColor *color[IWM_STATES];
    YTimer *fUpdateTimer;
    YSMListener *smActionListener;
    bool ShowRamUsage, ShowSwapUsage, ShowAcpiTemp, ShowCpuFreq,
         ShowAcpiTempInGraph;
    FILE *m_nCachedFd;

    YColor *tempColor;
    static ref<YFont> tempFont;
    static void getCPUStatusCombined(YWindow *aParent, CPUStatus **&fCPUStatus);
    static void getCPUStatus(YWindow *aParent, CPUStatus **&fCPUStatus, unsigned ncpus);
};
#else
#undef CONFIG_APPLET_CPU_STATUS
#endif

#endif
