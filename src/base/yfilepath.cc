#include "config.h"

#pragma implementation

#include "sysdep.h"
#include "ycstring.h"
#include "yfilepath.h"

#include <sys/stat.h>

YFilePath::YFilePath(const char *path) {
    fPath = CStr::newstr(path);
}

YFilePath::YFilePath(const CStr *path) {
    fPath = path->clone();
}

YFilePath::~YFilePath() {
    delete fPath;
}

//YFilePath *YFilePath::getParent() {}

YFilePath *YFilePath::getRelative(const char *sub) const {
    CStr *newpath = 0;
    YFilePath *np = 0;

    if (fPath->lastChar() == '/')
        newpath = CStr::join(fPath->c_str(), sub, 0);
    else
        newpath = CStr::join(fPath->c_str(), "/", sub, 0);
    np = new YFilePath(newpath);
    delete newpath;
    return np;
}

bool YFilePath::isRegularFile() {
    struct stat sb;

    if (fPath == 0 || fPath->c_str() == 0)
        return false;

    if (stat(fPath->c_str(), &sb) != 0)
        return false;
    if (!S_ISREG(sb.st_mode))
        return false;
    return true;
}

//bool YFilePath::isDirectory() {}

//bool YFilePath::isSpecialFile() {}
