#ifndef __CPUSTATUS_H
#define __CPUSTATUS_H

#if (defined(linux) || defined(HAVE_KSTAT_H))

#ifdef HAVE_KSTAT_H
#include <kstat.h>
#include <sys/sysinfo.h>
#endif /* have_kstat_h */


#define IWM_USER   (0)
#define IWM_NICE   (1)
#define IWM_SYS    (2)
#define IWM_IDLE   (3)
#define IWM_STATES (4)

#include "ywindow.h"
#include "ytimer.h"

class CPUStatus: public YWindow, public YTimerListener {
public:
    CPUStatus(YWindow *aParent = 0);
    virtual ~CPUStatus();
    
    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);

    virtual bool handleTimer(YTimer *t);

    virtual void handleClick(const XButtonEvent &up, int count);

    void updateStatus();
    void getStatus();
    void updateToolTip();

private:
    unsigned int nsamples;
    int **cpu;
    long last_cpu[IWM_STATES];
    YColor *color[IWM_STATES];
    YTimer fUpdateTimer;
    const char * fCPUCommand;

    static YColorPrefProperty gColorUser;
    static YColorPrefProperty gColorSys;
    static YColorPrefProperty gColorNice;
    static YColorPrefProperty gColorIdle;

private: // not-used
    CPUStatus(const CPUStatus &);
    CPUStatus &operator=(const CPUStatus &);
};

#endif

#endif
