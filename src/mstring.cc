/*
 * IceWM
 *
 * Copyright (C) 2004,2005 Marko Macek
 */
#include "config.h"

#pragma implementation

#include "mstring.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "base.h"

cstring::cstring(const mstring &s): str(s) {
    if (str.fStr) {
        if (str.data()[str.fCount] != 0) {
            str = mstring::newstr(str.data(), str.fCount);
        }
    }
}

mstring::mstring(MStringData *fStr, int fOffset, int fCount):
    fStr(fStr),
    fOffset(fOffset),
    fCount(fCount)
{ 
    PRECONDITION(fOffset >= 0);
    PRECONDITION(fCount >= 0);
    if (fStr) acquire(); 
}

#if 0
mstring::mstring(const mstring &r):
    fStr(r.fStr),
    fOffset(r.fOffset),
    fCount(r.fCount)

{
    PRECONDITION(fOffset >= 0);
    PRECONDITION(fCount >= 0);
    if (fStr) acquire();
}
#endif

mstring::mstring(const char *str) {
    init(str, str ? strlen(str) : 0);
}

mstring::mstring(const char *str, int len) {
    init(str, len);
}

void mstring::init(const char *str, int len) {
    if (str) {
        int count = len;
        MStringData *ud = 
            (MStringData *)malloc(sizeof(MStringData) + count + 1);
       
        ud->fRefCount = 0;
        memcpy(ud->fStr, str, count);
        ud->fStr[count] = 0;
        
        fStr = ud;
        fOffset = 0;
        fCount = count;
        acquire();
    } else {
        fStr = 0;
        fOffset = 0;
        fCount = 0; 
    }
}

void mstring::destroy() {
    free(fStr); fStr = 0;
}

mstring::~mstring() {
    if (fStr)
        release();
}

mstring mstring::operator=(const mstring& rv) {
    if (fStr != rv.fStr) {
        if (fStr) release();
        fStr = rv.fStr;
        if (fStr) acquire();
    }
    fOffset = rv.fOffset;
    fCount = rv.fCount;
    return *this;
}

mstring mstring::operator=(const class null_ref &) {
    if (fStr)
        release();
    fStr = 0;
    fCount = 0;
    fOffset = 0;
    return *this;
}

mstring mstring::fromMultiByte(const char *str, int len) {
    return newstr(str, len);
}

mstring mstring::fromMultiByte(const char *str) {
    return newstr(str);
}
mstring mstring::newstr(const char *str) {
    return newstr(str, str ? strlen(str) : 0);
}

mstring mstring::newstr(const char *str, int count) {
    PRECONDITION(count >= 0);
    PRECONDITION(str != 0 || count == 0);
    MStringData *ud = (MStringData *)malloc(sizeof(MStringData) + count + 1);

    ud->fRefCount = 0;
    memcpy(ud->fStr, str, count);
    ud->fStr[count] = 0;

    return mstring(ud, 0, count);
}

mstring mstring::substring(int pos) {
    PRECONDITION(pos >= 0);
    PRECONDITION(pos <= length());
    return mstring(fStr, fOffset + pos, fCount - pos);
}

mstring mstring::substring(int pos, int len) {
    PRECONDITION(pos >= 0);
    PRECONDITION(len >= 0);
    PRECONDITION(pos <= length());
    PRECONDITION(pos + len <= length());

    return mstring(fStr, fOffset + pos, len);
}

bool mstring::split(unsigned char token, mstring *left, mstring *remain) const {
    int i = fOffset;
    int e = fOffset + fCount;
    int c = 0;
    unsigned char ch;

    PRECONDITION(token < 128);
    while (i < e) {
        ch = fStr->fStr[i];
        if (ch == token) {
            mstring l(fStr, fOffset, i - fOffset);
            mstring r(fStr, i + 1, e - i - 1);

            *left = l;
            *remain = r;
            return true;
        }
        if (ch <= 0x7F || (ch >= 0xC0 && ch <= 0xFD)) {
            c++;
        }
        i++;
    }
    return false;
}

bool mstring::splitall(unsigned char token, mstring *left, mstring *remain) const {
    if (split(token, left, remain))
        return true;

    if (length() > 0) {
        *left = *this;
        *remain = null;
        return true;
    }
    return false;
}

int mstring::charAt(int pos) const {
    if (pos >= 0 && pos < length())
        return fStr->fStr[pos + fOffset];
    else
        return -1;
}

bool mstring::startsWith(const mstring &s) const {
    if (length() < s.length())
        return false;
    if (memcmp(data(), s.data(), s.length()) == 0)
        return true;
    return false;
}

bool mstring::endsWith(const mstring &s) const {
    if (length() < s.length())
        return false;
    if (memcmp(data() + length() - s.length(), s.data(), s.length()) == 0)
        return true;
    return false;
}

int mstring::indexOf(char ch) const {
    char *s = (char *)memchr(data(), ch, fCount);
    if (s == NULL)
        return -1;
    return s - fStr->fStr - fOffset;
}

bool mstring::equals(const mstring &s) const {
    return compareTo(s) == 0;
}

int mstring::compareTo(const mstring &s) const {
#if upstream_comp
    if (s.length() > length()) {
        return -1;
    } else if (s.length() == length()) {
        if (fCount == 0)
            return 0;
        else
            return memcmp(s.data(), data(), fCount);
    } else {
        return 1;
    }
#else
    int res=memcmp(data(), s.data(), min(s.length(), length()));
    if (s.length() == length())
       return res;
    if(res) // different length, left part already not equal
       return res;
    return length()-s.length();
#endif
}

bool mstring::copy(char *dst, size_t len) const {
    strncpy(dst, data(), len);
    if ((size_t) fCount < len)
	dst[fCount] = '\0';
    dst[len - 1] = '\0';
    return (size_t) fCount < len;
}

mstring mstring::replace(int position, int len, const mstring &insert) {
    PRECONDITION(position >= 0);
    PRECONDITION(len >= 0);
    PRECONDITION(position + len <= length());
    int count = length() - len + insert.length();

    MStringData *ud = (MStringData *)malloc(sizeof(MStringData) + count + 1);

    ud->fRefCount = 0;
    memcpy(ud->fStr, data(), position);
    memcpy(ud->fStr + position, insert.data(), insert.fCount);
    memcpy(ud->fStr + position + insert.fCount, data() + position + len, fCount - len - position);
    ud->fStr[count] = 0;
    return mstring(ud, 0, count);
}

mstring mstring::remove(int position, int len) {
    PRECONDITION(position >= 0);
    PRECONDITION(len >= 0);
    PRECONDITION(position + len <= length());
    int count = length() - len;

    MStringData *ud = (MStringData *)malloc(sizeof(MStringData) + count + 1);

    ud->fRefCount = 0;
    memcpy(ud->fStr, data(), position);
    memcpy(ud->fStr + position, data() + position + len, fCount - len - position);
    ud->fStr[count] = 0;
    return mstring(ud, 0, count);
}

mstring mstring::insert(int position, const mstring &s) {
    return replace(position, 0, s);
}

mstring mstring::append(const mstring &s) {
    return replace(length(), 0, s);
}

mstring mstring::trim() const {
    int pos = 0;
    int len = fCount;
    while (pos < len) {
        char ch = fStr->fStr[fOffset + pos];
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
            pos++;
            len--;
        } else
            break;
    }
    while (len > 0) {
        char ch = fStr->fStr[fOffset + pos + len - 1];
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
            len--;
        } else
            break;
    }
    return mstring(fStr, fOffset + pos, len);
}
