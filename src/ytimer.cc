/*
 * IceWM
 *
 * Copyright (C) 1998-2001 Marko Macek
 */
#include "config.h"
#include "ytimer.h"
#include "yapp.h"
#include "yprefs.h"
#include <unistd.h>
#include <time.h>

/*
 * Note that POSIX.1-2008 marks gettimeofday as obsolete,
 * recommending the use of clock_gettime instead.
 */
static inline timeval timeofday() {
    timeval tv = {0, 0L};
    gettimeofday(&tv, 0);
    return tv;
}

/*
 * Convert timespec to timeval.
 */
#if _POSIX_TIMERS >= 200112L
static inline timeval fromspec(timespec ts) {
    return (timeval) { ts.tv_sec, ts.tv_nsec / 1000L };
}
#endif

/*
 * Give real wall clock time in seconds + microseconds.
 */
timeval walltime() {
#if _POSIX_TIMERS >= 200112L
    timespec ts = {0, 0L};
#if defined(CLOCK_REALTIME_COARSE) && defined(__linux__)
    if (clock_gettime(CLOCK_REALTIME_COARSE, &ts) == 0)
        return fromspec(ts);
#elif defined(CLOCK_REALTIME_FAST) && !defined(__linux__)
    if (clock_gettime(CLOCK_REALTIME_FAST, &ts) == 0)
        return fromspec(ts);
#endif
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0)
        return fromspec(ts);
#endif
    return timeofday();
}

/*
 * Obtain a monotonic time for reliable intervals similar to the X11 server.
 * This makes it possible to be in sync with the X11 event time stamps.
 */
timeval monotime() {
#if _POSIX_TIMERS >= 200112L && defined(_POSIX_MONOTONIC_CLOCK)
    timespec ts = {0, 0L};
#if defined(CLOCK_MONOTONIC_COARSE) && defined(__linux__)
    if (clock_gettime(CLOCK_MONOTONIC_COARSE, &ts) == 0)
        return fromspec(ts);
#elif defined(CLOCK_MONOTONIC_FAST) && !defined(__linux__)
    if (clock_gettime(CLOCK_MONOTONIC_FAST, &ts) == 0)
        return fromspec(ts);
#endif
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
        return fromspec(ts);
#endif
    return timeofday();
}

/*
 * Construct a timeval with tv_usec in [0, 999.999].
 * Either parameter may be negative, so avoid modulo.
 */
timeval maketime(long sec, long usec) {
    const long million = 1000L*1000L;
    if (usec >= million) {
        long k = usec / million;
        sec  += k;
        usec -= k * million;
    }
    else if (usec < 0) {
        long k = (million - 1L - usec) / million;
        sec  -= k;
        usec += k * million;
    }
    return (timeval) { sec, usec };
}

timeval zerotime() {
    return (timeval) { 0, 0L };
}

timeval millitime(long msec) {
    return maketime( 0L, msec * 1000L );
}

/*
 * time(NULL) may return a different number of seconds
 * than gettimeofday() or clock_gettime() does.
 * Therefore it is better to avoid time(NULL) and
 * use the same single source of time everywhere.
 */
long seconds() {
    return walltime().tv_sec;
}

YTimer::YTimer(long ms) :
    fListener(0), fInterval(0), fRunning(false), fFixed(false)
{
    if (ms > 0L) {
        setInterval(ms);
    }
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
