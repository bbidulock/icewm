#ifndef __MSTRING_H
#define __MSTRING_H

#include "base.h"
#include "ref.h"
#include <string.h>

#pragma interface

struct MStringData {
    int fRefCount;
    char fStr[];
};

class mstring {
private:
    friend class cstring;

    MStringData *fStr;
    int fOffset;
    int fCount;

//    mstring(unsigned char *str, int len);

    void acquire() {
        ++fStr->fRefCount;
//        msg("+");
    }
    void release() {
//        msg("-");
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
    mstring(int);

    mstring(const class null_ref &):
        fStr(0),
        fOffset(0),
        fCount(0)
    {}
//    mstring(const mstring &r);

    mstring(const mstring &r):
        fStr(r.fStr),
        fOffset(r.fOffset),
        fCount(r.fCount)

    {
        if (fStr) acquire();
    }
    ~mstring();

    int length() const { return fCount; }

#if 0
    const char *c_str() const { if (fStr) return fStr->fStr + fOffset; else return 0; }
    int c_str_len() const { return fCount; }
#endif

    mstring operator=(const mstring& rv);

    bool operator==(const class null_ref &) const { return fStr == 0; }
    bool operator!=(const class null_ref &) const { return fStr != 0; }

    mstring operator=(const class null_ref &);
    mstring substring(int pos);
    mstring substring(int pos, int len);

    int charAt(int pos) const;
    int indexOf(char ch) const;

    bool equals(const mstring &s) const;
    int compareTo(const mstring &s) const;
    bool copy(char *dst, size_t len) const;

    bool startsWith(const mstring &s) const;
    bool endsWith(const mstring &s) const;

    bool split(unsigned char token, mstring *left, mstring *remain) const;
    bool splitall(unsigned char token, mstring *left, mstring *remain) const;
    mstring join(const mstring &append) const;
    mstring trim() const;
    mstring replace(int position, int len, const mstring &insert);
    mstring remove(int position, int len);
    mstring insert(int position, const mstring &s);
    mstring append(const mstring &s);
public:
#if 0
    static mstring fromUTF32(const UChar *str, int len);
    static mstring fromUTF8(const unsigned char *str, int len);
#endif
    static mstring fromMultiByte(const char *str, int len);
    static mstring fromMultiByte(const char *str);
    static mstring newstr(const char *str);
    static mstring newstr(const char *str, int len);
    static mstring format(const char *fmt, ...);
    static mstring join(const char *str, ...);
};

typedef class mstring ustring;

class cstring {
public:
    cstring(const mstring &str);

    const char *c_str() const {
        if (str.fStr)
            return str.fStr->fStr + str.fOffset;
        else
            return "";
    }
    int c_str_len() const { return str.fCount; }
private:
    mstring str;

    cstring(const cstring &);
    cstring &operator=(const cstring &);
};

#endif
