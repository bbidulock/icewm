#ifndef __YTIMER_H
#define __YTIMER_H

#include "base.h"

#include <ctime>

class YTimer;

class YTimerListener {
public:
    virtual bool handleTimer(YTimer *timer) = 0;
};

class YTimer {
public:
    YTimer(long ms = 0);
    ~YTimer();

    void setTimerListener(YTimerListener *listener) { fListener = listener; }
    YTimerListener *getTimerListener() const { return fListener; }
    
    void setInterval(long ms) { fInterval = ms; }
    long getInterval() const { return fInterval; }

    void startTimer();
    void stopTimer();
    void runTimer(); // run timer handler immediatelly
    bool isRunning() const { return fRunning; }

private:
    YTimerListener *fListener;
    long fInterval;
    bool fRunning;
    YTimer *fPrev;
    YTimer *fNext;

    struct timeval timeout;

    friend class YApplication;
};

#endif
