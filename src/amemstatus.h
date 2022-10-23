#ifndef __MEMSTATUS_H
#define __MEMSTATUS_H

#if defined(__linux__)

#include "ypointer.h"

// graphed from the bottom up:
#define MEM_USER    (0)
#define MEM_BUFFERS (1)
#define MEM_CACHED  (2)
#define MEM_FREE    (3)
#define MEM_STATES  (4)

class IAppletContainer;

class MEMStatus:
    public IApplet,
    private Picturer,
    private YTimerListener,
    private YActionListener
{
public:
    MEMStatus(IAppletContainer* taskBar, YWindow *aParent);
    virtual ~MEMStatus();

    virtual void actionPerformed(YAction action, unsigned int modifiers);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual bool handleTimer(YTimer *t);

    void updateStatus();
    void getStatus();
    virtual void updateToolTip();

private:
    static void printAmount(char *out, size_t outSize,
                            unsigned long long amount);
    static unsigned long long parseField(const char *buf,
                                         size_t bufLen,
                                         const char *needle);

    typedef unsigned long long membytes;
    YMulti<membytes> samples;
    YColorName color[MEM_STATES];
    lazy<YTimer> fUpdateTimer;

    bool picture();
    void fill(Graphics& g);
    void draw(Graphics& g);

    int statusUpdateCount;
    int unchanged;
    osmart<YMenu> fMenu;
    IAppletContainer* taskBar;
};
#endif

#endif

// vim: set sw=4 ts=4 et:
