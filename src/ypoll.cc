/*
 * IceWM
 *
 * Copyright (C) 2005 Marko Macek
 */
#include "config.h"

YPoll::~YPoll() {
    if (fFd != -1) {
        fFd = -1;
    }
    unregisterPoll(this);
}

