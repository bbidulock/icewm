#ifndef __YTIMER_H
#define __YTIMER_H

#include "base.h"

#include <sys/time.h>

class YTimer;

class YTimerListener {
public:
    virtual bool handleTimer(YTimer *timer) = 0;
};

class YTimer {
public:
    YTimer(YTimerListener *listener, long ms);
    ~YTimer();

    void setTimerListener(YTimerListener *listener) { fListener = listener; }
    YTimerListener *getTimerListener() const { return fListener; }
    
    void setInterval(long ms) { fInterval = ms; }
    long getInterval() const { return fInterval; }

    void startTimer();
    void stopTimer();
    void runTimer(); // run timer handler immediatelly
    bool isRunning() const { return fRunning; } // is timer (not handler) running

private:
    YTimer *fPrev;
    YTimer *fNext;
    YTimerListener *fListener;
    long fInterval;
    bool fRunning;
    struct timeval timeout;

    friend class YApplication;
};

#endif
