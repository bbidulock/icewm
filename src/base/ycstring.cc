#include "config.h"
//#include "base.h"
#include "ycstring.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

static char nullCStr[] = ""; // just a \0;

CStr::CStr(const char *str, int len) {
    if (len == 0) {
        fStr = nullCStr;
        fLen = 0;
    } else {
        fStr = new char [len + 1];
        if (fStr) {
            memcpy(fStr, str, len);
            fStr[len] = 0;
            fLen = len;
        } else
            fLen = 0;
    }
}

CStr::~CStr() {
    if (fStr != nullCStr)
        delete [] fStr;
}

CStr *CStr::clone() const {
    return CStr::newstr(fStr, fLen);
}

CStr *CStr::newstr(const char *str) {
    if (str)
        return newstr(str, strlen(str));
    else
        return 0;
}

CStr *CStr::newstr(const char *str, int len) {
    if (str) {
        CStr *s = new CStr(str, len);

        if (s->c_str() == 0) {
            delete s;
            return 0;
        }

        return s;
    } else
        return 0;
}

CStr *CStr::format(const char *fmt, ...) {
    va_list ap;
    int size = 100; // first guess, try not to waste space
    char *buffer = new char [size];

    while (1) {
        if (buffer == 0)
            return 0;
        va_start(ap, fmt);
        int nchars = vsnprintf(buffer, size, fmt, ap);
        va_end(ap);

        if (nchars >= -1 && nchars < size) {
            CStr *n = newstr(buffer);
            delete [] buffer;
            return n;
        }
        if (nchars > -1)
            size = nchars + 1;
        else
            size *= 2;
        delete [] buffer;
        buffer = new char[size];
    }
}

bool CStr::isWhitespace() const {
    if (fStr)
        for (const char *p = fStr; *p; p++)
            if (*p > ' ') // !!! really?
                return false;
    return true;
}

int CStr::firstChar() const {
    if (fLen > 0)
        return fStr[0];
    else
        return -1;
}

int CStr::lastChar() const {
    if (fLen > 0)
        return fStr[fLen - 1];
    else
        return -1;
}

CStr *CStr::join(const char *str, ...) {
    va_list ap;
    const char *s;
    char *res, *p;
    int len1 = 0;

    if (str == 0)
        return 0;

    va_start(ap, str);
    s = str;
    while (s) {
        len1 += strlen(s);
        s = va_arg(ap, char *);
    }
    va_end(ap);

    if ((p = res = new char[len1 + 1]) == 0)
        return 0;

    va_start(ap, str);
    s = str;
    while (s) {
        int len = strlen(s);
        memcpy(p, s, len);
        p += len;
        s = va_arg(ap, char *);
    }
    va_end(ap);
    *p = 0;
    CStr *cs = CStr::newstr(res, len1);
    delete [] res;
    return cs;
}
