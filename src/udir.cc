#include "config.h"
#include "udir.h"
#include <dirent.h>

cdir::cdir(const char* path)
    : fPath(path), impl(0)
{
    if (path) {
        open();
    }
}

void cdir::close() {
    if (impl) {
        closedir(static_cast<DIR*>(impl));
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
        struct dirent *de;
        while ((de = readdir(static_cast<DIR*>(impl))) != 0) {
            if ('.' != *de->d_name) {
                strlcpy(fEntry, de->d_name, sizeof fEntry);
                return true;
            }
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

udir::udir(const upath& path)
    : fPath(path), impl(0)
{
    if (fPath.nonempty()) {
        open();
    }
}

void udir::close() {
    if (impl) {
        closedir(static_cast<DIR*>(impl));
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
        struct dirent *de;
        while ((de = readdir(static_cast<DIR*>(impl))) != 0) {
            if ('.' != *de->d_name) {
                fEntry = de->d_name;
                return true;
            }
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

