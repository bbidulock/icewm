/*
 * IceWM
 *
 * Copyright (C) 1999 Marko Macek
 */
#include "config.h"
#include "yconfig.h"
#include "yapp.h"
#include "base.h"
#include "sysdep.h"

YConfig::YConfig(const char *name) {
    fCached = false;
    fName = newstr(name);
}

YConfig::~YConfig() {
    delete [] fName;
    fName = 0;
}

bool YApplication::getConfig(YBoolConfig *c) {
}

int YApplication::getConfig(YIntConfig *c) {
}

const char *YApplication::getConfig(YStringConfig *c) {
}
