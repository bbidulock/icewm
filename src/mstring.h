#ifndef MSTRING_H
#define MSTRING_H

#ifdef YARRAY_H
#error include yarray.h after mstring.h
#endif

#include "ref.h"
#include <cstdlib>
#include <cinttypes>
#include <utility>

class mstring;
class precompiled_regex;

class mstring_view {
    const char *m_data;
    std::size_t m_size;
public:
    mstring_view(const char *s);
    mstring_view(const char *s, std::size_t len) :
            m_data(s), m_size(len) {
    }
    mstring_view(null_ref &) : m_data(nullptr), m_size(0) {}
    mstring_view() : mstring_view(null) {}
    mstring_view(const mstring& s);
    std::size_t length() const { return m_size; }
    const char* data() const { return m_data; }
    bool operator==(mstring_view rv) const;
    bool isEmpty() const { return length() == 0; }
};
/*
 * Mutable strings with a small string optimization.
 */
class mstring {
public:
    using size_type = std::size_t;
private:
    friend mstring operator+(const char* s, const mstring& m);
    friend void swap(mstring& a, mstring& b);
    friend class mstring_view;

#if defined(__GNUC__) && ! defined(NOUTYPUN)
    struct TRefData {
        char *p;
        // for now: inflate a bit, later: to implement set preservation
        uint64_t nReserved;
    };

// local area minus SSO counter minus terminator minus local flag
#define MSTRING_INPLACE_MAXLEN (sizeof(mstring::spod) - 3)
#define MSTRING_INPLACE_FLAG   spod.extraBytes[sizeof(spod.extraBytes)-1]

    // can compress more where union-based type punning are legal!
    struct {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        union {
            size_type count;
            uint8_t cBytes[sizeof(size_type)];
        };
#endif
        union {
            TRefData ext;
            // for easier debugging & compensate smaller pointer size on 32bit
            char extraBytes[sizeof(ext)];
        };
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
        union {
            size_type count;
            uint8_t cBytes[sizeof(size_type)];
        };
#endif
    } spod;
#else
    // for other compilers: no punning on count var and extra careful access
#define SSO_NOUTYPUN

    // local area minus SSO counter minus terminator
#define MSTRING_INPLACE_BUFSIZE (3*sizeof(size_type))
#define MSTRING_INPLACE_FLAG spod.cBytes[MSTRING_INPLACE_BUFSIZE - 1]
#define MSTRING_INPLACE_MAXLEN (MSTRING_INPLACE_BUFSIZE - 2)

    struct {
        size_type count;
        char cBytes[MSTRING_INPLACE_BUFSIZE];
    } spod;
#endif

    const char* data() const;
    char* data();
    bool isLocal() const { return MSTRING_INPLACE_FLAG; }
    void markLocal(bool val) { MSTRING_INPLACE_FLAG = val; }
    void term(size_type len);
    void extendBy(size_type amount);
    void extendTo(size_type new_len);
    // detect parameter data coming from this string, to take the slower path
    bool input_from_here(mstring_view sv);
    // adjust internal length and inplace mark; no allocaiton, no termination
    void set_len(size_type len, bool forceExternal = false);
    void set_ptr(char* p);
    char* get_ptr();
    const char* get_ptr() const;

public:
    mstring(const char *s, size_type len);
    mstring(mstring_view sv) : mstring(sv.data(), sv.length()) {}
    mstring(const mstring& s) : mstring(s.data(), s.length()) {}
    mstring(const char *s) : mstring(mstring_view(s)) {}
    mstring(mstring&& other);

    // fast in-place concatenation for often uses
    mstring(mstring_view a, mstring_view b);
    mstring(mstring_view a, mstring_view b, mstring_view c);
    mstring(mstring_view a, mstring_view b, mstring_view c,
            mstring_view d, mstring_view e = mstring_view());

    mstring(null_ref &) { set_len(0); data()[0] = 0x0; }
    mstring() : mstring(null) {}

    ~mstring() { clear(); };

    size_type length() const {
#ifdef SSO_NOUTYPUN
        return spod.count;
#else
        return isLocal() ? (spod.count & 0xff) : spod.count;
#endif
    }
    bool isEmpty() const {
        return 0 == length();
    }
    bool nonempty() const { return !isEmpty(); }

    mstring& operator=(mstring_view rv);
    mstring& operator=(const mstring& rv) { return *this = mstring_view(rv); }
    mstring& operator+=(const mstring_view& rv);
    mstring& operator+=(const mstring& rv) {return *this += mstring_view(rv); }
    mstring operator+(const mstring_view& rv) const;
    mstring operator+(const char* s) const { return mstring(*this, s); }
    mstring operator+(const mstring& s) const { return mstring(*this, s); }
    // moves might just steal the other buffer
    mstring& operator=(mstring&& rv);
    mstring& operator+=(mstring&& rv);
    mstring operator+(mstring&& rv) const;
    // plain types
    mstring& operator=(const char* sz) { return *this = mstring_view(sz); }
    mstring& operator+=(const char* s) { return *this += mstring_view(s); }

    bool operator==(const char * rv) const { return equals(rv); }
    bool operator==(mstring_view rv) const { return equals(rv); }
    bool operator!=(const char *rv) const { return !equals(rv); }
    bool operator!=(mstring_view rv) const { return !equals(rv); }
    bool operator==(const mstring &rv) const { return equals(rv); }
    bool operator!=(const mstring &rv) const { return !equals(rv); }
    bool operator==(null_ref &) const { return isEmpty(); }
    bool operator!=(null_ref &) const { return nonempty(); }

    mstring operator=(null_ref &) { clear(); return mstring(); }
    mstring substring(size_type pos) const;
    mstring substring(size_type pos, size_type len) const;
    mstring match(const char* regex, const char* flags = nullptr) const;
    mstring match(const precompiled_regex&) const;

    int operator[](int pos) const { return charAt(pos); }
    int charAt(int pos) const;
    int indexOf(char ch) const;
    int lastIndexOf(char ch) const;
    int count(char ch) const;

    bool equals(const char *&sz) const;
    bool equals(mstring_view sv) const { return sv == *this; }
    bool equals(const mstring &s) const { return mstring_view(s) == *this; };
    bool equals(const char *sz, size_type len) const {
        return equals(mstring_view(sz, len));
    }

    int collate(const mstring& s, bool ignoreCase = false) const;
    int compareTo(const mstring &s) const;
    bool operator<(const mstring& other) const { return compareTo(other) < 0; }
    bool copyTo(char *dst, size_type len) const;

    bool startsWith(mstring_view sv) const;
    bool endsWith(mstring_view sv) const;
    int find(mstring_view) const;

    bool split(unsigned char token, mstring *left, mstring *remain) const;
    bool splitall(unsigned char token, mstring *left, mstring *remain) const;
    mstring trim() const;
    mstring replace(size_type position, size_type len, const mstring &insert) const;
    mstring remove(size_type position, size_type len) const;
    mstring insert(size_type position, const mstring &s) const;
    mstring searchAndReplaceAll(const mstring& s, const mstring& r) const;
    mstring lower() const;
    mstring upper() const;
    /**
     * Calculates the required amount of memory (first run), then formats
     * the data via vsnprintf.
     */
    mstring& appendFormat(const char *fmtString, ...);
    /**
     * Like appendFormat before but first takes an optimistic approach and
     * allocates the specified amount of memory beforehand.
     *
    mstring& appendFormat(size_type sizeHint, const char *fmtString, ...);
     */

    explicit mstring(long val) : mstring() { appendFormat("%ld", val); }

    operator const char *() const { return c_str(); }
    const char* c_str() const { return data();}

    void clear();

    // transfer the ownership of the buffer to a newly created mstring
    // Caller guarantees that buf is 1 char longer which is the terminator!
    static mstring take(char *buf, size_type buf_len);
};

inline bool operator==(const char* s, const mstring& c) {
    return c == s;
}
inline bool operator!=(const char* s, const mstring& c) {
    return !(c == s);
}
inline mstring operator+(const char* s, const mstring& m) {
    return (s && *s) ? (mstring(s) + m) : m;
}
inline mstring_view::mstring_view(const mstring &s) :
        mstring_view(s.data(), s.length()) {
}
void swap(mstring &a, mstring &b);

#endif

// vim: set sw=4 ts=4 et:
