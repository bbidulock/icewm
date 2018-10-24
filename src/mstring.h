#ifndef __MSTRING_H
#define __MSTRING_H

#ifdef __YARRAY_H
#error include yarray.h after mstring.h
#endif

#include "ref.h"
#include <string.h>

/*
 * A reference counted string buffer of arbitrary but fixed size.
 */
struct MStringData {
    int fRefCount;
    char fStr[];

    static MStringData *alloc(int length);
    static MStringData *create(const char *str, int length);
    static MStringData *create(const char *str);
};

/*
 * Mutable strings with a reference counted string buffer.
 */
class mstring {
private:
    friend class cstring;
    friend class MStringArray;

    MStringData *fStr;
    size_t fOffset;
    size_t fCount;

    void acquire() {
        ++fStr->fRefCount;
    }
    void release() {
        if (--fStr->fRefCount == 0)
            destroy();
        fStr = 0;
    }
    void destroy();
    mstring(MStringData *fStr, size_t fOffset, size_t fCount);
    void init(const char *str, size_t len);
    const char *data() const { return fStr->fStr + fOffset; }
public:
    mstring(const char *str);
    mstring(const char *str1, const char *str2);
    mstring(const char *str1, const char *str2, const char *str3);
    mstring(const char *str, size_t len);
    explicit mstring(long);

    mstring(null_ref &): fStr(0), fOffset(0), fCount(0) { }
    mstring():           fStr(0), fOffset(0), fCount(0) { }

    mstring(const mstring &r):
        fStr(r.fStr),
        fOffset(r.fOffset),
        fCount(r.fCount)
    {
        if (fStr) acquire();
    }
    ~mstring();

    size_t length() const { return fCount; }
    size_t offset() const { return fOffset; }
    bool isEmpty() const { return 0 == fCount; }
    bool nonempty() const { return 0 < fCount; }

    mstring& operator=(const mstring& rv);
    mstring& operator+=(const mstring& rv);
    mstring operator+(const mstring& rv) const;

    bool operator==(const char *rv) const { return equals(rv); }
    bool operator!=(const char *rv) const { return !equals(rv); }
    bool operator==(const mstring &rv) const { return equals(rv); }
    bool operator!=(const mstring &rv) const { return !equals(rv); }
    bool operator==(null_ref &) const { return fStr == 0; }
    bool operator!=(null_ref &) const { return fStr != 0; }
//    bool operator==(const char *rv, size_t len) const { return equals(rv, len); }
//    bool operator!=(const char *rv, size_t len) const { return !equals(rv, len); }

    mstring& operator=(null_ref &);
    mstring substring(size_t pos) const;
    mstring substring(size_t pos, size_t len) const;
    mstring match(const char* regex, const char* flags = 0) const;

    int operator[](int pos) const { return charAt(pos); }
    int charAt(int pos) const;
    int indexOf(char ch) const;
    int lastIndexOf(char ch) const;
    int count(char ch) const;

    bool equals(const char *s) const;
    bool equals(const char *s, unsigned len) const;
    bool equals(const mstring &s) const;
    int collate(const mstring &s, bool ignoreCase = false) const;
    int compareTo(const mstring &s) const;
    bool copyTo(char *dst, size_t len) const;

    bool startsWith(const mstring &s) const;
    bool endsWith(const mstring &s) const;
    int find(const mstring &s) const;

    bool split(unsigned char token, mstring *left, mstring *remain) const;
    bool splitall(unsigned char token, mstring *left, mstring *remain) const;
    mstring trim() const;
    mstring replace(int position, int len, const mstring &insert) const;
    mstring remove(int position, int len) const;
    mstring insert(int position, const mstring &s) const;
    mstring append(const mstring &s) const;
    mstring searchAndReplaceAll(const mstring& s, const mstring& r) const;
    mstring lower() const;
    mstring upper() const;

#if 0
    static mstring fromUTF32(const UChar *str, int len);
    static mstring fromUTF8(const unsigned char *str, int len);
#endif
    static mstring fromMultiByte(const char *str, int len);
    static mstring fromMultiByte(const char *str);
    static mstring newstr(const char *str);
    static mstring newstr(const char *str, int len);
    void normalize();
};

typedef class mstring ustring;

/*
 * Constant strings with a conversion to nul-terminated C strings.
 */
class cstring {
private:
    mstring str;

public:
    cstring() : str() {}
    cstring(const mstring &str);
    cstring(const char *cstr) : str(cstr) {}
    cstring(const null_ref &): str() {}
    explicit cstring(long n): str(n) {}

    cstring& operator=(const cstring& cs);
    cstring operator+(const mstring& rv) const { return cstring(m_str() + rv); }
    operator const char *() const { return c_str(); }
    const char *c_str() const {
        return str.length() > 0 ? str.data() : "";
    }
    const mstring& m_str() const { return str; }
    operator const mstring&() const { return str; }
    bool operator==(const char* cstr) const { return str == cstr; }
    bool operator!=(const char* cstr) const { return str != cstr; }
    bool operator==(const null_ref &) const { return str == null; }
    bool operator!=(const null_ref &) const { return str != null; }
    bool operator==(const cstring& c) const { return str == c.str; }
    bool operator!=(const cstring& c) const { return str != c.str; }
    int c_str_len() const { return str.length(); }
    int length()    const { return str.length(); }
};

inline bool operator==(const char* s, cstring c) { return c == s; }
inline bool operator!=(const char* s, cstring c) { return c != s; }

inline mstring operator+(const char* s, const mstring& m) {
    return mstring(s) + m;
}

#endif

// vim: set sw=4 ts=4 et:
