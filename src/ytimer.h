/*  IceWM - Time related classes
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU General Public License
 */

#ifndef __YTIMER_H
#define __YTIMER_H

#include "ylists.h"
#include <X11/Xos.h>


struct YTimeout:
public timeval {
    YTimeout(long sec = 0, long usec = 0) { tv_sec = sec; tv_usec = usec; }

    YTimeout & operator = (long interval);
    YTimeout & operator += (long interval);
    YTimeout & operator -= (long interval);

    YTimeout & operator += (timeval const & other);
    YTimeout & operator -= (timeval const & other);

    bool operator < (timeval const & other) const {
        return timercmp(this, &other, <);
    }
    bool operator <= (timeval const & other) const {
        return timercmp(this, &other, <=);
    }
    bool operator >= (timeval const & other) const {
        return timercmp(this, &other, >=);
    }
    bool operator > (timeval const & other) const {
        return timercmp(this, &other, >);
    }

    bool operator == (timeval const & other) const {
        return tv_sec == other.tv_sec && tv_usec == other.tv_usec;
    }

    bool operator != (timeval const & other) const {
        return tv_sec != other.tv_sec || tv_usec != other.tv_usec;
    }
    
    void update(void) { gettimeofday(this, 0); }
};


struct YTimeOfDay:
public YTimeout {
    YTimeOfDay() { update(); }
};


class YTimer:
public YSingleList<YTimer>::Item {
public:
    class Listener {
    public:
        virtual bool handleTimer(YTimer *timer) = 0;
    };

    YTimer(long ms = 0): fRunning(false), fInterval(ms), fListener(NULL) {}
    ~YTimer(void) { stop(); }

    void timerListener(Listener *listener) { fListener = listener; }
    Listener *timerListener() const { return fListener; }
    
    void interval(long ms) { fInterval = ms; }
    long interval() const { return fInterval; }
    YTimeout const & timeout() const { return fTimeout; }

    void start(void);
    void stop(void);
    void run(void); // run timer handler immediatelly
    bool running(void) const { return fRunning; }

    static void nextTimeout(YTimeout & timeout); // timeout for next timer expired
    static void handleTimeouts(void);

private:
    bool fRunning;
    long fInterval;

    Listener *fListener;
    YTimeout fTimeout;

    static YSingleList<YTimer> timers;
};


#endif
