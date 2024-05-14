#include "config.h"
#include "udir.h"
#include "base.h"
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>

class DirPtr {
private:
    DIR *ptr;
    dirent *de;

    void open(const char* path) { ptr = opendir(path); }
    bool read() { return (de = readdir(ptr)) != nullptr; }
    bool dots() const { return '.' == *name(); }

public:
    DirPtr(const char* path) : ptr(nullptr), de(nullptr) { open(path); }
    ~DirPtr() { close(); }
    void close() { if (ptr) { closedir(ptr); ptr = nullptr; de = nullptr; } }

    operator DIR*() const { return ptr; }
    operator bool() const { return ptr != nullptr; }

    char* name() const { return de->d_name; }
    int length() const { return int(strlen(name())); }
    int size() const { return 1 + length(); }
    int descriptor() const { return dirfd(ptr); }
    dirent* next() {
        while (read() && dots());
        return de;
    }
    void rewind() const { rewinddir(ptr); }
    bool isDir() {
        if (de) {
#ifdef DT_DIR
            if (de->d_type) {
                return de->d_type == DT_DIR;
            }
#endif
            struct stat st;
            if (fstatat(dirfd(ptr), name(), &st, 0) == 0) {
                return S_ISDIR(st.st_mode);
            }
        }
        return false;
    }
    bool isFile() {
        if (de) {
#ifdef DT_REG
            if (de->d_type && de->d_type != DT_LNK) {
                return de->d_type == DT_REG;
            }
#endif
            struct stat st;
            if (fstatat(dirfd(ptr), name(), &st, 0) == 0) {
                return S_ISREG(st.st_mode);
            }
        }
        return false;
    }
    bool isLink() {
        if (de) {
#ifdef DT_LNK
            if (de->d_type) {
                return de->d_type == DT_LNK;
            }
#endif
            struct stat st;
            if (fstatat(dirfd(ptr), name(), &st, 0) == 0) {
                return S_ISLNK(st.st_mode);
            }
        }
        return false;
    }
};

cdir::cdir(const char* path)
    : dirp(nullptr)
{
    fEntry[0] = '\0';
    if (path) {
        open(path);
    }
}

void cdir::close() {
    if (dirp) {
        delete dirp;
        dirp = nullptr;
    }
}

bool cdir::open(const char* path) {
    close();
    dirp = new DirPtr(path);
    if (dirp && *dirp == nullptr)
        close();
    return isOpen();
}

bool cdir::isDir() const {
    return dirp && dirp->isDir();
}

bool cdir::isFile() const {
    return dirp && dirp->isFile();
}

bool cdir::isLink() const {
    return dirp && dirp->isLink();
}

bool cdir::next() {
    if (dirp && dirp->next()) {
        strlcpy(fEntry, dirp->name(), sizeof fEntry);
        return true;
    }
    return false;
}

bool cdir::nextDir() {
    while (next()) {
        if (dirp->isDir())
            return true;
    }
    return false;
}

bool cdir::nextExt(const char *extension) {
    int xlen = int(strlen(extension));
    while (next()) {
        int start = int(strlen(fEntry)) - xlen;
        if (start >= 0 && 0 == strcmp(fEntry + start, extension)) {
            return true;
        }
    }
    return false;
}

bool cdir::nextFile() {
    while (next()) {
        if (dirp->isFile())
            return true;
    }
    return false;
}

void cdir::rewind() {
    if (isOpen()) {
        dirp->rewind();
    }
}

int cdir::descriptor() const {
    return dirp ? dirp->descriptor() : -1;
}

adir::adir(const char* path)
    : fLast(-1)
{
    if (path) {
        open(path);
    }
}

void adir::close() {
    fName.clear();
    fLast = -1;
}

bool adir::open(const char* path) {
    close();
    DirPtr dirp(path);
    if (dirp) {
        while (dirp.next()) {
            fName.append(dirp.name());
        }
        fName.sort();
        return true;
    }
    return false;
}

bool adir::next() {
    return 1 + fLast < count() && ++fLast >= 0;
}

const char* adir::entry() const {
    if (fLast >= 0 && fLast < count()) {
        return fName[fLast];
    }
    return nullptr;
}

bool adir::nextExt(const char *extension) {
    int xlen = int(strlen(extension));
    while (next()) {
        int start = int(strlen(entry())) - xlen;
        if (start >= 0 && 0 == strcmp(entry() + start, extension)) {
            return true;
        }
    }
    return false;
}

udir::udir(upath path)
    : dirp(nullptr)
{
    open(path);
}

void udir::close() {
    if (dirp) {
        delete dirp;
        dirp = nullptr;
    }
}

bool udir::open(upath path) {
    close();
    dirp = new DirPtr(path.string());
    if (dirp && *dirp == false)
        close();
    return isOpen();
}

bool udir::isDir() const {
    return dirp && dirp->isDir();
}

bool udir::isFile() const {
    return dirp && dirp->isFile();
}

bool udir::isLink() const {
    return dirp && dirp->isLink();
}

bool udir::next() {
    if (dirp && dirp->next()) {
        fEntry = dirp->name();
        return true;
    }
    return false;
}

bool udir::nextDir() {
    while (next()) {
        if (dirp->isDir())
            return true;
    }
    return false;
}

bool udir::nextExt(const mstring& extension) {
    while (next()) {
        if (fEntry.endsWith(extension)) {
            return true;
        }
    }
    return false;
}

bool udir::nextFile() {
    while (next()) {
        if (dirp->isFile())
            return true;
    }
    return false;
}

int udir::descriptor() const {
    return dirp ? dirp->descriptor() : -1;
}

sdir::sdir(upath path)
    : fLast(-1)
{
    open(path);
}

void sdir::close() {
    fName.clear();
    fLast = -1;
}

bool sdir::open(upath path) {
    close();
    DirPtr dirp(path.string());
    if (dirp) {
        while (dirp.next()) {
            fName.append(dirp.name());
        }
        fName.sort();
        return true;
    }
    return false;
}

bool sdir::next() {
    return 1 + fLast < count() && ++fLast >= 0;
}

bool sdir::nextExt(const mstring& extension) {
    while (next()) {
        if (entry().endsWith(extension)) {
            return true;
        }
    }
    return false;
}


// vim: set sw=4 ts=4 et:
