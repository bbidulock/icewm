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
    bool expires() const;

    timeval fuzziness() const { return (timeval) { 0L, fFuzziness*1000L }; }
    const timeval& timeout() const { return fTimeout; }
    timeval timeout_min() const { return fTimeout - fuzziness(); }
    timeval timeout_max() const { return fTimeout + fuzziness(); }
    void decreaseTimeout(const timeval& diff) { fTimeout += diff; }

private:
    void enlist();
    void fuzzTimer();

    YTimerListener *fListener;
    struct timeval fTimeout;
    long fInterval;
    long fFuzziness;
    bool fRunning;
    bool fFixed;
};

#endif

// vim: set sw=4 ts=4 et:
