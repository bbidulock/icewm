/*
 * IceWM
 *
 * Copyright (C) 1998 Marko Macek
 */
#pragma implementation
#include "config.h"

#include "ytimer.h"
#include "yapp.h"
#include "ypaint.h"

#define __need_timeval
#include <sys/time.h>
#include <time.h>

YTimer::YTimer(YTimerListener *listener, long ms):
    fPrev(0), fNext(0),
    fListener(listener), fInterval(ms), fRunning(false), timeout_secs(0), timeout_usecs(0)
{
}

YTimer::~YTimer() {
    if (fRunning == true) {
        fRunning = false;
        app->unregisterTimer(this);
    }
}

void YTimer::startTimer() {
    struct timeval timeout;

    gettimeofday(&timeout, 0);
    timeout.tv_usec += fInterval * 1000;
    while (timeout.tv_usec >= 1000000) {
        timeout.tv_usec -= 1000000;
        timeout.tv_sec++;
    }

    timeout_secs = timeout.tv_sec;
    timeout_usecs = timeout.tv_usec;

    if (fRunning == false) {
        fRunning = true;
        app->registerTimer(this);
    }
}

void YTimer::runTimer() {
    struct timeval timeout;

    gettimeofday(&timeout, 0);
    timeout_secs = timeout.tv_sec;
    timeout_usecs = timeout.tv_usec;
    if (fRunning == false) {
        fRunning = true;
        app->registerTimer(this);
    }
}

void YTimer::stopTimer() {
    if (fRunning == true) {
        fRunning = false;
        app->unregisterTimer(this);
    }
}
