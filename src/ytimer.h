#ifndef __YTIMER_H
#define __YTIMER_H

#include <sys/time.h>

timeval walltime();
timeval monotime();
timeval millitime(long msec);
timeval maketime(long sec, long usec);
long seconds();

inline bool operator<(const timeval& a, const timeval& b) {
    return a.tv_sec != b.tv_sec ?
           a.tv_sec  < b.tv_sec :
           a.tv_usec < b.tv_usec;
}

inline bool operator==(const timeval& a, const timeval& b) {
    return a.tv_sec == b.tv_sec && a.tv_usec == b.tv_usec;
}

inline timeval operator+(const timeval& a, const timeval& b) {
    return maketime( a.tv_sec + b.tv_sec, a.tv_usec + b.tv_usec );
}

inline timeval operator+(const timeval& a, long sec) {
    return maketime( a.tv_sec + sec, a.tv_usec );
}

inline timeval operator-(const timeval& a, const timeval& b) {
    return maketime( a.tv_sec - b.tv_sec, a.tv_usec - b.tv_usec );
}

inline timeval& operator+=(timeval& a, const timeval& b) {
    return a = a + b;
}

inline double toDouble(const timeval& t) {
    return (double) t.tv_sec + 1e-6 * t.tv_usec;
}

inline timeval zerotime() { return (timeval) { 0L, 0L }; }

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
