#ifndef MSTRING_H
#define MSTRING_H

// this is just for strlen since forward declaration might be not portable
#include <string.h>

// if type punning is disabled, the strategy is:
// (dis)assemble pointer by memcpy
// count variable remains a dedicated field (no sharing there)
#if defined(DEBUG_NOUTYPUN) || !defined(__GNUC__)
#define SSO_NOUTYPUN 1
#endif

/*
 * NOTE: possible options to tune here:
 * - DEBUG_NOUTYPUN: force pure standard conform pointer conversion
 * - DEBUG_SSOLIMIT: optional, can be used to push even short strings onto
 *                   heap, although zero-length is still not allocating
 */

class mstring;
class null_ref;

/**
 * String memory slice, defined by pointer and size.
 * Inspired by std::string_view and Rust string slices.
 * No zero termination is guaranteed unless specified otherwise.
 * Caller or user of this is responsible for the lifetime of referred memory.
 */
class mslice {
    const char *m_data;
    size_t m_size;
public:
    mslice(const char *s, size_t len) : m_data(s), m_size(len) {}
    mslice(const char *s) : mslice(s, s ? strlen(s) : 0) {}
    mslice(null_ref &) : mslice() {}
    mslice() : mslice(nullptr, 0) {}
    mslice(const mstring& s);
    size_t length() const { return m_size; }
    const char* data() const { return m_data; }
    bool operator==(mslice rv) const;
    bool operator!=(mslice rv) const { return ! (rv == *this); }
    bool isEmpty() const { return length() == 0; }
    bool nonempty() const { return ! isEmpty(); }

    mslice trim() const;
    int lastIndexOf(char ch) const;
    int indexOf(char ch) const;
    bool split(unsigned char token, mslice& left, mslice& remain) const;
    bool splitall(unsigned char token, mslice& left, mslice& remain) const;
    mslice substring(size_t pos, size_t len) const;
    bool startsWith(mslice sv) const;
    mstring toMstring() const; // costly conversion to mstring
};

/*
 * Mutable strings with external OR internal storage (small string optimization)
 */
class mstring {
public:
    using size_type = unsigned long;
protected:
    friend void swap(mstring& a, mstring& b);
    friend class mslice;

#ifndef SSO_NOUTYPUN
    struct TRefData {
        char *p;
        // for now: inflate a bit, later: to implement set preservation
        char nReserved[8];
    };
#ifdef DEBUG_SSOLIMIT
    // local area minus SSO counter minus terminator minus local flag
    #define MSTRING_INPLACE_MAXLEN DEBUG_SSOLIMIT
#else
    #define MSTRING_INPLACE_MAXLEN (sizeof(mstring::spod) - 3)
#endif
#define MSTRING_EXTERNAL_FLAG   spod.extraBytes[sizeof(spod.extraBytes)-1]
    // can compress more where union-based type punning are legal!
    struct {
        union {
            size_type count;
            unsigned char cBytes[sizeof(size_type)];
        };
        union {
            TRefData ext;
            // for easier debugging & compensate smaller pointer size on 32bit
            char extraBytes[sizeof(ext)];
        };
    } spod;
#else
    // local area minus SSO counter minus terminator
#define MSTRING_INPLACE_BUFSIZE (3*sizeof(size_type))
#define MSTRING_EXTERNAL_FLAG spod.cBytes[MSTRING_INPLACE_BUFSIZE - 1]
#ifdef DEBUG_SSOLIMIT
#define MSTRING_INPLACE_MAXLEN DEBUG_SSOLIMIT
#else
#define MSTRING_INPLACE_MAXLEN (MSTRING_INPLACE_BUFSIZE - 2)
#endif
    struct {
        size_type count;
        char cBytes[MSTRING_INPLACE_BUFSIZE];
    } spod;
#endif
    const char* data() const;
    char* data();
private:
    bool isLocal() const { return ! MSTRING_EXTERNAL_FLAG; }
    void markExternal(bool val) { MSTRING_EXTERNAL_FLAG = val; }
    void term(size_type len);
    void extendBy(size_type amount);
    void extendTo(size_type new_len);
    // detect parameter data coming from this string, to take the slower path
    bool input_from_here(mslice sv);
    // adjusts internal length and allocation mark (no data ops)
    void set_len(size_type len, bool forceExternal = false);
    void set_ptr(char* p);
    char* get_ptr();
    const char* get_ptr() const { return const_cast<mstring*>(this)->get_ptr();}

public:
    mstring() { spod.count = 0; markExternal(false); }
    mstring(const char *s, size_type len);
    mstring(mslice sv) : mstring(sv.data(), sv.length()) {}
    mstring(const mstring& s) : mstring(s.data(), s.length()) {}
    mstring(const char *s) : mstring(mslice(s)) {}
    mstring(mstring&& other);
    explicit mstring(long val) : mstring() { appendFormat("%ld", val); }
    mstring(null_ref &) : mstring() {}

    // shortcuts for faster in-place concatenation for often uses
    mstring(mslice a, mslice b);
    mstring(mslice a, mslice b, mslice c);
    mstring(mslice a, mslice b, mslice c, mslice d, mslice e = mslice());

    ~mstring();
#ifdef SSO_NOUTYPUN
    size_type length() const { return spod.count; }
#else
    size_type length() const { return isLocal() ? spod.cBytes[0] : spod.count; }
#endif
    bool isEmpty() const { return 0 == length(); }
    bool nonempty() const { return !isEmpty(); }

    mstring& operator=(mslice rv);
    mstring& operator=(const mstring& rv) { return *this = mslice(rv); }
    mstring& operator+=(mslice rv);
    // XXX: since this is used for string building, implement over-allocation!
    mstring& operator<<(mslice rv) { return *this += rv;}
    mstring operator+(mslice rv) const { return mstring(*this, rv); }

    // moves might just steal the other buffer
    mstring& operator=(mstring&& rv);
    // plain types
    mstring& operator=(const char* sz) { return *this = mslice(sz); }

    bool operator==(const char * rv) const { return equals(rv); }
    bool operator==(mslice rv) const { return equals(rv); }
    bool operator!=(const char *rv) const { return !equals(rv); }
    bool operator!=(mslice rv) const { return !equals(rv); }
    bool operator==(const mstring &rv) const { return equals(rv); }
    bool operator!=(const mstring &rv) const { return !equals(rv); }
    bool operator==(null_ref &) const { return isEmpty(); }
    bool operator!=(null_ref &) const { return nonempty(); }

    mstring& operator=(null_ref &) { clear(); return *this; }
    mslice substring(size_type pos) const;
    mslice substring(size_type pos, size_type len) const {
        return mslice(*this).substring(pos, len);
    }

    int operator[](int pos) const { return charAt(pos); }
    int charAt(int pos) const;
    int indexOf(char ch) const { return mslice(*this).indexOf(ch); }
    int lastIndexOf(char c) const { return mslice(*this).lastIndexOf(c);}
    int count(char ch) const;

    bool equals(const char *&sz) const;
    bool equals(mslice sv) const { return sv == *this; }
    bool equals(const char *sz, size_type len) const {
        return equals(mslice(sz, len));
    }

    int collate(const mstring& s, bool ignoreCase = false) const;
    int compareTo(const mstring &s) const;
    bool operator<(const mstring& other) const { return compareTo(other) < 0; }
    bool copyTo(char *dst, size_type len) const;

    bool startsWith(mslice sv) const {
        return mslice(*this).startsWith(sv);
    }
    bool endsWith(mslice sv) const;
    int find(mslice, size_type startPos = 0) const;

    mslice trim() const { return mslice(*this).trim(); }
    mstring replace(size_type position, size_type len, mslice insert) const;
    mstring remove(size_type position, size_type len) const;
    mstring insert(size_type position, mslice s) const;
    mstring lower() const;
    mstring upper() const;
    mstring modified(char (*mod)(char)) const;
    /**
     * Calculates the required amount of memory (first run), then formats
     * the data via vsnprintf.
     */
    mstring& appendFormat(const char *fmtString, ...);

    operator const char *() const { return c_str(); }
    const char* c_str() const { return data();}

    void clear();

    // transfer the ownership of the buffer to a newly created mstring
    // Caller guarantees that buf is 1 char longer which is the terminator!
    static mstring take(char *buf, size_type buf_len);
};

inline mslice::mslice(const mstring &s) : mslice(s.data(), s.length()) {}
inline mstring mslice::toMstring() const { return mstring(*this); }

#endif

// vim: set sw=4 ts=4 et:
