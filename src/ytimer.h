#ifndef YTIMER_H
#define YTIMER_H

#include "ytime.h"

class YTimer;

class YTimerListener {
public:
    virtual bool handleTimer(YTimer *timer) = 0;
protected:
    virtual ~YTimerListener() {}
};

class YTimer {
public:
    YTimer(long ms = 0L);
    YTimer(long ms, YTimerListener *listener, bool start, bool fixed = false);
    ~YTimer();

    void setTimerListener(YTimerListener *listener) { fListener = listener; }
    YTimerListener *getTimerListener() const { return fListener; }
    void disableTimerListener(YTimerListener *listener);
    void setTimer(long interval, YTimerListener *listener, bool start);

    void setInterval(long ms);
    long getInterval() const { return fInterval; }

    void setFixed();

    void startTimer();
    void startTimer(long ms);
    void stopTimer();
    void runTimer(); // run timer handler immediately
    bool isRunning() const { return fRunning; }
    bool isFixed() const;

private:
    void enlist(bool enable);
    void fuzzTimer();

    YTimerListener *fListener;
    long fInterval;
    bool fRunning;
    bool fFixed;

    struct timeval timeout_min, timeout, timeout_max;

    friend class YApplication;
};

#endif

// vim: set sw=4 ts=4 et:
