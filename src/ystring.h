/*
 *  IceWM - string classes
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#ifndef __YSTRING_H
#define __YSTRING_H

#include "ylocale.h"
#include <string.h>

template <class DataType> class YString {
public:
    typedef DataType data_t;

    YString(data_t const * str): fData(NULL) {
        set(str);
    }

    YString(data_t const * str, size_t len): fData(NULL) {
        set(str, len);
    }

    virtual ~YString() {
        delete[] fData;
    }

    void set(data_t const * str) {
        set(str, length(str));
    }

    void set(data_t const * str, size_t len) {
        delete[] fData;

        fSize = (fLength = len) + 1;
        fData = new data_t[fSize];

        ::memcpy(fData, str, sizeof(data_t) * fLength);
    }

    void set(size_t index, data_t const & value) {
        size_t const size(index + 1);

        if (size > fSize) {
            data_t * data(new data_t[size]);
            ::memcpy(data, fData, fSize * sizeof(data_t));
            ::memset(data + fSize, 0, (size - fSize - 1) * sizeof(data_t));

            delete[] fData;

            fData = data;
            fLength = index;
            fSize = size;
        }

        fData[index] = value;
    }

    data_t const * cStr() {
        set(fLength, 0);
        return fData;
    }

    data_t const & get(size_t index) const {
        return (index < fSize ? fData[index] : 0);
    }

    data_t const & operator[](size_t index) const {  get(index); }
    data_t const * data() const { return fData; }
    size_t length() const { return fLength; }
    size_t size() const { return fSize; }

    static size_t length(data_t const * str) {
        if (NULL == str) return 0;

        size_t length(0);
        while (*str++) ++length;
        return length;
    }

    static size_t size(data_t const * str) {
        return length(str) + 1;
    }

protected:
    void assign(data_t * data, size_t length, size_t size) {
        if (data != fData) {
            delete fData;
            fData = data;
        }

        fLength = length;
        fSize = size;
    }

protected:
    YString(): fData(NULL), fLength(0), fSize(0) {}

private:
    data_t * fData;
    size_t fLength, fSize;
};

#ifdef CONFIG_I18N

class YUnicodeString : public YString<YUChar> {
public:
    YUnicodeString(YUChar const * str):
        YString<YUChar>(str) {}
    YUnicodeString(YUChar const * str, size_t len):
        YString<YUChar>(str, len) {}
    YUnicodeString(YLChar const * lstr):
        YString<YUChar>()
    {
        size_t ulen(0);
        YUChar * ustr(YLocale::unicodeString(lstr, strlen(lstr), ulen));
        assign(ustr, ulen, ulen + 1);
    }
    YUnicodeString(YLChar const * lstr, size_t llen):
        YString<YUChar>()
    {
        size_t ulen(0);
        YUChar * ustr(YLocale::unicodeString(lstr, llen, ulen));
        assign(ustr, ulen, ulen + 1);
    }
};

#endif

class YLocaleString : public YString<YLChar> {
public:
    YLocaleString(YLChar const * str): YString<YLChar>(str) {}
    YLocaleString(YLChar const * str, size_t len): YString<YLChar>(str, len) {}
};

#endif

// vim: set sw=4 ts=4 et:
