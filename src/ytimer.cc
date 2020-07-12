/*
 * IceWM
 *
 * Copyright (C) 1998-2001 Marko Macek
 */
#include "config.h"
#include "ytimer.h"
#include "yapp.h"
#include "yprefs.h"

YTimer::YTimer(long ms) :
    fListener(nullptr), fInterval(0), fRunning(false), fFixed(false)
{
    if (ms > 0L) {
        setInterval(ms);
    }
}

YTimer::YTimer(long ms, YTimerListener* listener, bool start, bool fixed) :
    fListener(listener),
    fInterval(max(0L, ms)),
    fRunning(false),
    fFixed(fixed)
{
    if (start)
        startTimer();
}

YTimer::~YTimer() {
    stopTimer();
}

void YTimer::setFixed() {
    // Fixed here means: not fuzzy, but exact.
    fFixed = true;
}

bool YTimer::isFixed() const {
    return fFixed || timeout_min == timeout_max;
}

void YTimer::setInterval(long ms) {
    fInterval = max(0L, ms);
}

void YTimer::startTimer(long ms) {
    setInterval(ms);
    startTimer();
}

void YTimer::startTimer() {
    timeout = monotime() + millitime(fInterval);
    fuzzTimer();
    enlist(true);
}

void YTimer::fuzzTimer() {
    if (false == fFixed && inrange(DelayFuzziness, 1, 100)) {
        // non-fixed timer: configure fuzzy timeout range
        // to allow for merging of several timers
        timeval fuzz = millitime((fInterval * DelayFuzziness) / 100L);
        timeout_min = timeout - fuzz;
        timeout_max = timeout + fuzz;
    } else {
        timeout_min = timeout;
        timeout_max = timeout;
    }
}

void YTimer::runTimer() {
    timeout = monotime();
    fuzzTimer();
    enlist(true);
}

void YTimer::stopTimer() {
    enlist(false);
}

void YTimer::enlist(bool enable) {
    if (fRunning != enable) {
        fRunning = enable;
        if (enable) {
            mainLoop->registerTimer(this);
        } else {
            mainLoop->unregisterTimer(this);
        }
    }
}

void YTimer::disableTimerListener(YTimerListener *listener) {
    // a global timer may be in use by several listeners.
    // here one of those listeners asks to be deregistered.
    // if it was the active listener then also stop the timer.
    YTimer* nonNull(this);
    if (nonNull && fListener == listener) {
        fListener = nullptr;
        stopTimer();
    }
}

void YTimer::setTimer(long interval, YTimerListener *listener, bool start) {
    stopTimer();
    setInterval(interval);
    setTimerListener(listener);
    if (start)
        startTimer();
}

// vim: set sw=4 ts=4 et:
