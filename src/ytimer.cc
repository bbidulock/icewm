/*
 * IceWM
 *
 * Copyright (C) 1998-2001 Marko Macek
 */
#include "config.h"
#include "ytimer.h"
#include "yapp.h"

YTimer::YTimer() :
    fListener(0), fInterval(0), fRunning(false), fPrev(0), fNext(0)
{
}

YTimer::YTimer(long ms) :
    fListener(0), fInterval(ms), fRunning(false), fPrev(0), fNext(0)
{
}

YTimer::~YTimer() {
    if (fRunning == true) {
        fRunning = false;
        mainLoop->unregisterTimer(this);
    }
}

void YTimer::startTimer(long ms) {
    setInterval(ms);
    startTimer();
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
        mainLoop->registerTimer(this);
    }
}

void YTimer::runTimer() {
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
