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
#include <fcntl.h>

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
    if (dot > sep + 1 && dot + 1 < length())
        return path().substring(dot);
    return null;
}

bool upath::isAbsolute() const {
    return isSeparator(path()[0]);
}

bool upath::isRelative() const {
    return false == isAbsolute() && false == hasProtocol();
}

bool upath::fileExists() const {
    struct stat sb;
    return stat(&sb) == 0 && S_ISREG(sb.st_mode);
}

off_t upath::fileSize() const {
    struct stat sb;
    return stat(&sb) == 0 ? sb.st_size : (off_t) -1;
}

bool upath::dirExists() const {
    struct stat sb;
    return stat(&sb) == 0 && S_ISDIR(sb.st_mode);
}

bool upath::isReadable() const {
    return access(R_OK) == 0;
}

int upath::access(int mode) const {
    return ::access(string(), mode);
}

bool upath::isWritable() const {
    return access(W_OK) == 0;
}

bool upath::isExecutable() const {
    return access(X_OK) == 0;
}

int upath::mkdir(int mode) const {
    return ::mkdir(string(), mode);
}

int upath::open(int flags, int mode) const {
    return ::open(string(), flags, mode);
}

FILE* upath::fopen(const char *mode) const {
    return ::fopen(string(), mode);
}

int upath::stat(struct stat *st) const {
    return ::stat(string(), st);
}

int upath::remove() const {
    return ::remove(string());
}

int upath::renameAs(const pstring& dest) const {
    return ::rename(string(), cstring(dest));
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

#include <glob.h>
#include "yarray.h"

bool upath::glob(const char* pattern, class YStringArray& list) const {
    bool okay = false;
    glob_t gl = {};
    if (0 == ::glob(pattern, 0, 0, &gl)) {
        double limit = 1e6;
        if (gl.gl_pathc < limit) {
            int count = int(gl.gl_pathc);
            list.clear();
            list.setCapacity(count);
            for (int i = 0; i < count; ++i) {
                list.append(gl.gl_pathv[i]);
            }
            okay = true;
        }
        globfree(&gl);
    }
    return okay;
}

// vim: set sw=4 ts=4 et:
