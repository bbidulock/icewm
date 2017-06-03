#ifndef __UDIR_H
#define __UDIR_H

#include "upath.h"

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

private:
    cdir(const cdir&);  // unavailable
    cdir& operator=(const cdir&);  // unavailable

    const char* fPath;
    void *impl;
    char fEntry[256];
};

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

#endif
