#ifndef __ARGUMENT_H
#define __ARGUMENT_H

// class Argument is helpful in configuration parsing
// and stores the result of a parsed string.

class Argument {
private:
    char arg1[128];
    char* arg2;
    int siz;
    int len;
    void dispose() { if (arg2) delete[] arg2; }
    char* copyTo(char *p) const { memcpy(p, cstr(), len+1); return p; }
    void replace(char *p, int n) { dispose(); arg2 = p; siz = n; }
public:
    Argument() : arg2(nullptr), siz(128), len(0) { *arg1 = 0; }
    ~Argument() { dispose(); }

    operator char *() { return str(); }
    char *str() { return arg2 ? arg2 : arg1; }

    operator const char *() const { return cstr(); }
    const char *cstr() const { return arg2 ? arg2 : arg1; }

    int size() const { return siz; }
    int length() const { return len; }
    void reset() { dispose(); arg2 = nullptr; siz = 128; arg1[len = 0] = 0; }
    void expand(int n) { if (n > siz) replace(copyTo(new char[n]), n); }
    char operator[](int at) const { return at < len ? cstr()[at] : 0; }
    void push(char ch) {
        if (siz < len + 2) expand(2 * len);
        str()[len + 1] = 0;
        str()[len++] = ch;
    }
    void operator+=(char ch) { push(ch); }

    Argument(const Argument& c) : arg2(nullptr), siz(128) { *arg1 = 0; *this = c; }
    Argument& operator=(const Argument& copy) {
        if (this != &copy) {
            int k = 1 + copy.len;
            if (k < siz) {
                memcpy(str(), copy.cstr(), k);
                len = k - 1;
            } else {
                replace(copy.copyTo(new char[k]), k);
            }
        }
        return *this;
    }

};

#endif

// vim: set sw=4 ts=4 et:
