/*
 * IceWM
 *
 * Copyright (C) 1998 Marko Macek
 */
#include "config.h"
#include "ytimer.h"
#include "yapp.h"

YTimer::YTimer(YTimerListener *listener, long ms) {
    fRunning = false;
    fPrev = fNext = 0;
    fInterval = ms;
    fListener = listener;
}

YTimer::~YTimer() {
    if (fRunning == true) {
        fRunning = false;
        app->unregisterTimer(this);
    }
}

void YTimer::startTimer() {
    gettimeofday(&timeout, 0);
    timeout.tv_usec += fInterval * 1000;
    while (timeout.tv_usec >= 1000000) {
        timeout.tv_usec -= 1000000;
        timeout.tv_sec++;
    }
    if (fRunning == false) {
        fRunning = true;
        app->registerTimer(this);
    }
}

void YTimer::runTimer() {
    gettimeofday(&timeout, 0);
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
