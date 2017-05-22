#ifndef __MEMSTATUS_H
#define __MEMSTATUS_H

#if defined(__linux__)

// graphed from the bottom up:
#define MEM_USER    (0)
#define MEM_BUFFERS (1)
#define MEM_CACHED  (2)
#define MEM_FREE    (3)
#define MEM_STATES  (4)

#include "ywindow.h"

class MEMStatus: public YWindow, public YTimerListener {
public:
    MEMStatus(YWindow *aParent = 0);
    virtual ~MEMStatus();

    virtual void paint(Graphics &g, const YRect &r);

    virtual bool handleTimer(YTimer *t);

    void updateStatus();
    void getStatus();
    void updateToolTip();

private:
    static void printAmount(char *out, size_t outSize,
                            unsigned long long amount);
    static unsigned long long parseField(const char *buf,
                                         size_t bufLen,
                                         const char *needle);

    unsigned long long int **samples;
    YColor *color[MEM_STATES];
    YTimer *fUpdateTimer;
};
#else
#undef CONFIG_APPLET_MEM_STATUS
#endif

#endif
