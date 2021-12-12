#include "config.h"
#include "ystring.h"
#include "ylocale.h"
#include "mstring.h"
#include "base.h"

YWideString::YWideString(const char* lstr, size_t llen) :
    fLength(0),
#ifdef CONFIG_I18N
    fData(YLocale::unicodeString(lstr, llen, fLength))
#else
    fData(YLocale::wideCharString(lstr, llen, fLength))
#endif
{
}

YWideString::YWideString(mstring mstr) :
    fLength(0),
#ifdef CONFIG_I18N
    fData(YLocale::unicodeString(mstr, mstr.length(), fLength))
#else
    fData(YLocale::wideCharString(mstr, mstr.length(), fLength))
#endif
{
}

YWideString::YWideString(const YWideString& copy)
    : fLength(copy.fLength)
    , fData(nullptr)
{
    if (fLength) {
        fData = new wchar_t[fLength + 1];
        memcpy(fData, copy.fData, fLength * sizeof(wchar_t));
        fData[fLength] = 0;
    }
}

void YWideString::operator=(mstring s) {
    YWideString w(s);
    swap(fLength, w.fLength);
    swap(fData, w.fData);
}

void YWideString::operator=(const YWideString& copy) {
    if (this != &copy) {
        delete[] fData;
        if (copy.fLength) {
            fData = new wchar_t[fLength + 1];
            memcpy(fData, copy.fData, fLength * sizeof(wchar_t));
            fData[fLength] = 0;
        } else {
            fData = nullptr;
            fLength = 0;
        }
    }
}

YWideString::operator mstring() {
    size_t len = 0;
    char* str  = YLocale::narrowString(fData, fLength + 1, len);
    if (str) {
        mstring m(str, len);
        delete[] str;
        return m;
    } else {
        return null;
    }
}

void YWideString::replace(size_t pos, size_t plen, const YWideString& insert)
{
    PRECONDITION(pos <= fLength && plen <= fLength && pos + plen <= fLength);
    size_t size = fLength - plen + insert.fLength;
    wchar_t* copy = new wchar_t[size + 1];
    size_t k = 0;
    if (0 < pos) {
        memcpy(copy + k, fData, pos * sizeof(wchar_t));
        k += pos;
    }
    if (0 < insert.fLength) {
        memcpy(copy + k, insert.fData, insert.fLength * sizeof(wchar_t));
        k += insert.fLength;
    }
    if (pos + plen < fLength) {
        size_t n = fLength - pos - plen;
        memcpy(copy + k, fData + pos + plen, n * sizeof(wchar_t));
        k += n;
    }
    if (k) {
        copy[k] = 0;
        delete[] fData;
        fData = copy;
        fLength = k;
    } else {
        delete[] fData;
        fData = nullptr;
        fLength = 0;
    }
}

YWideString YWideString::copy(size_t offset, size_t length) {
    if (offset < fLength && 0 < length) {
        size_t size = min(fLength - offset, length);
        wchar_t* copy = new wchar_t[size + 1];
        memcpy(copy, fData + offset, size);
        copy[size] = 0;
        return YWideString(size, copy);
    } else {
        return YWideString();
    }
}

