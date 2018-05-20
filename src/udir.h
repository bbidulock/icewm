#ifndef __UDIR_H
#define __UDIR_H

#include "upath.h"
#include "yarray.h"

// unsorted directory for const C-style strings.
class cdir {
public:
    explicit cdir(const char* path = 0);
    ~cdir() { close(); }
    void close();
    const char* path() const { return fPath; }
    const char* entry() const { return fEntry; }
    operator bool() const { return isOpen(); }

    bool open(const char* path);
    bool open();
    bool isOpen() const { return impl; }
    bool next();
    bool nextExt(const char *extension);
    void rewind();

private:
    cdir(const cdir&);  // unavailable
    cdir& operator=(const cdir&);  // unavailable

    const char* fPath;
    void *impl;
    char fEntry[256];
};

// sorted directory for const C-style strings.
class adir {
public:
    explicit adir(const char* path = 0);
    ~adir() { close(); }
    void close();
    const char* path() const { return fPath; }
    const char* entry() const;
    operator bool() const { return isOpen() && fLast < count(); }

    bool open(const char* path);
    bool open();
    bool isOpen() const { return count(); }
    bool next();
    bool nextExt(const char *extension);
    void rewind() { fLast = -1; }
    int count() const { return fName.getCount(); }

private:
    adir(const adir&);  // unavailable
    adir& operator=(const adir&);  // unavailable

    const char* fPath;
    YStringArray fName;
    int fLast;
};

// upath directory returns ustrings.
class udir {
public:
    explicit udir(const upath& path = null);
    ~udir() { close(); }
    void close();
    const upath& path() const { return fPath; }
    const ustring& entry() const { return fEntry; }
    operator bool() const { return isOpen(); }

    bool open(const upath& path);
    bool open();
    bool isOpen() const { return impl; }
    bool next();
    bool nextExt(const ustring& extension);

private:
    udir(const udir&);  // unavailable
    udir& operator=(const udir&);  // unavailable

    upath fPath;
    void *impl;
    ustring fEntry;
};

// sorted directory for ustrings.
class sdir {
public:
    explicit sdir(const upath& path = null);
    ~sdir() { close(); }
    void close();
    const upath& path() const { return fPath; }
    const ustring& entry() const;
    operator bool() const { return isOpen() && fLast < count(); }

    bool open(const upath& path);
    bool open();
    bool isOpen() const { return fPath.nonempty() && count(); }
    bool next();
    bool nextExt(const ustring& extension);
    void rewind() { fLast = -1; }
    int count() const { return fName.getCount(); }

private:
    sdir(const sdir&);  // unavailable
    sdir& operator=(const sdir&);  // unavailable

    upath fPath;
    MStringArray fName;
    int fLast;
};

#endif

// vim: set sw=4 ts=4 et:
