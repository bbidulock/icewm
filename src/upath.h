#ifndef __UPATH_H
#define __UPATH_H

#include "mstring.h"

typedef mstring pstring;

class upath {
public:
    upath(const class null_ref &): fPath(null) {}
    upath(const pstring& path): fPath(path) {}
    upath(const char *path): fPath(path) {}
    upath(const char *path, int len): fPath(path, len) {}
    upath(const upath& path): fPath(path.fPath) {};
    upath(): fPath(null) {};

    int length() const { return fPath.length(); }
    bool isEmpty() const { return fPath.isEmpty(); }
    bool nonempty() const { return fPath.nonempty(); }

    upath parent() const;
    pstring name() const;
    upath relative(const upath &path) const;
    upath child(const char *path) const;
    upath addExtension(const char *ext) const;
    pstring getExtension() const;

    bool fileExists() const;
    bool dirExists() const;
    bool isAbsolute() const;
    bool isRelative() const;
    bool isReadable() const;
    bool isWritable() const;
    bool isExecutable() const;
    bool isHttp() const;
    bool hasProtocol() const;
    int access(int mode) const;

    upath& operator=(const upath& p) {
        fPath = p.fPath;
        return *this;
    }

    upath& operator=(const class null_ref &) {
        fPath = null;
        return *this;
    }

    upath& operator+=(const upath& rv) { return *this = *this + rv; }
    upath operator+(const upath& rv) const { return relative(rv); }
    bool operator==(const upath& rv) const { return equals(rv); }
    bool operator!=(const upath& rv) const { return !equals(rv); }
    bool operator==(const class null_ref &) const { return fPath == null; }
    bool operator!=(const class null_ref &) const { return fPath != null; }

    bool equals(const upath &s) const;

    const pstring& path() const { return fPath; }
    operator const pstring&() const { return path(); }

    static const pstring& sep() { return slash; }
    static const upath& root() { return rootPath; }
private:
    pstring fPath;

    bool isSeparator(int) const;

    static const pstring slash;
    static const upath rootPath;
};

#endif
