/*  IceWM - Time related classes
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU General Public License
 */

#include "config.h"
#include "ytimer.h"


/*******************************************************************************
 * C++ timeval wrapper
 ******************************************************************************/

YTimeout & YTimeout::operator = (long interval) {
    tv_sec = 0;
    tv_usec = interval;

    while (tv_usec >= 1000000) { tv_usec -= 1000000; ++tv_sec; }
    while (tv_usec < 0) { tv_usec += 1000000; --tv_sec; }
    return *this;
}

YTimeout & YTimeout::operator+= (long interval) {
    tv_usec+= interval;

    while (tv_usec >= 1000000) { tv_usec -= 1000000; ++tv_sec; }
    return *this;
}

YTimeout & YTimeout::operator += (timeval const & other) {
    tv_sec += other.tv_sec;
    tv_usec += other.tv_usec;

    while (tv_usec >= 1000000) { tv_usec -= 1000000; ++tv_sec; }
    return *this;
}

YTimeout & YTimeout::operator -= (timeval const & other) {
    tv_sec -= other.tv_sec;
    tv_usec -= other.tv_usec;

    while (tv_usec < 0) { tv_usec += 1000000; --tv_sec; }
    return *this;
}


/*******************************************************************************
 * Timer infrastructure
 ******************************************************************************/

YSingleList<YTimer> YTimer::timers;


void YTimer::start() {
    gettimeofday(&fTimeout, 0);
    fTimeout+= fInterval * 1000;

    if (!fRunning) {
        fRunning = true;
        timers.prepend(this);
    }
}

void YTimer::run() {
    gettimeofday(&fTimeout, 0);

    if (!fRunning) {
        fRunning = true;
        timers.prepend(this);
    }
}

void YTimer::stop() {
    if (fRunning) {
        fRunning = false;
        timers.remove(this);
    }
}


void YTimer::nextTimeout(YTimeout & timeout) {
    YTimeOfDay curtime;
    timeout+= curtime;

    if (timers.filled()) {
        timeout = timers.head()->timeout();

        for (YTimer::Iterator timer(timers); timer; ++timer)
            if (timer->running() && timeout > timer->timeout())
                timeout = timer->timeout();
    }

    if (curtime >= timeout) timeout = 1;
    else timeout-= curtime;
        
    PRECONDITION(timeout.tv_sec >= 0);
    PRECONDITION(timeout.tv_usec >= 0);
}

void YTimer::handleTimeouts(void) {
    YTimeOfDay curtime;

    for (YTimer::Iterator timer(timers); timer; ++timer)
        if (timer->running() && curtime > timer->timeout()) {
            timer->stop();

            Listener * listener(timer->timerListener());
            if (listener && listener->handleTimer(timer)) timer->start();
        }
}
