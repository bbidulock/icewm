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
#include <regex.h>
#include <new>
#include "base.h"
#include "ascii.h"

void MStringRef::alloc(size_t len) {
    fStr = new (malloc(sizeof(MStringData) + len + 1)) MStringData();
}

void MStringRef::create(const char* str, size_t len) {
    if (len) {
        alloc(len);
        if (str) {
            strncpy(fStr->fStr, str, len);
            fStr->fStr[len] = 0;
        } else {
            memset(fStr->fStr, 0, len + 1);
        }
    } else {
        fStr = nullptr;
    }
}

mstring::mstring(const MStringRef& str, size_t offset, size_t count):
    fRef(str),
    fOffset(offset),
    fCount(count)
{
    acquire();
}

mstring::mstring(const char* str1, size_t len1, const char* str2, size_t len2):
    fRef(len1 + len2), fOffset(0), fCount(len1 + len2)
{
    if (fRef) {
        if (len1)
            strncpy(fRef->fStr, str1, len1);
        if (len2)
            strncpy(fRef->fStr + len1, str2, len2);
        fRef[fCount] = 0;
        fRef.acquire();
    }
}

mstring::mstring(const char* str):
    mstring(str, str ? strlen(str) : 0)
{
}

mstring::mstring(const char* str, size_t len):
    fRef(str, len), fOffset(0), fCount(len)
{
    acquire();
}

mstring::mstring(const char *str1, const char *str2):
    mstring(str1, str1 ? strlen(str1) : 0, str2, str2 ? strlen(str2) : 0)
{
}

mstring::mstring(const char *str1, const char *str2, const char *str3) {
    size_t len1 = str1 ? strlen(str1) : 0;
    size_t len2 = str2 ? strlen(str2) : 0;
    size_t len3 = str3 ? strlen(str3) : 0;
    fOffset = 0;
    fCount = len1 + len2 + len3;
    fRef.alloc(fCount);
    if (len1) memcpy(fRef->fStr, str1, len1);
    if (len2) memcpy(fRef->fStr + len1, str2, len2);
    if (len3) memcpy(fRef->fStr + len1 + len2, str3, len3);
    fRef[fCount] = 0;
    fRef.acquire();
}

mstring::mstring(long n):
    fRef(size_t(23)), fOffset(0), fCount(0)
{
    snprintf(fRef->fStr, 23, "%ld", n);
    fCount = strlen(fRef->fStr);
    fRef.acquire();
}

mstring mstring::operator+(const mstring& rv) const {
    return rv.isEmpty() ? *this : isEmpty() ? rv :
        mstring(data(), length(), rv.data(), rv.length());
}

void mstring::operator+=(const mstring& rv) {
    *this = *this + rv;
}

mstring& mstring::operator=(const mstring& rv) {
    if (fRef != rv.fRef) {
        release();
        fRef = rv.fRef;
        acquire();
    }
    fOffset = rv.fOffset;
    fCount = rv.fCount;
    return *this;
}

mstring mstring::substring(size_t pos) const {
    return pos <= length()
        ? mstring(fRef, fOffset + pos, fCount - pos)
        : null;
}

mstring mstring::substring(size_t pos, size_t len) const {
    return pos <= length()
        ? mstring(fRef, fOffset + pos, min(len, fCount - pos))
        : null;
}

bool mstring::split(unsigned char token, mstring *left, mstring *remain) const {
    PRECONDITION(token < 128);
    int splitAt = indexOf(char(token));
    if (splitAt >= 0) {
        size_t i = size_t(splitAt);
        mstring l(substring(0, i));
        mstring r(substring(i + 1, length() - i - 1));
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

int mstring::collate(mstring s, bool ignoreCase) {
    if (!ignoreCase)
        return strcoll(*this, s);
    else
        return strcoll(this->lower(), s.lower());
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
    mstring mstr(nullptr, fCount);
    for (size_t i = 0; i < fCount; ++i) {
        mstr.fRef[i] = ASCII::toLower(data()[i]);
    }
    return mstr;
}

mstring mstring::upper() const {
    mstring mstr(nullptr, fCount);
    for (size_t i = 0; i < fCount; ++i) {
        mstr.fRef[i] = ASCII::toUpper(data()[i]);
    }
    return mstr;
}

mstring mstring::trim() const {
    size_t k = 0, n = length();
    while (k < n && ASCII::isWhiteSpace(data()[k])) {
        ++k;
    }
    while (k < n && ASCII::isWhiteSpace(data()[n - 1])) {
        --n;
    }
    return substring(k, n - k);
}

const char* mstring::c_str()
{
    if (isEmpty()) {
        return "";
    }
    else if (data()[fCount]) {
        if (fRef->fRefCount == 1) {
            fRef[fOffset + fCount] = '\0';
        } else {
            const char* str = data();
            fRef.release();
            fRef.create(str, fCount);
            fRef.acquire();
            fOffset = 0;
        }
    }
    return data();
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

// vim: set sw=4 ts=4 et:
