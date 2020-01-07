/*
 * IceWM
 *
 * Copyright (C) 2004,2005 Marko Macek
 */
#include "config.h"

#include "mstring.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <regex.h>
#include "base.h"
#include "ascii.h"

MStringData *MStringData::alloc(size_t length) {
    size_t size = sizeof(MStringData) + length + 1;
    MStringData *ud = static_cast<MStringData *>(malloc(size));
    ud->fRefCount = 0;
    return ud;
}

MStringData *MStringData::create(const char *str, size_t length) {
    MStringData *ud = MStringData::alloc(length+1);
    memcpy(ud->fStr, str, length);
    ud->fStr[length] = 0;
    return ud;
}

MStringData *MStringData::create(const char *str) {
    return create(str, strlen(str));
}

mstring::mstring(MStringData *fStr, size_t fOffset, size_t fCount):
    fStr(fStr),
    fOffset(fOffset),
    fCount(fCount)
{
    if (fStr) acquire();
}

void mstring::init(const char *str, size_t len) {
    if (str) {
        fStr = MStringData::create(str, len);
        fOffset = 0;
        fCount = len;
        acquire();
    } else {
        fStr = nullptr;
        fOffset = 0;
        fCount = 0;
    }
}

mstring::mstring(const char *str) {
    init(str, str ? strlen(str) : 0);
}

mstring::mstring(const char *str, size_t len) {
    init(str, len);
}

mstring::mstring(const char *str1, const char *str2) {
    size_t len1 = str1 ? strlen(str1) : 0;
    size_t len2 = str2 ? strlen(str2) : 0;
    if (len1) {
        init(str1, len1 + len2);
        if (len2)
            strlcpy(fStr->fStr + len1, str2, len2 + 1);
    } else {
        init(str2, len2);
    }
}

mstring::mstring(const char *str1, const char *str2, const char *str3) {
    size_t len1 = str1 ? strlen(str1) : 0;
    size_t len2 = str2 ? strlen(str2) : 0;
    size_t len3 = str3 ? strlen(str3) : 0;
    fOffset = 0;
    fCount = len1 + len2 + len3;
    fStr = MStringData::alloc(fCount);
    memcpy(fStr->fStr, str1, len1);
    memcpy(fStr->fStr + len1, str2, len2);
    memcpy(fStr->fStr + len1 + len2, str3, len3);
    fStr->fStr[fCount] = 0;
    acquire();
}

mstring::mstring(long n) {
    char num[24];
    snprintf(num, sizeof num, "%ld", n);
    init(num, strlen(num));
}

void mstring::destroy() {
    free(fStr); fStr = nullptr;
}

mstring::~mstring() {
    if (fStr)
        release();
}

mstring mstring::operator+(const mstring& rv) const {
    if (isEmpty()) return rv;
    if (rv.isEmpty()) return *this;
    size_t newCount = length() + rv.length();
    MStringData *ud = MStringData::alloc(newCount);
    memcpy(ud->fStr, data(), length());
    memcpy(ud->fStr + length(), rv.data(), rv.length());
    ud->fStr[newCount] = 0;
    return mstring(ud, 0, newCount);
}

mstring& mstring::operator+=(const mstring& rv) {
    return *this = *this + rv;
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

mstring& mstring::operator=(null_ref &) {
    if (fStr) {
        release();
        fStr = nullptr;
        fCount = 0;
        fOffset = 0;
    }
    return *this;
}

mstring mstring::substring(size_t pos) const {
    return pos <= length()
        ? mstring(fStr, fOffset + pos, fCount - pos)
        : null;
}

mstring mstring::substring(size_t pos, size_t len) const {
    return pos <= length()
        ? mstring(fStr, fOffset + pos, min(len, fCount - pos))
        : null;
}

bool mstring::split(unsigned char token, mstring *left, mstring *remain) const {
    PRECONDITION(token < 128);
    int splitAt = indexOf(char(token));
    if (splitAt >= 0) {
        size_t i = size_t(splitAt);
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

    if (nonempty()) {
        *left = *this;
        *remain = null;
        return true;
    }
    return false;
}

int mstring::charAt(int pos) const {
    return size_t(pos) < length() ? data()[pos] : -1;
}

bool mstring::startsWith(const mstring &s) const {
    return s.isEmpty() || (s.length() <= length()
        && 0 == memcmp(data(), s.data(), s.length()));
}

bool mstring::endsWith(const mstring &s) const {
    return s.isEmpty() || (s.length() <= length()
        && 0 == memcmp(data() + length() - s.length(), s.data(), s.length()));
}

int mstring::find(const mstring &str) const {
    const char* found = (str.isEmpty() || isEmpty()) ? nullptr :
        static_cast<const char*>(memmem(
                data(), length(), str.data(), str.length()));
    return found ? int(found - data()) : (str.isEmpty() - 1);
}

int mstring::indexOf(char ch) const {
    const char *str = isEmpty() ? nullptr :
        static_cast<const char *>(memchr(data(), ch, fCount));
    return str ? int(str - data()) : -1;
}

int mstring::lastIndexOf(char ch) const {
    const char *str = isEmpty() ? nullptr :
        static_cast<const char *>(memrchr(data(), ch, fCount));
    return str ? int(str - data()) : -1;
}

int mstring::count(char ch) const {
    int num = 0;
    for (const char* str = data(), *end = str + length(); str < end; ++str) {
        str = static_cast<const char *>(memchr(str, ch, end - str));
        if (str == nullptr)
            break;
        ++num;
    }
    return num;
}

bool mstring::equals(const char *str) const {
    return equals(str, str ? strlen(str) : 0);
}

bool mstring::equals(const char *str, size_t len) const {
    return len == length() && 0 == memcmp(str, data(), len);
}

bool mstring::equals(const mstring &str) const {
    return equals(str.data(), str.length());
}

int mstring::collate(const mstring &s, bool ignoreCase) const {
    if (!ignoreCase)
        return strcoll(cstring(*this), cstring(s));
    else
        return strcoll(cstring(this->lower()), cstring(s.lower()));
}

int mstring::compareTo(const mstring &s) const {
    size_t len = min(s.length(), length());
    int cmp = len ? memcmp(data(), s.data(), len) : 0;
    return cmp ? cmp : int(length()) - int(s.length());
}

bool mstring::copyTo(char *dst, size_t len) const {
    if (len > 0) {
        size_t copy = min(len - 1, fCount);
        if (copy) memcpy(dst, data(), copy);
        dst[copy] = 0;
    }
    return fCount < len;
}

mstring mstring::replace(int pos, int len, const mstring &insert) const {
    return substring(0, size_t(pos)) + insert + substring(size_t(pos + len));
}

mstring mstring::remove(int pos, int len) const {
    return substring(0, size_t(pos)) + substring(size_t(pos + len));
}

mstring mstring::insert(int pos, const mstring &str) const {
    return substring(0, size_t(pos)) + str + substring(size_t(pos));
}

mstring mstring::searchAndReplaceAll(const mstring& s, const mstring& r) const {
    mstring modified(*this);
    const int step = int(1 + r.length() - s.length());
    for (int offset = 0; size_t(offset) + s.length() <= modified.length(); ) {
        int found = offset + modified.substring(size_t(offset)).find(s);
        if (found < offset) {
            break;
        }
        modified = modified.replace(found, int(s.length()), r);
        offset = max(0, offset + step);
    }
    return modified;
}

mstring mstring::lower() const {
    if (fStr) {
        MStringData *ud = MStringData::create(data(), fCount);
        for (unsigned i = 0; i < fCount; ++i) {
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
        for (unsigned i = 0; i < fCount; ++i) {
            ud->fStr[i] = ASCII::toUpper(ud->fStr[i]);
        }
        return mstring(ud, 0, fCount);
    }
}

mstring mstring::trim() const {
    size_t k = 0, n = length();
    while (k < n && isspace(static_cast<unsigned char>(data()[k]))) {
        ++k;
    }
    while (k < n && isspace(static_cast<unsigned char>(data()[n - 1]))) {
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

mstring mstring::match(const char* regex, const char* flags) const {
    int compFlags = REG_EXTENDED;
    int execFlags = 0;
    for (int i = 0; flags && flags[i]; ++i) {
        switch (flags[i]) {
            case 'i': compFlags |= REG_ICASE; break;
            case 'n': compFlags |= REG_NEWLINE; break;
            case 'B': execFlags |= REG_NOTBOL; break;
            case 'E': execFlags |= REG_NOTEOL; break;
        }
    }

    regex_t preg;
    int comp = regcomp(&preg, regex, compFlags);
    if (comp) {
        if (testOnce(regex, __LINE__)) {
            char rbuf[123] = "";
            regerror(comp, &preg, rbuf, sizeof rbuf);
            warn("match regcomp: %s", rbuf);
        }
        return null;
    }

    regmatch_t pos;
    int exec = regexec(&preg, data(), 1, &pos, execFlags);

    regfree(&preg);

    if (exec)
        return null;

    return mstring(data() + pos.rm_so, size_t(pos.rm_eo - pos.rm_so));
}

cstring::cstring(const mstring &s): str(s) {
    str.normalize();
}

cstring& cstring::operator=(const cstring& cs) {
    str = cs.str;
    return *this;
}


// vim: set sw=4 ts=4 et:
