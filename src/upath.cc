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
#include "base.h"
#include "yapp.h"
#include "ypointer.h"

const pstring upath::slash("/");
const upath upath::rootPath(slash);

bool upath::isSeparator(int ch) const {
    return ch == '/';
}

upath upath::parent() const {
    int len = length();
    int sep = fPath.lastIndexOf('/');
    while (0 < sep && 1 + sep == len) {
        len = sep;
        sep = fPath.substring(0, len).lastIndexOf('/');
    }
    len = max(0, sep);
    while (len > 1 && isSeparator(fPath[len - 1])) {
        --len;
    }
    return upath( fPath.substring(0, len) );
}

pstring upath::name() const {
    int start = 1 + fPath.lastIndexOf('/');
    return fPath.substring(start, length() - start);
}

upath upath::relative(const upath &npath) const {
    if (npath.isEmpty())
        return *this;
    else if (isEmpty()) {
        return npath;
    }
    else if (isSeparator(path()[length() - 1])) {
        if (isSeparator(npath.path()[0]))
            return upath(path() + npath.path().substring(1));
        else
            return upath(path() + npath.path());
    }
    else if (isSeparator(npath.path()[0]))
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

upath upath::removeExtension() const {
    return fPath.substring(0, length() - getExtension().length());
}

upath upath::replaceExtension(const char* ext) const {
    return removeExtension().addExtension(ext);
}

cstring upath::expand() const {
    int c = fPath[0];
    if (c == '~') {
        int k = fPath[1];
        if (k == -1 || isSeparator(k))
            return (YApplication::getHomeDir() +
                    fPath.substring(min(2, length()))).fPath;
    }
    else if (c == '$') {
        mstring m(fPath.match("^\\$[_A-Za-z][_A-Za-z0-9]*"));
        if (m.nonempty()) {
            const char* e = getenv(cstring(m.substring(1)));
            if (e && *e && *e != '~' && *e != '$') {
                return e + fPath.substring(m.length());
            }
        }
    }
    return fPath;
}

bool upath::isAbsolute() const {
    int c = path()[0];
    if (isSeparator(c))
        return true;
    if (c == '~' || c == '$') {
        c = expand().m_str()[0];
        if (isSeparator(c))
            return true;
    }
    return false;
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

char* upath::loadText() const {
    return ::load_text_file(expand());
}

bool upath::copyFrom(const upath& from, int mode) const {
    csmart text(from.loadText());
    if (text == 0)
        return false;
    int fd = open(O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (fd == -1)
        return false;
    size_t len = strlen(text);
    ssize_t wr = write(fd, text, len);
    close(fd);
    return 0 <= wr && size_t(wr) == len;
}

bool upath::testWritable(int mode) const {
    int fd = open(O_WRONLY | O_CREAT | O_APPEND, mode);
    if (0 <= fd) close(fd);
    return 0 <= fd;
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

bool upath::hasglob(const char* pattern) {
    const char* s = pattern;
    while (*s && *s != '*' && *s != '?' && *s != '[')
        s += 1 + (*s == '\\' && s[1]);
    return *s != 0;
}

bool upath::glob(const char* pattern, YStringArray& list, const char* flags) {
    bool okay = false;
    int flagbits = 0;
    int (*const errfunc) (const char *epath, int eerrno) = 0;
    glob_t gl = {};

    if (flags) {
        for (int i = 0; flags[i]; ++i) {
            switch (flags[i]) {
                case '/': flagbits |= GLOB_MARK; break;
                case 'C': flagbits |= GLOB_NOCHECK; break;
                case 'E': flagbits |= GLOB_NOESCAPE; break;
                case 'S': flagbits |= GLOB_NOSORT; break;
                default: break;
            }
        }
    }

    if (0 == ::glob(pattern, flagbits, errfunc, &gl)) {
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
