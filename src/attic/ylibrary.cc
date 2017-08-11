/*
 *  IceWM - Implementation of support for runtime bound shared libraries
 *  Copyright (C) 2002 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#include "ylibrary.h"
#include <dlfcn.h>

YSharedLibrary::YSharedLibrary(const char *filename, bool global, bool lazy) {
    fLibrary = ::dlopen(filename, (global ? RTLD_GLOBAL : 0)
                                | (lazy ? RTLD_LAZY : RTLD_NOW));
}
    
YSharedLibrary::~YSharedLibrary() {
    unload();
}

void *YSharedLibrary::getSymbol(const char *symname) {
    return (fLibrary ? dlsym(fLibrary, symname) : 0);
}

const char *YSharedLibrary::getLastError() {
    return dlerror();
}

void YSharedLibrary::unload() {
    if (fLibrary) dlclose(fLibrary);
    fLibrary = 0;
}
