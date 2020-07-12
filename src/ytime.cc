/*
 * IceWM
 *
 * Copyright (C) 1998-2001 Marko Macek
 */
#include "ytime.h"
#include <unistd.h>
#include <time.h>
#include <math.h>

/*
 * Note that POSIX.1-2008 marks gettimeofday as obsolete,
 * recommending the use of clock_gettime instead.
 */
static inline timeval timeofday() {
    timeval tv;
    gettimeofday(&tv, nullptr);
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
    timespec ts;
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
    timespec ts;
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

bool fsleep(double delay) {
    int ret = 0;
    if (delay > 0) {
        long sec = long(trunc(delay));
        long nano = long((delay - double(sec)) * 1e9);
        struct timespec req = { sec, nano };
        ret = nanosleep(&req, nullptr);
    }
    return ret == 0;
}

// vim: set sw=4 ts=4 et:
