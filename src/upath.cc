/*
 * IceWM
 *
 * Copyright (C) 2004,2005 Marko Macek
 */
#include "config.h"

#pragma implementation

#include "upath.h"
#include "unistd.h"
#include <sys/types.h>
#include <sys/stat.h>

upath upath::parent() const {
    return null;
}

pstring upath::name() const {
    return null;

}

upath upath::relative(const upath &npath) const {
    if (path().endsWith("/") || npath.path().startsWith("/"))
        return upath(path().append(npath.path()));
    else
        return upath(path().append("/").append(npath.path()));
}

upath upath::child(const char *npath) const {
    if (path().endsWith("/"))
        return upath(path().append(npath));
    else
        return upath(path().append("/").append(npath));
}

upath upath::addExtension(const char *ext) const {
    return upath(path().append(ext));
}

bool upath::isAbsolute() {
    return path().startsWith("/");
}

bool upath::fileExists() {
    struct stat sb;
    cstring cs(path());
    return (stat(cs.c_str(), &sb) == 0 && S_ISREG(sb.st_mode));
}

bool upath::dirExists() {
    struct stat sb;
    cstring cs(path());
    return (stat(cs.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode));
}

bool upath::isReadable() {
    return access(R_OK) == 0;
}

int upath::access(int mode) {
    cstring cs(fPath);

    return ::access(cs.c_str(), mode);
}

bool upath::equals(const upath &s) const {
    if (path() == null) {
        if (s.path() == null)
            return true;
        else
            return false;
    } else
        return fPath.equals(s.path());
}
