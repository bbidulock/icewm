#ifndef __MSTRING_H
#define __MSTRING_H

#include "base.h"
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

    MStringData *fStr;
    int fOffset;
    int fCount;

    void acquire() {
        ++fStr->fRefCount;
    }
    void release() {
        if (--fStr->fRefCount == 0)
            destroy();
        fStr = 0;
    }
    void destroy();
    mstring(MStringData *fStr, int fOffset, int fCount);
    void init(const char *str, int len);
    const char *data() const { return fStr->fStr + fOffset; }
public:
    mstring(const char *str);
    mstring(const char *str, int len);

    mstring(const class null_ref &):
        fStr(0),
        fOffset(0),
        fCount(0)
    {}
    mstring():
        fStr(0),
        fOffset(0),
        fCount(0)
    {}

    mstring(const mstring &r):
        fStr(r.fStr),
        fOffset(r.fOffset),
        fCount(r.fCount)
    {
        if (fStr) acquire();
    }
    ~mstring();

    int length() const { return fCount; }
    bool isEmpty() const { return 0 == fCount; }
    bool nonempty() const { return 0 < fCount; }

    mstring& operator=(const mstring& rv);
    mstring& operator+=(const mstring& rv);
    mstring operator+(const mstring& rv) const;

    bool operator==(const mstring &rv) const { return equals(rv); }
    bool operator!=(const mstring &rv) const { return !equals(rv); }
    bool operator==(const class null_ref &) const { return fStr == 0; }
    bool operator!=(const class null_ref &) const { return fStr != 0; }

    mstring& operator=(const class null_ref &);
    mstring substring(int pos) const;
    mstring substring(int pos, int len) const;

    int operator[](int pos) const { return charAt(pos); }
    int charAt(int pos) const;
    int indexOf(char ch) const;
    int lastIndexOf(char ch) const;
    int count(char ch) const;

    bool equals(const mstring &s) const;
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

    cstring& operator=(const cstring& cs);
    operator const char *() const { return c_str(); }
    const char *c_str() const {
        return str.length() > 0 ? str.data() : "";
    }
    const mstring& m_str() const { return str; }
    operator const mstring&() const { return str; }
    bool operator==(const null_ref &) const { return str == null; }
    bool operator!=(const null_ref &) const { return str != null; }
    bool operator==(const cstring& c) const { return str == c.str; }
    bool operator!=(const cstring& c) const { return str != c.str; }
    int c_str_len() const { return str.length(); }
    int length()    const { return str.length(); }
};

#endif
