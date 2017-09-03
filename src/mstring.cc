/*
 * IceWM
 *
 * Copyright (C) 2004,2005 Marko Macek
 */
#include "config.h"

#include "mstring.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "base.h"
#include "ascii.h"

MStringData *MStringData::alloc(int length) {
    size_t size = sizeof(MStringData) + (size_t) length + 1;
    MStringData *ud = (MStringData *) malloc(size);
    ud->fRefCount = 0;
    return ud;
}

MStringData *MStringData::create(const char *str, int length) {
    MStringData *ud = MStringData::alloc(length);
    strncpy(ud->fStr, str, length);
    ud->fStr[length] = 0;
    return ud;
}

MStringData *MStringData::create(const char *str) {
    return create(str, (int) strlen(str));
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

mstring::mstring(const char *str) {
    init(str, str ? strlen(str) : 0);
}

mstring::mstring(const char *str, int len) {
    init(str, len);
}

mstring::mstring(const char *str1, const char *str2) {
    int len1 = (int)(str1 ? strlen(str1) : 0);
    int len2 = (int)(str2 ? strlen(str2) : 0);
    if (len1) {
        init(str1, len1 + len2);
        if (len2)
            strcpy(fStr->fStr + len1, str2);
    } else {
        init(str2, len2);
    }
}

mstring::mstring(const char *str1, const char *str2, const char *str3) {
    int len1 = (int)(str1 ? strlen(str1) : 0);
    int len2 = (int)(str2 ? strlen(str2) : 0);
    int len3 = (int)(str3 ? strlen(str3) : 0);
    fOffset = 0;
    fCount = len1 + len2 + len3;
    fStr = MStringData::alloc(fCount);
    memcpy(fStr->fStr, str1, len1);
    memcpy(fStr->fStr + len1, str2, len2);
    memcpy(fStr->fStr + len1 + len2, str3, len3);
    fStr->fStr[fCount] = 0;
    acquire();
}

void mstring::init(const char *str, int len) {
    if (str) {
        fStr = MStringData::create(str, len);
        fOffset = 0;
        fCount = len;
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

mstring mstring::operator+(const mstring& rv) const {
    if (!length()) return rv;
    if (!rv.length()) return *this;
    int newCount = length() + rv.length();
    MStringData *ud = MStringData::alloc(newCount);
    memcpy(ud->fStr, data(), length());
    memcpy(ud->fStr + length(), rv.data(), rv.length());
    ud->fStr[newCount] = 0;
    return mstring(ud, 0, newCount);
}

mstring& mstring::operator+=(const mstring& rv) {
    *this = *this + rv;
    return *this;
}

mstring& mstring::operator=(const mstring& rv) {
    if (fStr != rv.fStr) {
        if (fStr) release();
        fStr = rv.fStr;
        if (fStr) acquire();
    }
    fOffset = rv.fOffset;
    fCount = rv.fCount;
    return *this;
}

mstring& mstring::operator=(const class null_ref &) {
    if (fStr) {
        release();
        fStr = 0;
        fCount = 0;
        fOffset = 0;
    }
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

    MStringData *ud = MStringData::create(str, count);
    return mstring(ud, 0, count);
}

mstring mstring::substring(int pos) const {
    PRECONDITION(pos >= 0);
    PRECONDITION(pos <= length());
    return mstring(fStr, fOffset + pos, fCount - pos);
}

mstring mstring::substring(int pos, int len) const {
    PRECONDITION(pos >= 0);
    PRECONDITION(len >= 0);
    PRECONDITION(pos <= length());

    return mstring(fStr, fOffset + pos, min(len, fCount - pos));
}

bool mstring::split(unsigned char token, mstring *left, mstring *remain) const {
    PRECONDITION(token < 128);
    int i = indexOf((char) token);
    if (i >= 0) {
        mstring l = substring(0, i);
        mstring r = substring(i + 1, length() - i - 1);
        *left = l;
        *remain = r;
        return true;
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
        return data()[pos];
    else
        return -1;
}

bool mstring::startsWith(const mstring &s) const {
    if (length() < s.length())
        return false;
    if (s.length() == 0)
        return true;
    if (memcmp(data(), s.data(), s.length()) == 0)
        return true;
    return false;
}

bool mstring::endsWith(const mstring &s) const {
    if (length() < s.length())
        return false;
    if (s.length() == 0)
        return true;
    if (memcmp(data() + length() - s.length(), s.data(), s.length()) == 0)
        return true;
    return false;
}

int mstring::find(const mstring &s) const {
    int stop = length() - s.length();
    for (int start = 0; start <= stop; ++start) {
        for (int i = 0; ; ++i) {
            if (i == s.length()) return start;
            if (data()[i + start] != s.data()[i]) break;
        }
    }
    return -1;
}

int mstring::indexOf(char ch) const {
    if (length() == 0)
        return -1;
    char *s = (char *)memchr(data(), ch, fCount);
    if (s == NULL)
        return -1;
    return s - data();
}

int mstring::lastIndexOf(char ch) const {
    for (int k = length() - 1; k >= 0; --k) {
        if (ch == data()[k]) return k;
    }
    return -1;
}

int mstring::count(char ch) const {
    int n = 0;
    for (int k = 0; k < length(); ++k) {
        n += ch == data()[k];
    }
    return n;
}

bool mstring::equals(const char *s) const {
    int len = s ? (int) strlen(s) : 0;
    return len == length() && 0 == memcmp(s, data(), len);
}

bool mstring::equals(const mstring &s) const {
    return compareTo(s) == 0;
}

int mstring::collate(const mstring &s, bool ignoreCase) const {
    if (!ignoreCase)
        return strcoll(cstring(*this), cstring(s));
    else
        return strcoll(cstring(this->lower()), cstring(s.lower()));
}

int mstring::compareTo(const mstring &s) const {
#if upstream_comp
    if (s.length() > length()) {
        return -1;
    } else if (s.length() == length()) {
        if (length() == 0)
            return 0;
        return memcmp(s.data(), data(), fCount);
    } else {
        return 1;
    }
#else
    int minlen = min(s.length(), length());
    if (minlen > 0) {
        int res = memcmp(data(), s.data(), minlen);
        if (res)
           return res;
    }
    return length() - s.length();
#endif
}

bool mstring::copyTo(char *dst, size_t len) const {
    if (len > 0 && fCount > 0) {
        size_t minlen = min(len - 1, (size_t) fCount);
        memcpy(dst, data(), minlen);
        dst[minlen] = 0;
    }
    return (size_t) fCount < len;
}

mstring mstring::replace(int position, int len, const mstring &insert) const {
    PRECONDITION(position >= 0);
    PRECONDITION(len >= 0);
    PRECONDITION(position + len <= length());
    return substring(0, position) + insert
        + substring(position + len, length() - position - len);
}

mstring mstring::remove(int position, int len) const {
    return replace(position, len, mstring(null));
}

mstring mstring::insert(int position, const mstring &s) const {
    return replace(position, 0, s);
}

mstring mstring::append(const mstring &s) const {
    return *this + s;
}

mstring mstring::searchAndReplaceAll(const mstring& s, const mstring& r) const {
    mstring modified(*this);
    const int step = 1 + r.length() - s.length();
    for (int offset = 0; offset + s.length() <= modified.length(); ) {
        int found = offset + modified.substring(offset).find(s);
        if (found < offset) {
            break;
        }
        modified = modified.replace(found, s.length(), r);
        offset = max(0, offset + step);
    }
    return modified;
}

mstring mstring::lower() const {
    if (fStr) {
        MStringData *ud = MStringData::create(data(), fCount);
        for (int i = 0; i < fCount; ++i) {
            ud->fStr[i] = ASCII::toLower(ud->fStr[i]);
        }
        return mstring(ud, 0, fCount);
    }
    return null;
}

mstring mstring::upper() const {
    if (*this == null) {
        return null;
    } else {
        MStringData *ud = MStringData::create(data(), fCount);
        for (int i = 0; i < fCount; ++i) {
            ud->fStr[i] = ASCII::toUpper(ud->fStr[i]);
        }
        return mstring(ud, 0, fCount);
    }
}

mstring mstring::trim() const {
    int k = 0, n = length();
    while (k < n && isspace((unsigned char) charAt(k))) {
        ++k;
    }
    while (k < n && isspace((unsigned char) charAt(n - 1))) {
        --n;
    }
    return substring(k, n - k);
}

void mstring::normalize()
{
    if (fStr) {
        if (data()[fCount]) {
            MStringData *ud = MStringData::create(data(), fCount);
            release();
            fStr = ud;
            fOffset = 0;
            acquire();
        }
    }
}

cstring::cstring(const mstring &s): str(s) {
    str.normalize();
}

cstring& cstring::operator=(const cstring& cs) {
    str = cs.str;
    return *this;
}

