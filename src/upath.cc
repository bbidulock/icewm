/*
 * IceWM
 *
 * Copyright (C) 2004,2005 Marko Macek
 */
#include "config.h"

#include "upath.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

const pstring upath::slash("/");
const upath upath::rootPath(slash);

bool upath::isSeparator(int ch) const {
    return ch == '/';
}

upath upath::parent() const {
    int len = length();
    while (len > 1 && isSeparator(fPath[len - 1])) {
        --len;
    }
    for (; len > 0; --len) {
        if (isSeparator(fPath[len - 1])) {
            break;
        }
    }
    while (len > 1 && isSeparator(fPath[len - 1])) {
        --len;
    }
    return upath( fPath.substring(0, len) );
}

pstring upath::name() const {
    int start = length();
    for (; start > 0; --start) {
        if (isSeparator(fPath[start - 1])) {
            break;
        }
    }
    return fPath.substring(start, length() - start);
}

upath upath::relative(const upath &npath) const {
    if (npath.isEmpty())
        return *this;
    else if (isEmpty()) {
        if (npath.isAbsolute())
            return upath(pstring(".") + npath.path());
        else
            return npath;
    }
    else if (path().endsWith(slash)) {
        if (npath.isAbsolute())
            return upath(path() + npath.path().substring(1));
        else
            return upath(path() + npath.path());
    }
    else if (npath.isAbsolute())
        return upath(path() + npath.path());
    else
        return upath(path() + slash + npath.path());
}

upath upath::child(const char *npath) const {
    return relative(upath(npath));
}

upath upath::addExtension(const char *ext) const {
    return upath(path().append(ext));
}

pstring upath::getExtension() const {
    int dot = path().lastIndexOf('.');
    int sep = path().lastIndexOf('/');
    return dot > sep ? path().substring(dot + 1) : pstring();
}

bool upath::isAbsolute() const {
    return isSeparator(path()[0]);
}

bool upath::isRelative() const {
    return false == isAbsolute() && false == hasProtocol();
}

bool upath::fileExists() const {
    struct stat sb;
    return stat(cstring(path()), &sb) == 0 && S_ISREG(sb.st_mode);
}

bool upath::dirExists() const {
    struct stat sb;
    return stat(cstring(path()), &sb) == 0 && S_ISDIR(sb.st_mode);
}

bool upath::isReadable() const {
    return access(R_OK) == 0;
}

int upath::access(int mode) const {
    return ::access(cstring(path()), mode);
}

bool upath::isWritable() const {
    return access(W_OK) == 0;
}

bool upath::isExecutable() const {
    return access(X_OK) == 0;
}

bool upath::hasProtocol() const {
    int k = path().indexOf('/');
    return k > 0 && path()[k-1] == ':' && path()[k+1] == '/';
}

bool upath::isHttp() const {
    return path().startsWith("http") && hasProtocol();
}

bool upath::equals(const upath &s) const {
    if (path() != null && s.path() != null)
        return path().equals(s.path());
    else
        return path() == null && s.path() == null;
}

