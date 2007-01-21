#ifndef __UPATH_H
#define __UPATH_H

#include "mstring.h"

typedef mstring pstring;

class upath {
public:
    upath(const class null_ref &): fPath(null) {}
    upath(pstring path): fPath(path) {}
    upath(const char *path): fPath(path) {}
    upath(): fPath(null) {};

    upath parent() const;
    pstring name() const;
    upath relative(const upath &path) const;
    upath child(const char *path) const;

    bool isAbsolute();
    bool fileExists();
    bool dirExists();
    bool isReadable();
    int access(int mode);
    upath addExtension(const char *ext) const;

    upath operator=(const upath& p) {
        fPath = p.fPath;
        return *this;
    }

    upath operator=(const class null_ref &) {
        fPath = null;
        return *this;
    }

    bool operator==(const class null_ref &) const { return fPath == null; }
    bool operator!=(const class null_ref &) const { return fPath != null; }

    bool equals(const upath &s) const;

    pstring path() const { return fPath; }
private:
    pstring fPath;
};

#endif
