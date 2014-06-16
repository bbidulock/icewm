/*
 * IceWM
 *
 * Copyright (C) 1998-2001 Marko Macek
 */
#include "config.h"
#include "ytimer.h"
#include "yapp.h"
#include "yprefs.h"

YTimer::YTimer(long ms) {
    fRunning = false;
    fFixed = false;
    fPrev = fNext = 0;
    fInterval = ms;
    fListener = 0;
}

YTimer::~YTimer() {
    if (fRunning == true) {
        fRunning = false;
        mainLoop->unregisterTimer(this);
    }
}

void YTimer::setFixed() {
    fFixed = true;
}

void YTimer::startTimer(long ms) {
    setInterval(ms);
    startTimer();
}

void YTimer::startTimer() {
    long offs = fInterval * 1000;

    gettimeofday(&timeout, 0);

    timeout.tv_usec += offs;
    while (timeout.tv_usec >= 1000000) {
        timeout.tv_usec -= 1000000;
        timeout.tv_sec++;
    }

    timeout_min = timeout;
    timeout_max = timeout;

    if ((!fFixed) && (DelayFuzziness)) {
        // non-fixed timer: configure fuzzy timeout range
	// to allow for merging of several timers
	struct timeval diff;

	offs = (offs * DelayFuzziness) / 100;

	timerclear(&diff);
	diff.tv_usec = offs ;

	timersub(&timeout_min, &diff, &timeout_min);
        while (timeout_min.tv_usec >= 1000000) {
            timeout_min.tv_usec -= 1000000;
            timeout_min.tv_sec++;
        }

	timeradd(&timeout_max, &diff, &timeout_max);
        while (timeout_max.tv_usec >= 1000000) {
            timeout_max.tv_usec -= 1000000;
            timeout_max.tv_sec++;
        }
    }

    if (fRunning == false) {
        fRunning = true;
        mainLoop->registerTimer(this);
    }
}

void YTimer::runTimer() {
    // might need to handle fuzziness just like startTimer,
    // but so far runTimer() is called by one fixed timer only (taskbar clock)
    gettimeofday(&timeout, 0);
    if (fRunning == false) {
        fRunning = true;
        mainLoop->registerTimer(this);
    }
}

void YTimer::stopTimer() {
    if (fRunning == true) {
        fRunning = false;
        mainLoop->unregisterTimer(this);
    }
}
