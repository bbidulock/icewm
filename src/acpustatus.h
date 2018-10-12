#ifndef __CPUSTATUS_H
#define __CPUSTATUS_H

#if defined(__linux__) || defined(HAVE_KSTAT_H) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__)

#define IWM_USER   (0)
#define IWM_NICE   (1)
#define IWM_SYS    (2)
#define IWM_INTR   (3)
#define IWM_IOWAIT (4)
#define IWM_SOFTIRQ (5)
#define IWM_IDLE   (6)
#define IWM_STEAL  (7)
#define IWM_STATES (8)

class YSMListener;

typedef unsigned long long cpubytes;

class CPUStatusHandler {
public:
    virtual ~CPUStatusHandler() { }
    virtual void handleClick(const XButtonEvent &up, int cpuid) = 0;
    virtual void runCommandOnce(const char *resource, const char *cmdline) = 0;
};

class CPUStatus: public IApplet, private Picturer, private YTimerListener {
public:
    CPUStatus(YWindow *aParent, CPUStatusHandler *aHandler, int cpuid = -1);
    virtual ~CPUStatus();

    virtual void paint(Graphics &g, const YRect &r);
    virtual bool handleTimer(YTimer *t);
    virtual void handleClick(const XButtonEvent &up, int count);

    void updateStatus();
    void getStatus();
    void getStatusPlatform();
    int getAcpiTemp(char* tempbuf, int buflen);
    float getCpuFreq(unsigned int cpu);
    int getCpuID() const { return fCpuID; }
    virtual void updateToolTip();

private:
    int fCpuID;
    int statusUpdateCount;
    int unchanged;
    YMulti<cpubytes> cpu;
    cpubytes last_cpu[IWM_STATES];
    YColorName color[IWM_STATES];
    lazy<YTimer> fUpdateTimer;
    CPUStatusHandler *fHandler;
    bool ShowRamUsage, ShowSwapUsage, ShowAcpiTemp, ShowCpuFreq,
         ShowAcpiTempInGraph;

    YColorName tempColor;

    bool picture();
    void fill(Graphics& g);
    void draw(Graphics& g);
    void temperature(Graphics& g);

    static ref<YFont> tempFont;
};

class CPUStatusControl : private CPUStatusHandler, public YActionListener
{
public:
    typedef YObjectArray<CPUStatus> ArrayType;
    typedef ArrayType::IterType IterType;

    CPUStatusControl(YSMListener *smActionListener, IAppletContainer *iapp, YWindow *aParent);
    virtual ~CPUStatusControl() { }

    IterType getIterator() { return fCPUStatus.iterator(); }

private:
    void GetCPUStatus(bool combine);
    void getCPUStatusCombined();
    void getCPUStatus(unsigned ncpus);
    CPUStatus* createStatus(unsigned cpu = -1);

    virtual void actionPerformed(YAction action, unsigned int modifiers);
    virtual void handleClick(const XButtonEvent &up, int cpu);
    virtual void runCommandOnce(const char *resource, const char *cmdline);

    YSMListener *smActionListener;
    IAppletContainer *iapp;
    YWindow *aParent;
    ArrayType fCPUStatus;
    osmart<YMenu> fMenu;
    int fMenuCPU;
    long fPid;
};

#endif

#endif

// vim: set sw=4 ts=4 et:
