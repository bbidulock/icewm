#include "config.h"
#include "udir.h"
#include "base.h"
#include <dirent.h>
#include <stdlib.h>

class DirPtr {
private:
    bool own;
    DIR *ptr;
    struct dirent *de;

    void open(const char *path) { ptr = opendir(path); own = true; }
    bool read() { return (de = readdir(ptr)) != 0; }
    bool dots() const { return '.' == *name(); }

public:
    DirPtr(const char * path) : own(false), ptr(0), de(0) { open(path); }
    DirPtr(const upath& path) : own(false), ptr(0), de(0) { open(path.string()); }
    DirPtr(void *vp) : own(false), ptr(static_cast<DIR*>(vp)), de(0) { }

    ~DirPtr() { if (own) close(); }
    void close() { if (ptr) { closedir(ptr); ptr = 0; own = false; de = 0; } }

    operator DIR *() const { return ptr; }

    char* name() const { return de->d_name; }
    int length() const { return strlen(name()); }
    int size() const { return 1 + length(); }
    struct dirent *next() {
        while (read() && dots());
        return de;
    }
    void rewind() const { rewinddir(ptr); }
};

cdir::cdir(const char* path)
    : fPath(path), impl(0)
{
    if (path) {
        open();
    }
}

void cdir::close() {
    if (impl) {
        DirPtr(impl).close();
        impl = 0;
    }
}

bool cdir::open(const char* path) {
    fPath = path;
    return open();
}

bool cdir::open() {
    close();
    if (fPath) {
        impl = (void *) opendir(fPath);
    }
    return isOpen();
}

bool cdir::next() {
    if (impl) {
        DirPtr dirp(impl);
        if (dirp.next()) {
            strlcpy(fEntry, dirp.name(), sizeof fEntry);
            return true;
        }
    }
    return false;
}

bool cdir::nextExt(const char *extension) {
    int xlen = (int) strlen(extension);
    while (next()) {
        int start = ((int) strlen(fEntry)) - xlen;
        if (start >= 0 && 0 == strcmp(fEntry + start, extension)) {
            return true;
        }
    }
    return false;
}

void cdir::rewind() {
    if (isOpen()) {
        DirPtr(impl).rewind();
    }
}

adir::adir(const char* path)
    : fPath(path), fName(0), fString(0), fCount(0), fLast(-1)
{
    if (path) {
        open();
    }
}

void adir::close() {
    if (fName) {
        delete[] fString;
        delete[] fName;
        fName = 0;
        fCount = 0;
        fLast = -1;
    }
}

bool adir::open(const char* path) {
    fPath = path;
    return open();
}

static int compare_strings(const void *p1, const void *p2) {
    return strcoll(*(char *const *)p1, *(char *const *)p2);
}

bool adir::open() {
    close();
    if (fPath) {
        DirPtr dirp(fPath);
        if (dirp) {
            unsigned int count = 0;
            size_t size = 0;
            for (; dirp.next(); ++count) {
                size += dirp.size();
            }
            fName = new char*[count+10];
            fString = new char[size+100];
            fName[0] = fString;
            fName[0][0] = 0;
            fCount = 0;
            size_t offset = 0;
            for (dirp.rewind(); dirp.next() && fCount < count; ) {
                size_t len = dirp.size();
                if (offset + len <= size) {
                    fName[fCount] = fString + offset;
                    memcpy(fName[fCount], dirp.name(), len);
                    offset += len;
                    ++fCount;
                }
            }
            fLast = -1;
            qsort(fName, fCount, sizeof(*fName), compare_strings);
        }
    }
    return isOpen();
}

bool adir::next() {
    return fName && 1 + fLast < fCount && ++fLast != unsigned(-1);
}

const char* adir::entry() const {
    if (fName && fLast != unsigned(-1) && fLast < fCount) {
        return fName[fLast];
    }
    return 0;
}

bool adir::nextExt(const char *extension) {
    int xlen = (int) strlen(extension);
    while (next()) {
        int start = ((int) strlen(entry())) - xlen;
        if (start >= 0 && 0 == strcmp(entry() + start, extension)) {
            return true;
        }
    }
    return false;
}

udir::udir(const upath& path)
    : fPath(path), impl(0)
{
    if (fPath.nonempty()) {
        open();
    }
}

void udir::close() {
    if (impl) {
        DirPtr(impl).close();
        impl = 0;
    }
}

bool udir::open(const upath& path) {
    fPath = path;
    return open();
}

bool udir::open() {
    close();
    if (fPath.nonempty()) {
        impl = (void *) opendir(fPath.string());
    }
    return isOpen();
}

bool udir::next() {
    if (impl) {
        DirPtr dirp(impl);
        if (dirp.next()) {
            fEntry = dirp.name();
            return true;
        }
    }
    return false;
}

bool udir::nextExt(const ustring& extension) {
    while (next()) {
        if (fEntry.endsWith(extension)) {
            return true;
        }
    }
    return false;
}

sdir::sdir(const upath& path)
    : fPath(path), fLast(-1)
{
    if (path.nonempty()) {
        open();
    }
}

void sdir::close() {
    fName.reset();
    fLast = -1;
}

bool sdir::open(const upath& path) {
    fPath = path;
    return open();
}

static int compare_ustrings(const void *p1, const void *p2)
{
    const ustring *u1 = (const ustring *) p1;
    const ustring *u2 = (const ustring *) p2;
    return u1->collate(*u2);
}

bool sdir::open() {
    close();
    if (fPath.nonempty()) {
        DirPtr dirp(fPath);
        if (dirp) {
            fName.preserve(4);
            while(dirp.next())
                fName.add(dirp.name());
            qsort(fName.data, fName.size, sizeof(fName[0]), compare_ustrings);
        }
        fLast = -1;
    }
    return isOpen();
}

bool sdir::next() {
    return fName.data && 1 + fLast < fName.size && ++fLast != unsigned(-1);
}

const ustring& sdir::entry() const {
    return fName[fLast];
}

bool sdir::nextExt(const ustring& extension) {
    while (next()) {
        if (entry().endsWith(extension)) {
            return true;
        }
    }
    return false;
}

