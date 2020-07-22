#ifndef MSTRING_H
#define MSTRING_H

#ifdef YARRAY_H
#error include yarray.h after mstring.h
#endif

#include "ref.h"
#include <stdlib.h>

/*
 * A reference counted string buffer of arbitrary but fixed size.
 */
class MStringData {
public:
    MStringData() : fRefCount(0) {}

    int fRefCount;
    char fStr[];
};

class MStringRef {
public:
    MStringRef(): fStr(nullptr) {}
    explicit MStringRef(MStringData* data): fStr(data) {}
    explicit MStringRef(size_t len) { alloc(len); }
    MStringRef(const char* str, size_t len) { create(str, len); }

    void alloc(size_t len);
    void create(const char* str, size_t len);
    operator bool() const { return fStr; }
    MStringData* operator->() const { return fStr; }
    bool operator!=(const MStringRef& r) const { return fStr != r.fStr; }
    char& operator[](size_t index) const { return fStr->fStr[index]; }
    void operator=(MStringData* data) { fStr = data; }
    void acquire() const { fStr->fRefCount++; }
    void release() const { if (fStr->fRefCount-- == 1) free(fStr); }

private:

    MStringData* fStr;
};

/*
 * Mutable strings with a reference counted string buffer.
 */
class mstring {
private:
    friend class MStringArray;
    friend mstring operator+(const char* s, const mstring& m);

    MStringRef fRef;
    size_t fOffset;
    size_t fCount;

    void acquire() {
        if (fRef) { fRef.acquire(); }
    }
    void release() {
        if (fRef) { fRef.release(); }
    }
    mstring(const MStringRef& str, size_t offset, size_t count);
    mstring(const char* str1, size_t len1, const char* str2, size_t len2);
    const char* data() const { return &fRef[fOffset]; }

public:
    mstring(const char *str);
    mstring(const char *str1, const char *str2);
    mstring(const char *str1, const char *str2, const char *str3);
    mstring(const char *str, size_t len);
    explicit mstring(long);

    mstring(null_ref &): fRef(nullptr), fOffset(0), fCount(0) { }
    mstring():           fRef(nullptr), fOffset(0), fCount(0) { }

    mstring(const mstring &r):
        fRef(r.fRef),
        fOffset(r.fOffset),
        fCount(r.fCount)
    {
        acquire();
    }
    ~mstring() {
        release();
    }

    size_t length() const { return fCount; }
    size_t offset() const { return fOffset; }
    bool isEmpty() const { return 0 == fCount; }
    bool nonempty() const { return 0 < fCount; }

    mstring& operator=(const mstring& rv);
    void operator+=(const mstring& rv);
    mstring operator+(const mstring& rv) const;

    bool operator==(const char *rv) const { return equals(rv); }
    bool operator!=(const char *rv) const { return !equals(rv); }
    bool operator==(const mstring &rv) const { return equals(rv); }
    bool operator!=(const mstring &rv) const { return !equals(rv); }
    bool operator==(null_ref &) const { return isEmpty(); }
    bool operator!=(null_ref &) const { return nonempty(); }

    mstring operator=(null_ref &) { return *this = mstring(); }
    mstring substring(size_t pos) const;
    mstring substring(size_t pos, size_t len) const;
    mstring match(const char* regex, const char* flags = nullptr) const;

    int operator[](int pos) const { return charAt(pos); }
    int charAt(int pos) const;
    int indexOf(char ch) const;
    int lastIndexOf(char ch) const;
    int count(char ch) const;

    bool equals(const char *s) const;
    bool equals(const char *s, size_t len) const;
    bool equals(const mstring &s) const;
    int collate(mstring s, bool ignoreCase = false);
    int compareTo(const mstring &s) const;
    bool operator<(const mstring& other) const { return compareTo(other) < 0; }
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
    mstring append(const mstring &str) const { return *this + str; }
    mstring searchAndReplaceAll(const mstring& s, const mstring& r) const;
    mstring lower() const;
    mstring upper() const;

    operator const char *() { return c_str(); }
    const char* c_str();
};

inline bool operator==(const char* s, const mstring& c) { return c == s; }
inline bool operator!=(const char* s, const mstring& c) { return c != s; }

inline mstring operator+(const char* s, const mstring& m) {
    return mstring(s) + m;
}

#endif

// vim: set sw=4 ts=4 et:
