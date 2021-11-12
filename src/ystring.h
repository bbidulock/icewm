/*
 *  IceWM - string classes
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#ifndef YSTRING_H
#define YSTRING_H

#include <string.h>

#ifdef CONFIG_I18N

#include "ylocale.h"

class YUnicodeString {
public:
    YUnicodeString(char const* lstr, size_t llen) :
        fLength(0),
        fData(YLocale::unicodeString(lstr, llen, fLength))
    {
    }
    ~YUnicodeString() {
        delete[] fData;
    }
    size_t length() const { return fLength; }
    wchar_t* data() const { return fData; }
private:
    size_t fLength;
    wchar_t* fData;
};

#endif

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
