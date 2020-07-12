/*
 *  IceWM - string classes
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#ifndef YSTRING_H
#define YSTRING_H

#include "ylocale.h"
#include <string.h>

template <class DataType>
class YString {
public:
    typedef DataType data_t;

    YString(data_t const* str):
        fSize(size(str)),
        fData(new data_t[fSize])
    {
        memcpy(fData, str, sizeof(data_t) * fSize);
    }

    YString(data_t const* str, size_t len):
        fSize(1 + len),
        fData(new data_t[fSize])
    {
        memcpy(fData, str, sizeof(data_t) * len);
        fData[fSize - 1] = 0;
    }

    virtual ~YString() {
        delete[] fData;
    }

    void set(size_t index, data_t value) {
        size_t const size(index + 1);

        if (size > fSize) {
            data_t* data(new data_t[size]);
            if (fSize && fData) {
                memcpy(data, fData, fSize * sizeof(data_t));
            }
            memset(data + fSize, 0, (size - fSize) * sizeof(data_t));

            delete[] fData;

            fData = data;
            fSize = size;
        }

        fData[index] = value;
    }

    data_t const* cStr() {
        return fData;
    }

    data_t get(size_t index) const {
        return (index < fSize ? fData[index] : 0);
    }

    data_t operator[](size_t index) const {  get(index); }
    data_t const* data() const { return fData; }
    size_t length() const { return fSize - 1; }
    size_t size() const { return fSize; }

    static size_t length(data_t const* str) {
        size_t length(0);
        if (str) {
            while (str[length]) {
                ++length;
            }
        }
        return length;
    }

    static size_t size(data_t const* str) {
        return length(str) + 1;
    }

protected:
    void assign(data_t* data, size_t len) {
        fData = data;
        fSize = len + 1;
    }

    YString():
        fSize(0),
        fData(nullptr)
    { }

private:
    size_t fSize;
    data_t* fData;
};

#ifdef CONFIG_I18N

class YUnicodeString : public YString<YUChar> {
public:
    YUnicodeString(YUChar const* str):
        YString<YUChar>(str)
    { }
    YUnicodeString(YUChar const* str, size_t len):
        YString<YUChar>(str, len)
    { }
    YUnicodeString(YLChar const* lstr):
        YString<YUChar>()
    {
        size_t ulen(0);
        YUChar* ustr(YLocale::unicodeString(lstr, strlen(lstr), ulen));
        assign(ustr, ulen);
    }
    YUnicodeString(YLChar const* lstr, size_t llen):
        YString<YUChar>()
    {
        size_t ulen(0);
        YUChar* ustr(YLocale::unicodeString(lstr, llen, ulen));
        assign(ustr, ulen);
    }
};

#endif

class YLocaleString : public YString<YLChar> {
public:
    YLocaleString(YLChar const* str):
        YString<YLChar>(str)
    { }
    YLocaleString(YLChar const* str, size_t len):
        YString<YLChar>(str, len)
    { }
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
