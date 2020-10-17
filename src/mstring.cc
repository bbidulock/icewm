/*
 * IceWM
 *
 * Copyright (C) 2004-2009 Marko Macek
 * Copyright (C) 2017-2020 Bert Gijsbers
 * Copyright (C) 2017-2018 Brian Bidulock
 * Copyright (C) 2009,2015,2017-2020 Eduard Bloch
 */
#include "config.h"
#include "ref.h"
#include "mregex.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <new>
#include <cstdint>
#include <cstdarg>
#include <type_traits>

#include "base.h"
#include "ascii.h"

// inplace mode: counter in first byte, data following
#define offsetPodCounter (offsetof(decltype(mstring::spod), cBytes))
#define offsetPodData (offsetPodCounter + 1)

mstring::mstring(const char *str, size_type len) {
    static_assert(std::is_trivial<decltype(spod)>::value
            && std::is_standard_layout<decltype(spod)>::value,
            "Error: union based small string optimization N/A on this arch! "
            "Consider adding -DDEBUG_NOUTYPUN as workaround.");
    memset(&spod, 0, sizeof(spod));
    if (len) {
        extendTo(len);
        memcpy(data(), str, len);
    }
    term(len);
}

mstring::mstring(mstring &&other) {
    if (this == &other)
        return;
    // minimum required state for move operator to work correctly
    spod.count = 0;
    markExternal(false);
    *this = std::move(other);
}

mstring& mstring::operator=(mslice rv) {
    if (input_from_here(rv)) // we are the source -> copy in any case
        return *this = mstring(rv);
    auto l = rv.length();
    // release buffer if going back to local storage
    if (l <= MSTRING_INPLACE_MAXLEN)
        clear();
    extendTo(l);
    memcpy(data(), rv.data(), l);
    term(l);
    return *this;
}

mstring& mstring::operator=(mstring &&rv) {
    if (this == &rv)
        return *this;
    // cannot optimize?
    if (input_from_here(rv))
        return *this = mstring(mslice(rv));
    if (!isLocal())
        free(get_ptr());
    spod = rv.spod;
    if (isLocal()) // invalidates foreign pointer
        rv.set_len(0);
    return *this;
}

mstring& mstring::operator +=(mslice rv) {
    // if realloc() might damage the input, create it from scratch!
    if (input_from_here(rv))
        return *this = mstring(*this, rv);

    auto len = rv.length();
    auto curLen = length();
    if (len) {
        extendBy(len);
        memcpy(data() + curLen, rv.data(), len);
    }
    term(curLen + len);
    return *this;
}

inline void mstring::term(size_type len) {
    data()[len] = 0x0;
}

void mstring::set_len(size_type len, bool forceExternal) {
    if (len > MSTRING_INPLACE_MAXLEN || forceExternal) {
        spod.count = len;
        markExternal(true);
    } else {
#ifdef SSO_NOUTYPUN
        spod.count = len;
#else
        if (len)
            spod.cBytes[offsetPodCounter] = uint8_t(len);
        else
            spod.count = 0;
#endif
        markExternal(false);
    }
}

void mstring::clear() {
    if (!isLocal())
        free(data());
    set_len(0);
    term(0);
}

void swap(mstring &a, mstring &b) {
    swap(a.spod, b.spod);
}

inline void mstring::extendBy(size_type amount) {
    return extendTo(length() + amount);
}

// user must fill in the extended data (=amount) AND terminator later
inline void mstring::extendTo(size_type new_len) {
#ifdef SSO_NOUTYPUN
    if (isLocal()) {
        if (new_len <= MSTRING_INPLACE_MAXLEN) {
            set_len(new_len);
        } else {
            // local storage before, to become external now
            auto p = (char*) malloc(new_len + 1);
            if(!p) abort();
            memcpy(p, data(), length() + 1);
            memcpy((void*) spod.cBytes, &p, sizeof(p));
            set_len(new_len);
        }
    } else {
        auto p = data();
        p = (char*) realloc((void*) p, new_len + 1);
        // XXX: do something more useful if failed?
        if (!p) abort();
        memcpy((void*) spod.cBytes, &p, sizeof(p));
        set_len(new_len);
    }
#else
    if (isLocal()) {
        if (new_len <= MSTRING_INPLACE_MAXLEN) {
            set_len(new_len);
        } else {
            // local before, to become external now
            auto p = (char*) malloc(new_len + 1);
            if(!p) abort();
            memcpy(p, data(), length() + 1);
            set_ptr(p);
            set_len(new_len);
        }
    } else {
        auto pNew = realloc(get_ptr(), new_len + 1);
        // XXX: do something more useful than terminating?
        if (!pNew)
            abort();
        set_ptr((char*) pNew);
        set_len(new_len);
    }
#endif
}

const char* mstring::data() const {
#ifdef SSO_NOUTYPUN
    if(isLocal())
        return spod.cBytes;
    char *ret;
    memcpy(&ret, spod.cBytes, sizeof(ret));
    return ret;
#else
    return isLocal() ? (const char*) (spod.cBytes + offsetPodData) : get_ptr();
#endif
}
char* mstring::data() {
#ifdef SSO_NOUTYPUN
    if(isLocal())
        return spod.cBytes;
    char *ret;
    memcpy(&ret, spod.cBytes, sizeof(ret));
    return ret;
#else
    return isLocal() ? (char*) (spod.cBytes + offsetPodData) : get_ptr();
#endif
}

mslice mstring::substring(size_type pos) const {
    auto l = length();
    return pos <= l ? mslice(data() + pos, l - pos) : null;
}

mslice mslice::substring(size_t pos, size_t len) const {
    if (pos > length())
        return null;
    return mslice(data() + pos, min(len, length() - pos));
}

bool mslice::split(unsigned char token, mslice& left,
        mslice& remain) const {
    PRECONDITION(token < 128);
    int splitAt = indexOf(char(token));
    if (splitAt < 0)
        return false;
    auto l(substring(0, size_t(splitAt)));
    auto r(substring(size_t(splitAt) + 1, length() - size_t(splitAt) - 1));
    left = l;
    remain = r;
    return true;
}

bool mslice::splitall(unsigned char token, mslice& left, mslice& remain) const {
    if (split(token, left, remain))
        return true;
    if (isEmpty())
        return false;
    left = *this;
    remain = null;
    return true;
}

int mstring::charAt(int pos) const {
    return size_t(pos) < length() ? data()[pos] : -1;
}

// XXX: can actually also move this to mslice like lastIndexOf
bool mslice::startsWith(mslice s) const {
    if (s.isEmpty())
        return true;
    if (s.length() > length())
        return false;
    return 0 == memcmp(data(), s.data(), s.length());
}
// XXX: can actually also move this to mslice like lastIndexOf
bool mstring::endsWith(mslice s) const {
    if (s.isEmpty())
        return true;
    if (s.length() > length())
        return false;
    return (0 == memcmp(data() + length() - s.length(), s.data(), s.length()));
}

int mstring::find(mslice str, size_type startPos) const {
    if (startPos > length())
        return -1;
    mslice srange(data() + startPos, length() - startPos);
    if (srange.isEmpty())
        return str.isEmpty() ? 0 : -1;
    if (str.isEmpty())
        return 0;
    if (str.length() > srange.length()) // catch right here before memmem
        return -1;
    auto hit = static_cast<const char*>(memmem(srange.data(), srange.length(),
            str.data(), str.length()));
    return hit ? (hit - data()) : -1;
}

int mslice::indexOf(char ch) const {
    const char *str = isEmpty() ? nullptr :
        static_cast<const char*>(memchr(data(), ch, length()));
    return str ? int(str - data()) : -1;
}

int mslice::lastIndexOf(char ch) const {
    const char *str = isEmpty() ? nullptr :
            static_cast<const char*>(memrchr(m_data, ch, m_size));
    return str ? int(str - m_data) : -1;
}

int mstring::count(char ch) const {
    int num = 0;
    for (auto* str = data(), *end = str + length(); str < end; ++str) {
        str = static_cast<const char*>(memchr(str, ch, end - str));
        if (str == nullptr)
            break;
        ++num;
    }
    return num;
}

bool mslice::operator==(mslice sv) const {
    const auto lenA = sv.length();
    return lenA == length() && 0 == memcmp(sv.data(), data(), lenA);
}

int mstring::collate(const mstring &s, bool ignoreCase) const {
    return ignoreCase ? strcoll(this->lower(), s.lower()) :
            strcoll(c_str(), s.c_str());
}

int mstring::compareTo(const mstring &s) const {
    auto len = min(s.length(), length());
    int cmp = len ? memcmp(data(), s.data(), len) : 0;
    return cmp ? cmp : int(length()) - int(s.length());
}

bool mstring::copyTo(char *dst, size_type len) const {
    if (len > 0) {
        auto copy = min(len - 1, length());
        if (copy)
            memcpy(dst, data(), copy);
        dst[copy] = 0;
    }
    return length() < len;
}

mstring mstring::replace(size_type pos, size_type len, mslice insert) const {
    return mstring(substring(0, size_t(pos)), insert, substring(size_t(pos + len)));
}

mstring mstring::remove(size_type p, size_type l) const {
    return mstring(mslice(data(), size_t(p)),
            mslice(data() + size_t(p) + size_t(l), length() - p - l));
}

mstring mstring::insert(size_type pos, mslice str) const {
    mslice right(data() + pos, length() - pos);
    return mstring(mslice(data(), pos), str, right);
}

mstring mstring::modified(char (*mod)(char)) const {
    mstring ret;
    auto l = length();
    ret.extendTo(l);
    for (size_type i = 0; i < l; ++i) {
        ret.data()[i] = mod(data()[i]);
    }
    ret.term(l);
    return ret;
}

mstring mstring::lower() const {
    return modified(ASCII::toLower);
}

mstring mstring::upper() const {
    return modified(ASCII::toUpper);
}

mslice mslice::trim() const {
    size_t n = length(), k = 0;
    while (k < n && ASCII::isWhiteSpace(m_data[k])) {
        ++k;
    }
    while (k < n && ASCII::isWhiteSpace(m_data[n - 1])) {
        --n;
    }
    return mslice(m_data + k, n - k);
}

inline bool mstring::input_from_here(mslice sv) {
    const char *svdata = sv.data(), *svend = svdata + sv.length(), *begin =
            data(), *end = begin + length();
    // inrange() suits here because of extra terminator char
    return inrange(svdata, begin, end) || inrange(svend, begin, end);
}

// this is extra copy-pasty in order to let the compiler optimize it better
mstring::mstring(mslice a, mslice b, mslice c, mslice d, mslice e) :
        mstring() {

    auto len = a.length() + b.length() + c.length() + d.length() + e.length();
    extendBy(len);
    term(len);
    size_type pos(0);
    auto app = [&](mslice& z) {
        if(z.isEmpty()) return;
        memcpy(data() + pos, z.data(), z.length());
        pos += z.length();
    };
    app(a);
    app(b);
    app(c);
    app(d);
    app(e);
    term(pos);
    assert(pos == len);
}

mstring::mstring(mslice a, mslice b) :
        mstring(a, b, mslice(), mslice(), mslice()) {
}

mstring::mstring(mslice a, mslice b, mslice c) :
        mstring(a, b, c, mslice(), mslice()) {
}

mstring::~mstring() {
    if (!isLocal())
        free(data());
}

bool mstring::equals(const char *&sz) const {
    return 0 == strcmp(sz, c_str());
}

mstring& mstring::appendFormat(const char *fmt, ...) {
    auto startLen = length();
    // it might fit into locally available space, can try that for free!
    auto haveLocalSpace =
            startLen < MSTRING_INPLACE_MAXLEN ?
                    (MSTRING_INPLACE_MAXLEN + 1 - startLen) : 0;
    va_list ap;
    va_start(ap, fmt);
    auto wantsLen = vsnprintf(haveLocalSpace ? data() + startLen : nullptr,
            haveLocalSpace, fmt, ap);
    va_end(ap);
    if (wantsLen < 0)
    {
        warn("bad format string or parameters: %s", fmt);
        data()[startLen] = 0x0;
        return *this;
    }
    if (haveLocalSpace) {
        if (size_type(wantsLen) < haveLocalSpace) {
            // it did all fit? Great!
            set_len(startLen + wantsLen);
            return *this;
        }
        // not lucky, undo the effects
        data()[startLen] = 0x0;
    }
    extendBy(wantsLen);
    va_start(ap, fmt);
    vsnprintf(data() + startLen, wantsLen + 1, fmt, ap);
    va_end(ap);
    return *this;
}

mstring mstring::take(char *buf, size_type buf_len) {
    mstring ret;
    ret.set_len(buf_len, true);
    ret.set_ptr(buf);
    return ret;
}

void mstring::set_ptr(char *p) {
#ifdef SSO_NOUTYPUN
    memcpy(spod.cBytes, &p, sizeof(p));
#else
    spod.ext.p = (char *) p;
#endif
}

char* mstring::get_ptr()
{
#ifdef SSO_NOUTYPUN
    char *ret;
    memcpy(&ret, spod.cBytes, sizeof(ret));
    return ret;
#else
    return spod.ext.p;
#endif
}

// vim: set sw=4 ts=4 et:
