/*
 *  IceWM - string classes
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#ifndef YSTRING_H
#define YSTRING_H

#include <string.h>

class null_ref;
class mstring;

class YWideString {
public:
    YWideString(const char* lstr, size_t llen);
    YWideString(size_t size, wchar_t* text) : fLength(size), fData(text) { }
    YWideString(mstring);
    YWideString() : fLength(0), fData(nullptr) { }
    YWideString(const YWideString& copy);
    ~YWideString() {
        delete[] fData;
    }
    size_t length() const { return fLength; }
    wchar_t* data() const { return fData; }
    wchar_t& operator[](size_t index) const { return fData[index]; }
    wchar_t charAt(size_t index) const { return fData[index]; }
    void replace(size_t pos, size_t plen, const YWideString& insert);
    void operator=(const YWideString& copy);
    void operator=(mstring s);
    operator mstring();
    bool isEmpty() const { return fLength == 0; }
    bool nonempty() const { return fLength > 0; }
    bool operator==(null_ref &) const { return isEmpty(); }
    bool operator!=(null_ref &) const { return nonempty(); }
    YWideString copy(size_t offset, size_t length);
private:
    size_t fLength;
    wchar_t* fData;
};

extern "C" {
    extern int XFree(void*);
    extern void XFreeStringList(char** list);
}

class YStringList {
public:
    char** strings;
    int count;

    YStringList() : strings(nullptr), count(0) { }
    ~YStringList() { XFreeStringList(strings); }
    char* operator[](int index) const { return strings[index]; }
    char* last() const {
        return 0 < count ? strings[count - 1] : nullptr;
    }

    bool operator==(const YStringList& o) const {
        if (count != o.count)
            return false;
        for (int i = 0; i < count; ++i)
            if (strcmp(strings[i], o.strings[i]))
                return false;
        return true;
    }
    bool operator!=(const YStringList& o) const {
        return !(o == *this);
    }
};

#endif

// vim: set sw=4 ts=4 et:
