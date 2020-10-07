#ifndef MSTRING_H
#define MSTRING_H

// this is just for strlen since forward declaration might be not portable
#include <cstring>

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
class precompiled_regex;
class null_ref;

class mstring_view {
    const char *m_data;
    std::size_t m_size;
public:
    mstring_view(const char *s, std::size_t len) :
            m_data(s), m_size(len) {
    }
    mstring_view(const char *s) : m_data(s),
            m_size(s ? strlen(s) : 0) {}
    mstring_view(null_ref &) : mstring_view() {}
    mstring_view() : m_data(nullptr), m_size(0) {}
    mstring_view(const mstring& s);
    std::size_t length() const { return m_size; }
    const char* data() const { return m_data; }
    bool operator==(mstring_view rv) const;
    bool operator!=(mstring_view rv) const { return ! (rv == *this); }
    bool isEmpty() const { return length() == 0; }
    bool nonempty() const { return ! isEmpty(); }

    mstring_view trim() const;
    int lastIndexOf(char ch) const;
    int indexOf(char ch) const;
    bool split(unsigned char token, mstring_view& left,
            mstring_view& remain) const;
    bool splitall(unsigned char token, mstring_view& left,
            mstring_view& remain) const;
    mstring_view substring(size_t pos, size_t len) const;
    bool startsWith(mstring_view sv) const;
};

/*
 * Mutable strings with external OR internal storage (small string optimization)
 */
class mstring {
public:
    using size_type = unsigned long;
protected:
    friend void swap(mstring& a, mstring& b);
    friend class mstring_view;

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
    bool isLocal() const { return ! MSTRING_EXTERNAL_FLAG; }
    void markExternal(bool val) { MSTRING_EXTERNAL_FLAG = val; }
    void term(size_type len);
    void extendBy(size_type amount);
    void extendTo(size_type new_len);
    // detect parameter data coming from this string, to take the slower path
    bool input_from_here(mstring_view sv);
    // adjusts internal length and allocation mark (no data ops)
    void set_len(size_type len, bool forceExternal = false);
    void set_ptr(char* p);
    char* get_ptr();
    const char* get_ptr() const { return const_cast<mstring*>(this)->get_ptr();}

public:
    mstring() { spod.count = 0; markExternal(false); }
    mstring(const char *s, size_type len);
    mstring(mstring_view sv) : mstring(sv.data(), sv.length()) {}
    mstring(const mstring& s) : mstring(s.data(), s.length()) {}
    mstring(const char *s) : mstring(mstring_view(s)) {}
    mstring(mstring&& other);
    explicit mstring(long val) : mstring() { appendFormat("%ld", val); }
    mstring(null_ref &) : mstring() {}

    // shortcuts for faster in-place concatenation for often uses
    mstring(mstring_view a, mstring_view b);
    mstring(mstring_view a, mstring_view b, mstring_view c);
    mstring(mstring_view a, mstring_view b, mstring_view c,
            mstring_view d, mstring_view e = mstring_view());

    ~mstring();
#ifdef SSO_NOUTYPUN
    size_type length() const { return spod.count; }
#else
    size_type length() const { return isLocal() ? spod.cBytes[0] : spod.count; }
#endif
    bool isEmpty() const { return 0 == length(); }
    bool nonempty() const { return !isEmpty(); }

    mstring& operator=(mstring_view rv);
    mstring& operator=(const mstring& rv) { return *this = mstring_view(rv); }
    mstring& operator+=(mstring_view rv);
    // XXX: since this is used for string building, implement over-allocation!
    mstring& operator<<(mstring_view rv) { return *this += rv;}
    mstring operator+(mstring_view rv) const { return mstring(*this, rv); }

    // moves might just steal the other buffer
    mstring& operator=(mstring&& rv);
    // plain types
    mstring& operator=(const char* sz) { return *this = mstring_view(sz); }

    bool operator==(const char * rv) const { return equals(rv); }
    bool operator==(mstring_view rv) const { return equals(rv); }
    bool operator!=(const char *rv) const { return !equals(rv); }
    bool operator!=(mstring_view rv) const { return !equals(rv); }
    bool operator==(const mstring &rv) const { return equals(rv); }
    bool operator!=(const mstring &rv) const { return !equals(rv); }
    bool operator==(null_ref &) const { return isEmpty(); }
    bool operator!=(null_ref &) const { return nonempty(); }

    mstring& operator=(null_ref &) { clear(); return *this; }
    mstring substring(size_type pos) const;
    mstring substring(size_type pos, size_type len) const {
        return mstring_view(*this).substring(pos, len);
    }
    mstring match(const char* regex, const char* flags = nullptr) const;
    mstring match(const precompiled_regex&) const;

    int operator[](int pos) const { return charAt(pos); }
    int charAt(int pos) const;
    int indexOf(char ch) const { return mstring_view(*this).indexOf(ch); }
    int lastIndexOf(char c) const { return mstring_view(*this).lastIndexOf(c);}
    int count(char ch) const;

    bool equals(const char *&sz) const;
    bool equals(mstring_view sv) const { return sv == *this; }
    bool equals(const char *sz, size_type len) const {
        return equals(mstring_view(sz, len));
    }

    int collate(const mstring& s, bool ignoreCase = false) const;
    int compareTo(const mstring &s) const;
    bool operator<(const mstring& other) const { return compareTo(other) < 0; }
    bool copyTo(char *dst, size_type len) const;

    bool startsWith(mstring_view sv) const {
        return mstring_view(*this).startsWith(sv);
    }
    bool endsWith(mstring_view sv) const;
    int find(mstring_view, size_type startPos = 0) const;

    mstring_view trim() const { return mstring_view(*this).trim(); }
    mstring replace(size_type position, size_type len, const mstring &insert) const;
    mstring remove(size_type position, size_type len) const;
    mstring insert(size_type position, const mstring &s) const;
    mstring searchAndReplaceAll(const mstring& s, const mstring& r) const;
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

inline mstring_view::mstring_view(const mstring &s) :
        mstring_view(s.data(), s.length()) {
}

#endif

// vim: set sw=4 ts=4 et:
