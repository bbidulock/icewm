#ifndef __CSTRING_H__
#define __CSTRING_H__

class CStr {
private:
    CStr(char *str, int len);
public:
    ~CStr();

    CStr *clone() const;

    int length() const { return fLen; }

    // guaranteed to be non-null!
    operator const char *() const { return fStr; }
    const char *c_str() const { return fStr; }

    bool isWhitespace() const;

    int firstChar() const; // char or -1
    int lastChar() const; // char or -1

private:

    int fLen;
    char *fStr;
public:
    static CStr *newstr(const char *str);
    static CStr *newstr(const char *str, int len);
    static CStr *format(const char *fmt, ...);

    static CStr *join(const char *str, ...);

    CStr *replace(int pos, int len, const CStr *str);

    friend class CStaticStr;
private: //not-used
    CStr(const CStr &);
    CStr &operator=(const CStr &);
};

#if 1 // !!! hmm ;)
class CStaticStr {
public:
    CStaticStr(const char *str);
    ~CStaticStr() { s.fStr = 0; s.fLen = 0; }
    operator const CStr *() { return &s; }
private:
    CStr s;
private: // not-used
    CStaticStr(const CStaticStr &);
    CStaticStr &operator=(const CStaticStr &);
};
#endif

#endif
