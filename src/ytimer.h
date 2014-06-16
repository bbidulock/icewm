#ifndef __YTIMER_H
#define __YTIMER_H

#include "base.h"
#include <X11/Xos.h>

class YTimer;

class YTimerListener {
public:
    virtual bool handleTimer(YTimer *timer) = 0;
protected:
    virtual ~YTimerListener() {};
};

class YTimer {
public:
    YTimer(long ms = 0);
    ~YTimer();

    void setTimerListener(YTimerListener *listener) { fListener = listener; }
    YTimerListener *getTimerListener() const { return fListener; }
    
    void setInterval(long ms) { fInterval = ms; }
    long getInterval() const { return fInterval; }

    void setFixed();

    void startTimer();
    void startTimer(long ms);
    void stopTimer();
    void runTimer(); // run timer handler immediately
    bool isRunning() const { return fRunning; }
    bool isFixed() const { return (!memcmp(&timeout_min, &timeout_max, sizeof(timeout_min))); }

private:
    YTimerListener *fListener;
    long fInterval;
    bool fRunning;
    bool fFixed;
    YTimer *fPrev;
    YTimer *fNext;

    struct timeval timeout_min, timeout, timeout_max;

    friend class YApplication;
};

#endif
