#ifndef UDIR_H
#define UDIR_H

#include "upath.h"
#include "yarray.h"

class DirPtr;

// unsorted directory for const C-style strings.
class cdir {
public:
    explicit cdir(const char* path = nullptr);
    ~cdir() { close(); }
    void close();
    const char* entry() const { return fEntry; }
    operator bool() const { return isOpen(); }
    int descriptor() const;

    bool open(const char* path);
    bool isOpen() const { return dirp != nullptr; }
    bool isDir() const;
    bool isFile() const;
    bool isLink() const;
    bool next();
    bool nextDir();
    bool nextExt(const char *extension);
    bool nextFile();
    void rewind();

private:
    cdir(const cdir&);  // unavailable
    cdir& operator=(const cdir&);  // unavailable

    DirPtr* dirp;
    char fEntry[256];
};

// sorted directory for const C-style strings.
class adir {
public:
    explicit adir(const char* path = nullptr);
    ~adir() { close(); }
    void close();
    const char* entry() const;
    operator bool() const { return isOpen() && fLast < count(); }

    bool open(const char* path);
    bool isOpen() const { return count(); }
    bool next();
    bool nextExt(const char *extension);
    void rewind() { fLast = -1; }
    int count() const { return fName.getCount(); }

    const char* const* begin() const { return fName.begin(); }
    const char* const* end() const { return fName.end(); }

private:
    adir(const adir&);  // unavailable
    adir& operator=(const adir&);  // unavailable

    YStringArray fName;
    int fLast;
};

// upath directory returns mstrings.
class udir {
public:
    explicit udir(upath path);
    ~udir() { close(); }
    void close();
    mstring& entry() { return fEntry; }
    operator bool() const { return isOpen(); }
    int descriptor() const;

    bool open(upath path);
    bool isOpen() const { return dirp != nullptr; }
    bool isDir() const;
    bool isFile() const;
    bool isLink() const;
    bool next();
    bool nextDir();
    bool nextExt(const mstring& extension);
    bool nextFile();

private:
    udir(const udir&);  // unavailable
    udir& operator=(const udir&);  // unavailable

    DirPtr* dirp;
    mstring fEntry;
};

// sorted directory for mstrings.
class sdir {
public:
    explicit sdir(upath path);
    ~sdir() { close(); }
    void close();
    mstring& entry() { return fName[fLast]; }
    operator bool() const { return isOpen() && fLast < count(); }

    bool open(upath path);
    bool isOpen() const { return count(); }
    bool next();
    bool nextExt(const mstring& extension);
    void rewind() { fLast = -1; }
    int count() const { return fName.getCount(); }

    mstring* begin() const { return fName.begin(); }
    mstring* end() const { return fName.end(); }

private:
    sdir(const sdir&);  // unavailable
    sdir& operator=(const sdir&);  // unavailable

    MStringArray fName;
    int fLast;
};

#endif

// vim: set sw=4 ts=4 et:
