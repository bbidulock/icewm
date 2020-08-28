#ifndef UPATH_H
#define UPATH_H

#include "mstring.h"

#include "yfileio.h"

// some primitives needed for fileptr below
#include <stdio.h>

class YStringArray;

class upath {
public:
    upath(const class null_ref &): fPath(null) {}
    upath(const mstring& path): fPath(path) {}
    upath(const char *path): fPath(path) {}
    upath(const char *path, size_t len): fPath(path, len) {}
    upath(const upath& path): fPath(path.fPath) {}
    upath(): fPath(null) {}

    int length() const { return int(fPath.length()); }
    bool isEmpty() const { return fPath.isEmpty(); }
    bool nonempty() const { return fPath.nonempty(); }

    upath parent() const;
    mstring name() const;
    upath relative(const upath &path) const;
    upath child(const char *path) const;
    upath addExtension(const char *ext) const;
    mstring getExtension() const;
    upath removeExtension() const;
    upath replaceExtension(const char *ext) const;
    mstring expand() const;

    bool fileExists();
    bool dirExists();
    bool isAbsolute() const;
    bool isRelative() const;
    bool isReadable();
    bool isWritable();
    bool isExecutable();
    bool isHttp() const;
    bool hasProtocol() const;
    int access(int mode = 0);
    int mkdir(int mode = 0700);
    int open(int flags, int mode = 0666);
    FILE* fopen(const char *mode);
    int stat(struct stat *st);
    int remove();
    int renameAs(mstring dest);
    off_t fileSize();
    fcsmart loadText();
    bool copyFrom(upath from, int mode = 0666);
    bool testWritable(int mode = 0666);
    int fnMatch(const char* pattern, int flags = 0);

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

    const mstring& path() const { return fPath; }
    operator const mstring&() const { return path(); }
    const char* string() { return fPath; }

    static const mstring& sep() { return slash; }
    static const upath& root() { return rootPath; }

    static bool hasglob(mstring pattern);
    static bool glob(mstring pat, YStringArray& list, const char* opt = nullptr);

    bool hasglob() const { return hasglob(path()); }
    bool glob(YStringArray& list, const char* opt = nullptr) const {
        return glob(path(), list, opt);
    }

private:
    mstring fPath;

    bool isSeparator(int) const;

    static const mstring slash;
    static const upath rootPath;
};

class fileptr {
    FILE* fp;
public:
    fileptr(const char* n, const char* m) : fp(fopen(n, m)) { }
    explicit fileptr(FILE* fp) : fp(fp) { }
    ~fileptr() { close(); }
    void close() { if (fp) { fclose(fp); fp = nullptr; } }
    operator FILE*() const { return fp; }
    FILE* operator->() const { return fp; }
private:
    void operator=(FILE* ptr) { close(); fp = ptr; }
};

#endif

// vim: set sw=4 ts=4 et:
