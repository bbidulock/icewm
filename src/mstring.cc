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
#include "mstringex.h"
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

#include <fnmatch.h>
#include <regex.h>

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

mstring& mstring::operator=(mstring_view rv) {
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
        return *this = mstring(mstring_view(rv));
    if (!isLocal())
        free(get_ptr());
    spod = rv.spod;
    rv.set_len(0);
    return *this;
}

mstring& mstring::operator +=(mstring_view rv) {
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
        spod.cBytes[offsetPodCounter] = uint8_t(len);
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
            if(!p)
                abort();
            memcpy(p, data(), length() + 1);
            set_ptr(p);
            set_len(new_len);
        }
    } else {
        auto pNew = realloc(get_ptr(), new_len + 1);
        // XXX: do something more useful if failed?
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

mstring mstring::substring(size_type pos) const {
    auto l = length();
    return pos <= l ? mstring(data() + pos, l - pos) : null;
}

mstring mstring::substring(size_type pos, size_type len) const {
    return pos <= length() ?
            mstring(data() + pos, min(len, length() - pos)) : null;
}

bool mstring::split(unsigned char token, mstring *left, mstring *remain) const {
    PRECONDITION(token < 128);
    int splitAt = indexOf(char(token));
    if (splitAt < 0)
        return false;
    size_type i = size_t(splitAt);
    *left = substring(0, i);
    *remain = substring(i + 1, length() - i - 1);
    return true;
}

bool mstring::splitall(unsigned char token, mstring *left,
        mstring *remain) const {
    if (split(token, left, remain))
        return true;
    if (isEmpty())
        return false;
    *left = *this;
    *remain = null;
    return true;
}

int mstring::charAt(int pos) const {
    return size_t(pos) < length() ? data()[pos] : -1;
}

bool mstring::startsWith(mstring_view s) const {
    if (s.isEmpty())
        return true;
    if (s.length() > length())
        return false;
    return 0 == memcmp(data(), s.data(), s.length());
}

bool mstring::endsWith(mstring_view s) const {
    if (s.isEmpty())
        return true;
    if (s.length() > length())
        return false;
    return (0 == memcmp(data() + length() - s.length(), s.data(), s.length()));
}

int mstring::find(mstring_view str) const {
    const char* found = (str.isEmpty() || isEmpty()) ? nullptr :
        static_cast<const char*>(memmem(data(), length(),
                str.data(), str.length()));
    return found ? int(found - data()) : (str.isEmpty() - 1);
}

int mstring::indexOf(char ch) const {
    const char *str = isEmpty() ? nullptr :
        static_cast<const char*>(memchr(data(), ch, length()));
    return str ? int(str - data()) : -1;
}

int mstring::lastIndexOf(char ch) const {
    const char *str = isEmpty() ? nullptr :
            static_cast<const char*>(memrchr(data(), ch, length()));
    return str ? int(str - data()) : -1;
}

int mstring::count(char ch) const {
    int num = 0;
    for (const char* str = data(), *end = str + length(); str < end; ++str) {
        str = static_cast<const char*>(memchr(str, ch, end - str));
        if (str == nullptr)
            break;
        ++num;
    }
    return num;
}

bool mstring_view::operator==(mstring_view sv) const {
    auto lenA = sv.length();
    auto lenB = length();
    return lenA == lenB && 0 == memcmp(sv.data(), data(), lenA);
}

int mstring::collate(const mstring &s, bool ignoreCase) const {
    if (ignoreCase)
        return strcoll(this->lower(), s.lower());
    return strcoll(c_str(), s.c_str());
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

mstring mstring::replace(size_type pos, size_type len,
        const mstring &insert) const {
    return substring(0, size_t(pos)) + insert + substring(size_t(pos + len));
}

mstring mstring::remove(size_type p, size_type l) const {
    return mstring(mstring_view(data(), size_t(p)),
            mstring_view(data() + size_t(p) + size_t(l), length() - p - l));
}

mstring mstring::insert(size_type pos, const mstring &str) const {
    mstring_view right(data() + pos, length() - pos);
    return mstring(mstring_view(data(), pos), str, right);
}

mstring mstring::searchAndReplaceAll(const mstring &s, const mstring &r) const {
    mstring modified(*this);
    const int step = int(1 + r.length() - s.length());
    for (int offset = 0; size_t(offset) + s.length() <= modified.length();) {
        int found = offset + modified.substring(size_t(offset)).find(s);
        if (found < offset)
            break;
        modified = modified.replace(found, int(s.length()), r);
        offset = max(0, offset + step);
    }
    return modified;
}

mstring mstring::lower() const {
    mstring ret;
    auto l = length();
    ret.extendTo(l);
    for (size_type i = 0; i < l; ++i) {
        ret.data()[i] = ASCII::toLower(data()[i]);
    }
    ret.term(l);
    return ret;
}

mstring mstring::upper() const {
    mstring ret;
    auto l = length();
    ret.extendBy(l);
    for (size_type i = 0; i < l; ++i) {
        ret.data()[i] = ASCII::toUpper(data()[i]);
    }
    ret.term(l);
    return ret;
}

mstring mstring::trim() const {
    size_type k = 0, n = length();
    while (k < n && ASCII::isWhiteSpace(data()[k])) {
        ++k;
    }
    while (k < n && ASCII::isWhiteSpace(data()[n - 1])) {
        --n;
    }
    return substring(k, n - k);
}

inline bool mstring::input_from_here(mstring_view sv) {
    const char *svdata = sv.data(), *svend = svdata + sv.length(), *begin =
            data(), *end = begin + length();
    // inrange() suits here because of extra terminator char
    return inrange(svdata, begin, end) || inrange(svend, begin, end);
}

// this is extra copy-pasty in order to let the compiler optimize it better
mstring::mstring(mstring_view a, mstring_view b, mstring_view c, mstring_view d,
        mstring_view e) : mstring(null) {
    auto len = a.length() + b.length() + c.length() + d.length() + e.length();
    extendBy(len);
    term(len);
    size_type pos(0);
    auto app = [&](mstring_view& z) {
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

mstring::mstring(mstring_view a, mstring_view b) :
        mstring(a, b, mstring_view(), mstring_view(), mstring_view()) {
}

mstring::mstring(mstring_view a, mstring_view b, mstring_view c) :
        mstring(a, b, c, mstring_view(), mstring_view()) {
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

enum STATE_FLAGS {
    STATE_NONE = 0,
    STATE_COMPILED = 1,
    STATE_ERROR = 2
};

mstring mstring::match(const precompiled_regex &rex) const {
    if (rex.stateFlags == STATE_ERROR) {
        warn("match regcomp: %s", rex.mCompError);
        return null;
    }
    regmatch_t pos;
    int exec = regexec(&rex.preg, data(), 1, &pos, rex.execFlags);
    return exec ?
            null : mstring(data() + pos.rm_so, size_t(pos.rm_eo - pos.rm_so));
}

mstring mstring::match(const char *regex, const char *flags) const {
    return match(precompiled_regex(regex, flags));
}

precompiled_regex::precompiled_regex(const char *regex, const char *flags) :
        execFlags(0), stateFlags(0) {
    auto compFlags = REG_EXTENDED;
    for (int i = 0; flags && flags[i]; ++i) {
        switch (flags[i]) {
        case 'i':
            compFlags |= REG_ICASE;
            break;
        case 'n':
            compFlags |= REG_NEWLINE;
            break;
        case 'B':
            execFlags |= REG_NOTBOL;
            break;
        case 'E':
            execFlags |= REG_NOTEOL;
            break;
        }
    }
    int comp = regcomp(&preg, regex, compFlags);
    if (0 == comp) {
        stateFlags = STATE_COMPILED;
    } else {
        stateFlags = STATE_ERROR;
        auto sResult = new char[200];
        regerror(comp, &preg, sResult, 200);
        sResult[199] = 0x0;
        regfree(&preg);
        mCompError = sResult;
    }
}

precompiled_regex::~precompiled_regex() {
    switch(stateFlags)
    {
    case STATE_NONE: return;
    case STATE_COMPILED:
        regfree(&preg);
        return;
    case STATE_ERROR:
        delete [] mCompError;
        return;
    }
}

bool precompiled_regex::matchIn(const char *s) const {
    if (STATE_ERROR == stateFlags) {
        warn("match regcomp: %s", mCompError);
        return false;
    }
    regmatch_t pos;
    return 0 == regexec(&preg, s, 1, &pos, execFlags);
}


// vim: set sw=4 ts=4 et:
