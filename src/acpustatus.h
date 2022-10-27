#ifndef CPUSTATUS_H
#define CPUSTATUS_H

#include "ywindow.h"
#include "applet.h"
#include "ytimer.h"
#include "yaction.h"
#include "ymenu.h"

#define IWM_STATES  8

enum IwmState {
    IWM_USER,
    IWM_NICE,
    IWM_SYS,
    IWM_INTR,
    IWM_IOWAIT,
    IWM_SOFTIRQ,
    IWM_IDLE,
    IWM_STEAL,
};

class YSMListener;
class YStat;
class YTemp;

typedef unsigned long long cpubytes;

class CPUStatusHandler {
public:
    virtual ~CPUStatusHandler() { }
    virtual void handleClick(const XButtonEvent &up, int cpuid) = 0;
    virtual void runCommandOnce(const char *resource, const char *cmdline) = 0;
};

class CPUStatus: public IApplet, private Picturer, private YTimerListener {
public:
    CPUStatus(YWindow* parent, CPUStatusHandler* handler, int cpuid, YStat* stat);
    virtual ~CPUStatus();

    virtual bool handleTimer(YTimer *t);
    virtual void handleClick(const XButtonEvent &up, int count);

    void updateStatus();
    void getStatus();
    void getStatusPlatform();
    void getStatusLinux();
    int getAcpiTemp(char* tempbuf, int buflen, bool best);
    float getCpuFreq(int cpu);
    int getCpuID() const { return fCpuID; }
    virtual void updateToolTip();
    static void freeFont() { tempFont = null; }
    static void freeTemp();

private:
    int fCpuID;
    int statusUpdateCount;
    int unchanged;
    YMulti<cpubytes> cpu;
    cpubytes last_cpu[IWM_STATES];
    YColorName color[IWM_STATES];
    CPUStatusHandler *fHandler;
    YStat* fStat;
    YColorName fTempColor;

    bool picture();
    void fill(Graphics& g);
    void draw(Graphics& g);
    void temperature(Graphics& g);

    static YFont tempFont;
    static YTemp* fTemp;
};

class CPUStatusControl : private CPUStatusHandler, public YActionListener
                       , private YTimerListener
{
public:
    typedef YObjectArray<CPUStatus> ArrayType;
    typedef ArrayType::IterType IterType;

    CPUStatusControl(YSMListener *smActionListener, IAppletContainer *iapp, YWindow *aParent);
    virtual ~CPUStatusControl();

    IterType getIterator() { return fCPUStatus.iterator(); }

private:
    void GetCPUStatus(bool combine);

    virtual void actionPerformed(YAction action, unsigned int modifiers);
    virtual bool handleTimer(YTimer* timer);
    virtual void handleClick(const XButtonEvent &up, int cpu);
    virtual void runCommandOnce(const char *resource, const char *cmdline);

    YSMListener *smActionListener;
    IAppletContainer *iapp;
    YWindow *aParent;
    YStat* fStat;
    YTimer fUpdateTimer;
    ArrayType fCPUStatus;
    osmart<YMenu> fMenu;
    int fMenuCPU;
    int fSamples;
    long fPid;
};

#endif

// vim: set sw=4 ts=4 et:
